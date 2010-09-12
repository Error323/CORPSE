#include <GL/gl.h>
#include <SDL/SDL.h>

#include <sstream>

#include "../Math/vec3fwd.hpp"
#include "../Math/mat44fwd.hpp"

#include "../Sim/SimThread.hpp"
#include "../System/EngineAux.hpp"
#include "../System/EventHandler.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Logger.hpp"

#include "../Sim/SimObjectDefHandler.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../Sim/SimObjectDef.hpp"
#include "../Sim/SimObject.hpp"

#include "./Scene.hpp"
#include "./Camera.hpp"
#include "./VertexArray.hpp"

#include "../Map/Ground.hpp"
#include "../Map/MapInfo.hpp"

#ifdef CUSTOM_SMF_RENDERER
#include "./SMFRenderer.hpp"
#else
#include "../Map/BaseGroundDrawer.hpp"
#include "../Map/ReadMap.hpp"
#endif

#include "./Env/SkyBox.hpp"
#include "./Env/ShadowHandler.hpp"

#include "./Models/ModelReaderBase.hpp"
#include "./Models/ModelReaderS3O.hpp"
#include "./Models/ModelDrawerS3O.hpp"
#include "./Shaders/ShaderHandler.hpp"
#include "./Textures/TextureHandlerBase.hpp"



CScene::CScene() {
	#ifdef CUSTOM_SMF_RENDERER
	smfRenderer = new CSMFRenderer();
	#endif

	skyBox = new CSkyBox();

	// the drawer and reader base classes are abstract
	readerS3O = new CModelReaderS3O();

	SetBoundingRadius();
	LoadTeamColors();
	LoadObjectModels();
	shadowHandler->GenDepthTextureFBO();
	InitLight();

	this->SetPriority(456);
	eventHandler->AddReceiver(this);

	const LuaTable* rootTbl = LUA->GetRoot();
	const LuaTable* srvrTbl = rootTbl->GetTblVal("server");

	// set the number of SDL ticks between two sim-frames; this
	// does not change at run-time even when the multiplier is
	// increased or decreased
	simFrameIntervalTicks  = (srvrTbl->GetFltVal("simFrameRate", 25.0f) / srvrTbl->GetFltVal("simFrameMult", 1.0f)) * 1000;
	simFrameDeltaTickRatio = 1.0f;

	// for interpolation we need to know which sim-frame we are
	// currently at and the corresponding SDL tick (timestamp)
	currSimFrame     = 0;
	currSimFrameTick = SDL_GetTicks();
}

CScene::~CScene() {
	eventHandler->DelReceiver(this);

	#ifdef CUSTOM_SMF_RENDERER
	delete smfRenderer; smfRenderer = NULL;
	#endif

	delete skyBox; skyBox = NULL;
	delete sun;

	delete readerS3O; readerS3O = NULL;

	shaderHandler->ReleaseProgramObjects("[S3O]");
}

void CScene::LoadTeamColors() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* teamsTable = rootTable->GetTblVal("teams");

	std::list<int> teamKeys;
	std::list<int>::const_iterator teamKeysIt;

	teamsTable->GetIntTblKeys(&teamKeys);
	teamColors.resize(teamKeys.size());

	unsigned int teamIdx = 0;

	for (teamKeysIt = teamKeys.begin(); teamKeysIt != teamKeys.end(); ++teamKeysIt) {
		const LuaTable* teamTable = teamsTable->GetTblVal(*teamKeysIt);
		const vec4f& teamColor = teamTable->GetVec<vec4f>("color", 4);

		teamColors[teamIdx++] = teamColor;
	}

	PFFG_ASSERT(!teamColors.empty());
}

void CScene::LoadObjectModels() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");
	const LuaTable* modelsTable = rootTable->GetTblVal("models");

	const std::string modelDir = generalTable->GetStrVal("modelsDir", "data/models/");
	const std::string shaderDir = generalTable->GetStrVal("shadersDir", "data/shaders/");

	for (unsigned int n = 0; n < simObjectDefHandler->GetNumDefs(); n++) {
		SimObjectDef* def = simObjectDefHandler->GetDef(n);

		ModelBase* modelBase = def->GetModel();
		CModelReaderBase* modelReader = NULL;

		PFFG_ASSERT(modelBase == NULL);

		std::string mdlName    = def->GetModelName();
		std::string mdlFile    = modelDir + mdlName;
		std::string mdlFileExt = &mdlFile.data()[mdlFile.size() - 3];

		if (mdlFileExt == "s3o") { modelReader = readerS3O; }
		if (mdlFileExt == "3do") { modelReader = NULL; }

		PFFG_ASSERT(modelReader != NULL);
		modelBase = modelReader->Load(mdlFile);

		{
			const LuaTable* mdlTable = modelsTable->GetTblVal(mdlName);

			// load the model shader program
			const std::string vShaderFileS3O = mdlTable->GetStrVal("vShader", "");
			const std::string fShaderFileS3O = mdlTable->GetStrVal("fShader", "");
			const std::string vShaderPathS3O = shaderDir + vShaderFileS3O;
			const std::string fShaderPathS3O = shaderDir + fShaderFileS3O;

			if (!vShaderFileS3O.empty() || !fShaderFileS3O.empty()) {
				if (!vShaderFileS3O.empty()) { LOG << "[CScene] loading vert. shader " << vShaderPathS3O << " for model " << mdlFile << "\n"; }
				if (!fShaderFileS3O.empty()) { LOG << "[CScene] loading frag. shader " << fShaderPathS3O << " for model " << mdlFile << "\n"; }

				Shader::IProgramObject* pObj = shaderHandler->CreateProgramObject("[S3O]", "S3OShader", vShaderPathS3O, "", fShaderPathS3O, "", false);

				LOG << "[CScene] shader program object info\n";
				LOG << "\tpObj->GetObjID(): " << pObj->GetObjID() << "\n";
				LOG << "\tpObj->IsValid():  " << pObj->IsValid() << "\n";

				if (pObj->IsValid()) {
					modelBase->SetShaderProgramObj(pObj);

					pObj->SetUniformLocation("teamColor");  // idx 0
					pObj->SetUniformLocation("diffuseMap"); // idx 1
					pObj->SetUniformLocation("shadowMap");  // idx 2
					pObj->SetUniformLocation("shadowMat");  // idx 3
					pObj->SetUniformLocation("viewMat");    // idx 4

					pObj->Enable();
					pObj->SetUniform1i(1, 0);               // (idx 1, texunit 0)
					pObj->SetUniform1i(2, 7);               // (idx 2, texunit 7)
					pObj->Disable();
				} else {
					PFFG_ASSERT(false);
				}

				LOG << "\n";
				LOG << "\tpObj.GetLog():\n";
				LOG << pObj->GetLog() << "\n";

				const std::vector<const Shader::IShaderObject*>& sObjs = pObj->GetAttachedShaderObjs();

				for (std::vector<const Shader::IShaderObject*>::const_iterator it = sObjs.begin(); it != sObjs.end(); it++) {
					LOG << "\n" << ((*it)->GetLog()) << "\n";
				}
			}
		}

		def->SetModel(modelBase);
	}


	// load models for the initial objects
	const std::set<unsigned int>& simObjectIDs = simObjectHandler->GetSimObjectUsedIDs();

	for (std::set<unsigned int>::const_iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); ++it) {
		LoadObjectModel(*it);
	}
}

void CScene::LoadObjectModel(unsigned int objectID) {
	SimObject* obj = simObjectHandler->GetSimObject(objectID);
	ModelBase* objMdl = obj->GetDef()->GetModel();

	// NOTE: can't do it this way, all objects share the same shader
	// Shader::IProgramObject* objShader = const_cast<Shader::IProgramObject*>(objMdl->GetShaderProgramObj());
	// objShader->SetUniform4fv(0, const_cast<float*>( &teamColors[ obj->GetTeamID() ].x ));

	obj->SetModel(new LocalModel(objMdl));
	obj->SetRadius(objMdl->radius);
}



bool CScene::WantsEvent(int eventType) const {
	return (eventType == EVENT_SIMOBJECT_CREATED || eventType == EVENT_SIMOBJECT_DESTROYED);
}

void CScene::OnEvent(const IEvent* e) {
	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			// all object-defs are loaded at this point
			LoadObjectModel(objectID);
		} break;

		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			SimObject* obj = simObjectHandler->GetSimObject(objectID);
			delete (obj->GetModel());
			obj->SetModel(NULL);
		} break;

		default: {
			PFFG_ASSERT(false);
		}
	}
}

void CScene::DrawModels(Camera* eye, bool inShadowPass) {
	VertexArray tracerVA;

	glPushAttrib(GL_POLYGON_BIT);
		#ifdef WIREFRAME_DRAW
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		#endif

		// S3O models have reversed face-windings wrt. the OGL default
		// glFrontFace(GL_CW);
		glCullFace(GL_FRONT);

		if (!inShadowPass) {
			glEnable(GL_TEXTURE_2D);
		}

		const std::set<unsigned int>& simObjectIDs = simObjectHandler->GetSimObjectUsedIDs();

		for (std::set<unsigned int>::const_iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); ++it) {
			const SimObject* obj = simObjectHandler->GetSimObject(*it);

			if (!eye->InView(obj->GetPos())) {
				continue;
			}

			const std::list<PhysicalState>& objPrevPhysStates = obj->GetPrevPhysicalStates();

			const mat44f& objMat = obj->GetMat();
			const LocalModel* objLM = obj->GetModel();
			const ModelBase*  objMB = objLM->GetModelBase();

			// interpolate the draw-position between sim-frames
			// NOTE:
			//  we add a vertical offset so we can inspect
			//  the CC density-texture (large models block
			//  all texels)
			const vec3f offsetPos = vec3f(0.0f, obj->GetRadius(), 0.0f);
			const vec3f objRenderPos =
				objMat.GetPos() +
				(objMat.GetZDir() * obj->GetPhysicalState().speed * simFrameDeltaTickRatio) +
				offsetPos;
			const mat44f objRenderMat(objRenderPos, objMat.GetXDir(), objMat.GetYDir(), objMat.GetZDir());
			const vec4f& objCol = teamColors[obj->GetTeamID() % teamColors.size()];

			Shader::IProgramObject* shObj = const_cast<Shader::IProgramObject*>(objMB->GetShaderProgramObj());

			if (!inShadowPass) {
				shObj->Enable();
					shObj->SetUniformMatrix4fv(3, false, const_cast<float*>(shadowHandler->GetShadowProjectionMatrix()));
					shObj->SetUniformMatrix4fv(4, false, const_cast<float*>(eye->GetViewMatrix()));
					shObj->SetUniform4f(0, objCol.x, objCol.y, objCol.z, objCol.w);

					glActiveTexture(GL_TEXTURE7);
					glBindTexture(GL_TEXTURE_2D, shadowHandler->GetDepthTextureID());

					glPushMatrix();
						glMultMatrixf(objRenderMat.m);
						objMB->texturer->Bind(objMB);
						objMB->drawer->Draw(objMB->rootPiece);
						objMB->texturer->UnBind();
					glPopMatrix();

					glBindTexture(GL_TEXTURE_2D, 0);
					glActiveTexture(GL_TEXTURE0);
				shObj->Disable();


				if (!objPrevPhysStates.empty()) {
					glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_ALPHA_TEST);
					glColor4f(objCol.x, objCol.y, objCol.z, 0.8f);

					tracerVA.Initialize();
					tracerVA.EnlargeArrays(objPrevPhysStates.size() * 2, 0, VA_SIZE_N);

					for (std::list<PhysicalState>::const_iterator it = objPrevPhysStates.begin(); it != objPrevPhysStates.end(); ++it) {
						const mat44f& tmat = (*it).mat;
						const vec3f&  tpos = tmat.GetPos();
						const vec3f&  tdir = tmat.GetXDir();

						const vec3f v0 = (tpos - tdir * readMap->SQUARE_SIZE) + offsetPos;
						const vec3f v1 = (tpos + tdir * readMap->SQUARE_SIZE) + offsetPos;

						tracerVA.AddVertexN(v0, YVECf);
						tracerVA.AddVertexN(v1, YVECf);
					}

					tracerVA.DrawArrayN(GL_QUAD_STRIP);
					glPopAttrib();
				}
			} else {
				glPushMatrix();
					glMultMatrixf(objRenderMat.m);
					objMB->drawer->Draw(objMB->rootPiece);
				glPopMatrix();
			}
		}

		if (!inShadowPass) {
			glDisable(GL_TEXTURE_2D);
		}

		#ifdef WIREFRAME_DRAW
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		#endif
	glPopAttrib();
}



void CScene::DrawMapAndModels(Camera* eye, bool shadows) {
	if (!shadows) {
		eye->ApplyViewProjTransform();
		skyBox->Draw(eye);

		#ifdef CUSTOM_SMF_RENDERER
		smfRenderer->Render(eye);
		#else
		readMap->GetGroundDrawer()->Draw(eye, false);
		#endif

		DrawModels(eye, false);
	} else {
		glPushAttrib(GL_ENABLE_BIT);
			// for the shadow pass, don't want to
			// have lighting or texturing enabled
			//
			// pointless to re-enable afterwards,
			// shaders need to compute lighting
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
			glDisable(GL_COLOR_MATERIAL);

			// first pass, draw everything into the depth-buffer
			sun->ApplyViewProjTransform();
				shadowHandler->GenShadowProjectionMatrix(sun);
				shadowHandler->BindDepthTextureFBO();
					// draw raw geometry to the FBO-attached depth-texture from the light's
					// POV; note that this only works well if sun->zNear is >= ~1024 (which
					// brings its own issues; namely excessive clipping unless sun is placed
					// such that entire map falls within view frustum)
					glPushMatrix();
						#ifdef CUSTOM_SMF_RENDERER
						smfRenderer->Render(sun);
						#else
						readMap->GetGroundDrawer()->Draw(sun, true);
						#endif
						DrawModels(sun, true);
					glPopMatrix();
				shadowHandler->UnBindDepthTextureFBO();

			eye->ApplyViewProjTransform();
				skyBox->Draw(eye);

				// draw the objects with full lighting & texturing state
				// to onscreen texture from camera POV; shader reads the
				// depth texture to cast shadows
				glCullFace(GL_BACK);

				#ifndef CUSTOM_SMF_RENDERER
					glPushMatrix();
						readMap->GetGroundDrawer()->Draw(eye, false);
					glPopMatrix();
				#else
					smfRenderer->Render(eye);
				#endif

				DrawModels(eye, false);
				// shadowHandler->DrawDepthTexture();
		glPopAttrib();
	}
}



void CScene::Draw(Camera* eye) {
	if (sThread->GetFrame() != currSimFrame) {
		currSimFrame     = sThread->GetFrame();
		currSimFrameTick = SDL_GetTicks();

		simFrameDeltaTickRatio = ((SDL_GetTicks() - currSimFrameTick) / simFrameIntervalTicks);
	}

	// DrawMapAndModels(eye, false);
	DrawMapAndModels(eye, true);
}



void CScene::InitLight(void) {
	const GLfloat* ambientLightCol  = (GLfloat*) &mapInfo->light.groundAmbientColor.x;
	const GLfloat* diffuseLightCol  = (GLfloat*) &mapInfo->light.groundDiffuseColor.x;
	const GLfloat* specularLightCol = (GLfloat*) &mapInfo->light.groundSpecularColor.x;
	const GLfloat* lightDir         = (GLfloat*) &mapInfo->light.sunDir.x;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
		glLoadIdentity();
		// if we multiply by the view-matrix here, the shaders
		// don't need to transform the dir with gl_NormalMatrix
		//
		// glMultMatrixf(camCon->GetCurrCam()->GetViewMatrix());
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLightCol);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLightCol);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specularLightCol);
		glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
	glPopMatrix();


	// sun-dir initially points toward map-origin, not map-center
	const vec3f sunVRP = vec3f((readMap->mapx * readMap->SQUARE_SIZE) * 0.5f, 0.0f, (readMap->mapy * readMap->SQUARE_SIZE) * 0.5f);
	const vec3f sunPos = (mapInfo->light.sunDir * (boundingRadius * 2.0f) * 1.25f) + sunVRP;

	sun = new FPSCamera(sunPos, sunVRP, Camera::CAM_PROJ_MODE_ORTHO);
	sun->zNearDistance = 2.0f;
	sun->zFarDistance *= 2.0f;

	// note: the sun-cam does not get registered with the camera
	// controller, but is present in the input-receiver list so
	// we disable it here
	sun->DisableInput();
}

void CScene::SetBoundingRadius() {
	boundingRadiusSq =
		(readMap->mapx * readMap->mapx * 0.5f * 0.5f * readMap->SQUARE_SIZE * readMap->SQUARE_SIZE) +
		(readMap->mapy * readMap->mapy * 0.5f * 0.5f * readMap->SQUARE_SIZE * readMap->SQUARE_SIZE);
	boundingRadius = fastmath::sqrt1(boundingRadiusSq);
}

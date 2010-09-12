#define SHADOWS_BASIC


uniform sampler2D diffuseMap;
uniform sampler2D overlayMap;

#ifdef SHADOWS_BASIC
uniform sampler2D shadowMap;
#else
uniform sampler2DShadow shadowMap; // for PCF
#endif

varying vec3 lightDir;
varying vec3 halfDir;
varying vec3 vertexNormal;
varying vec4 vertexDepthTexCoors;



void main() {
	vec4 vertexDepthTexCoorsNorm = vertexDepthTexCoors / vertexDepthTexCoors.w;
	vec4 vertexDepthTexCoorsOffset;
	vec2 vertexDiffuseTexCoors = gl_TexCoord[0].st;


	bool vertexInShadow = false;
	float pixelDistanceFromLight = 0.0;
	float colorScalar = 1.0;
	float xPixelOffset = (1.0 / (1024.0 * 4.0));
	float yPixelOffset = (1.0 / ( 768.0 * 4.0));

	/*
	#ifdef SHADOWS_BASIC
		pixelDistanceFromLight = texture2D(shadowMap, vertexDepthTexCoorsNorm.st).z;

		if (vertexDepthTexCoors.w > 0.0) {
	#else
		if (vertexDepthTexCoors.w > 1.0) {
	#endif
			#ifdef SHADOWS_BASIC
				// reduce Moiré patterns and self-shadowing
				// (similar to the glPolygonOffset trick)
				vertexDepthTexCoorsNorm.z += 0.00005;
				pixelDistanceFromLight    += 0.02;

				vertexInShadow = (vertexDepthTexCoorsNorm.z > pixelDistanceFromLight);
				colorScalar = (vertexInShadow? 0.333: 1.0);
			#else
				// PCF-based SM (hardware can do this natively: VSM)
				colorScalar = 0.0;
				vertexDepthTexCoorsOffset.z = 0.05;
				vertexDepthTexCoorsOffset.w = 0.0;

				for (float y = -3.5; y <= 3.5; y += 1.0) {
					for (float x = -3.5; x <= 3.5; x += 1.0) {
						#ifndef USE_SHADOW2DPROJ
							// PCF-based, but basic depth comparison
							vertexDepthTexCoorsOffset.x  = x * xPixelOffset;
							vertexDepthTexCoorsOffset.y  = y * yPixelOffset;
							vertexDepthTexCoorsNorm.z   += 0.00005;
							pixelDistanceFromLight       = texture2D(shadowMap, vertexDepthTexCoorsNorm.st + vertexDepthTexCoorsOffset.xy).z;
							pixelDistanceFromLight      += 0.02;

							vertexInShadow = (vertexDepthTexCoorsNorm.z > pixelDistanceFromLight);
							colorScalar += (vertexInShadow? 0.333: 1.0);
						#else
							vertexDepthTexCoorsOffset.x = x * xPixelOffset * vertexDepthTexCoors.w;
							vertexDepthTexCoorsOffset.y = y * yPixelOffset * vertexDepthTexCoors.w;

							colorScalar += shadow2DProj(shadowMap, vertexDepthTexCoors + vertexDepthTexCoorsOffset).w;
						#endif
					}
				}

				colorScalar /= 64.0;
			#endif
	}
	*/

	float cosAngleDiffuse = max(dot(normalize(lightDir), normalize(vertexNormal)), 0.0);
	float cosAngleSpecular = max(dot(normalize(halfDir), normalize(vertexNormal)), 0.0);
	float specExp = 8.0;
	float specMult = (/*cosAngleDiffuse == 0.0 ||*/ colorScalar < 1.0)? 0.0: 1.0;

	// remove the specular contribution
	// for fragments that are in shadow
	// or have a zero diffuse component
	gl_FragColor =
		gl_LightSource[0].ambient +
		gl_LightSource[0].diffuse * texture2D(diffuseMap, vertexDiffuseTexCoors) * cosAngleDiffuse * colorScalar +
		gl_LightSource[0].specular * pow(cosAngleSpecular, specExp) * specMult * 0.1;
	gl_FragColor += texture2D(overlayMap, gl_TexCoord[1].st);
}

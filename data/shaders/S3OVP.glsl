uniform mat4 shadowMat;
uniform mat4 viewMat;
varying vec4 vertexDepthTexCoors;
varying vec3 vertexNormal;
varying vec3 lightDir;
varying vec3 viewDir;
varying vec3 halfDir;

void main() {
	vertexDepthTexCoors = shadowMat * gl_Vertex;
	vertexNormal = gl_NormalMatrix * gl_Normal;

	mat4 modelMat = viewMat * gl_ModelViewMatrixInverse;
	vec4 vertexPos = modelMat * gl_Vertex;

	lightDir = normalize(gl_LightSource[0].position.xyz);                 // WS
	lightDir.x = -lightDir.x;
	viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0)); // OS
	viewDir = normalize(viewDir - vertexPos.xyz);                         // OS - WS
	halfDir = normalize(lightDir + viewDir);                              // WS + OS

	gl_Position = ftransform();
	gl_FrontColor = gl_Color;

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;
	gl_TexCoord[2] = gl_MultiTexCoord2;
}

uniform mat4 shadowMat;
uniform int texSquareX;
uniform int texSquareZ;
varying vec4 vertexDepthTexCoors;
varying vec3 vertexNormal;
varying vec3 lightDir;
varying vec3 viewDir;
varying vec3 halfDir;

void main() {
	vertexDepthTexCoors = shadowMat * gl_Vertex;
	vertexNormal = /* gl_NormalMatrix * */ gl_Normal;

	lightDir = /*gl_NormalMatrix * */ normalize(gl_LightSource[0].position.xyz);
	viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = /* gl_NormalMatrix * */ normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir + viewDir);

	gl_Position = ftransform();
	// gl_Position = shadowMat * gl_Vertex;
	gl_FrontColor = gl_Color;

	gl_TexCoord[0].s = float(int(gl_Vertex.x) - (texSquareX * 1024)) / 1024.0;
	gl_TexCoord[0].t = float(int(gl_Vertex.z) - (texSquareZ * 1024)) / 1024.0;
}

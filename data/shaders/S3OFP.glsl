uniform vec4 teamColor;

uniform sampler2D diffuseMap;
uniform sampler2D shadowMap;

varying vec3 lightDir;
varying vec3 halfDir;
varying vec3 vertexNormal;
varying vec4 vertexDepthTexCoors;

void main() {
	vec2 vertexDiffuseTexCoors = gl_TexCoord[0].st;
	vec4 vertexDepthTexCoorsNorm = vertexDepthTexCoors / vertexDepthTexCoors.w;

	float pixelDistanceFromLight = texture2D(shadowMap, vertexDepthTexCoorsNorm.st).z;
	float colorScalar = 1.0;
	bool vertexInShadow = false;

	if (vertexDepthTexCoors.w > 0.0) {
		// reduce Moiré patterns and self-shadowing
		// (similar to the glPolygonOffset trick)
		vertexDepthTexCoorsNorm.z += 0.00005;
		pixelDistanceFromLight += 0.02;
		vertexInShadow = (vertexDepthTexCoorsNorm.z > pixelDistanceFromLight);
		colorScalar = (vertexInShadow? 0.333: 1.0);
	}


	vec4 diffuseColor = texture2D(diffuseMap, vertexDiffuseTexCoors);

	float cosAngleDiffuse = max(dot(normalize(lightDir), normalize(vertexNormal)), 0.0);
	float cosAngleSpecular = max(dot(normalize(halfDir), normalize(vertexNormal)), 0.0);
	float teamColorScale = diffuseColor.a;
	float specExp = 16.0;
	float specMult = (/*cosAngleDiffuse == 0.0 ||*/ colorScalar < 1.0)? 0.0: 1.0;

	gl_FragColor =
		gl_LightSource[0].ambient +
		gl_LightSource[0].diffuse * diffuseColor * cosAngleDiffuse * colorScalar +
		gl_LightSource[0].specular * pow(cosAngleSpecular, specExp) * specMult;
	gl_FragColor.rgb = mix(gl_FragColor.rgb, teamColor.rgb, teamColorScale);
}

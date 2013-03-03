varying vec3 lightVec;
varying vec3 eyeVec;
varying vec2 texCoord;
uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform sampler2D heightMap;
uniform float invRadius;

void main(void)
{
	float distSqr = dot(lightVec, lightVec);
	float att = clamp(1.0 - invRadius * sqrt(distSqr), 0.0, 1.0);
	vec3 lVec = lightVec * inversesqrt(distSqr);
	
	vec3 vVec = normalize(eyeVec);
	
	float h = texture2D(heightMap, texCoord).x - 0.5;
	vec2 offset = vec2(0.1 * h, 0.1 * h) * vec2( vVec.x, vVec.y );
	vec4 base = texture2D(colorMap, texCoord + offset);
	vec3 bump = normalize( texture2D(normalMap, texCoord).xyz * 2.0 - vec3(1,1,1));
	
	vec4 vAmbient = vec4(0.3,0.3,0.3,1);
	
	float diffuse = max( dot(lVec, bump), 0.0 );
	
	vec4 vDiffuse = gl_LightSource[0].diffuse * diffuse;	
	
	float specular = pow(clamp(dot(reflect(-lVec, bump), vVec), 0.0, 1.0), 
	gl_FrontMaterial.shininess );
	
	vec4 vSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * 
	specular;
	
	gl_FragColor =  vec4(2,2,2,1) * ( vAmbient * base + vDiffuse * base);
}
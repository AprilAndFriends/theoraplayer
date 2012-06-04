varying vec3 normal;
varying vec4 pos;
uniform sampler2D diffuseMap;

void main()
{
	vec4 color = gl_FrontMaterial.diffuse;
	vec4 matspec = gl_FrontMaterial.specular;
	float shininess = gl_FrontMaterial.shininess;
	vec4 lightspec = gl_LightSource[0].specular;
	vec4 lpos = gl_LightSource[0].position;
	vec4 s = -normalize(pos-lpos); 
	
	vec3 light = s.xyz;
	vec3 n = normalize(normal);
	vec3 r = -reflect(light, n);
	r = normalize(r);
	vec3 v = -pos.xyz;
	v = normalize(v);
    
	vec3 tex = texture2D(diffuseMap, gl_TexCoord[0].st).xyz * vec3(3,3,3);

	vec4 diffuse  = vec4(tex, 1) * max(0.3, dot(n, s.xyz)) * gl_LightSource[0].diffuse;
	
	gl_FragColor = diffuse;
}
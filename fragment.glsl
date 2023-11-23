#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor; 
uniform vec3 objectColor;

void main ()
{
	float ambientLight = 0.3f;
	vec3 ambient = ambientLight * lightColor * objectColor; 

	vec3 normalVector = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos); 

	float diffuseLight = max (dot (normalVector, lightDir), 0.0); 

	vec3 diffuse = diffuseLight * lightColor * objectColor; 

	vec3 result = ambient + diffuse; 
	FragColor = vec4 (result, 1.0); 
}
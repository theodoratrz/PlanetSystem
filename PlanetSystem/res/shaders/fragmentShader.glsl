#version 330 core
out vec4 FragColor;

in vec3 FragPos;      // Fragment position in world space
in vec3 Normal;       // Original normal vector
in vec2 TexCoords;    // Texture coordinates

uniform vec3 viewPos;  // Camera position in world space
uniform vec3 lightPos; // Sun position in world space
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform int planet;

uniform sampler2D textureSampler;
uniform sampler2D bumpSampler;
uniform sampler2D darktextureSampler;

void main()
{
  vec3 lightDir;
  vec3 texColor;
  vec3 result;
  vec3 perturbedNormal;
  vec3 diffuse;
  float diff;

  if(planet == 1){
    lightDir = normalize(lightPos);
    diff = max(dot(Normal, lightDir), 0.0);
    diffuse = diffuseStrength * diff * lightColor;
    texColor = texture(textureSampler, TexCoords).xyz;
    result = lightColor * texColor;
  }
  else if(planet == 2){
    lightDir = normalize(lightPos - FragPos);
    diff = max(dot(Normal, lightDir), 0.0);
    diffuse = diffuseStrength * diff * lightColor;
    if (any(greaterThan(diffuse, vec3(0.2)))) {
        texColor = texture(textureSampler, TexCoords).xyz;
    }
    else {
        texColor = texture(darktextureSampler, TexCoords).xyz;
    }
    result = (ambientStrength + diffuse) * texColor;
  }
  else if (planet == 3){
    // Sample the bump map to get the perturbed normal vector
    perturbedNormal = texture(bumpSampler, TexCoords).xyz * 2.0 - 1.0;
    perturbedNormal = normalize(perturbedNormal);

    lightDir = normalize(lightPos - FragPos);
    diff = max(dot(Normal, lightDir), 0.0);
    diffuse = diffuseStrength * diff * lightColor;

    texColor = texture(textureSampler, TexCoords).xyz;
    result = (ambientStrength + diffuse) * texColor;
  }
  

  FragColor = vec4(result, 1.0);
}
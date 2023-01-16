#version 330 core
//out vec4 FragColor;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    float ka = 0.2;
    FragColor = vec4(texture(skybox, TexCoords).xyz * ka, 1.0);

    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
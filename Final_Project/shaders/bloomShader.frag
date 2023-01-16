#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{             
    vec3 sceneColor = texture(scene, TexCoords).rgb;   
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    float exposure = 2.2f;
    vec3 total_color = sceneColor + bloomColor;
    total_color = vec3(1.0) - exp(-total_color * exposure);
    FragColor = vec4(total_color, 1.0);
}
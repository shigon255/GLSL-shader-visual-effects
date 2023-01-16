#version 330 core
//out vec4 FragColor;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main()
{   
    // Make the frame be exposed light to create a blur effect
    // Set the color over [0,1] range to fill in the BrightColor buffer
    vec4 bloomColor = vec4(10.0, 0.0, 0.0, 1.0);
    FragColor = bloomColor;
    BrightColor = bloomColor;
}
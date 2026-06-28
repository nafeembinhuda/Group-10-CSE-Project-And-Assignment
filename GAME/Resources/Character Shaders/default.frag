#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;


// Inputs the color from the Vertex Shader

in vec2 texCoord;

uniform sampler2D sprite;
uniform float uOffset;
uniform int numFrames;

void main()
{
    vec2 animTexCoord = vec2(texCoord.x * (1.0f / float(numFrames)) + uOffset, texCoord.y);
    FragColor = texture(sprite, animTexCoord);
}
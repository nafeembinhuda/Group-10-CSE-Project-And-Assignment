#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform vec2 uPos;
uniform vec2 uSize;
uniform float uUVOffset;
uniform float uUVWidth;

out vec2 texCoord;

void main()
{
    gl_Position = vec4(uPos + aPos * uSize, 0.0, 1.0);
    texCoord = vec2(aUV.x * uUVWidth + uUVOffset, aUV.y);
}
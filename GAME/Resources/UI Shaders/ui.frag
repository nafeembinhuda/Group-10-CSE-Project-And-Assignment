#version 330 core
in vec2 texCoord;
out vec4 FragColor;

uniform vec4      uColor;
uniform sampler2D uTex;
uniform bool      uUseTexture;

void main()
{
    if (uUseTexture)
        FragColor = texture(uTex, texCoord);
    else
        FragColor = uColor;
}
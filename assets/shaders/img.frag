#version 460 core

in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D tex0;
uniform float opacity;

void main()
{
	vec4 colour = texture2D(tex0, texCoord);
	FragColor = vec4(colour.rgb, colour.a * opacity);
}
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aBar;
layout (location = 3) in vec4 aBColor;
layout (location = 4) in vec4 aFColor;

out vec2 texCoord;
out vec3 bar;
out float xpos;
out vec4 bColor;
out vec4 fColor;

uniform mat4 transform;
uniform mat4 projection;

void main()
{
	xpos = aPos.x;
	bar = aBar;
	texCoord = aTexCoord;
	bColor = aBColor;
	fColor = aFColor;
	gl_Position = projection * transform * vec4(aPos, 1.0);
}
#version 460 core

in vec2 texCoord;
in vec3 bar;
in float xpos;
in vec4 bColor;
in vec4 fColor;

out vec4 FragColor;
uniform sampler2D tex0;
uniform float opacity;

void main()
{
	if (texture2D(tex0, texCoord).a < 0.1)
	{
		discard;
	}
	float begin = bar.x;
	float diff = bar.y;
	float value = bar.z;

	float fragment = xpos - begin;
	if (fragment > diff * (value / 100))
	{
		vec4 colour = texture2D(tex0, texCoord) * bColor;
		FragColor = vec4(colour.rgb, colour.a * opacity);
	}
	else
	{
		vec4 colour = texture2D(tex0, texCoord) * fColor;
		FragColor = vec4(colour.rgb, colour.a * opacity);
	}
}
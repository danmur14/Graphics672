#version 410 core

uniform vec4 colorMode; // distinct color
out vec4 fragmentColor; // output color

void main()
{
	fragmentColor = colorMode;
}


#version 110

// Set once
uniform sampler2D renderedTexture;

varying vec2 UV;

void main()
{
	gl_FragColor = texture2D(renderedTexture, UV);
}
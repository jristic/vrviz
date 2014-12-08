#version 110

varying vec2 UV;

void main()
{
	gl_Position = gl_Vertex;
	UV = (gl_Vertex.xy + 1.0) / 2.0;
}

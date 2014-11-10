#version 110

uniform float size;
uniform float theta;

void main()
{
	float x = gl_Vertex.x;
	float y = gl_Vertex.y;

	gl_Position = vec4(
		size * (x*cos(theta) - y*sin(theta)),
		size * (x*sin(theta) + y*cos(theta)),
		gl_Vertex.zw);
}

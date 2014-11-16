#version 110

// Set once
uniform float aspect;
// Set every frame
uniform int index;
uniform int frame;

#define PI 3.14159265359

void main()
{
	const float size = 0.1;
	const float rotate_interval = 70.0;
	float theta = (float(frame) / rotate_interval) * PI;

	float x = gl_Vertex.x;
	float y = gl_Vertex.y;

	const float orbit_interval = 300.0;
	const float orbit_x = 0.8;
	const float orbit_y = 0.3;
	float phi = (float(frame) / orbit_interval) * PI;
	vec4 offset = vec4( orbit_x * cos(phi), orbit_y * sin(phi), 0.0, 0.0 );

	gl_Position = 
		vec4(
			(x*cos(theta) - y*sin(theta)) * size,
			(x*sin(theta) + y*cos(theta)) * size * aspect,
			gl_Vertex.zw
		)
		+
		offset;
}

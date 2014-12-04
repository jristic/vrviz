#version 110

// Set once
uniform float aspect;
// Set every frame
uniform int index;
uniform int frame;

#define PI 3.14159265359

void main()
{
	const float rotate_interval = 70.0;
	float theta_offset = float(index) * 0.1;
	float theta = (float(frame) / rotate_interval - theta_offset) * PI;

	float x = gl_Vertex.x;
	float y = gl_Vertex.y;

	const float orbit_interval = 200.0;
	const float orbit_x = 0.8;
	const float orbit_y = 0.3;
	float phi_offset = -float(index) * 0.15;
	float phi = (float(frame) / orbit_interval - phi_offset) * PI;
	vec4 offset = vec4( orbit_x * cos(phi), orbit_y * sin(phi), 0.0, 0.0 );

	float size = 0.05 + ((sin(phi)+1.0)*0.015);

	gl_Position = 
		vec4(
			(x*cos(theta) + y*sin(theta)) * size,
			(-x*sin(theta) + y*cos(theta)) * size * aspect,
			gl_Vertex.zw)
		+
		offset;
}

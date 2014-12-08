#version 110

// Set every frame
uniform int digit;

void main()
{
	vec3 color = vec3(float(2-digit)*0.5, float(digit)*0.5, 1.0);

	gl_FragColor = vec4(color, 1.0);
}
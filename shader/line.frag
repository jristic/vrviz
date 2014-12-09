#version 110

// Set every frame
uniform int digit;

void main()
{
	vec3 color;
	if (digit <= 2)
		color = vec3(float(2-digit)*0.5, float(digit)*0.5, 1.0);
	else if (digit <= 5)
		color = vec3(1.0, float(5-digit)*0.5, float(digit-3)*0.5);
	else if (digit <= 8)
		color = vec3(float(digit-6)*0.5, float(8-digit)*0.5, 1.0);

	gl_FragColor = vec4(color, 1.0);
}
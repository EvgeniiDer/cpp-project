#version 330 core

layout(location = 0) in vec2 aPos;
void main()
{
	// Vertex shader code for grid rendering would go here
	gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

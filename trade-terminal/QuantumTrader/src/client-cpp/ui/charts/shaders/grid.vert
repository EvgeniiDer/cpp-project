#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 mvp_matrix;
void main()
{
	// Vertex shader code for grid rendering would go here
	gl_Position = mvp_matrix * vec4(aPos.x, aPos.y, 0.0, 1.0);
}

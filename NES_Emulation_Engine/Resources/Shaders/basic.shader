#shader vertex
#version 330 core

layout(location = 0) in vec4 position;

void main() {
	gl_Position = position;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 colour;

void main() {
	colour = vec4(0.2, 0.4, 0.8, 1.0);
};
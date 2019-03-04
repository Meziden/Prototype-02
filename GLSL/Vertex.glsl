#version 450 core

layout (location = 0) in vec3 VecPos;
layout (location = 1) in vec2 TexPosV;
out vec2 TexPos;
uniform mat4 trans_mod;
uniform mat4 trans_view;
uniform mat4 trans_proj;

void main()
{
	gl_Position = trans_proj * trans_view * trans_mod * vec4(VecPos,1.0);
	TexPos = vec2(TexPosV.x,1.0 - TexPosV.y);
}

uniform mat4 mvp;                       // model-view-projection matrix

// per vertex input attributes 
in vec3 vtx_position;            // object space position

void main(void)
{
    gl_Position = mvp * vec4(vtx_position, 1);
}

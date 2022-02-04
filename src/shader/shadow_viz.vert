in vec4 vtx_position;
in vec2 vtx_texcoord;

out vec2 vTexCoord;

void main() {
    vTexCoord = vtx_texcoord; 
	gl_Position = vtx_position;
}




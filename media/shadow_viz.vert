
varying vec2 vTexCoord;

void main() {
    vTexCoord = vec2(gl_MultiTexCoord0.x, gl_MultiTexCoord0.y); 
	gl_Position = gl_Vertex;
}




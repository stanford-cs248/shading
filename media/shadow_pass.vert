
attribute vec3 vtx_position;            // object space position

void main() {
   gl_Position = gl_ModelViewProjectionMatrix * vec4(vtx_position, 1);
}




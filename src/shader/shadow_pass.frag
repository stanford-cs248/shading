in vec3 normal_vec; 
out vec4 fragColor;
void main() {
   vec3 color = (normal_vec.xyz+1.0)/2.0;
   // gl_FragColor = vec4(1.0, 0, gl_FragCoord.z, 1.0);
   fragColor = vec4(color.x, color.y, color.z, 1.0);
}

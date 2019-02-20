uniform sampler2D myTexture;

varying vec2 vTexCoord;

void main(void) {

   // visualize depth
   float depth = texture2D(myTexture, vTexCoord).x;
   gl_FragColor = vec4(depth, depth, depth, 1.0); 

}
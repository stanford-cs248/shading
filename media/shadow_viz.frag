uniform sampler2DArray depthTextureArray;
uniform sampler2DArray colorTextureArray;

in vec2 vTexCoord;
out vec4 fragColor;

float linearize_depth(float depth)
{
    float near_plane = 10.0;
    float far_plane = 400.0;
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main(void) {

   float depth = texture(depthTextureArray, vec3(vTexCoord,0)).x;  // Using the first layer
   fragColor = vec4(depth, depth, depth, 1.0); // Grayscale depth image

   // vec3 color = texture(colorTextureArray, vec3(vTexCoord,0)).rgb;
   // fragColor = vec4(color, 1.0);
}

uniform mat4 obj2world;                 // object to world space transform
uniform mat4 obj2shadowlight0;          // object to light space transform for light 0
uniform mat4 obj2shadowlight1;          // object to light space transform for light 1
uniform mat3 obj2worldNorm;             // object to world transform for normals
uniform vec3 camera_position;           // world space camera position           


uniform bool useNormalMapping;         // true if normal mapping should be used

// per vertex input attributes 
attribute vec3 vtx_position;            // object space position
attribute vec3 vtx_tangent;
attribute vec3 vtx_normal;              // object space normal
attribute vec2 vtx_texcoord;
attribute vec3 vtx_diffuse_color; 

// per vertex outputs 
varying vec3 position;                  // world space position
varying vec4 position_shadowlight0;     // surface position in light space
varying vec4 position_shadowlight1;     // surface position in light space
varying vec3 normal;                    // either object space normal or world space
                                        // normal depending on whether normal mapping is used
varying vec3 vertex_diffuse_color;
varying vec2 texcoord;
varying vec3 dir2camera;                // world space vector from surface point to camera
varying mat3 tan2world;                 // tangent space rotation matrix multiplied by obj2WorldNorm

void main(void)
{

    position = vec3(obj2world * vec4(vtx_position, 1));

    //
    // TODO CS248: Part 4:
    //
    // Compute light-space surface position by multiplying object space position
    // (given by vtx_position) with the obj2shadowlight0 and obj2shadowlight1
    // transforms, placing result in position_shadowlight0
    // and position_shadowlight1 respectively
    //
    // Recall for shadow mapping we need to know the position of the surface relative
    // to each shadowed light source.  In this assignment you'll consider scenes with
    // up to two shadowed light sources.
    //

    if (useNormalMapping) {

       //
       // TODO CS248: PART 2: compute 3x3 tangent space to world space matrix here: tan2world
       //
       
       // Tips:
       //
       // (1) Make sure you normalize all columns of the matrix so that it is a rotation matrix.
       //
       // (2) You can initialize a 3x3 matrix using 3 vectors as shown below:
       // vec3 a, b, c;
       // mat3 mymatrix = mat3(a, b, c)

       // (3) obj2worldNorm is a 3x3 matrix transforming object space normals to world space normals

       
       // pass through object-space normal unmodified to fragment shader
       normal = vtx_normal;
       
    } else {
    
       // just transform normal into world space 
       normal = obj2worldNorm * vtx_normal; 
    }

    vertex_diffuse_color = vtx_diffuse_color;
    texcoord = vtx_texcoord;
    dir2camera = camera_position - position;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vtx_position, 1);

}

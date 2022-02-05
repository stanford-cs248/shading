uniform mat4 obj2world;                 // object to world transform
uniform mat3 obj2worldNorm;             // object to world transform for normals
uniform vec3 camera_position;           // world space camera position           

uniform mat4 mvp;                       // model-view-projection matrix

uniform bool useNormalMapping;         // true if normal mapping should be used

// per vertex input attributes 
in vec3 vtx_position;            // object space position
in vec3 vtx_tangent;
in vec3 vtx_normal;              // object space normal
in vec2 vtx_texcoord;
in vec3 vtx_diffuse_color; 

// per vertex outputs 
out vec3 position;                  // world space position
out vec3 vertex_diffuse_color;
out vec2 texcoord;
out vec3 dir2camera;                // world space vector from surface point to camera
out vec3 normal;
out mat3 tan2world;                 // tangent space rotation matrix multiplied by obj2WorldNorm

void main(void)
{
    position = vec3(obj2world * vec4(vtx_position, 1));

    // TODO CS248 Part3: Normal Mapping
    // compute 3x3 tangent space to world space matrix here: tan2world
    
       
    // Tips:
    //
    // (1) Make sure you normalize all columns of the matrix so that it is a rotation matrix.
    //
    // (2) You can initialize a 3x3 matrix using 3 vectors as shown below:
    // vec3 a, b, c;
    // mat3 mymatrix = mat3(a, b, c)
    // (3) obj2worldNorm is a 3x3 matrix transforming object space normals to world space normals
    // compute tangent space to world space matrix
    
    normal = obj2worldNorm * vtx_normal;

    vertex_diffuse_color = vtx_diffuse_color;
    texcoord = vtx_texcoord;
    dir2camera = camera_position - position;
    gl_Position = mvp * vec4(vtx_position, 1);
}

#ifndef CS248_DYNAMICSCENE_MESH_H
#define CS248_DYNAMICSCENE_MESH_H

#include "scene.h"

#include "../collada/polymesh_info.h"
#include "../shader.h"
#include "../gl_resource_manager.h"

#include <map>

namespace CS248 {
namespace DynamicScene {


// We need to define new vector structs for fp32 fields. In other words, using floats, not doubles. 
// Note that in the CS248 starter codebase the Vector2D/3D types have fields that are doubles.
struct Vector2Df {
    float x, y;
};

struct Vector3Df {
    float x, y, z;
};


class Mesh : public SceneObject {
  public:
    Mesh(Collada::PolymeshInfo& polyMesh, const Matrix4x4& transform);
    ~Mesh();

    void draw(const Matrix4x4& worldToNDC) const override;
    void drawShadow(const Matrix4x4& worldToNDC) const override;
    BBox getBBox() const override;
    void reloadShaders() override;

 private:

    // Helper called by draw() and drawShadow()
    void internalDraw(bool shadowPass, const Matrix4x4& worldToNDC) const;
      
    int numTriangles_;

    // Per vertex mesh data.  These allocations are the host-side buffers whose contents are
    // copied into OpenGL vertex buffers.  Note that we do not use an indexed format.  
    vector<Vector3Df> positionData_;
    vector<Vector3Df> diffuseColorData_;
    vector<Vector3Df> normalData_;
    vector<Vector2Df> texcoordData_;
    vector<Vector3Df> tangentData_;
    
    // (wrapped) OpenGL program object
    Shader* shader_;
    GLResourceManager* gl_mgr_;

    // OpenGL vertex array object
    VertexArrayId vertexArrayId_;

    // OpenGL vertex buffer objects
    VertexBufferId positionBufferId_;
    VertexBufferId diffuseColorBufferId_;
    VertexBufferId normalBufferId_;
    VertexBufferId texcoordBufferId_;
    VertexBufferId tangentBufferId_;
    
    // OpenGL texture objects
    TextureId diffuseTextureId_;
    TextureId normalTextureId_;
    TextureId environmentTextureId_;

    // will be passed as shader uniforms
    bool  doTextureMapping_;
    bool  doNormalMapping_;
    bool  doEnvironmentMapping_;
    bool  useMirrorBrdf_;
    float phongSpecExponent_;

};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_MESH_H

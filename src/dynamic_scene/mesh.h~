#ifndef CS248_DYNAMICSCENE_MESH_H
#define CS248_DYNAMICSCENE_MESH_H

#include "scene.h"

#include "../collada/polymesh_info.h"
#include "../shader.h"

#include <map>

namespace CS248 {
namespace DynamicScene {

// A structure for holding linear blend skinning information
class LBSInfo {
 public:
  Vector3D blendPos;
  double distance;
};

struct Vector2Df {
public:
  float x, y;
};

struct Vector3Df {
public:
	float x, y, z;
};

class Mesh : public SceneObject {
 public:
  Mesh(Collada::PolymeshInfo &polyMesh, const Matrix4x4 &transform, const std::string shader_prefix = "");

  ~Mesh();

  virtual void draw() override;

  void draw_pretty() override;

  StaticScene::SceneObject *get_transformed_static_object(double t) override;

  BBox get_bbox() override;

  StaticScene::SceneObject *get_static_object() override;

 private:
  // Helpers for draw().
  void draw_faces(bool smooth = false) const;

  // Texture map
  vector<unsigned char> diffuse_texture;
  vector<unsigned char> normal_texture;
  vector<unsigned char> environment_texture;
  vector<unsigned char> alpha_texture;
  vector<unsigned char> stub1_texture;
  vector<unsigned char> stub2_texture;
  vector<unsigned char> stub3_texture;
  unsigned int diffuse_texture_width, diffuse_texture_height;
  unsigned int normal_texture_width, normal_texture_height;
  unsigned int environment_texture_width, environment_texture_height;
  unsigned int alpha_texture_width, alpha_texture_height;
  unsigned int stub1_texture_width, stub1_texture_height;
  unsigned int stub2_texture_width, stub2_texture_height;
  unsigned int stub3_texture_width, stub3_texture_height;
  
  string vertex_shader_program;
  string fragment_shader_program;

  vector<vector<size_t>> polygons;
  vector<Collada::Polygon> polygons_carbon_copy;
  
  // Per v
  vector<Vector3Df> vertices;
  vector<Vector3Df> normals;
  vector<Vector2Df> texture_coordinates;
  vector<Vector3Df> tangentData;
  vector<Vector3Df> bitangents;
  
  // Per f
  vector<Vector3Df> diffuse_colors;
  
  // Packed
  vector<Vector3Df> vertexData;
  vector<Vector3Df> diffuse_colorData;
  vector<Vector3Df> normalData;
  vector<Vector2Df> texcoordData;

  vector<Shader> shaders;

  std::vector<std::string> uniform_strings;
  std::vector<float> uniform_values;

  float glObj2World[16];
  float glObj2WorldNorm[9];
    
  GLuint vao;
  
  GLuint vertexBuffer;
  GLuint diffuse_colorBuffer;
  GLuint normalBuffer;
  GLuint texcoordBuffer;
  GLuint tangentBuffer;
  
  GLuint diffuseId;
  GLuint diffuse_colorId;
  GLuint normalId;
  GLuint environmentId;
  GLuint alphaId;
  GLuint stub1Id;
  GLuint stub2Id;
  GLuint stub3Id;

  bool simple_renderable;
  bool simple_colors;
  bool do_texture_mapping;
  bool do_normal_mapping;
  bool do_environment_mapping;
  bool do_blending;
  bool do_disney_brdf;
};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_MESH_H

#ifndef CS248_COLLADA_MESHINFO_H
#define CS248_COLLADA_MESHINFO_H

#include "CS248/vector2D.h"

#include "collada_info.h"

namespace CS248 {
namespace Collada {

struct Polygon {
  std::vector<size_t> vertex_indices;    ///< indices into vertex array
  std::vector<size_t> normal_indices;    ///< indices into normal array
  std::vector<size_t> texcoord_indices;  ///< indices into texcoord array

};  // struct Polygon

typedef std::vector<Polygon> PolyList;
typedef PolyList::iterator PolyListIter;

struct Pattern {
  std::string handle;
  Vector3D value;
  bool is_texture;
}; // struct Pattern

struct PolymeshInfo : Instance {
  std::vector<Vector3D> vertices;   ///< polygon vertex array
  std::vector<Vector3D> normals;    ///< polygon normal array
  std::vector<Vector2D> texcoords;  ///< texture coordinate array

  std::vector<Polygon> polygons;  ///< polygons

  std::vector<std::string> material_names;  ///< material of the mesh (simply for parsing)
  std::vector<Vector3D> material_diffuse_values;  ///< material of the mesh (simply for parsing)

  std::vector<Vector3D> material_diffuse_parameters;  ///< material of the mesh

  std::vector<std::string> uniform_strings;
  std::vector<float> uniform_values;

  std::string diffuse_filename;  ///< diffuse texture map filename
  std::string normal_filename;  ///< normal texture map filename
  std::string environment_filename;  ///< environment texture map filename
  std::string alpha_filename;  ///< alpha texture map filename
  std::string stub1_filename;  ///< stub1 texture map filename
  std::string stub2_filename;  ///< stub2 texture map filename
  std::string stub3_filename;  ///< stub3 texture map filename

  bool  is_mirror_brdf;
  float phong_spec_exp;

  std::string vert_filename;  ///< vertex shader filename
  std::string frag_filename;  ///< fragment shader filename

  Vector3D position;  ///< translation part of the transformation
  Vector3D rotation;  ///< rotation part of the transformation
  Vector3D scale;  ///< scale part of the transformation

  bool is_obj_file;  ///< obj file type indicator
  bool is_mtl_file;  ///< mtl file type indicator

  bool is_disney;
};  // struct Polymesh

std::ostream& operator<<(std::ostream& os, const PolymeshInfo& polymesh);

}  // namespace Collada
}  // namespace CS248

#endif  // CS248_COLLADA_MESHINFO_H

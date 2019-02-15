#ifndef CS248_DYNAMICSCENE_SPHERE_H
#define CS248_DYNAMICSCENE_SPHERE_H

#include "scene.h"

#include "../collada/sphere_info.h"

namespace CS248 {
namespace DynamicScene {

class Sphere : public SceneObject {
 public:
  Sphere(const Collada::SphereInfo& sphereInfo, const Vector3D& position,
         const double scale);

  virtual void draw();

  BBox get_bbox();
  
  StaticScene::SceneObject* get_static_object();
  
 private:
  double r;
  Vector3D p;
};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_SPHERE_H

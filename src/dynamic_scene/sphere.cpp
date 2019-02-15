#include "sphere.h"

#include "../static_scene/object.h"

namespace CS248 {
namespace DynamicScene {

Sphere::Sphere(const Collada::SphereInfo& info, const Vector3D& position,
               const double scale)
    : p(position), r(info.radius * scale) {
}

void Sphere::draw() {
}

BBox Sphere::get_bbox() {
  return BBox(p.x - r, p.y - r, p.z - r, p.x + r, p.y + r, p.z + r);
}

StaticScene::SceneObject* Sphere::get_static_object() {
  return new StaticScene::SphereObject(p, r);
}

}  // namespace DynamicScene
}  // namespace CS248

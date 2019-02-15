#ifndef CS248_STATICSCENE_OBJECT_H
#define CS248_STATICSCENE_OBJECT_H

#include "scene.h"

namespace CS248 {
namespace StaticScene {

/**
 * A sphere object.
 */
class SphereObject : public SceneObject {
 public:
  /**
  * Constructor.
  * Construct a static sphere for rendering from given parameters
  */
  SphereObject(const Vector3D& o, double r) {
	  this->o = o;
	  this->r = r;
  }

  Vector3D o;  ///< origin
  double r;    ///< radius
};  // class SphereObject

}  // namespace StaticScene
}  // namespace CS248

#endif  // CS248_STATICSCENE_OBJECT_H

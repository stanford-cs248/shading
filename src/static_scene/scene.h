#ifndef CS248_STATICSCENE_SCENE_H
#define CS248_STATICSCENE_SCENE_H

#include "CS248/CS248.h"
#include "primitive.h"

#include <vector>

namespace CS248 {
namespace StaticScene {

/**
 * Interface for objects in the scene.
 */
class SceneObject {
};

/**
 * Interface for lights in the scene.
 */
class SceneLight {
 public:
  virtual bool is_delta_light() const = 0;
};

/**
 * Represents a scene. All data is already transformed to world space.
 */
struct Scene {
  Scene(const std::vector<SceneObject*>& objects,
        const std::vector<SceneLight*>& lights)
      : objects(objects), lights(lights) {}

  // kept to make sure they don't get deleted, in case the
  //  primitives depend on them (e.g. Mesh Triangles).
  std::vector<SceneObject*> objects;

  // for sake of consistency of the scene object Interface
  std::vector<SceneLight*> lights;
};

}  // namespace StaticScene
}  // namespace CS248

#endif  // CS248_STATICSCENE_SCENE_H

#ifndef CS248_DYNAMICSCENE_POINTLIGHT_H
#define CS248_DYNAMICSCENE_POINTLIGHT_H

#include "scene.h"
#include "../static_scene/light.h"

using std::cout;
using std::endl;

namespace CS248 {
namespace DynamicScene {

class PointLight : public SceneLight {
  public:
    PointLight(const Collada::LightInfo& light_info, const Matrix4x4& transform) {
        this->spectrum = light_info.spectrum;
        this->position = (transform * Vector4D(light_info.position, 1)).to3D();
    }

    StaticScene::SceneLight* getStaticLight() const {
        StaticScene::PointLight* l = new StaticScene::PointLight(spectrum, position);
        return l;
    }

   private:
      Spectrum spectrum;
      Vector3D position;
};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_POINTLIGHT_H

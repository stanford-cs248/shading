#ifndef CS248_DYNAMICSCENE_SPOTLIGHT_H
#define CS248_DYNAMICSCENE_SPOTLIGHT_H

#include "scene.h"
#include "../static_scene/light.h"

namespace CS248 {
namespace DynamicScene {

class SpotLight : public SceneLight {
  public:
    SpotLight(const Collada::LightInfo& light_info, const Matrix4x4& transform) {
        this->position = (transform * Vector4D(light_info.position, 1)).to3D();
        this->direction = (transform * Vector4D(light_info.direction, 1)).to3D();
        this->direction.normalize();
        this->cone_angle = light_info.falloff_deg;
        this->radiance = light_info.spectrum; 
    }

    StaticScene::SceneLight* getStaticLight() const {
        StaticScene::SpotLight* l = new StaticScene::SpotLight(radiance, position, direction, cone_angle);
        return l;
    }

   private:
      Spectrum radiance;
      Vector3D direction;
      Vector3D position;
      float    cone_angle;
};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_SPOTLIGHT_H

#ifndef CS248_DYNAMICSCENE_AMBIENTLIGHT_H
#define CS248_DYNAMICSCENE_AMBIENTLIGHT_H

#include "scene.h"
#include "../static_scene/light.h"

namespace CS248 {
namespace DynamicScene {

class AmbientLight : public SceneLight {
 public:
	AmbientLight(const Collada::LightInfo& light_info) {
		this->spectrum = light_info.spectrum;
  	}

  	StaticScene::SceneLight* getStaticLight() const {
		StaticScene::InfiniteHemisphereLight* l = new StaticScene::InfiniteHemisphereLight(spectrum);
	    return l;
  	}

 private:
  	Spectrum spectrum;
};

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_AMBIENTLIGHT_H

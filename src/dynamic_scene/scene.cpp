#include "scene.h"
#include "mesh.h"
#include <fstream>

using namespace std;
using std::cout;
using std::endl;

namespace CS248 {
namespace DynamicScene {

Scene::Scene(std::vector<SceneObject *> _objects,
             std::vector<SceneLight *> _lights) {
  for (int i = 0; i < _objects.size(); i++) {
    _objects[i]->scene = this;
    objects.insert(_objects[i]);
  }

  for (int i = 0; i < _lights.size(); i++) {
    lights.insert(_lights[i]);
  }

  for(SceneLight *slo : lights) {
    StaticScene::SceneLight *light = slo->get_static_light();
    if(dynamic_cast<StaticScene::DirectionalLight *>(light)) {
        directional_lights.push_back((StaticScene::DirectionalLight *)light);
    }
    if(dynamic_cast<StaticScene::InfiniteHemisphereLight *>(light)) {
        hemi_lights.push_back((StaticScene::InfiniteHemisphereLight *)light);
    }
    if(dynamic_cast<StaticScene::PointLight *>(light)) {
        point_lights.push_back((StaticScene::PointLight *)light);
    }
    if(dynamic_cast<StaticScene::SpotLight *>(light)) {
        spot_lights.push_back((StaticScene::SpotLight *)light);
    }
    if(dynamic_cast<StaticScene::AreaLight *>(light)) {
        area_lights.push_back((StaticScene::AreaLight *)light);
    }
    if(dynamic_cast<StaticScene::SphereLight *>(light)) {
        sphere_lights.push_back((StaticScene::SphereLight *)light);
    }
  }
  current_pattern_id = 0;
  current_pattern_subid = 0;
  scaling_factor = .05f;
}

Scene::~Scene() {
}

BBox Scene::get_bbox() {
  BBox bbox;
  for (SceneObject *obj : objects) {
    bbox.expand(obj->get_bbox());
  }
  return bbox;
}

bool Scene::addObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i != objects.end()) {
    return false;
  }

  o->scene = this;
  objects.insert(o);
  return true;
}

bool Scene::removeObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i == objects.end()) {
    return false;
  }

  objects.erase(o);
  return true;
}

void Scene::render_in_opengl() {
  // Renderpass 1
  for (SceneObject *obj : objects) {
    if (obj->isVisible) {
      obj->draw();
    }
  }

  // Renderpass 2
}

void Scene::prevPattern() {
    current_pattern_id = min(max((int)current_pattern_id - 1, (int)0), (int)patterns.size() - 1);
}

void Scene::nextPattern() {
    current_pattern_id = min(max((int)current_pattern_id + 1, (int)0), (int)patterns.size() - 1);
}

void Scene::decreaseCurrentPattern(double scale) {
    if(patterns.size() > 0) {
        int type = patterns[current_pattern_id].type;
        if(type == 0) {
            patterns[current_pattern_id].v[current_pattern_subid] -= scaling_factor;
        }
        else if(type == 1) {
            patterns[current_pattern_id].s -= scaling_factor;
            patterns[current_pattern_id].s = (patterns[current_pattern_id].s < 0.f) ? 0.f : patterns[current_pattern_id].s;
        }
    }
}

void Scene::increaseCurrentPattern(double scale) {
    if(patterns.size() > 0) {
        int type = patterns[current_pattern_id].type;
        if(type == 0)
            patterns[current_pattern_id].v[current_pattern_subid] += scaling_factor;
        else if(type == 1) {
            patterns[current_pattern_id].s += scaling_factor;
            patterns[current_pattern_id].s = (patterns[current_pattern_id].s > 1.0f) ? 1.0f : patterns[current_pattern_id].s;
        }
    }
}

StaticScene::Scene *Scene::get_static_scene() {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_static_object();
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }

  return new StaticScene::Scene(staticObjects, staticLights);
}

StaticScene::Scene *Scene::get_transformed_static_scene(double t) {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_transformed_static_object(t);
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }
  return new StaticScene::Scene(staticObjects, staticLights);
}

Matrix4x4 SceneObject::getTransformation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::translation(position) *
         Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z) *
         Matrix4x4::scaling(scale);
}

Matrix4x4 SceneObject::getRotation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z);
}

}  // namespace DynamicScene
}  // namespace CS248

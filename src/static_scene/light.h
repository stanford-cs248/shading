#ifndef CS248_STATICSCENE_LIGHT_H
#define CS248_STATICSCENE_LIGHT_H

#include "CS248/vector3D.h"
#include "CS248/matrix3x3.h"
#include "CS248/spectrum.h"

#include "scene.h"   // SceneLight
#include "object.h"

namespace CS248 {
namespace StaticScene {

// Directional Light //

class DirectionalLight : public SceneLight {
 public:
  DirectionalLight(const Spectrum& rad, const Vector3D& lightDir);
  bool is_delta_light() const { return true; }

  Spectrum radiance;
  Vector3D lightDir;

};  // class Directional Light

// Infinite Hemisphere Light //

class InfiniteHemisphereLight : public SceneLight {
 public:
  InfiniteHemisphereLight(const Spectrum& rad);
  bool is_delta_light() const { return false; }

  Spectrum radiance;
  Matrix3x3 sampleToWorld;

};  // class InfiniteHemisphereLight

// Point Light //

class PointLight : public SceneLight {
 public:
  PointLight(const Spectrum& rad, const Vector3D& pos);
  bool is_delta_light() const { return true; }

  Spectrum radiance;
  Vector3D position;

};  // class PointLight

// Spot Light //

class SpotLight : public SceneLight {
 public:
  SpotLight(const Spectrum& rad, const Vector3D& pos, const Vector3D& dir,
            float cone_angle);
  bool is_delta_light() const { return true; }

  void rotate(float delta);

  Spectrum radiance;
  Vector3D position;
  Vector3D direction;
  float angle;
  float delta;

};  // class SpotLight

// Area Light //

class AreaLight : public SceneLight {
 public:
  AreaLight(const Spectrum& rad, const Vector3D& pos, const Vector3D& dir,
            const Vector3D& dim_x, const Vector3D& dim_y);
  bool is_delta_light() const { return false; }

  Spectrum radiance;
  Vector3D position;
  Vector3D direction;
  Vector3D dim_x;
  Vector3D dim_y;
  float area;

};  // class AreaLight

// Sphere Light //

class SphereLight : public SceneLight {
 public:
  SphereLight(const Spectrum& rad, const SphereObject* sphere);
  bool is_delta_light() const { return false; }

  const SphereObject* sphere;
  Spectrum radiance;

};  // class SphereLight

}  // namespace StaticScene
}  // namespace CS248

#endif  // CS248_STATICSCENE_BSDF_H

#include "light.h"

#include <iostream>

namespace CS248 {
namespace StaticScene {

// Directional Light //

DirectionalLight::DirectionalLight(const Spectrum& rad,
                                   const Vector3D& lightDir)
    : radiance(rad) {
  dirToLight = -lightDir.unit();
}

// Infinite Hemisphere Light //

InfiniteHemisphereLight::InfiniteHemisphereLight(const Spectrum& rad)
    : radiance(rad) {
  sampleToWorld[0] = Vector3D(1, 0, 0);
  sampleToWorld[1] = Vector3D(0, 0, -1);
  sampleToWorld[2] = Vector3D(0, 1, 0);
}

// Point Light //

PointLight::PointLight(const Spectrum& rad, const Vector3D& pos)
    : radiance(rad), position(pos) {}

// Spot Light //

SpotLight::SpotLight(const Spectrum& rad, const Vector3D& pos,
                     const Vector3D& dir, float angle) {}

// Area Light //

AreaLight::AreaLight(const Spectrum& rad, const Vector3D& pos,
                     const Vector3D& dir, const Vector3D& dim_x,
                     const Vector3D& dim_y)
    : radiance(rad),
      position(pos),
      direction(dir),
      dim_x(dim_x),
      dim_y(dim_y),
      area(dim_x.norm() * dim_y.norm()) {}

// Sphere Light //

SphereLight::SphereLight(const Spectrum& rad, const SphereObject* sphere) {}

}  // namespace StaticScene
}  // namespace CS248

#include "light.h"

#include "CS248/matrix4x4.h"
#include "CS248/vector3D.h"
#include <iostream>

namespace CS248 {
namespace StaticScene {

// Directional Light //

DirectionalLight::DirectionalLight(const Spectrum& rad,
                                   const Vector3D& lightingDir)
    : radiance(rad) {
  lightDir = lightingDir.unit();
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
                     const Vector3D& dir, float cone_angle)
{
    radiance = rad;
    position = pos;
    direction = dir;
    angle = cone_angle;
}

void SpotLight::rotate(float delta) {
  // rotate around Y axis
  Matrix4x4 r = Matrix4x4::rotation(delta, Matrix4x4::Axis::Y);
  Vector3D newpos = r * position;
  // Keep looking at the target on XZ plane.
  Vector3D targetXZ = position - ((position.y / direction.y)*direction);
  direction = targetXZ - newpos;
  direction.normalize();
  position = newpos;
}

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

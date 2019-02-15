#ifndef CS248_COLLADA_CAMERAINFO_H
#define CS248_COLLADA_CAMERAINFO_H

#include "collada_info.h"

namespace CS248 {
namespace Collada {

/*
  Note that hFov_ and vFov_ are expected to be in DEGREES.
*/
struct CameraInfo : public Instance {
  Vector3D pos;
  Vector3D view_dir;
  Vector3D up_dir;

  float hFov, vFov, nClip, fClip;

  bool default_flag;
};

std::ostream& operator<<(std::ostream& os, const CameraInfo& camera);

}  // namespace Collada
}  // namespace CS248

#endif  // CS248_COLLADA_CAMERAINFO_H

#ifndef CS248_COLLADA_SPHEREINFO_H
#define CS248_COLLADA_SPHEREINFO_H

#include "collada_info.h"

namespace CS248 {
namespace Collada {

struct SphereInfo : Instance {
  float radius;            ///< radius
};                         // struct Sphere

std::ostream& operator<<(std::ostream& os, const SphereInfo& sphere);

}  // namespace Collada
}  // namespace CS248

#endif  // CS248_COLLADA_SPHEREINFO_H

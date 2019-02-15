#include "sphere_info.h"

using namespace std;

namespace CS248 {
namespace Collada {

std::ostream& operator<<(std::ostream& os, const SphereInfo& sphere) {
  return os << "Sphere: " << sphere.name << " (id:" << sphere.id << ") ["
            << " radius=" << sphere.radius << " ]";
}

}  // namespace Collada
}  // namespace CS248

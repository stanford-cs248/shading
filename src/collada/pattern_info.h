#ifndef CS248_COLLADA_PATTERNINFO_H
#define CS248_COLLADA_PATTERNINFO_H

#include "collada_info.h"

namespace CS248 {
namespace Collada {

struct PatternInfo : public Instance {
  std::string name;
  std::string display_name;
  Vector3D v;
  float s;
  int pattern_type;
};

std::ostream& operator<<(std::ostream& os, const PatternInfo& pattern);

}  // namespace Collada
}  // namespace CS248

#endif  // CS248_COLLADA_CAMERAINFO_H

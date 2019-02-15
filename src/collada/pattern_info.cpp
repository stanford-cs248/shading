#include "pattern_info.h"

namespace CS248 {
namespace Collada {

std::ostream& operator<<(std::ostream& os, const PatternInfo& pattern) {
  return os << "PatternInfo: " << pattern.name << " (id: " << pattern.id << ")";
}

}  // namespace Collada
}  // namespace CS248

#ifndef CS248_STATICSCENE_PRIMITIVE_H
#define CS248_STATICSCENE_PRIMITIVE_H

#include "../bbox.h"

namespace CS248 {
namespace StaticScene {

/**
 * The abstract base class primitive
 */
class Primitive {
 public:
  /**
   * Get the world space bounding box of the primitive.
   * \return world space bounding box of the primitive
   */
  virtual BBox get_bbox() const = 0;

  /**
   * Draw with OpenGL (for visualization)
   * \param c desired highlight color
   */
  virtual void draw(const Color& c) const = 0;

  /**
   * Draw outline with OpenGL (for visualization)
   * \param c desired highlight color
   */
  virtual void drawOutline(const Color& c) const = 0;
};

}  // namespace StaticScene
}  // namespace CS248

#endif  // CS248_STATICSCENE_PRIMITIVE_H

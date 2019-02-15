#ifndef CS248_STATICSCENE_SPHERE_H
#define CS248_STATICSCENE_SPHERE_H

#include "object.h"
#include "primitive.h"

namespace CS248 {
namespace StaticScene {

/**
 * A sphere from a sphere object.
 * To be consistent with the triangle interface, each sphere primitive is
 * encapsulated in a sphere object. The have exactly the same origin and
 * radius. The sphere primitive may refer back to the sphere object for
 * other information such as surface material.
 */
class Sphere : public Primitive {
 public:
  /**
   * Parameterized Constructor.
   * Construct a sphere with given origin & radius.
   */
  Sphere(const SphereObject* object, const Vector3D& o, double r)
      : object(object), o(o), r(r), r2(r * r) {}

  /**
   * Get the world space bounding box of the sphere.
   * \return world space bounding box of the sphere
   */
  BBox get_bbox() const {
    return BBox(o - Vector3D(r, r, r), o + Vector3D(r, r, r));
  }

  /**
   * Compute the normal at a point of intersection.
   * NOTE: This is required for all scene objects but we only need it
   * during shading so it's not made part of IntersectInfo.
   * \param p point of intersection. Note that this assumes a valid point of
   *          intersection and does not check if it's actually on the sphere
   * \return normal at the given point of intersection
   */
  Vector3D normal(Vector3D p) const { return (p - o).unit(); }

  /**
   * Draw with OpenGL (for visualizer)
   */
  void draw(const Color& c) const {}

  /**
  * Draw outline with OpenGL (for visualizer)
  */
  void drawOutline(const Color& c) const {}

 private:

  const SphereObject* object;  ///< pointer to the sphere object

  Vector3D o;  ///< origin of the sphere
  double r;    ///< radius
  double r2;   ///< radius squared

};  // class Sphere

}  // namespace StaticScene
}  // namespace CS248

#endif  // CS248_STATICSCENE_SPHERE_H

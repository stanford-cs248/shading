#ifndef CS248_DYNAMICSCENE_SCENE_H
#define CS248_DYNAMICSCENE_SCENE_H

#include <string>
#include <vector>
#include <set>
#include <iostream>

#include "CS248/CS248.h"
#include "CS248/color.h"

#include "GL/glew.h"

#include "../camera.h"
#include "../shader.h"

#include "../static_scene/scene.h"
#include "../static_scene/light.h"

#define SCENE_MAX_SHADOWED_LIGHTS 2

typedef std::vector<std::string> Info;

namespace CS248 {

void checkGLError(string str);
    
namespace DynamicScene {

// Forward declarations
class Scene;
class Selection;
class XFormWidget;

class PatternObject {
public:
    std::string name;
    std::string display_name;
    Vector3D v;
    float s;
    int type;
};

/**
 * Interface that all physical objects in the scene conform to.
 * Note that this doesn't include properties like material that may be treated
 * as separate entities in a COLLADA file, or lights, which are treated
 * specially.
 */
class SceneObject {
 public:
  SceneObject()
      : scene(NULL), isVisible(true), isPickable(true) {}

  /**
   * Renders the object in OpenGL, assuming that the camera and projection
   * matrices have already been set up.
   */
  virtual void draw() = 0;
  virtual void draw_shadow() {};  // don't require draw_shadow()

  virtual void draw_pretty() { draw(); }

  /**
   * Given a transformation matrix from local to space to world space, returns
   * a bounding box of the object in world space. Note that this doesn't have
   * to be the smallest possible bbox, in case that's difficult to compute.
   */
  virtual BBox get_bbox() = 0;
  
  /**
   * Converts this object to an immutable, raytracer-friendly form. Passes in a
   * local-space-to-world-space transformation matrix, because the raytracer
   * expects all the objects to be
   */
  virtual StaticScene::SceneObject *get_static_object() = 0;
  /**
   * Does the same thing as get_static_object, but applies the object's
   * transformations first.
   */
  virtual StaticScene::SceneObject *get_transformed_static_object(double t) {
    return get_static_object();
  }
  
  virtual Matrix4x4 getTransformation();

  virtual Matrix4x4 getRotation();

  /**
   * Pointer to the parent scene containing this object.
   */
  Scene *scene;

  /* World-space position, rotation, scale */
  Vector3D position;
  Vector3D rotation;
  Vector3D scale;

  /**
   * Is this object drawn in the scene?
   */
  bool isVisible;

  /**
   * Is this object pickable right now?
   */
  bool isPickable;
};

// A Selection stores information about any object or widget that is
// selected in the scene, which could include a mesh, a light, a
// camera, or any piece of an object, such as a vertex in a mesh or
// a rotation handle on a camera.
class Selection {
 public:
  // Types used for scene elements that have well-
  // defined axes (e.g., transformation widgets)
  enum class Axis { X, Y, Z, Center, None };

  Selection() { clear(); }

  void clear() {
    object = nullptr;
    coordinates = Vector3D(0., 0., 0.);
    axis = Axis::None;
  }

  bool operator==(const Selection &s) const {
    return object == s.object && axis == s.axis &&
           coordinates.x == s.coordinates.x &&
           coordinates.y == s.coordinates.y && coordinates.z == s.coordinates.z;
  }

  bool operator!=(const Selection &s) const { return !(*this == s); }

  SceneObject *object;       // the selected object
  Vector3D coordinates;      // for optionally selecting a single point
  Axis axis;                 // for optionally selecting an axis
};

/**
 * A light.
 */
class SceneLight {
 public:
  virtual StaticScene::SceneLight *get_static_light() const = 0;
};

/**
 * The scene that meshEdit generates and works with.
 */
class Scene {
 public:
  Scene(std::vector<SceneObject *> _objects, std::vector<SceneLight *> _lights, const std::string& base_shader_dir);
  ~Scene();

  static Scene deep_copy(Scene s);
  void apply_transforms(double t);

  /**
   * Attempts to add object o to the scene, returning
   * false if it is already in the scene.
   */
  bool addObject(SceneObject *o);

  /**
   * Attempts to remove object o from the scene, returning
   * false if it is not already in the scene.
   */
  bool removeObject(SceneObject *o);

  /**
   * Renders the scene in OpenGL, assuming the camera and projection
   * transformations have been applied elsewhere.
   */
  void render_in_opengl();

  // renders a shadow pass
  void render_shadow_pass();

  // visualization mode
  void visualize_shadow_map();
    
  // true if shadow pass is necessary
  bool requires_shadow_pass() const { return do_shadow_pass; }

  /**
   * Gets a bounding box for the entire scene in world space coordinates.
   * May not be the tightest possible.
   */
  BBox get_bbox();

  int current_pattern_id;
  int current_pattern_subid;
  double scaling_factor;

  void prevPattern();
  void nextPattern();
  void decreaseCurrentPattern(double scale = 1);
  void increaseCurrentPattern(double scale = 1);

  Shader*   get_shadow_shader() { return shadow_shader; }
  GLuint    get_shadow_texture(int lightid) { return shadow_texture[lightid]; }
  Matrix4x4 get_world_to_shadowlight(int lightid) { return world_to_shadowlight[lightid]; }
  int       get_num_shadowed_lights() const;

  /**
   * Builds a static scene that's equivalent to the current scene and is easier
   * to use in raytracing, but doesn't allow modifications.
   */
  StaticScene::Scene *get_static_scene();
  /**
   * Does the same thing as get_static_scene, but applies all objects'
   * transformations.
   */
  StaticScene::Scene *get_transformed_static_scene(double t);
  
  std::set<SceneObject *> objects;
  std::set<SceneLight *> lights;
  std::vector<PatternObject> patterns;
  std::vector<StaticScene::DirectionalLight *> directional_lights;
  std::vector<StaticScene::InfiniteHemisphereLight *> hemi_lights;
  std::vector<StaticScene::PointLight *> point_lights;
  std::vector<StaticScene::SpotLight *> spot_lights;
  std::vector<StaticScene::AreaLight *> area_lights;
  std::vector<StaticScene::SphereLight *> sphere_lights;

  Camera *camera;

  bool     do_shadow_pass;
  int      shadow_texture_size;
  Shader*  shadow_shader;
  Shader*  shadow_shader2;
  Shader*  shadow_viz_shader;
  GLuint   shadow_framebuffer[SCENE_MAX_SHADOWED_LIGHTS];
  GLuint   shadow_texture[SCENE_MAX_SHADOWED_LIGHTS];  
  GLuint   shadow_color_texture[SCENE_MAX_SHADOWED_LIGHTS];
  Matrix4x4 world_to_shadowlight[SCENE_MAX_SHADOWED_LIGHTS];
    
};

// Mapping between integer and 8-bit RGB values (used for picking)
static inline void IndexToRGB(int i, unsigned char &R, unsigned char &G, unsigned char &B) {
  R = (i & 0x000000FF) >> 0;
  G = (i & 0x0000FF00) >> 8;
  B = (i & 0x00FF0000) >> 16;
}

// Mapping between 8-bit RGB values and integer (used for picking)
static inline int RGBToIndex(unsigned char R, unsigned char G, unsigned char B) {
  return R + G * 256 + 256 * 256 * B;
}

}  // namespace DynamicScene
}  // namespace CS248

#endif  // CS248_DYNAMICSCENE_DYNAMICSCENE_H

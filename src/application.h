#ifndef CS248_APPLICATION_H
#define CS248_APPLICATION_H

// STL
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

// libCS248
#include "CS248/CS248.h"
#include "CS248/renderer.h"
#include "CS248/osdtext.h"

// COLLADA
#include "collada/collada.h"
#include "collada/light_info.h"
#include "collada/sphere_info.h"
#include "collada/polymesh_info.h"
#include "collada/pattern_info.h"

// MeshEdit
#include "dynamic_scene/scene.h"

// PathTracer
#include "static_scene/scene.h"

// Shared modules
#include "camera.h"

using namespace std;

namespace CS248 {

class Application : public Renderer {
 public:
  Application();

  ~Application();

  void init();
  void render();
  void resize(size_t w, size_t h);

  std::string name();
  std::string info();
  std::string pattern_info();

  void cursor_event(float x, float y);
  void scroll_event(float offset_x, float offset_y);
  void mouse_event(int key, int event, unsigned char mods);
  void keyboard_event(int key, int event, unsigned char mods);
  void char_event(unsigned int codepoint);

  void load(Collada::SceneInfo* sceneInfo);
  void loadScene(const char* filename);
  void render_scene(std::string saveFileLocation);

 private:
  // Mode determines which type of data is visualized/
  // which mode we're currently in (e.g., modeling vs. rendering vs. animation)
  enum Mode { SHADER_MODE };
  Mode mode;

  // Action determines which action will be taken when
  // the user clicks/drags/etc.
  enum class Action {
    Navigate,
    Iterate_Pattern
  };
  Action action;

  void to_shader_mode();
  void toggle_pattern_action();

  DynamicScene::Scene* scene;

  // View Frustrum Variables.
  // On resize, the aspect ratio is changed. On reset_camera, the position and
  // orientation are reset but NOT the aspect ratio.
  Camera camera;
  Camera canonicalCamera;

  size_t screenW;
  size_t screenH;

  // Length of diagonal of bounding box for the mesh.
  // Guranteed to not have the camera occlude with the mes.
  double canonical_view_distance;

  // Rate of translation on scrolling.
  double scroll_rate;

  /*
    Called whenever the camera fov or screenW/screenH changes.
  */
  void set_projection_matrix();

  /**
   * Reads and combines the current modelview and projection matrices.
   */
  Matrix4x4 get_world_to_3DH();

  // Initialization functions to get the opengl cooking with oil.
  std::string init_pattern(Collada::PatternInfo& pattern, vector<DynamicScene::PatternObject> &patterns);
  void init_camera(Collada::CameraInfo& camera, const Matrix4x4& transform);
  DynamicScene::SceneLight* init_light(Collada::LightInfo& light,
                                       const Matrix4x4& transform);
  DynamicScene::SceneObject* init_sphere(Collada::SphereInfo& polymesh,
                                         const Matrix4x4& transform);
  DynamicScene::SceneObject* init_polymesh(Collada::PolymeshInfo& polymesh,
                                           const Matrix4x4& transform,
                                           const std::string shader_prefix = "");

  void set_scroll_rate();

  // Resets the camera to the canonical initial view position.
  void reset_camera();

  // Rendering functions.
  void update_gl_camera();

  // Internal event system //

  float mouseX, mouseY;
  enum e_mouse_button {
    LEFT = MOUSE_LEFT,
    RIGHT = MOUSE_RIGHT,
    MIDDLE = MOUSE_MIDDLE
  };

  bool leftDown;
  bool rightDown;
  bool middleDown;

  // Only draw the pick buffer so often
  // as an optimization.
  int pickDrawCountdown = 0;
  int pickDrawInterval = 5;

  // Event handling //
  void mouse_pressed(e_mouse_button b);   // Mouse pressed.
  void mouse_released(e_mouse_button b);  // Mouse Released.
  void mouse1_dragged(float x, float y);  // Left Mouse Dragged.
  void mouse2_dragged(float x, float y);  // Right Mouse Dragged.
  void mouse_moved(float x, float y);     // Mouse Moved.
  
  // OSD text manager //
  OSDText textManager;
  Color text_color;
  vector<int> messages;

  // Coordinate System //
  bool show_coordinates;
  void draw_coordinates();

  // HUD //
  bool show_hud;
  void draw_hud();
  inline void draw_string(float x, float y, string str, size_t size,
                          const Color& c);

  bool lastEventWasModKey;

  // Animator timeline
  void enter_2D_GL_draw_mode();
  void exit_2D_GL_draw_mode();

  // Intersects mouse position x, y in screen coordinates with a plane
  // going through the origin, and returns the intersecting position
  Vector3D getMouseProjection(double dist=std::numeric_limits<double>::infinity());
};  // class Application

}  // namespace CS248

#endif  // CS248_APPLICATION_H

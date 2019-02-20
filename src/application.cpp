#include "application.h"
#include "dynamic_scene/ambient_light.h"
#include "dynamic_scene/directional_light.h"
#include "dynamic_scene/area_light.h"
#include "dynamic_scene/point_light.h"
#include "dynamic_scene/spot_light.h"
#include "dynamic_scene/sphere.h"
#include "dynamic_scene/mesh.h"

#include "CS248/lodepng.h"

//#define GLFW_INCLUDE_GLCOREARB
#include "GLFW/glfw3.h"

#include <sstream>
#include <chrono>
#include <thread>

using namespace std;

namespace CS248 {

using Collada::PatternInfo;
using Collada::CameraInfo;
using Collada::LightInfo;
using Collada::PolymeshInfo;
using Collada::SceneInfo;
using Collada::SphereInfo;

void checkGLError(std::string str) {
    /*

    // uncomment for debugging

    GLenum err;
    do {
        err = glGetError();
        if (err != GL_NO_ERROR)
            printf("*** GL error: %s: %s\n", str.c_str(), gluErrorString(err));
    } while (err != GL_NO_ERROR);
    */
}
    
Application::Application() {
  scene = nullptr;
}

Application::~Application() {
  if (scene != nullptr) delete scene;
}

void Application::init() {
  if (scene != nullptr) {
    delete scene;
    scene = nullptr;
  }

  checkGLError("pre text init");
  
  textManager.init(use_hdpi);
  text_color = Color(1.0, 1.0, 1.0);

  checkGLError("post text init");
  
  // Setup all the basic internal state to default values,
  // as well as some basic OpenGL state (like depth testing
  // and lighting).

  // Set the integer bit vector representing which keys are down.
  leftDown = false;
  rightDown = false;
  middleDown = false;

  show_coordinates = false;
  show_hud = true;

  // Lighting needs to be explicitly enabled.
  glEnable(GL_LIGHTING);

  // Enable anti-aliasing and circular points.
  glEnable(GL_LINE_SMOOTH);
  // glEnable( GL_POLYGON_SMOOTH ); // XXX causes cracks!
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  // glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

  mode = SHADER_MODE;
  action = Action::Navigate;
  scene = nullptr;
  visualize_shadow_map = false;


  // Make a dummy camera so resize() doesn't crash before the scene has been
  // loaded.
  // NOTE: there's a chicken-and-egg problem here, because load()
  // requires init, and init requires init_camera (which is only called by
  // load()).
  screenW = screenH = 600;  // Default value
  CameraInfo cameraInfo;
  cameraInfo.hFov = 20;
  cameraInfo.vFov = 28;
  cameraInfo.nClip = 0.1;
  cameraInfo.fClip = 100;
  camera.configure(cameraInfo, screenW, screenH);
  canonicalCamera.configure(cameraInfo, screenW, screenH);

}

void Application::enter_2D_GL_draw_mode() {
  int screen_w = screenW;
  int screen_h = screenH;
  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, screen_w, screen_h);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screen_w, screen_h, 0, 0, 1);  // Y flipped !
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0, 0, -1);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
}

void Application::exit_2D_GL_draw_mode() {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void Application::render() {

  // Update the hovered element using the pick buffer once very n iterations.
  // We do this here rather than on mouse move, because some platforms generate
  // an excessive number of mouse move events which incurs a performance hit.
  if(pickDrawCountdown < 0) {
    pickDrawCountdown += pickDrawInterval;
  } else {
    pickDrawCountdown--;
  }

  // pass 1, generate shadow map for the first directional light source

  if (scene->requires_shadow_pass())
     scene->render_shadow_pass();

  // pass 2, beauty pass, render the scene (using the shadow map)
  
  glViewport(0, 0, screenW, screenH);

  glClearColor(0., 0., 0., 0.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (visualize_shadow_map) {
    scene->visualize_shadow_map();
  } else {
  
    set_projection_matrix();
    update_gl_camera();
    
    if (show_coordinates)
        draw_coordinates();

    scene->render_in_opengl();

    if (show_hud)
        draw_hud();
  }
}

void Application::update_gl_camera() {
  // Call resize() every time we draw, since it doesn't seem
  // to get called by the Viewer upon initial window creation
  // (this should probably be fixed!).
  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);
  if (view[2] != screenW || view[3] != screenH) {
    resize(view[2], view[3]);
  }

  // Control the camera to look at the mesh.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  const Vector3D &c = camera.position();
  const Vector3D &r = camera.view_point();
  const Vector3D &u = camera.up_dir();

  //cout << camera.position() << endl;

  gluLookAt(c.x, c.y, c.z, r.x, r.y, r.z, u.x, u.y, u.z);

  scene->camera = &camera;
}

void Application::resize(size_t w, size_t h) {
  screenW = w;
  screenH = h;
  camera.set_screen_size(w, h);
  textManager.resize(w, h);
  set_projection_matrix();
}

void Application::set_projection_matrix() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(camera.v_fov(),
                 camera.aspect_ratio(),
                 camera.near_clip(),
                 camera.far_clip());
}

string Application::name() { return "Shader Assignment"; }

string Application::info() {
  return "";
}

string Application::pattern_info() {
  if(action != Action::Iterate_Pattern) return "";
  if(!scene) return "No scene object";
  if(scene->patterns.size() == 0) return "No patterns";
  DynamicScene::PatternObject &po = scene->patterns[scene->current_pattern_id];
  //string pattern_prefix = "Pattern (" + to_string(scene->current_pattern_id + 1) + "/" + to_string(scene->patterns.size()) + ")";
  string pattern_prefix = "Rust threshold: ";
  string pattern;
  if(po.type == 0) {
    pattern = " = (" + to_string(po.v.x) + ", " + to_string(po.v.y) + ", " + to_string(po.v.z) + ")";
  } else if(po.type ==1) {
    pattern = " = " + to_string(po.s);
  }
  return pattern_prefix + pattern;
}
    
void Application::load(SceneInfo *sceneInfo) {
  vector<Collada::Node> &nodes = sceneInfo->nodes;
  vector<DynamicScene::SceneLight *> lights;
  vector<DynamicScene::SceneObject *> objects;

  // save camera position to update camera control later
  CameraInfo *c;
  Vector3D c_pos = Vector3D();
  Vector3D c_dir = Vector3D();

  vector<DynamicScene::PatternObject> patterns;
  std::string shader_prefix = "";

  int len = nodes.size();
  for (int i = 0; i < len; i++) {
    Collada::Node &node = nodes[i];
    Collada::Instance *instance = node.instance;
    if(instance->type == Collada::Instance::PATTERN) {
        init_pattern(static_cast<PatternInfo &>(*instance), patterns);
    }
  }
  for (int i = 0; i < len; i++) {
    Collada::Node &node = nodes[i];
    Collada::Instance *instance = node.instance;
    const Matrix4x4 &transform = node.transform;

    switch (instance->type) {
      case Collada::Instance::PATTERN:
        break;
      case Collada::Instance::CAMERA:
        c = static_cast<CameraInfo *>(instance);
        c_pos = (transform * Vector4D(c->pos, 1)).to3D();
        c_dir = (transform * Vector4D(c->view_dir, 1)).to3D().unit();
        init_camera(*c, transform);
        break;
      case Collada::Instance::LIGHT: {
        lights.push_back(
            init_light(static_cast<LightInfo &>(*instance), transform));
        break;
      }
      case Collada::Instance::SPHERE:
        objects.push_back(
            init_sphere(static_cast<SphereInfo &>(*instance), transform));
        break;
      case Collada::Instance::POLYMESH:
        objects.push_back(
            init_polymesh(static_cast<PolymeshInfo &>(*instance), transform, shader_prefix));
        break;
    default:
        // unknown instance type
        break;
    }
  }

  if (lights.size() == 0) {  // no lights, default use ambient_light
    LightInfo default_light = LightInfo();
    lights.push_back(new DynamicScene::AmbientLight(default_light));
  }
  scene = new DynamicScene::Scene(objects, lights, sceneInfo->base_shader_dir);
  scene->patterns = patterns;

  const BBox &bbox = scene->get_bbox();
  if (!bbox.empty()) {
    Vector3D target = bbox.centroid();
    canonical_view_distance = bbox.extent.norm() / 2 * 1.5;

    double view_distance = canonical_view_distance * 2;
    double min_view_distance = canonical_view_distance / 10.0;
    double max_view_distance = canonical_view_distance * 20.0;

	canonicalCamera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z),
                          view_distance, min_view_distance, max_view_distance);

	if(!c->default_flag) {
		target = c_pos;
		view_distance = c->view_dir.norm();
	}

    camera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z), view_distance,
                 min_view_distance, max_view_distance);

    set_scroll_rate();
  }

  // cerr << "==================================" << endl;
  // cerr << "CAMERA" << endl;
  // cerr << "      hFov: " << camera.hFov << endl;
  // cerr << "      vFov: " << camera.vFov << endl;
  // cerr << "        ar: " << camera.ar << endl;
  // cerr << "     nClip: " << camera.nClip << endl;
  // cerr << "     fClip: " << camera.fClip << endl;
  // cerr << "       pos: " << camera.pos << endl;
  // cerr << " targetPos: " << camera.targetPos << endl;
  // cerr << "       phi: " << camera.phi << endl;
  // cerr << "     theta: " << camera.theta << endl;
  // cerr << "         r: " << camera.r << endl;
  // cerr << "      minR: " << camera.minR << endl;
  // cerr << "      maxR: " << camera.maxR << endl;
  // cerr << "       c2w: " << camera.c2w << endl;
  // cerr << "   screenW: " << camera.screenW << endl;
  // cerr << "   screenH: " << camera.screenH << endl;
  // cerr << "screenDist: " << camera.screenDist<< endl;
  // cerr << "==================================" << endl;
}

std::string Application::init_pattern(PatternInfo &patternInfo, vector<DynamicScene::PatternObject> &patterns) {
    DynamicScene::PatternObject pattern;
    pattern.name = patternInfo.name;
    printf("Init pattern: %s\n", pattern.name.c_str());
    pattern.display_name = patternInfo.display_name;
    pattern.v = patternInfo.v;
    pattern.s = patternInfo.s;
    pattern.type = patternInfo.pattern_type;
    patterns.push_back(pattern);
    std::string ret = "";
    if(pattern.type == 0) {
        ret += "uniform vec3 " + pattern.name + ";";
    } else if(pattern.type == 1) {
        ret += "uniform float " + pattern.name + ";";
    }
    return ret;
}

void Application::init_camera(CameraInfo &cameraInfo,
                              const Matrix4x4 &transform) {
  camera.configure(cameraInfo, screenW, screenH);
  canonicalCamera.configure(cameraInfo, screenW, screenH);
  set_projection_matrix();
}

void Application::reset_camera() { camera.copy_placement(canonicalCamera); }

DynamicScene::SceneLight *Application::init_light(LightInfo &light, const Matrix4x4 &transform) {
  switch (light.light_type) {
    case Collada::LightType::NONE:
      break;
    case Collada::LightType::AMBIENT:
      return new DynamicScene::AmbientLight(light);
    case Collada::LightType::DIRECTIONAL:
      return new DynamicScene::DirectionalLight(light, transform);
    case Collada::LightType::AREA:
      return new DynamicScene::AreaLight(light, transform);
    case Collada::LightType::POINT:
      return new DynamicScene::PointLight(light, transform);
    case Collada::LightType::SPOT:
      return new DynamicScene::SpotLight(light, transform);
    default:
      break;
  }
  return nullptr;
}

/**
 * The transform is assumed to be composed of translation, rotation, and
 * scaling, where the scaling is uniform across the three dimensions; these
 * assumptions are necessary to ensure the sphere is still spherical. Rotation
 * is ignored since it's a sphere, translation is determined by transforming the
 * origin, and scaling is determined by transforming an arbitrary unit vector.
 */
DynamicScene::SceneObject *Application::init_sphere(
  SphereInfo &sphere, const Matrix4x4 &transform) {
  const Vector3D &position = (transform * Vector4D(0, 0, 0, 1)).projectTo3D();
  double scale = (transform * Vector4D(1, 0, 0, 0)).to3D().norm();
  return new DynamicScene::Sphere(sphere, position, scale);
}

DynamicScene::SceneObject *Application::init_polymesh(
  PolymeshInfo &polymesh, const Matrix4x4 &transform, const std::string shader_prefix) {
  return new DynamicScene::Mesh(polymesh, transform, shader_prefix);
}

void Application::set_scroll_rate() {
  scroll_rate = canonical_view_distance / 10;
}

void Application::cursor_event(float x, float y) {
  if (leftDown && !middleDown && !rightDown) {
    mouse1_dragged(x, y);
  } else if (!leftDown && !middleDown && rightDown) {
    mouse2_dragged(x, y);
  } else if (!leftDown && !middleDown && !rightDown) {
    mouse_moved(x, y);
  }

  mouseX = x;
  mouseY = y;
}

void Application::scroll_event(float offset_x, float offset_y) {
  // update_style();

  switch (mode) {
    case SHADER_MODE:
      switch (action) {
        case Action::Navigate:
          camera.move_forward(-offset_y * scroll_rate);
          break;
        default:
          break;
      }
      break;
  }
}

void Application::mouse_event(int key, int event, unsigned char mods) {
  switch (event) {
    case EVENT_PRESS:
      switch (key) {
        case MOUSE_LEFT:
          mouse_pressed(LEFT);
          break;
        case MOUSE_RIGHT:
          mouse_pressed(RIGHT);
          break;
        case MOUSE_MIDDLE:
          mouse_pressed(MIDDLE);
          break;
      }
      break;
    case EVENT_RELEASE:
      switch (key) {
        case MOUSE_LEFT:
          mouse_released(LEFT);
          break;
        case MOUSE_RIGHT:
          mouse_released(RIGHT);
          break;
        case MOUSE_MIDDLE:
          mouse_released(MIDDLE);
          break;
      }
      break;
  }
}

void Application::char_event(unsigned int codepoint) {
  bool queued = false;

  switch (mode) {
    case SHADER_MODE:
      switch (codepoint) {
        case 'p':
        case 'P':
            toggle_pattern_action();
            break;
        case 'v':
        case 'V':
          visualize_shadow_map = !visualize_shadow_map;
          break;
        case 'c':
        case 'C':
          printf("Current camera info:\n");
          cout << "Pos:          " << camera.position() << endl;
          cout << "Lookat:       " << camera.view_point() << endl;
          cout << "Up:           " << camera.up_dir() << endl;
          cout << "aspect_ratio: " << camera.aspect_ratio() << endl;
          cout << "near_clip:    " << camera.near_clip() << endl;
          cout << "far_clip:     " << camera.far_clip() << endl;
          break;
    		case 'r':
		      	reset_camera();
			     break;
      }
  }
}

void Application::keyboard_event(int key, int event, unsigned char mods) {
  switch (mode) {
    case SHADER_MODE:
      switch (key) {
        case GLFW_KEY_TAB:
          if (event == GLFW_PRESS) {
            show_hud = !show_hud;
          }
          break;
        default:
          break;
      }
      break;
  }

  if (lastEventWasModKey && event == GLFW_RELEASE) {
  }

  lastEventWasModKey = false;
  if (mods) {
    lastEventWasModKey = true;
  }
}

void Application::loadScene(const char *filename) {
  cerr << "Loading scene from file " << filename << endl;

  Camera originalCamera = camera;
  Camera originalCanonicalCamera = canonicalCamera;

  Collada::SceneInfo *sceneInfo = new Collada::SceneInfo();
  if (Collada::ColladaParser::load(filename, sceneInfo) < 0) {
    cerr << "Warning: scene file failed to load." << endl;
    delete sceneInfo;
    return;
  }
  load(sceneInfo);

  camera = originalCamera;
  canonicalCamera = originalCanonicalCamera;
}

void Application::toggle_pattern_action() {
  if (action != Action::Iterate_Pattern) {
    action = Action::Iterate_Pattern;
  } else {
    action = Action::Navigate;
  }
}

Vector3D Application::getMouseProjection(double dist) {
  // get projection matrix from OpenGL stack.
  GLdouble projection[16];
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  Matrix4x4 projection_matrix(projection);

  // get view matrix from OpenGL stack.
  GLdouble modelview[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  Matrix4x4 modelview_matrix(modelview);

  // ray in clip coordinates
  double x = mouseX * 2 / screenW - 1;
  double y = screenH - mouseY;  // y is upside down
  y = y * 2 / screenH - 1;
  Vector4D ray_clip(x, y, -1.0, 1.0);
  // ray in eye coordinates
  Vector4D ray_eye = projection_matrix.inv() * ray_clip;

  // ray is into the screen and not a point.
  ray_eye.z = -1.0;
  ray_eye.w = 0.0;

  // ray in world coordinates
  Vector4D ray_wor4 = modelview_matrix * ray_eye;
  Vector3D ray_wor(ray_wor4.x, ray_wor4.y, ray_wor4.z);

  Vector3D ray_orig(camera.position());

  double t = dot(ray_orig, -ray_wor);
  if(std::isfinite(dist)) {
    // If a distance was given, use that instead
    ray_wor = ray_wor.unit();
    t = dist;
  }

  Vector3D intersect = ray_orig + t * ray_wor;

  return intersect;
}

void Application::mouse_pressed(e_mouse_button b) {
  switch (b) {
    case LEFT:
      leftDown = true;
      break;
    case RIGHT:
      rightDown = true;
      break;
    case MIDDLE:
      middleDown = true;
      break;
  }
}

void Application::mouse_released(e_mouse_button b) {
  switch (b) {
    case LEFT:
      leftDown = false;
      break;
    case RIGHT:
      rightDown = false;
      break;
    case MIDDLE:
      middleDown = false;
      break;
  }
}

/*
  When in edit mode and there is a selection, move the selection.
  When in visualization mode, rotate.
*/
void Application::mouse1_dragged(float x, float y) {
  float dx = (x - mouseX);
  float dy = (y - mouseY);

    camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
}

/*
  When the mouse is dragged with the right button held down, translate.
*/
void Application::mouse2_dragged(float x, float y) {
  float dx = (x - mouseX);
  float dy = (y - mouseY);
  // don't negate y because up is down.
  camera.move_by(-dx, dy, canonical_view_distance);
}

void Application::mouse_moved(float x, float y) {
  y = screenH - y;  // Because up is down.
                    // Converts x from [0, w] to [-1, 1], and similarly for y.
  // Vector2D p(x * 2 / screenW - 1, y * 2 / screenH - 1);
  Vector2D p(x, y);
  update_gl_camera();
}

void Application::to_shader_mode() {
  if (mode == SHADER_MODE) return;
  mode = SHADER_MODE;
}

Matrix4x4 Application::get_world_to_3DH() {
  Matrix4x4 P, M;
  glGetDoublev(GL_PROJECTION_MATRIX, &P(0, 0));
  glGetDoublev(GL_MODELVIEW_MATRIX, &M(0, 0));
  return P * M;
}

inline void Application::draw_string(float x, float y, string str, size_t size,
                                     const Color &c) {
  int line_index = textManager.add_line((x * 2 / screenW) - 1.0,
                                        (-y * 2 / screenH) + 1.0, str, size, c);
  messages.push_back(line_index);
}

void Application::draw_coordinates() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glLineWidth(2.);

  glBegin(GL_LINES);
  glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(1, 0, 0);

  glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(0, 1, 0);

  glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(0, 0, 1);

  glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
  for (int x = 0; x <= 8; ++x) {
    glVertex3i(x - 4, 0, -4);
    glVertex3i(x - 4, 0, 4);
  }
  for (int z = 0; z <= 8; ++z) {
    glVertex3i(-4, 0, z - 4);
    glVertex3i(4, 0, z - 4);
  }
  glEnd();

  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
}

void Application::draw_hud() {
  textManager.clear();
  messages.clear();

  const size_t size = 16;
  const float x0 = use_hdpi ? screenW - 300 * 2 : screenW - 300;
  const float y0 = use_hdpi ? 128 : 64;
  const int inc = use_hdpi ? 48 : 24;
  float y = y0 + inc - size;

  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);

  textManager.render();
}

void Application::render_scene(std::string saveFileLocation) {
}

}  // namespace CS248

#include "application.h"

#include "gl_utils.h"
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

using Collada::CameraInfo;
using Collada::LightInfo;
using Collada::PolymeshInfo;
using Collada::SceneInfo;
using Collada::SphereInfo;

Application::Application() {
    scene = nullptr;
}

Application::~Application() {
    if (scene != nullptr)
        delete scene;
}

void Application::init() {

    if (scene != nullptr) {
        delete scene;
        scene = nullptr;
    }

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);

    checkGLError("before OpenGL version detection");

    printf("Detected OpenGL version=%s, vendor=%s\n", version, vendor);
    // GLint num_exts;
    // glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
    // printf("Detected OpenGL Extensions:\n");
    // for (GLint i = 0; i < num_exts; ++i) {
    //   printf("%s\n", glGetStringi(GL_EXTENSIONS, i));
    // }

    checkGLError("begin Application::init");
  
    textManager.init(use_hdpi);
    textColor = Color(1.0, 1.0, 1.0);
  
    // Setup all the basic internal state to default values,
    // as well as some basic OpenGL state (like depth testing
    // and lighting).

    // Set the integer bit vector representing which keys are down.
    leftDown = false;
    rightDown = false;
    middleDown = false;
    showHUD = true;

    scene = nullptr;
    visualizeShadowMap = false;
    discoModeOn = false;

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

    checkGLError("end of Application::init");
}

void Application::render() {

    //printf("Top of application::render\n");
    checkGLError("start of Application::render");

    // Call resize() every time we draw, since it doesn't seem
    // to get called by the Viewer upon initial window creation
    // FIXME(kayvonf): look into and fix this
    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    if (view[2] != screenW || view[3] != screenH) {
        resize(view[2], view[3]);
    }

    if (discoModeOn) {
      scene->rotateSpotLights();
    }
    // pass 1, generate shadow map for the spotlights

    if (scene->needsShadowPass()) {
        for (int i=0; i<scene->getNumShadowedLights(); i++) {
            scene->renderShadowPass(i);
        }
    }

    // pass 2, beauty pass, render the scene (using the shadow map)

    checkGLError("pre beauty pass");
    
    glViewport(0, 0, screenW, screenH);

    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);


    if (visualizeShadowMap && scene->needsShadowPass()) {

      scene->visualizeShadowMap();

    } else {
    
      scene->render();

      if (showHUD)
          drawHUD();
    }

    //printf("End of application::render\n");
    checkGLError("end of Application::render");
}


void Application::resize(size_t w, size_t h) {
    screenW = w;
    screenH = h;
    camera.setScreenSize(w, h);
    textManager.resize(w, h);
}

string Application::name() {
    return "Shader Assignment";
}

string Application::info() {
    return "";
}

void Application::load(SceneInfo* sceneInfo) {

    // save camera position to update camera control later
    CameraInfo* camInfo;
    Vector3D camPos, camDir;
 
    vector<DynamicScene::SceneLight*> lights;
    vector<DynamicScene::SceneObject*> objects;

    vector<Collada::Node>& nodes = sceneInfo->nodes;

    for (size_t i=0; i<nodes.size(); i++) {
        Collada::Node& node = nodes[i];
        Collada::Instance* instance = node.instance;
        const Matrix4x4& transform = node.transform;

        switch (instance->type) {
          case Collada::Instance::CAMERA: {
              // printf("Creating camera\n"); fflush(stdout);
              camInfo = static_cast<CameraInfo*>(instance);
              camPos = (transform * Vector4D(camInfo->pos, 1)).to3D();
              camDir = (transform * Vector4D(camInfo->view_dir, 1)).to3D().unit();
              initCamera(*camInfo, transform);
              break;
          }
          case Collada::Instance::LIGHT: {
              // printf("Creating light\n"); fflush(stdout);  
              lights.push_back(
                  initLight(static_cast<LightInfo&>(*instance), transform));
              break;
          }
          case Collada::Instance::POLYMESH: {
              // printf("Creating mesh\n"); fflush(stdout);  
              objects.push_back(
                  initPolymesh(static_cast<PolymeshInfo&>(*instance), transform));
              break;
          }
        default:
            // unsupported instance type
            break;
        }
    }

    // create the scene
    scene = new DynamicScene::Scene(objects, lights, sceneInfo->base_shader_dir);
    scene->setCamera(&camera);  

    // given the size of the scene, determine a "canonical" camera position that's
    // outside the bounds of the scene's geometry
    BBox bbox = scene->getBBox();
    if (!bbox.empty()) {
        Vector3D target = bbox.centroid();
        canonicalViewDistance = bbox.extent.norm() / 2.0 * 1.5;

        double viewDistance = canonicalViewDistance * 2;
        double minViewDistance = canonicalViewDistance / 10.0;
        double maxViewDistance = canonicalViewDistance * 20.0;

      	canonicalCamera.place(target, acos(camDir.y), atan2(camDir.x, camDir.z),
                              viewDistance, minViewDistance, maxViewDistance);

    	  if (!camInfo->default_flag) {
		        target = camPos;
		        viewDistance = camInfo->view_dir.norm();
        }

        camera.place(target, acos(camDir.y), atan2(camDir.x, camDir.z),
                     viewDistance, minViewDistance, maxViewDistance);

        setScrollRate();
    }
}

void Application::initCamera(CameraInfo& cameraInfo, const Matrix4x4& transform) {
    camera.configure(cameraInfo, screenW, screenH);
    canonicalCamera.configure(cameraInfo, screenW, screenH);
}

void Application::resetCamera() { 
  camera.copyPlacement(canonicalCamera);
}

DynamicScene::SceneLight* Application::initLight(LightInfo& light, const Matrix4x4& transform) {
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

DynamicScene::SceneObject* Application::initPolymesh(PolymeshInfo& polymesh, const Matrix4x4& transform) {
    return new DynamicScene::Mesh(polymesh, transform);
}

void Application::setScrollRate() {
    scrollRate = canonicalViewDistance / 10;
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
    camera.moveForward(-offset_y * scrollRate);
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

void Application::char_event(unsigned int ch) {

    switch (ch) {
      case 'v':
      case 'V':
        visualizeShadowMap = !visualizeShadowMap;
        break;
      case 'c':
      case 'C':
        printf("Current camera info:\n");
        cout << "Pos:          " << camera.getPosition() << endl;
        cout << "Lookat:       " << camera.getViewPoint() << endl;
        cout << "Up:           " << camera.getUpDir() << endl;
        cout << "aspect_ratio: " << camera.getAspectRatio() << endl;
        cout << "near_clip:    " << camera.getNearClip() << endl;
        cout << "far_clip:     " << camera.getFarClip() << endl;
        break;
  		case 'r':
      case 'R':
	     	 resetCamera();
		     break;
      case 's':
      case 'S':
         scene->reloadShaders();
         break;
      case 'd':
      case 'D':
         discoModeOn = !discoModeOn;
         break;
    }
}

void Application::keyboard_event(int key, int event, unsigned char mods) {
    switch (key) {
      case GLFW_KEY_TAB:
        if (event == GLFW_PRESS)
          showHUD = !showHUD;
        break;
      default:
        break;
    }
}

Vector3D Application::getMouseProjection(double dist) {
  // get projection matrix from OpenGL stack.
  GLdouble projection[16];
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  Matrix4x4 projectionMatrix(projection);

  // get view matrix from OpenGL stack.
  GLdouble modelview[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  Matrix4x4 modelviewMatrix(modelview);

  // ray in clip coordinates
  double x = mouseX * 2 / screenW - 1;
  double y = screenH - mouseY;  // y is upside down
  y = y * 2 / screenH - 1;
  Vector4D rayClip(x, y, -1.0, 1.0);
  // ray in eye coordinates
  Vector4D rayEye = projectionMatrix.inv() * rayClip;

  // ray is into the screen and not a point.
  rayEye.z = -1.0;
  rayEye.w = 0.0;

  // ray in world coordinates
  Vector4D rayWor4 = modelviewMatrix * rayEye;
  Vector3D rayWor(rayWor4.x, rayWor4.y, rayWor4.z);

  Vector3D rayOrig(camera.getPosition());

  double t = dot(rayOrig, -rayWor);
  if (std::isfinite(dist)) {
    // If a distance was given, use that instead
    rayWor = rayWor.unit();
    t = dist;
  }

  Vector3D intersect = rayOrig + t * rayWor;

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
    camera.rotateBy(dy * (PI / screenH), dx * (PI / screenW));
}

/*
  When the mouse is dragged with the right button held down, translate.
*/
void Application::mouse2_dragged(float x, float y) {
    float dx = (x - mouseX);
    float dy = (y - mouseY);
    // don't negate y because up is down.
    camera.moveBy(-dx, dy, canonicalViewDistance);
}

void Application::mouse_moved(float x, float y) {
    y = screenH - y;  // Because up is down.
                      // Converts x from [0, w] to [-1, 1], and similarly for y.

    // Vector2D p(x * 2 / screenW - 1, y * 2 / screenH - 1);
    Vector2D p(x, y);
    //updateCamera();
}

/*
Matrix4x4 Application::getWorldTo3DH() {
    Matrix4x4 P, M;
    glGetDoublev(GL_PROJECTION_MATRIX, &P(0, 0));
    glGetDoublev(GL_MODELVIEW_MATRIX, &M(0, 0));
    return P * M;
}
*/

inline void Application::drawString(float x, float y, string str, size_t size, const Color& c) {
    int line_index = textManager.add_line((x * 2 / screenW) - 1.0, (-y * 2 / screenH) + 1.0, str, size, c);
    messages.push_back(line_index);
}

void Application::drawHUD() {
    textManager.clear();
    messages.clear();

    const size_t size = 16;
    const float x0 = use_hdpi ? screenW - 300 * 2 : screenW - 300;
    const float y0 = use_hdpi ? 128 : 64;
    const int inc = use_hdpi ? 48 : 24;
    float y = y0 + inc - size;

    textManager.render();

    checkGLError("end Application::drawHUD");
}


}  // namespace CS248

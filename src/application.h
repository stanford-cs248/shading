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

// COLLADA (scene parsing and loading)
#include "collada/collada.h"
#include "collada/light_info.h"
#include "collada/sphere_info.h"
#include "collada/polymesh_info.h"

#include "dynamic_scene/scene.h"

#include "camera.h"

using namespace std;

namespace CS248 {

class Application : public Renderer {
 public:

    enum RenderPassType {RENDER_PASS_SHADOW, RENDER_PASS_RENDER};

    Application();

    ~Application();

    void init();
    void render();
    void resize(size_t w, size_t h);

    std::string name();
    std::string info();

    void cursor_event(float x, float y);
    void scroll_event(float offset_x, float offset_y);
    void mouse_event(int key, int event, unsigned char mods);
    void keyboard_event(int key, int event, unsigned char mods);
    void char_event(unsigned int ch);

    void load(Collada::SceneInfo* sceneInfo);

 private:

    DynamicScene::Scene* scene;

    // View Frustrum Variables
    // On resize, the aspect ratio is changed. On reset_camera, the position and
    // orientation are reset but NOT the aspect ratio.
    Camera camera;
    Camera canonicalCamera;

    size_t screenW;
    size_t screenH;

    // Length of diagonal of bounding box for the mesh
    // Guaranteed to not have the camera occlude with scene objects.
    double canonicalViewDistance;

    // Rate of translation on scrolling
    double scrollRate;

    // true if rendering should visualize the shadow map
    bool visualizeShadowMap;

    // whether the scene is in disco mode (rotating spotlights)
    bool discoModeOn;

    /*
      Called whenever the camera fov or screenW/screenH changes
    */
    //void setProjectionMatrix();

    /**
     * Reads and combines the current modelview and projection matrices
     */
    //Matrix4x4 getWorldTo3DH();

    void initCamera(Collada::CameraInfo& camera, const Matrix4x4& transform);
    DynamicScene::SceneLight*  initLight(Collada::LightInfo& light, const Matrix4x4& transform);
    DynamicScene::SceneObject* initPolymesh(Collada::PolymeshInfo& polymesh, const Matrix4x4& transform);

    void setScrollRate();

    // Resets the camera to the canonical initial view position
    void resetCamera();

    // Rendering functions
    // void updateGLCamera();

    // Internal event system //
    enum e_mouse_button {
        LEFT   = MOUSE_LEFT,
        RIGHT  = MOUSE_RIGHT,
        MIDDLE = MOUSE_MIDDLE
    };

    float mouseX, mouseY;
    bool  leftDown;
    bool  rightDown;
    bool  middleDown;

    // Event handling // 
    void mouse_pressed(e_mouse_button b);   // Mouse pressed.
    void mouse_released(e_mouse_button b);  // Mouse Released.
    void mouse1_dragged(float x, float y);  // Left Mouse Dragged.
    void mouse2_dragged(float x, float y);  // Right Mouse Dragged.
    void mouse_moved(float x, float y);     // Mouse Moved.
    
    // OSD text rendering manager //
    OSDText textManager;
    Color   textColor;
    vector<int> messages;

    // HUD //
    bool showHUD;
    void drawHUD();
    inline void drawString(float x, float y, string str, size_t size, const Color& c);

    // Intersects mouse position x, y in screen coordinates with a plane
    // going through the origin, and returns the intersecting position
    Vector3D getMouseProjection(double dist=std::numeric_limits<double>::infinity());

};  // class Application

}  // namespace CS248

#endif  // CS248_APPLICATION_H

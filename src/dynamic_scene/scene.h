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
#include "../gl_resource_manager.h"

#include "../static_scene/scene.h"
#include "../static_scene/light.h"

#define SCENE_MAX_SHADOWED_LIGHTS 10

namespace CS248 {

namespace DynamicScene {

// Forward declarations
class Scene;


/**
 * Interface that all physical objects in the scene conform to.
 * Note that this doesn't include properties like material that may be treated
 * as separate entities in a COLLADA file, or lights, which are treated
 * specially.
 */
class SceneObject {
 public:
    SceneObject()
        : scene_(NULL) {}

    /**
     * Renders the object in OpenGL, assuming that the camera and projection
     * matrices have already been set up.
     */
    virtual void draw(const Matrix4x4& worldToNDC) const = 0;

    // same as above, but shadow pass form
    virtual void drawShadow(const Matrix4x4& worldToNDC) const = 0;

    // reload any shaders associated with object
    virtual void reloadShaders() = 0; 

    /**
     * Returns a world space bounding box of the object in world space.
     */
    virtual BBox getBBox() const = 0;

//  Matrix4x4 getTransformation() const;
//  Matrix4x4 getRotation() const;
//  Matrix4x4 getScale() const;
    Matrix4x4 getObjectToWorld() const {
   
        float deg2Rad = M_PI / 180.0;
    
        Matrix4x4 T = Matrix4x4::translation(position_);
        Matrix4x4 RX = Matrix4x4::rotation(rotation_.x * deg2Rad, Matrix4x4::Axis::X);
        Matrix4x4 RY = Matrix4x4::rotation(rotation_.y * deg2Rad, Matrix4x4::Axis::Y);
        Matrix4x4 RZ = Matrix4x4::rotation(rotation_.z * deg2Rad, Matrix4x4::Axis::Z);
        Matrix4x4 scaleXform = Matrix4x4::scaling(scale_);
    
        // object to world transformation
        Matrix4x4 objectToWorld = T * RX * RY * RZ * scaleXform;
        return objectToWorld;
    }

    Matrix3x3 getObjectToWorldForNormals() const {
   
        float deg2Rad = M_PI / 180.0;
    
        Matrix4x4 RX = Matrix4x4::rotation(rotation_.x * deg2Rad, Matrix4x4::Axis::X);
        Matrix4x4 RY = Matrix4x4::rotation(rotation_.y * deg2Rad, Matrix4x4::Axis::Y);
        Matrix4x4 RZ = Matrix4x4::rotation(rotation_.z * deg2Rad, Matrix4x4::Axis::Z);
        Matrix4x4 scaleXform = Matrix4x4::scaling(scale_);
    

        Matrix4x4 xformNorm = RX * RY * RZ * scaleXform;

        // convert to 3x3
        Matrix3x3 xform3D;
        for (int i=0; i<3; i++) {
            const Vector4D& col = xformNorm.column(i); 
            xform3D[i][0] = col[0];
            xform3D[i][1] = col[1];
            xform3D[i][2] = col[2];
        }

        return xform3D.inv().T(); 
    }

    void setScene(Scene* s) { scene_ = s; }


  protected:

    /**
     * Pointer to the scene containing this object.
     */
    Scene* scene_;

    /* World-space position, rotation, scale of object */
    Vector3D position_;
    Vector3D rotation_;
    Vector3D scale_;
};

/**
 * A light
 */
class SceneLight {
 public:
    virtual StaticScene::SceneLight* getStaticLight() const = 0;
};

/**
 * The scene
 */
class Scene {
 public:
    Scene(std::vector<SceneObject*> objects,
          std::vector<SceneLight*> lights,
          const std::string& baseShaderDir);
    ~Scene();

    /**
     * Renders the scene in OpenGL, assuming the camera and projection
     * transformations have been applied elsewhere.
     */
    void render();

    // renders a shadow pass
    void renderShadowPass(int shadowedLightIndex);

    // visualization mode
    void visualizeShadowMap();

    // disco mode
    void rotateSpotLights();
      
    // true if shadow pass is necessary
    bool needsShadowPass() const { return doShadowPass_; }

    // "hot" reloads shaders for all scene objects
    void reloadShaders();

    /**
     * Gets a bounding box for the entire scene in world space coordinates.
     * May not be the tightest possible.
     */
    BBox getBBox() const;

    Shader*   getShadowShader() const { return shadowShader_; }
    TextureArrayId getShadowTextureArrayId() const { return shadowDepthTextureArrayId_; }
    Matrix4x4 getWorldToShadowLight(int lightid) const { return worldToShadowLight_[lightid]; }

    size_t getNumShadowedLights() const;
    size_t getNumDirectionalLights() const { return directionalLights_.size(); }
    size_t getNumPointLights() const { return pointLights_.size(); }
    size_t getNumSpotLights() const { return spotLights_.size(); }

    const Camera* getCamera() const { return camera_; }
    const StaticScene::DirectionalLight* getDirectionalLight(int i) const { return directionalLights_[i]; }
    const StaticScene::PointLight*       getPointLight(int i) const { return pointLights_[i]; }
    const StaticScene::SpotLight*        getSpotLight(int i) const { return spotLights_[i]; }

    void setCamera(Camera* cam) { camera_ = cam; }

  private:

    Camera* camera_;

    std::set<SceneObject*> objects_;
    std::set<SceneLight*> lights_;
    std::vector<StaticScene::DirectionalLight*> directionalLights_;
    std::vector<StaticScene::PointLight*> pointLights_;
    std::vector<StaticScene::SpotLight*> spotLights_;
    std::vector<float> spotLightRotationSpeeds_;
    float           spotLightRotationSpeedRange_ = 0.1;
    //std::vector<StaticScene::InfiniteHemisphereLight*> hemi_lights;
    //std::vector<StaticScene::AreaLight*> area_lights;
    //std::vector<StaticScene::SphereLight*> sphere_lights;
    GLResourceManager* gl_mgr_;
    // resources for shadow mapping
    bool            doShadowPass_;
    int             shadowTextureSize_;
    Shader*         shadowShader_;
    Shader*         shadowVizShader_;
    FrameBufferId   shadowFrameBufferId_[SCENE_MAX_SHADOWED_LIGHTS];
    Matrix4x4       worldToShadowLight_[SCENE_MAX_SHADOWED_LIGHTS];
    TextureArrayId  shadowDepthTextureArrayId_;
    TextureArrayId  shadowColorTextureArrayId_;
    // OpenGL vertex array object
    VertexArrayId   shadowVizVertexArrayId_;
    // OpenGL vertex buffer objects
    VertexBufferId  shadowVizVtxBufferId_;
    VertexBufferId  shadowVizTexCoordBufferId_;
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

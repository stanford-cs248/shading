#include "scene.h"

#include <fstream>

#include "../gl_utils.h"
#include "mesh.h"

using namespace std;
using std::cout;
using std::endl;

namespace CS248 {
namespace DynamicScene {

namespace {

Matrix4x4 createPerspectiveMatrix(float fovy, float aspect, float near, float far) {

  float f = 1.0 / tan(radians(fovy)/2.0);

  Matrix4x4 m;
  m[0][0] = f / aspect;
  m[0][1] = 0.f;
  m[0][2] = 0.f;
  m[0][3] = 0.f;

  m[1][0] = 0.f;  
  m[1][1] = f;
  m[1][2] = 0.f;
  m[1][3] = 0.f;

  m[2][0] = 0.f;
  m[2][1] = 0.f;
  m[2][2] = (far + near) / (near - far);
  m[2][3] = -1.f;

  m[3][0] = 0.f;
  m[3][1] = 0.f;
  m[3][2] = (2.f * far * near) / (near - far);
  m[3][3] = 0.0;

  return m;
}

Matrix4x4 createWorldToCameraMatrix(const Vector3D& eye, const Vector3D& at, const Vector3D& up) {

  // TODO CS248 Part 1: Coordinate transform
  // Compute the matrix that transforms a point in world space to a point in camera space.

  return Matrix4x4::translation(Vector3D(-20,0,-150));

}

// Creates two triangles (6 positions, 18 floats) making up a square
// The square uniformly samples the texture space (6 vertices, 12 floats).
// Returns the vertex position and texcoord buffers
std::pair<std::vector<float>, std::vector<float>> getTextureVizBuffers(float z) {
  std::vector<float> vtx = {
    -1, -1, z,
    1, -1, z,
    1, 1, z,
    -1, -1, z,
    1, 1, z,
    -1, 1, z
  };
  std::vector<float> texcoords = {
    0, 0,
    1, 0,
    1, 1,
    0, 0,
    1, 1,
    0, 1
  };
  return std::make_pair(vtx, texcoords);
}

}  // namespace


Scene::Scene(std::vector<SceneObject*> argObjects,
             std::vector<SceneLight*>  argLights,
             const std::string& baseShaderDir) {

    for (int i = 0; i < argObjects.size(); i++) {
        argObjects[i]->setScene(this);
        objects_.insert(argObjects[i]);
    }

    for (int i = 0; i < argLights.size(); i++) {
        lights_.insert(argLights[i]);
    }

    for (SceneLight* sl : lights_) {
        StaticScene::SceneLight* light = sl->getStaticLight();
        if (dynamic_cast<StaticScene::DirectionalLight*>(light)) {
            directionalLights_.push_back((StaticScene::DirectionalLight*)light);
        }
        if (dynamic_cast<StaticScene::PointLight*>(light)) {
            pointLights_.push_back((StaticScene::PointLight*)light);
        }
        if (dynamic_cast<StaticScene::SpotLight*>(light)) {
            spotLights_.push_back((StaticScene::SpotLight*)light);
            float delta = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * spotLightRotationSpeedRange_ - spotLightRotationSpeedRange_ / 2.0;
            spotLightRotationSpeeds_.push_back(delta);
        }
        // if (dynamic_cast<StaticScene::InfiniteHemisphereLight *>(light)) {
        //     hemiLights.push_back((StaticScene::InfiniteHemisphereLight*)light);
        // }
        // if (dynamic_cast<StaticScene::AreaLight*>(light)) {
        //     areaLights.push_back((StaticScene::AreaLight*)light);
        // }
    }

    // the following code creates frame buffer objects to render shadows

    checkGLError("pre shadow fb setup");

    doShadowPass_ = false;

    if (getNumShadowedLights() > 0) {

        printf("Setting up shadow assets\n");

        doShadowPass_ = true;
        shadowTextureSize_ = 1024;

        gl_mgr_ = GLResourceManager::instance();
    
        for (int i=0; i<getNumShadowedLights(); i++) {

          shadowFrameBufferId_[i] = gl_mgr_->createFrameBuffer();
	        checkGLError("after creating framebuffer");

        }

        std::tie(shadowDepthTextureArrayId_, shadowColorTextureArrayId_) = gl_mgr_->createDepthAndColorTextureArrayFromFrameBuffers(
            shadowFrameBufferId_, getNumShadowedLights(), shadowTextureSize_);
        checkGLError("after binding shadow texture as attachment");

        // glDrawBuffer(GL_NONE); // No color buffer is drawn to
        // glReadBuffer(GL_NONE); // No color is read from

        for (int i=0; i<getNumShadowedLights();i++) {
            // sanity check
            if (!gl_mgr_->checkFrameBuffer(shadowFrameBufferId_[i])) {
                exit(1);
            }
        }

        printf("Done setting up shadow assets\n");

        checkGLError("post shadow framebuffer setup");
        
        printf("Creating shadow shaders\n");

        // create shader object for shadow passes
        string sepchar("/");
        shadowShader_ = new Shader(baseShaderDir + sepchar + "shadow_pass.vert",
                                  baseShaderDir + sepchar + "shadow_pass.frag");
        checkGLError("post shadow shader compile");
        // checkGLError("post shadow shader debug compile");
        shadowVizShader_ = new Shader(baseShaderDir + sepchar + "shadow_viz.vert",
                                     baseShaderDir + sepchar + "shadow_viz.frag");

        std::vector<float> vtx, texcoords;
        std::tie(vtx, texcoords) = getTextureVizBuffers(/*z=*/0.0);
        shadowVizVertexArrayId_ = gl_mgr_->createVertexArray();
        shadowVizVtxBufferId_ = gl_mgr_->createVertexBufferFromData(vtx.data(), vtx.size());
        shadowVizTexCoordBufferId_ = gl_mgr_->createVertexBufferFromData(texcoords.data(), texcoords.size());
        checkGLError("post shadow viz shader compile");

        printf("Shaders created.\n");
    }

    checkGLError("returning from Scene::Scene");  
}

Scene::~Scene() { }

size_t Scene::getNumShadowedLights() const {
    // for now, assume all spotlights (up to SCENE_MAX_SHADOWED_LIGHTS) are shadowed
    return std::min((int)spotLights_.size(), SCENE_MAX_SHADOWED_LIGHTS);
}

BBox Scene::getBBox() const {
    BBox bbox;
    for (SceneObject *obj : objects_) {
        bbox.expand(obj->getBBox());
    }
    return bbox;
}

void Scene::reloadShaders() {

    checkGLError("begin Scene::reloadShaders");

    printf("Reloading all shaders.\n");

    // FIXME(kayvonf): this breaks the abstraction that the shader class is the only place
    // where shader program bindings are changed.  Fix this later.  We may not need it at all.
    glUseProgram(0);


    if (getNumShadowedLights() > 0) {
      shadowShader_->reload();
      shadowVizShader_->reload();
    }

    for (SceneObject *obj : objects_)
        obj->reloadShaders();

    checkGLError("end Scene::reloadShaders");
}

void Scene::render() {
  
    checkGLError("begin Scene::render");

    Matrix4x4 worldToCamera = createWorldToCameraMatrix(camera_->getPosition(), camera_->getViewPoint(), camera_->getUpDir());
    Matrix4x4 proj = createPerspectiveMatrix(camera_->getVFov(), camera_->getAspectRatio(), camera_->getNearClip(), camera_->getFarClip());  
    Matrix4x4 worldToCameraNDC = proj * worldToCamera;

    for (SceneObject *obj : objects_)
        obj->draw(worldToCameraNDC);

    checkGLError("end Scene::render");

}

void Scene::renderShadowPass(int shadowedLightIndex) {

    checkGLError("begin shadow pass");

    Vector3D lightDir  = spotLights_[shadowedLightIndex]->direction;
    Vector3D lightPos  = spotLights_[shadowedLightIndex]->position;
    float    coneAngle = spotLights_[shadowedLightIndex]->angle;

    // I'm making the fovy (field of view in y direction) of the shadow map
    // rendering a bit larger than the cone angle just to be safe. Clamp at 60 degrees.
    float fovy = std::max(1.4f * coneAngle, 60.0f);
    float aspect = 1.0f;
    float near = 10.f;
    float far = 400.;

    // TODO CS248 Part 5.2 Shadow Mapping
    // Here we render the shadow map for the given light. You need to accomplish the following:
    // (1) You need to use gl_mgr_->bindFrameBuffer on the correct framebuffer to render into.
    // (2) You need to compute the correct worldToLightNDC matrix to pass into drawShadow by
    //     pretending there is a camera at the light source looking at the scene. Some fake camera
    //     parameters are provided to you in the code above.
    // (3) You need to compute a worldToShadowLight matrix that takes the point in world space and
    //     transforms it into "light space" for the fragment shader to use to sample from the shadow map.
    //     Note that this is almost worldToLightNDC with an additional transform that converts 
    //     coordinates in the [-w,w]^3 normalized device coordinate box 
    //     (the result of the perspective projection transform) to coordinates in a [0,w]^3 volume.
    //     After homogeneous divide. this means that x,y correspond to valid texture
    //     coordinates in the [0,1]^2 domain that can be used for a shadow map lookup in the shader.
    //     You should put it in the right place in worldToShadowLight_ array.
    // Caveat: GLResourceManager::bindFrameBuffer uses the RAII idiom (https://en.cppreference.com/w/cpp/language/raii)
    //     Which means you have to give the return value a name since its destructor will release the binding.
    //     Bad:
    //       gl_mgr_->bindFrameBuffer(100);  // Return value is destructed immediately!
    //       drawTriangles();  //  <- Framebuffer 100 is not bound!!!
    //     Good:
    //       auto fb_bind = gl_mgr_->bindFrameBuffer(100);
    //       drawTriangles();  //  <- Framebuffer 100 is bound, since fb_bind is still alive here.
    // 
    // Replaces the following lines with correct implementation.
    Matrix4x4 worldToLightNDC = Matrix4x4::identity();
    worldToShadowLight_[shadowedLightIndex].zero();

    glViewport(0, 0, shadowTextureSize_, shadowTextureSize_);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Now draw all the objects in the scene
    for (SceneObject *obj : objects_)
        obj->drawShadow(worldToLightNDC);

    checkGLError("end shadow pass");
    
}

void Scene::visualizeShadowMap() {
    checkGLError("pre viz shadow map");

    auto vertex_array_bind = gl_mgr_->bindVertexArray(shadowVizVertexArrayId_);
    auto shader_bind = shadowVizShader_->bind();
    shadowVizShader_->setVertexBuffer("vtx_position", 3, shadowVizVtxBufferId_);
    shadowVizShader_->setVertexBuffer("vtx_texcoord", 2, shadowVizTexCoordBufferId_);
    shadowVizShader_->setTextureArraySampler("depthTextureArray", shadowDepthTextureArrayId_);
    shadowVizShader_->setTextureArraySampler("colorTextureArray", shadowColorTextureArrayId_);
    // now issue the draw command to OpenGL
    checkGLError("before glDrawArrays for shadow viz");
    // 6 indices, 2 triangles to render
    glDrawArrays(GL_TRIANGLES, /*first=*/0, /*count=*/6);

    checkGLError("post viz shadow map");
}

void Scene::rotateSpotLights() {
  for (int i = 0; i < getNumSpotLights(); ++i) {
    spotLights_[i]->rotate(spotLightRotationSpeeds_[i]);
  }
}

}  // namespace DynamicScene
}  // namespace CS248

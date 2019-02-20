#include "scene.h"
#include "mesh.h"
#include <fstream>

using namespace std;
using std::cout;
using std::endl;

namespace CS248 {
namespace DynamicScene {

Scene::Scene(std::vector<SceneObject *> _objects,
             std::vector<SceneLight *> _lights,
             const std::string& base_shader_dir) {

  for (int i = 0; i < _objects.size(); i++) {
    _objects[i]->scene = this;
    objects.insert(_objects[i]);
  }

  for (int i = 0; i < _lights.size(); i++) {
    lights.insert(_lights[i]);
  }

  for(SceneLight *slo : lights) {
    StaticScene::SceneLight *light = slo->get_static_light();
    if(dynamic_cast<StaticScene::DirectionalLight *>(light)) {
        directional_lights.push_back((StaticScene::DirectionalLight *)light);
    }
    if(dynamic_cast<StaticScene::InfiniteHemisphereLight *>(light)) {
        hemi_lights.push_back((StaticScene::InfiniteHemisphereLight *)light);
    }
    if(dynamic_cast<StaticScene::PointLight *>(light)) {
        point_lights.push_back((StaticScene::PointLight *)light);
    }
    if(dynamic_cast<StaticScene::SpotLight *>(light)) {
        spot_lights.push_back((StaticScene::SpotLight *)light);
    }
    if(dynamic_cast<StaticScene::AreaLight *>(light)) {
        area_lights.push_back((StaticScene::AreaLight *)light);
    }
    if(dynamic_cast<StaticScene::SphereLight *>(light)) {
        sphere_lights.push_back((StaticScene::SphereLight *)light);
    }
  }
  current_pattern_id = 0;
  current_pattern_subid = 0;
  scaling_factor = .05f;

  // create a frame buffer object to render shadows into

  checkGLError("pre shadow fb setup");
  
  do_shadow_pass = false;

  if (get_num_shadowed_lights() > 0) {

    do_shadow_pass = true;
    shadow_texture_size = 1024;
    
    for (int i=0; i<get_num_shadowed_lights(); i++) {

      glGenFramebuffers(1, &shadow_framebuffer[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer[i]);

      glGenTextures(1, &shadow_texture[i]);
      glBindTexture(GL_TEXTURE_2D, shadow_texture[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadow_texture_size, shadow_texture_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glGenTextures(1, &shadow_color_texture[i]);
      glBindTexture(GL_TEXTURE_2D, shadow_color_texture[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadow_texture_size, shadow_texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadow_color_texture[i], 0);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_texture[i], 0);
      //glDrawBuffer(GL_NONE); // No color buffer is drawn to
      glReadBuffer(GL_NONE); // No color is read from   
      
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
          printf("Error: Frame buffer %d is not complete\n", i);
          GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
          switch (status) {
          case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
              fprintf(stderr, "Incomplete draw buffer\n");
              break;
          case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
              fprintf(stderr, "Incomplete read buffer\n");
              break;
          default:
              fprintf(stderr, "Unknown reason why fb is complete.\n");
          }
      }
    }

    checkGLError("post shadow framebuffer setup");
    
    // restore the screen as the render target
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // create shader object for shadow passes
    string sepchar("/");
    shadow_shader = new Shader(base_shader_dir + sepchar + "shadow_pass.vert",
                               base_shader_dir + sepchar + "shadow_pass.frag", "", "");
    checkGLError("post shadow shader compile");
    shadow_shader2 = new Shader(base_shader_dir + sepchar + "shadow_pass_debug.vert",
                                base_shader_dir + sepchar + "shadow_pass.frag", "", "");
    checkGLError("post shadow shader2 compile");
    shadow_viz_shader = new Shader(base_shader_dir + sepchar + "shadow_viz.vert",
                                   base_shader_dir + sepchar + "shadow_viz.frag", "", "");
    checkGLError("post shadow viz shader compile");
  }

  checkGLError("returning from Application::init");  
}


Scene::~Scene() { }

BBox Scene::get_bbox() {
  BBox bbox;
  for (SceneObject *obj : objects) {
    bbox.expand(obj->get_bbox());
  }
  return bbox;
}

bool Scene::addObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i != objects.end()) {
    return false;
  }

  o->scene = this;
  objects.insert(o);
  return true;
}

bool Scene::removeObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i == objects.end()) {
    return false;
  }

  objects.erase(o);
  return true;
}

void Scene::render_in_opengl() {
    for (SceneObject *obj : objects)
      if (obj->isVisible)
        obj->draw();
}

void Scene::visualize_shadow_map() {

    checkGLError("pre viz shadow map");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_color_texture[0]);
    //glBindTexture(GL_TEXTURE_2D, shadow_texture[0]);

    glUseProgram(shadow_viz_shader->_programID);

    glBegin(GL_TRIANGLES);
    float z = 0.0;

    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, z);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, z);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, z);

    //////

    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, z);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, z);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, z);

    glEnd();

    glUseProgram(0);

    checkGLError("post viz shadow map");
}

// helper
static Matrix4x4 glToMatrix4x4(float* glMatrix) {

  Matrix4x4 m;
  int idx = 0;
  for (int j=0; j<4; j++)
    for (int i=0; i<4; i++)
      m[j][i] = glMatrix[idx++];
  
  return m;
}

int Scene::get_num_shadowed_lights() const {
  return std::min( (int)spot_lights.size(), SCENE_MAX_SHADOWED_LIGHTS);
}

void Scene::render_shadow_pass() {

    checkGLError("begin shadow pass");

    for (int i=0; i<get_num_shadowed_lights(); i++) {

      Vector3D light_dir = spot_lights[i]->direction;
      Vector3D light_pos = spot_lights[i]->position;
      float    cone_angle = spot_lights[i]->angle;

      //printf("Spot light dir %f %f %f\n", light_dir.x, light_dir.y, light_dir.z);
      //printf("Spot light pos %f %f %f\n", light_pos.x, light_pos.y, light_pos.z);
      //printf("Spot light angle %f\n", cone_angle);

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_framebuffer[i]);
      glViewport(0, 0, shadow_texture_size, shadow_texture_size);

      //glClear(GL_DEPTH_BUFFER_BIT);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      // I'm making the fovy (field of view in y direction) of the shadow map
      // rendering a bit larger than the cone angle just to be safe. Clamp at 60 degrees.
      float fovy = std::max(1.4f * cone_angle, 60.0f);
      gluPerspective(fovy, 1.0f, 10.0f, 400.f);
      
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      // The spot light is positioned at light_pos and looking in the given direction.
      // Therefore it is looking at a point given by light_pos + light_dir
      Vector3D lookat_pos = light_pos + light_dir;
      gluLookAt(light_pos.x, light_pos.y, light_pos.z,
                lookat_pos.x, lookat_pos.y, lookat_pos.z,   // look at center of scene
                0.0, 1.0, 0.0);  // Y up direction

      // Since the assignment code relies on the gluPerspective/gluLookAt for constructing
      // matrices, I retrieve the matrices here to form a world-to-light-space matrix.
      // A more modern approach approach is to maintain the matrices in the application
      // and then provide them to the pipeline as uniforms
      GLfloat cameraMatrix[16]; 
      GLfloat projMatrix[16]; 
      glGetFloatv(GL_MODELVIEW_MATRIX, cameraMatrix); 
      glGetFloatv(GL_PROJECTION_MATRIX, projMatrix); 

      // The bias matrix converts coordinates in the [-w,w]^3 normalized device coordinate box (the
      // result of the perspective projection transform) to coordinates in a [0,w]^3 volume.
      // After homogeneous divide. this means that x,y correspond to valid texture
      // coordinates in the [0,1]^2 domain that can be used for a shadow map lookup in the shader.
      // Notice that the matrix is just a scale and translation as to be expected.
      Matrix4x4 bias = Matrix4x4::translation(Vector3D(0.5,0.5,0.5)) * Matrix4x4::scaling(0.5);
      Matrix4x4 cam = glToMatrix4x4(cameraMatrix);
      Matrix4x4 proj = glToMatrix4x4(projMatrix);

      // printf("Shadow %d info:\n", i); 
      //cout << bias << endl << endl;
      // cout << cam  << endl << endl;
      //cout << proj << endl << endl;

      world_to_shadowlight[i] = bias * proj * cam;
      //
      // end world-to-shadow space transform construction hack
      //

      // Now draw all the objects in the scene
      for (SceneObject *obj : objects)
        if (obj->isVisible)
          obj->draw_shadow();

      /*
      glUseProgram(shadow_shader2->_programID);
      glBegin(GL_TRIANGLES);
      glVertex3f(-1, -1, 0);
      glVertex3f(1, -1, 0);
      glVertex3f(-1, 1, 0);
      glEnd();
      glUseProgram(0);
      */

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      checkGLError("end shadow pass");
    }

}
    
void Scene::prevPattern() {
    current_pattern_id = min(max((int)current_pattern_id - 1, (int)0), (int)patterns.size() - 1);
}

void Scene::nextPattern() {
    current_pattern_id = min(max((int)current_pattern_id + 1, (int)0), (int)patterns.size() - 1);
}

void Scene::decreaseCurrentPattern(double scale) {
    if(patterns.size() > 0) {
        int type = patterns[current_pattern_id].type;
        if(type == 0) {
            patterns[current_pattern_id].v[current_pattern_subid] -= scaling_factor;
        }
        else if(type == 1) {
            patterns[current_pattern_id].s -= scaling_factor;
            patterns[current_pattern_id].s = (patterns[current_pattern_id].s < 0.f) ? 0.f : patterns[current_pattern_id].s;
        }
    }
}

void Scene::increaseCurrentPattern(double scale) {
    if(patterns.size() > 0) {
        int type = patterns[current_pattern_id].type;
        if(type == 0)
            patterns[current_pattern_id].v[current_pattern_subid] += scaling_factor;
        else if(type == 1) {
            patterns[current_pattern_id].s += scaling_factor;
            patterns[current_pattern_id].s = (patterns[current_pattern_id].s > 1.0f) ? 1.0f : patterns[current_pattern_id].s;
        }
    }
}

StaticScene::Scene *Scene::get_static_scene() {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_static_object();
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }

  return new StaticScene::Scene(staticObjects, staticLights);
}

StaticScene::Scene *Scene::get_transformed_static_scene(double t) {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_transformed_static_object(t);
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }
  return new StaticScene::Scene(staticObjects, staticLights);
}

Matrix4x4 SceneObject::getTransformation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::translation(position) *
         Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z) *
         Matrix4x4::scaling(scale);
}

Matrix4x4 SceneObject::getRotation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z);
}

}  // namespace DynamicScene
}  // namespace CS248

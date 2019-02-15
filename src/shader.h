#ifndef CS248_SHADER_H
#define CS248_SHADER_H

#include "static_scene/scene.h"
using CS248::StaticScene::Scene;

#include "GL/glew.h"

namespace CS248 {


/**
 * A shader
 */
class Shader {
 public:

  /**
   * Default constructor.
   * Creates a new pathtracer instance.
   */
  Shader(std::string vertex_shader_filename, std::string fragment_shader_filename, std::string vertex_shader_content_prefix = "", std::string fragment_shader_content_prefix = "");

  /**
   * Destructor.
   * Frees all the internal resources used by the pathtracer.
   */
  ~Shader();

  bool read(std::string filename, std::string& contents);
  bool compileAndAttachShader( GLuint& shaderID, GLenum shaderType, const char* shaderTypeStr, std::string filename, std::string &contents, std::string prefix = "" );
  bool link();

    // contents/filenames for the shaders
    std::string _vertexShaderFilename;
    std::string _vertexShaderString;

    std::string _geometryShaderFilename;
    std::string _geometryShaderString;

    std::string _fragmentShaderFilename;
    std::string _fragmentShaderString;

    // IDs of the different objects
    GLuint _vertexShaderID;
    GLuint _geometryShaderID;
    GLuint _fragmentShaderID;
    GLuint _programID;

    // where we keep track of the textures and where they are bound
    int _firstAvailableTextureUnit;
    //std::vector<BoundTexture> _boundTextures;

    // whether to actually print any GLSL errors that occur
    bool _printErrors;
};

}  // namespace CS248

#endif  // CS248_SHADER_H

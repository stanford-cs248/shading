#ifndef CS248_SHADER_H
#define CS248_SHADER_H

#include <map>

#include "CS248/matrix3x3.h"
#include "CS248/matrix4x4.h"
#include "CS248/vector3D.h"
#include "CS248/vector4D.h"

#include "GL/glew.h"

#include "gl_resource_manager.h"

namespace CS248 {

/**
 * A shader
 */
class Shader {
 public:

    // Constructor
    Shader();

    // Constructor: loads and compiles the specified vertex and fragment shaders 
    Shader(std::string vertex_shader_filename, std::string fragment_shader_filename);

    // Destructor
    ~Shader();

    // reload the shaders and recompile
    void reload();

    // bind the shader to the graphics pipeline (this shader will be used for subsequent draw calls until the returned cleanup goes out of scope)
    std::unique_ptr<Cleanup> bind();

    // the following are all for setting shading parameters
    bool setScalarParameter(const std::string& paramName, int value);
    bool setScalarParameter(const std::string& paramName, float value);
    bool setVectorParameter(const std::string& paramName, const Vector3D& value);
    bool setVectorParameter(const std::string& paramName, const Vector4D& value);
    bool setMatrixParameter(const std::string& paramName, const Matrix3x3& value);
    bool setMatrixParameter(const std::string& paramName, const Matrix4x4& value);
    bool setVertexBuffer(const std::string& paramName, int fieldsPerAttribute, VertexBufferId vertexBufferId);
    bool setTextureSampler(const std::string& paramName, TextureId textureId);
    bool setTextureArraySampler(const std::string& paramName, TextureArrayId textureArrayId);

  private:

    void init();
    void cleanup();
    bool createFullProgram();
    bool linkProgram();
    bool createVertexShader(const std::string& filename);
    bool createFragmentShader(const std::string& filename);
    bool prepareSourceCode(const std::string& filename, std::string* out_source);
    int getTextureUnitForParam(const std::string& name);

    GLResourceManager* gl_mgr_ = nullptr;

    // source filenames
    std::string vertexShaderFilename_;
    std::string fragmentShaderFilename_;

    // IDs of the different Open GL objects associated with this shader program
    ShaderId vertexShaderId_;
    ShaderId fragmentShaderId_;
    ProgramId programId_;

    std::map<std::string, int> paramNameToTextureUnit_;

    bool abort_if_error_during_init_ = true;
};

}  // namespace CS248


#endif  // CS248_SHADER_H

#include "shader.h"

#include <fstream>
#include <string>
#include <iostream>

#include "gl_utils.h"

namespace CS248 {
namespace {

using std::cerr;
using std::endl;

void convertToGLMatrix(const Matrix3x3& m, float* buf) {
    int idx = 0;
    for (int i=0; i<3; i++) {
        const Vector4D& c = m.column(i); 
        buf[idx++] = c[0]; buf[idx++] = c[1]; buf[idx++] = c[2];
    } 
}

void convertToGLMatrix(const Matrix4x4& m, float* buf) {
    int idx = 0;
    for (int i=0; i<4; i++) {
        const Vector4D& c = m.column(i); 
        buf[idx++] = c[0]; buf[idx++] = c[1]; buf[idx++] = c[2]; buf[idx++] = c[3];
    }
}

// read contents of file into a string
bool readFile(const std::string& filename, std::string& contents) {
    contents = "";
    std::ifstream file;

    // try to open the file
    file.open(filename.c_str() );
    if (!file.is_open())
        return false;
          
    // read it line by line
    std::string line;
      
    while(!file.eof()) {
        getline( file, line );
        contents += line + "\n";
    }
     
    file.close();  
    return true;
}

}  // namespace



Shader::Shader(std::string vertex_shader_filename, std::string fragment_shader_filename)
    : vertexShaderFilename_(vertex_shader_filename), fragmentShaderFilename_(fragment_shader_filename) {
    gl_mgr_ = GLResourceManager::instance();
    init();
    bool success = createFullProgram();
    if (!success && abort_if_error_during_init_) {
      exit(1);
    }
}

Shader::~Shader() {
    cleanup();
}

void Shader::init() {
    abort_if_error_during_init_ = true;
    vertexShaderId_ = ShaderId{0};
    fragmentShaderId_ = ShaderId{0};
    programId_ = ProgramId{0};
    paramNameToTextureUnit_.clear();
}

// creates the shader program object.  This involves loading and compiling all shaders.
bool Shader::createFullProgram() {
  programId_ = gl_mgr_->createProgram();
  bool success = true;
  // compile the vertex and fragment shader objects, and then attach them to the program object
  if (!createVertexShader(vertexShaderFilename_)) {
    cerr << vertexShaderFilename_ << " failed" << endl;
    success = false;
  }

  if (!createFragmentShader(fragmentShaderFilename_)) {
    cerr << fragmentShaderFilename_ << " failed" << endl;
    success = false;
  }


  if (success) {
    // attach and link the compiled vertex shader with the compiled fragment shader to
    // create a full shader program
    if (!linkProgram()) {
      cerr << "Failed to link " << vertexShaderFilename_ << " with " << fragmentShaderFilename_ << endl;
      success = false;
    }
  }

  return success;
}

// delete all associate GLSL program and shader objects
void Shader::cleanup() {
    // note: OpenGL API docs say it's okay to call delete on handles that do not point to
    // allocated objects, so that is why there are no "if exists" checks here. 
    gl_mgr_->freeShader(vertexShaderId_);
    gl_mgr_->freeShader(fragmentShaderId_);
    gl_mgr_->freeProgram(programId_);
}

// reload and recompile shaders
void Shader::reload() {
    cleanup();
    init();
    createFullProgram();
}



bool Shader::prepareSourceCode(const std::string& filename, std::string* out_source) {
  if (!readFile(filename, *out_source)) {
    return false;
  }
#ifdef __APPLE__
  std::string version = "#version 150\n";
#else
  std::string version = "#version 130\n";
#endif 
  *out_source  = version + (*out_source);
  return true;
}

bool Shader::createVertexShader(const std::string& filename) {
  std::string contents;
  if (!prepareSourceCode(filename, &contents)) {
    cerr << "Failed to read " << filename << endl;
    return false;
  }
  const char* source = contents.c_str();
  if (!gl_mgr_->createVertexShader(source, &vertexShaderId_)) {
    return false;
  }
  return true;
}

bool Shader::createFragmentShader(const std::string& filename) {
  std::string contents;
  if (!prepareSourceCode(filename, &contents)) {
    cerr << "Failed to read " << filename << endl;
    return false;
  }
  const char* source = contents.c_str();
  if (!gl_mgr_->createFragmentShader(source, &fragmentShaderId_)) {
    return false;
  }
  return true;
}

bool Shader::linkProgram() {
  std::vector<ShaderId> shaders = {vertexShaderId_, fragmentShaderId_};
  return gl_mgr_->attachShadersAndLinkProgram(programId_, shaders);
}

std::unique_ptr<Cleanup> Shader::bind() {
  return gl_mgr_->bindProgram(programId_);
}

bool Shader::setScalarParameter(const std::string& paramName, int value) {

    bool success = true;
    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniform1i(uniformLocation, value);
    else
        success = false;

    return success;
}

bool Shader::setScalarParameter(const std::string& paramName, float value) {

    bool success = true;
    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniform1f(uniformLocation, value);
    else
        success = false;

    return success;
}

bool Shader::setVectorParameter(const std::string& paramName, const Vector3D& value) {

    bool success = true;
    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniform3f(uniformLocation, value.x, value.y, value.z);
    else 
        success = false;

    return success;
}

bool Shader::setVectorParameter(const std::string& paramName, const Vector4D& value) {

    bool success = true;
    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniform4f(uniformLocation, value.x, value.y, value.z, value.w);
    else 
        success = false;

    return success;
}

bool Shader::setMatrixParameter(const std::string& paramName, const Matrix3x3& value) {

    bool success = true;

    float buf[9];
    convertToGLMatrix(value, buf);

    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, buf);
    else 
        success = false;

    return success;
}

bool Shader::setMatrixParameter(const std::string& paramName, const Matrix4x4& value) {

    bool success = true;

    float buf[16];
    convertToGLMatrix(value, buf);

    int uniformLocation = glGetUniformLocation(programId_.id, paramName.c_str());

    if (uniformLocation >= 0)
        glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, buf);
    else 
        success = false;

    return success;
}

bool Shader::setVertexBuffer(const std::string& paramName, int fieldsPerAttribute, VertexBufferId vertexBufferId) {

    return gl_mgr_->setVertexBuffer(programId_, paramName, fieldsPerAttribute, vertexBufferId);
}

int Shader::getTextureUnitForParam(const std::string& name) {
  auto result = paramNameToTextureUnit_.find(name);
  int textureUnit = -1;
  if (result != paramNameToTextureUnit_.end()) {
    textureUnit = result->second;
  } else {
    textureUnit = paramNameToTextureUnit_.size();
    paramNameToTextureUnit_[name] = textureUnit;
  }
  return textureUnit;
}

bool Shader::setTextureSampler(const std::string& paramName, TextureId textureId) {
  return gl_mgr_->setTextureSampler(programId_, paramName, textureId, getTextureUnitForParam(paramName));
}

bool Shader::setTextureArraySampler(const std::string& paramName, TextureArrayId textureArrayId) {
    return gl_mgr_->setTextureArraySampler(programId_, paramName, textureArrayId, getTextureUnitForParam(paramName));
}



}  // namespace CS248

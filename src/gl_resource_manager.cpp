#include "gl_resource_manager.h"

#include <iostream>
#include "GL/glew.h"

namespace CS248 {
namespace {

using std::cerr;
using std::endl;

class FrameBufferCleanup : public Cleanup {
 public:
  FrameBufferCleanup(FrameBufferId fbid) {
  	glBindFramebuffer(GL_FRAMEBUFFER, fbid.id);
  }
  ~FrameBufferCleanup() {
  	glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
};

class TextureCleanup : public Cleanup {
 public:
  template<typename TextureTypeId>
  TextureCleanup(GLenum texture_type, TextureTypeId texid) : texture_type_(texture_type) {
  	glBindTexture(texture_type_, texid.id);
  }
  ~TextureCleanup() {
  	glBindTexture(texture_type_, 0);
  }
 private:
  GLenum texture_type_;
};

class VertexArrayCleanup : public Cleanup {
 public:
  VertexArrayCleanup(VertexArrayId vaid) {
    glBindVertexArray(vaid.id);
  }
  ~VertexArrayCleanup() {
    glBindVertexArray(0);
  }
};

class VertexBufferCleanup : public Cleanup {
 public:
  VertexBufferCleanup(VertexBufferId vbid) {
    glBindBuffer(GL_ARRAY_BUFFER, vbid.id);
  }
  ~VertexBufferCleanup() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
};

class ProgramCleanup : public Cleanup {
 public:
  ProgramCleanup(ProgramId pid) {
    glUseProgram(pid.id);
  }
  ~ProgramCleanup() {
    glUseProgram(0);
  }
};

bool createShaderOfType(const char* source, GLenum shaderType, ShaderId* out_sid) {
  ShaderId sid{glCreateShader(shaderType)};
  glShaderSource(sid.id, /*count=*/1, &source, /*length=*/NULL);
  glCompileShader(sid.id);

  GLint compileStatus;
  glGetShaderiv(sid.id, GL_COMPILE_STATUS, &compileStatus);
  bool success = (compileStatus == GL_TRUE);

  if (!success) {
        // get the length of the error message
        GLint errorMessageLength;
        glGetShaderiv(sid.id, GL_INFO_LOG_LENGTH, &errorMessageLength);

        // get the error message itself
        char* errorMessage = new char[errorMessageLength];
        glGetShaderInfoLog(sid.id, errorMessageLength, &errorMessageLength, errorMessage);

        // print the error
        cerr << "GLSL Compile Error:" << endl;
        cerr << "================================================================================" << endl;
        cerr << errorMessage << endl << endl;
        delete [] errorMessage;
  }
  *out_sid = sid;
  return success;
}

}  // namespace

// static
GLResourceManager* GLResourceManager::instance() {
  // Object with static storage is never freed.
  static GLResourceManager* singleton = new GLResourceManager();
  return singleton;
}

std::unique_ptr<Cleanup> GLResourceManager::bindFrameBuffer(FrameBufferId fbid) {
  return std::unique_ptr<Cleanup>{ new FrameBufferCleanup(fbid) };
}

std::unique_ptr<Cleanup> GLResourceManager::bindTexture(TextureId texid) {
  return std::unique_ptr<Cleanup>{ new TextureCleanup(GL_TEXTURE_2D, texid) };
}

std::unique_ptr<Cleanup> GLResourceManager::bindTextureArray(TextureArrayId texaid) {
  return std::unique_ptr<Cleanup>{ new TextureCleanup(GL_TEXTURE_2D_ARRAY, texaid) };
}

std::unique_ptr<Cleanup> GLResourceManager::bindVertexArray(VertexArrayId vaid) {
  return std::unique_ptr<Cleanup>{ new VertexArrayCleanup(vaid) };
}

std::unique_ptr<Cleanup> GLResourceManager::bindVertexBuffer(VertexBufferId vbid) {
  return std::unique_ptr<Cleanup>{ new VertexBufferCleanup(vbid) };
}

std::unique_ptr<Cleanup> GLResourceManager::bindProgram(ProgramId pid) {
  return std::unique_ptr<Cleanup>{ new ProgramCleanup(pid) };
}

ProgramId GLResourceManager::createProgram() {
  return {glCreateProgram()};
}

bool GLResourceManager::createVertexShader(const char* source_code, ShaderId* out_sid) {
  bool success = createShaderOfType(source_code, GL_VERTEX_SHADER, out_sid);
  if (!success) {
    cerr << "Above Errors are for vertex shader" << endl;
  }
  return success;
}

bool GLResourceManager::createFragmentShader(const char* source_code, ShaderId* out_sid) {
  bool success = createShaderOfType(source_code, GL_FRAGMENT_SHADER, out_sid);
  if (!success) {
    cerr << "Above Errors are for fragment shader" << endl;
  }
  return success;
}

bool GLResourceManager::attachShadersAndLinkProgram(ProgramId pid, const std::vector<ShaderId>& sids) {
  for (const auto& sid : sids) {
    glAttachShader(pid.id, sid.id);
  }
  glLinkProgram(pid.id);
  GLint linkedOK = 0;
  glGetProgramiv(pid.id, GL_LINK_STATUS, &linkedOK);

  bool success = (linkedOK == GL_TRUE);

  if (!success) {
    // get the length of the error message
    GLint errorMessageLength;
    glGetProgramiv(pid.id, GL_INFO_LOG_LENGTH, &errorMessageLength);
    // get the error message itself
    char* errorMessage = new char[errorMessageLength];
    glGetProgramInfoLog(pid.id, errorMessageLength, &errorMessageLength, errorMessage);

    cerr << "GLSL Linker Error: " << linkedOK << endl;
    cerr << "================================================================================" << endl;
    cerr << errorMessage << endl << endl;
    delete [] errorMessage;
  }
  return success;
}



FrameBufferId GLResourceManager::createFrameBuffer() {
  GLuint id;
  glGenFramebuffers(1, &id);
  return {id};
}

TextureId GLResourceManager::createTexture() {
  GLuint id;
  glGenTextures(1, &id);
  return {id};
}

VertexArrayId GLResourceManager::createVertexArray() {
  GLuint id;
  glGenVertexArrays(1, &id);
  return {id};
}

VertexBufferId GLResourceManager::createVertexBufferFromData(const float* data, int num) {
  GLuint id;
  glGenBuffers(1, &id);
  VertexBufferId vbid{id};
  auto buffer_bind = bindVertexBuffer(vbid);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num, (const void*) data, GL_STATIC_DRAW);
  return vbid;
}

TextureId GLResourceManager::createTextureFromData(const unsigned char* data, int width, int height) {
  TextureId texid = createTexture();
  auto tex_bind = bindTexture(texid);
  glTexImage2D(GL_TEXTURE_2D, /*level=*/0, GL_RGB, width, height, /*border=*/0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
  //glGenerateMipmap(GL_TEXTURE_2D);
  return texid;
}

TextureId GLResourceManager::createDepthTextureFromFrameBuffer(FrameBufferId fbid, int texture_size) {
  TextureId texid = createTexture();
  {
  	auto tex_bind = bindTexture(texid);
    glTexImage2D(GL_TEXTURE_2D, /*level=*/0, GL_DEPTH_COMPONENT16, /*width=*/texture_size,
        /*height=*/texture_size, /*border=*/0, GL_DEPTH_COMPONENT, GL_FLOAT, /*data=*/0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  {
    auto fb_bind = bindFrameBuffer(fbid);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texid.id, /*level=*/0);
  }
  return texid;
}

std::pair<TextureArrayId, TextureArrayId> GLResourceManager::createDepthAndColorTextureArrayFromFrameBuffers(const FrameBufferId* fbids, int num, int texture_size) {
  // Texture Array is just one texture
  TextureArrayId depth_id{createTexture().id};
  {
    auto tex_bind = bindTextureArray(depth_id);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, /*level=*/0, GL_DEPTH_COMPONENT, /*width=*/texture_size,
        /*height=*/texture_size, /*depth=*/num, /*border=*/0, GL_DEPTH_COMPONENT, GL_FLOAT, /*data=*/0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  }
  TextureArrayId color_id{createTexture().id};
  {
    auto tex_bind = bindTextureArray(color_id);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, /*level=*/0, GL_RGB, /*width=*/texture_size,
        /*height=*/texture_size, /*depth=*/num, /*border=*/0, GL_RGBA, GL_UNSIGNED_BYTE, /*data=*/0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  }
  for (int i = 0; i < num; ++i) {
    auto fb_bind = bindFrameBuffer(fbids[i]);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_id.id, /*level=*/0, /*layer=*/i);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_id.id, /*level=*/0, /*layer=*/i);
  }
  return std::make_pair(depth_id, color_id);
}


TextureId GLResourceManager::createColorTextureFromFrameBuffer(FrameBufferId fbid, int texture_size) {
  TextureId texid = createTexture();
  {
  	auto tex_bind = bindTexture(texid);
  	glTexImage2D(GL_TEXTURE_2D, /*level=*/0, GL_RGB, /*width=*/texture_size,
        /*height=*/texture_size, /*border=*/0, GL_RGBA, GL_UNSIGNED_BYTE, /*data=*/0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  {
    auto fb_bind = bindFrameBuffer(fbid);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid.id, /*level=*/0);
  }
  return texid; 
}

bool GLResourceManager::checkFrameBuffer(FrameBufferId fbid) {
  auto fb_bind = bindFrameBuffer(fbid);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    cerr << "Error: Frame buffer " << fbid.id << " is not complete: ";
      switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
          cerr << "Incomplete draw buffer";
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
          cerr << "Incomplete read buffer";
          break;
		    case GL_FRAMEBUFFER_UNSUPPORTED:
		      cerr << "Unsupported operation for current GL implementation";
		      break;
		    case GL_FRAMEBUFFER_UNDEFINED:
          cerr << "Undefined framebuffer";
          break;
		    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		      cerr << "Incomplete attachment for frame buffer";
		      break;
		    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		      cerr << "Missing attachment for frame buffer";
		      break;
        default:
          cerr << "Other reason why fb is complete. Status code " << status;
       }
    cerr << endl;
    return false;
  }
  return true;
}

bool GLResourceManager::setTextureSampler(ProgramId pid, const std::string& paramName, TextureId texid, int textureUnit) {
    bool success = true;
    int textureLoc = glGetUniformLocation(pid.id, paramName.c_str());
    GLenum glTextureUnitEnum = GL_TEXTURE0 + textureUnit;
    if (textureLoc >= 0) {
        // bind the texture object given by texid to the pipeline.
        glActiveTexture(glTextureUnitEnum);
        // Cannot unbind this texture as it will point the active unit to an invalid texture
        glBindTexture(GL_TEXTURE_2D, texid.id);
        // make sure the shader knows with texture unit is providing data for the corresponding
        // shader sampler variable
        glUniform1i(textureLoc, textureUnit);
    } else {
        success = false;
    }
    return success;
}

bool GLResourceManager::setTextureArraySampler(ProgramId pid, const std::string& paramName, TextureArrayId texaid, int textureUnit) {
  bool success = true;
  int textureLoc = glGetUniformLocation(pid.id, paramName.c_str());
  GLenum glTextureUnitEnum = GL_TEXTURE0 + textureUnit;
  if (textureLoc >= 0) {
      // bind the texture object given by texid to the pipeline.
      glActiveTexture(glTextureUnitEnum);
      // Cannot unbind this texture as it will point the active unit to an invalid texture id 0.
      glBindTexture(GL_TEXTURE_2D_ARRAY, texaid.id);
      // make sure the shader knows with texture unit is providing data for the corresponding
      // shader sampler variable
      glUniform1i(textureLoc, textureUnit);
  } else {
      success = false;
  }
  return success;
}

bool GLResourceManager::setVertexBuffer(ProgramId pid, const std::string& paramName, int fieldsPerAttribute, VertexBufferId vbid) {
  bool success = true;
  int attribLoc = glGetAttribLocation(pid.id, paramName.c_str());
  if (attribLoc >= 0) {
      // make the specified vertex buffer object the active one
      auto buffer_bind = bindVertexBuffer(vbid);
      // create a vertex attribute that connects the shader's input attribute to the
      // currently active vertex buffer object
      glVertexAttribPointer(attribLoc, /*size=*/fieldsPerAttribute, GL_FLOAT, /*normalized=*/GL_FALSE, /*stride=*/0, /*pointer=*/0);
      glEnableVertexAttribArray(attribLoc);      
  } else {
      success = false;
  }

  return success;
}


void GLResourceManager::freeFrameBuffer(FrameBufferId fbid) { glDeleteFramebuffers(1, &fbid.id); }
void GLResourceManager::freeVertexArray(VertexArrayId vaid) { glDeleteVertexArrays(1, &vaid.id); }
void GLResourceManager::freeVertexBuffer(VertexBufferId vbid) { glDeleteBuffers(1, &vbid.id); }
void GLResourceManager::freeTexture(TextureId texid) { glDeleteTextures(1, &texid.id); }
void GLResourceManager::freeTextureArray(TextureArrayId texaid) { glDeleteTextures(1, &texaid.id); }
void GLResourceManager::freeShader(ShaderId sid) { glDeleteShader(sid.id); }
void GLResourceManager::freeProgram(ProgramId pid) { glDeleteProgram(pid.id); } 

}  // namespace CS248
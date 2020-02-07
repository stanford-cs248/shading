#ifndef CS248_GL_RESOURCE_MANAGER_H
#define CS248_GL_RESOURCE_MANAGER_H

#include <memory>
#include <utility>
#include <vector>
#include <string>

#include "GL/glew.h"

namespace CS248 {

namespace internal {

  template<typename Tag>
  struct GLIntId { GLuint id; };

  struct TextureTag {};
  struct TextureArrayTag {};
  struct FrameBufferTag {};	
  struct ProgramTag {};
  struct ShaderTag {};
  struct VertexArrayTag {};
  struct VertexBufferTag {};

}  // namespace internal

// Strong Types for GL ids for different types of resources
typedef internal::GLIntId<internal::ProgramTag> ProgramId;
typedef internal::GLIntId<internal::ShaderTag> ShaderId;
typedef internal::GLIntId<internal::VertexArrayTag> VertexArrayId;
typedef internal::GLIntId<internal::VertexBufferTag> VertexBufferId;
typedef internal::GLIntId<internal::TextureTag> TextureId;
typedef internal::GLIntId<internal::TextureArrayTag> TextureArrayId;
typedef internal::GLIntId<internal::FrameBufferTag> FrameBufferId;

class Cleanup {
 public:
  virtual ~Cleanup() {}
};

// GLResourceManager is not thread-safe.
// External synchronization is required if used by multiple threads.
class GLResourceManager {
 public:
  // There should only be one singleton instance of this class
  static GLResourceManager* instance();
  ~GLResourceManager() {}

  // Methods to allocate new resources

  // Creates a new program
  ProgramId createProgram();
  // Creates and compiles shaders from source code
  // If successful, function will return true and out_sid will be set to the newly allocated shader id.
  // If unsuccessful, will print to stderr and out_sid will not be modified and function will return false.
  bool createVertexShader(const char* source_code, ShaderId* out_sid);
  bool createFragmentShader(const char* source_code, ShaderId* out_sid);

  FrameBufferId createFrameBuffer();
  VertexArrayId createVertexArray();
  // Creates a vertex buffer by copying the given data buffer with `num` floats.
  VertexBufferId createVertexBufferFromData(const float* data, int num);
  // Creates a texture2D by copying the given data buffer of type unsigned char
  TextureId createTextureFromData(const unsigned char* data, int width, int height);
  // Create two Texture2D arrays from an array of `num` frame buffers.
  // The first texture array contains the depth images for each of the frame buffers.
  // The second texture array contains the color images for each of the frame buffers.
  std::pair<TextureArrayId, TextureArrayId> createDepthAndColorTextureArrayFromFrameBuffers(const FrameBufferId* fbids, int num, int texture_size);

  // Attach shaders to the program and link the program.
  // Shaders need to have successfully compiled.
  // If link is successful, will return true.
  // Otherwise will return false and print to stderr.
  bool attachShadersAndLinkProgram(ProgramId pid, const std::vector<ShaderId>& sids);

  // Methods to sanity check resources.
  // Writes errors to stderr.
  bool checkFrameBuffer(FrameBufferId fbid);

  // Methods to associate variables in the shader program to the allocated resources.
  bool setTextureSampler(ProgramId pid, const std::string& paramName, TextureId texid, int textureUnit);
  bool setTextureArraySampler(ProgramId pid, const std::string& paramName, TextureArrayId texaid, int textureUnit);
  // Needs to have a valid VertexArray bound in current context.
  bool setVertexBuffer(ProgramId pid, const std::string& paramName, int fieldsPerAttribute, VertexBufferId vbid);

  // Methods to bind a resource to the current GL context.
  // They all return a clean-up object that, upon going out of scope, will release the binding to the default.
  // Caveat: nested scoping of the same resource does not revert back to the surrounding scope:
  // {
  // 	// GLResourceManager* mgr;
  // 	// FrameBufferId fb1, fb2;
  // 	auto c1 = mgr->bindFrameBuffer(fb1);
  // 	// Do work on current framebuffer, which is fb1
  // 	{
  // 	  auto c2 = mgr->bindFrameBuffer(fb2);
  // 	  // Do work on current framebuffer, which is fb2
  // 	}
  // 	// Framebuffer binding is now reversed to default, which is 0, not fb1!
  // }
  std::unique_ptr<Cleanup> bindProgram(ProgramId pid);
  std::unique_ptr<Cleanup> bindFrameBuffer(FrameBufferId fbid);
  std::unique_ptr<Cleanup> bindVertexArray(VertexArrayId vaid);

  // Methods to free the allocated resource
  void freeFrameBuffer(FrameBufferId fbid);
  void freeVertexArray(VertexArrayId vaid);
  void freeVertexBuffer(VertexBufferId vbid);
  void freeTexture(TextureId texid);
  void freeTextureArray(TextureArrayId texaid);
  void freeShader(ShaderId sid);
  void freeProgram(ProgramId pid);

 private:
  GLResourceManager() {}
  TextureId createTexture();
  TextureId createDepthTextureFromFrameBuffer(FrameBufferId fbid, int texture_size);
  TextureId createColorTextureFromFrameBuffer(FrameBufferId fbid, int texture_size);
  std::unique_ptr<Cleanup> bindTexture(TextureId texid);
  std::unique_ptr<Cleanup> bindTextureArray(TextureArrayId texaid);
  std::unique_ptr<Cleanup> bindVertexBuffer(VertexBufferId vbid);
};
	
}  // namespace CS248


#endif  // CS248_GL_RESOURCE_MANAGER_H

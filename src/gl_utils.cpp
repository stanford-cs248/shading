#include "gl_utils.h"

#include <iostream>
#include "GL/glew.h"

namespace CS248 {

namespace {
  using std::cerr;
	using std::endl;
}  // namespace

void checkGLError(const std::string& str, bool abort_program_if_error) {
  GLenum err;
  bool has_error = false;
  do {
    err = glGetError();
    if (err != GL_NO_ERROR) {
       cerr << "*** GL error:" << str << " : " << gluErrorString(err) << endl;
       has_error = true;
	  }
  } while (err != GL_NO_ERROR);
  if (has_error && abort_program_if_error) {
    exit(1);
  }
}

void checkGLError(const std::string& str) {
  checkGLError(str, /*abort_program_if_error=*/false);
}

}  // namespace CS248
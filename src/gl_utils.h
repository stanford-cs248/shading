#ifndef CS248_GL_UTILS_H
#define CS248_GL_UTILS_H

#include <string>

namespace CS248 {

void checkGLError(const std::string& msg, bool abort_program_if_error);
void checkGLError(const std::string& msg);

}  // namespace CS248

#endif  // CS248_GL_UTILS_H

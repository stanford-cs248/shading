#include "shader.h"
#include <fstream>
#include <string>

using namespace CS248::StaticScene;

namespace CS248 {

Shader::Shader(std::string vertex_shader_filename, std::string fragment_shader_filename, std::string vertex_shader_content_prefix, std::string fragment_shader_content_prefix)
{
    _printErrors = true;
	_vertexShaderFilename = vertex_shader_filename;
	_fragmentShaderFilename = fragment_shader_filename;

    _programID = glCreateProgram();

    // compile and attach the different shader objects
    if( !compileAndAttachShader( _vertexShaderID, GL_VERTEX_SHADER, "Vertex Shader", _vertexShaderFilename, _vertexShaderString, vertex_shader_content_prefix ) )
        return;

    if( !compileAndAttachShader( _fragmentShaderID, GL_FRAGMENT_SHADER, "Fragment Shader", _fragmentShaderFilename, _fragmentShaderString, fragment_shader_content_prefix ) )
        return;

    link();
}

Shader::~Shader()
{
}

bool Shader::read(std::string filename, std::string& contents) {
  contents = "";
  std::ifstream file;

  // try to open the file
  file.open( filename.c_str() );
  if( !file.is_open() )
      return false;
      
  // read it line by line
  std::string line;
  
  while( !file.eof() )
  {
      getline( file, line );
      contents += line + "\n";
  }
 
  file.close();
  
  return true;
}

bool Shader::compileAndAttachShader( GLuint& shaderID, GLenum shaderType, const char* shaderTypeStr, std::string filename, std::string &contents, std::string prefix )
{


    // if there's no filename and no contents, then a shader for this type hasn't been
    // specified. That's not (necessarily) an error, so return true - if it's a problem
    // then there will be a link error.
    if( !filename.length() && !contents.length() )
        return true;


    // create this shader if it hasn't been created yet
    //if( !shaderID )
        shaderID = glCreateShader( shaderType );

    // If a shader was passed in, just use that. Otherwise try and load from the filename.
    if( !contents.length() ) {

        // try and load the file
        if( !read( filename, contents ) ) {
            printf( "DGLShader: Error reading file %s\n", filename.c_str() );
            return false;
        }
    }

    contents = prefix + contents;

    // try to compile the shader
    const char* source = contents.c_str();
    glShaderSource( shaderID, 1, &source, NULL );
    glCompileShader( shaderID );

    // if it didn't work, print the error message
    GLint compileStatus;
    glGetShaderiv( shaderID, GL_COMPILE_STATUS, &compileStatus );
    if( !compileStatus ) {

        // perhaps print the linker error
        if( _printErrors ) {

            // get the length of the error message
            GLint errorMessageLength;
            glGetShaderiv( shaderID, GL_INFO_LOG_LENGTH, &errorMessageLength );

            // get the error message itself
            char* errorMessage = new char[errorMessageLength];
            glGetShaderInfoLog( shaderID, errorMessageLength, &errorMessageLength, errorMessage );

            // print the error
            if( filename.length() )
                printf( "GLSL %s Compile Error (file: %s)\n", shaderTypeStr, filename.c_str() );
            else
                printf( "GLSL %s Compile Error (no shader file)\n", shaderTypeStr );
            printf( "================================================================================\n" );

            // also display shader source
            {
                GLint length;
                glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &length);
                GLchar *source= new GLchar [length];
                glGetShaderSource(shaderID, length, NULL, source);

                printf("%s\n", source);
                delete [] source;
            }

            printf( "%s\n", errorMessage );
            printf( "\n" );

            delete[] errorMessage;
        }

        return false;
    }

    // attach it to the program object
    glAttachShader( _programID, shaderID );
    return true;
}

bool Shader::link()
{
    // link the different shaders that are attached to the program object
    glLinkProgram( _programID );

    // did the link work?
    GLint linkedOK = 0;
    glGetProgramiv( _programID, GL_LINK_STATUS, &linkedOK);

    if( !linkedOK ) {

        // perhaps print the linker error
        if( _printErrors ) {

            // get the length of the error message
            GLint errorMessageLength;
            glGetProgramiv( _programID, GL_INFO_LOG_LENGTH, &errorMessageLength );

            // get the error message itself
            char* errorMessage = new char[errorMessageLength];
            glGetProgramInfoLog( _programID, errorMessageLength, &errorMessageLength, errorMessage );

            // print it
            printf( "GLSL Linker Error:\n" );
            printf( "================================================================================\n" );
            printf( "%s\n", errorMessage );
            printf( "\n" );

            delete[] errorMessage;
        }

        return false;
    }

    return true;
}


}  // namespace CS248

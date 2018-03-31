#ifndef GLSL_H_2016_06_16
#define GLSL_H_2016_06_16

#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef MAC_OS_X
#include <OpenGL/OpenGL.h>
#elif LINUX
#include <GL/gl.h>
#endif

#define CHECK_GL_ERROR()                                             \
  {                                                                  \
    GLenum errcode = glGetError();                                   \
    if (errcode != GL_NO_ERROR) {                                    \
      std::string errstring = getGlErrStr(errcode);                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " : " << errstring \
                << std::endl;                                        \
      exit(1);                                                       \
    }                                                                \
  }

GLuint create_shader(const char* filename, GLenum type);
GLuint create_shader_from_src(const char* source, GLenum type);

inline bool getAttribLoc(const char* attrib_name, GLuint& attrib_id,
                         GLuint program) {
  GLint ret = glGetAttribLocation(program, attrib_name);

  if (ret == -1) {
    fprintf(stderr, "Could not bind attribute %s\n", attrib_name);
    return false;
  }
  attrib_id = GLuint(ret);
  return true;
}

inline bool getUniLoc(const char* uniform_name, GLint& uniform_id,
                      GLuint program) {
  uniform_id = glGetUniformLocation(program, uniform_name);
  if (uniform_id == -1) {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return false;
  }
  return true;
}

inline std::string getGlErrStr(const GLenum& errcode) {
  if (errcode == GL_INVALID_ENUM) {
    return "GL_INVALID_ENUM";
  } else if (errcode == GL_INVALID_VALUE) {
    return "GL_INVALID_VALUE";
  } else if (errcode == GL_INVALID_OPERATION) {
    return "GL_INVALID_OPERATION";
  } else if (errcode == GL_STACK_OVERFLOW) {
    return "GL_STACK_OVERFLOW";
  } else if (errcode == GL_STACK_UNDERFLOW) {
    return "GL_STACK_UNDERFLOW";
  } else if (errcode == GL_OUT_OF_MEMORY) {
    return "GL_OUT_OF_MEMORY";
  } else if (errcode == GL_INVALID_FRAMEBUFFER_OPERATION) {
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  }
  return "other error";
}

inline void checkGlError(const std::string& label = std::string()) {
  GLenum errcode = glGetError();
  if (errcode != GL_NO_ERROR) {
    std::string errstring = getGlErrStr(errcode);
    if (!label.empty()) std::cout << label << " : ";
    std::cout << errstring << std::endl;
    exit(1);
  }
}

inline std::string getGLVesion() {
  return reinterpret_cast<const char*>(glGetString(GL_VERSION));
}

inline std::string getGLVendor() {
  return reinterpret_cast<const char*>(glGetString(GL_VENDOR));
}

inline std::string getGLRenderer() {
  return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
}

#endif /* glsl_hpp */

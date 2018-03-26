#include "glsl.h"

#include <iostream>

namespace {

char* file_read(const char* filename) {
  FILE* input = fopen(filename, "rb");
  if (input == NULL)
    return NULL;

  if (fseek(input, 0, SEEK_END) == -1)
    return NULL;
  long size = ftell(input);
  if (size == -1)
    return NULL;
  if (fseek(input, 0, SEEK_SET) == -1)
    return NULL;

  /*if using c-compiler: dont cast malloc's return value*/
  char* content = static_cast<char*>(malloc(size_t(size + 1)));
  if (content == NULL)
    return NULL;

  fread(content, 1, size_t(size), input);
  if (ferror(input)) {
    free(content);
    return NULL;
  }

  fclose(input);
  content[size] = '\0';
  return content;
}

void print_log(GLuint object) {
  GLint log_length = 0;
  if (glIsShader(object))
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else if (glIsProgram(object))
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else {
    fprintf(stderr, "printlog: Not a shader or a program\n");
    return;
  }

  char* log = static_cast<char*>(malloc(GLuint(log_length)));

  if (glIsShader(object))
    glGetShaderInfoLog(object, log_length, NULL, log);
  else if (glIsProgram(object))
    glGetProgramInfoLog(object, log_length, NULL, log);

  fprintf(stderr, "%s", log);
  free(log);
}

} // namespace

GLuint create_shader(const char* filename, GLenum type) {
  const GLchar* source = file_read(filename);
  if (source == NULL) {
    fprintf(stderr, "Error opening %s: ", filename);
    perror("");
    return 0;
  }
  GLuint res = glCreateShader(type);
  const GLchar* sources[2] = {"", source};
  glShaderSource(res, 2, sources, NULL);
  delete[] source;

  glCompileShader(res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE) {
    fprintf(stderr, "%s:", filename);
    print_log(res);
    glDeleteShader(res);
    return 0;
  }

  return res;
}

GLuint create_shader_from_src(const char* source, GLenum type) {
  GLuint res = glCreateShader(type);
  glShaderSource(res, 1, &source, NULL);

  glCompileShader(res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE) {
    print_log(res);
    glDeleteShader(res);
    return 0;
  }

  return res;
}

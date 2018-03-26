
#ifndef glsl_utility_h_20160803
#define glsl_utility_h_20160803

#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "glsl.h"

template <typename Datatype> constexpr GLint gltype();

template <> constexpr GLint gltype<GLint>() { return GL_INT; }
template <> constexpr GLint gltype<GLubyte>() { return GL_UNSIGNED_BYTE; }
template <> constexpr GLint gltype<GLfloat>() { return GL_FLOAT; }

template <GLint target> constexpr size_t Dimention();
template <> constexpr size_t Dimention<GL_TEXTURE_1D>() { return 1; }
template <> constexpr size_t Dimention<GL_TEXTURE_2D>() { return 2; }
template <> constexpr size_t Dimention<GL_TEXTURE_RECTANGLE>() { return 2; }

/**
 target is GL_TEXTURE_RECTANGLE or GL_TEXTURE_2D or GL_TEXTURE_1D.
 Datatype is GLint, GLfloat, GLubyte.
 **/
template <GLint target, typename Datatype> class OpenGLTexture {
public:
  GLint f_param;
  GLint w_param;
  GLenum internal_format;
  GLenum format;
  using Size = std::array<int, Dimention<target>()>;

private:
  GLuint tex_num;
  GLuint name;
  Size size;
  GLuint fbID = GLuint(-1);
  GLuint rbID = GLuint(-1);

public:
  OpenGLTexture() {}

  OpenGLTexture(const Size& size_, const int& tex_num_, GLenum internal_format_,
                GLenum format_, Datatype* pixels,
                GLint filter_param = GL_NEAREST,
                GLint wrap_param = GL_CLAMP_TO_EDGE) {
    static_assert(GL_TEXTURE_RECTANGLE == target or GL_TEXTURE_2D == target or
                      GL_TEXTURE_1D == target,
                  "target of OpenGLTexture is not support");
    init(size_, tex_num_, internal_format_, format_, pixels, filter_param,
         wrap_param);
  }

  ~OpenGLTexture() {
    glDeleteTextures(1, &name);
    if (fbID != GLuint(-1)) {
      glDeleteFramebuffers(1, &fbID);
      glDeleteRenderbuffers(1, &rbID);
    }
  }

  bool init(const Size& size, const int& tex_num_, GLenum internal_format_,
            GLenum format_, Datatype* pixels, GLint filter_param = GL_NEAREST,
            GLint wrap_param = GL_CLAMP_TO_EDGE);

  /** if target is Texture1D, y and h is dead status. **/
  bool subImage(const Size& pos, const Size& area, int format,
                Datatype* pixels);

  bool initFrameBuffer();
  bool copyColorBuffer(const Size& offset, const Size& pos, const Size& area);
  bool bindFB();
  void resetFB() const;
  GLubyte* getPixelData();
  void getPixelData(GLubyte* out);

  bool uniform(const GLuint& program, const char* name);

  GLuint get_num() { return tex_num; }
};

using Texture1Di = OpenGLTexture<GL_TEXTURE_1D, GLint>;

template <GLint target, typename Datatype>
using TextureP = std::shared_ptr<OpenGLTexture<target, Datatype>>;

using PTexture2Di = TextureP<GL_TEXTURE_2D, GLint>;
using PTexture2Df = TextureP<GL_TEXTURE_2D, GLfloat>;

/** For RayTrace with OpenGL **/
class QuadDrawer {
  GLuint attr_coord_id;
  GLuint vbo;
  GLuint program_id;
  GLuint vao;

public:
  QuadDrawer(const std::string& name, const GLuint& program_id_,
             const std::vector<GLfloat>& vert)
      : program_id(program_id_) {
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, long(sizeof(GLfloat) * vert.size()), &vert[0],
                 GL_STATIC_DRAW);

    getAttribLoc(name.c_str(), attr_coord_id, program_id);
    glEnableVertexAttribArray(attr_coord_id);

    glUseProgram(program_id);
    glVertexAttribPointer(attr_coord_id, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  }

  void draw() {
    glUseProgram(program_id);
    glEnableVertexAttribArray(attr_coord_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Describe our vertices array to OpenGL (it can't guess its format
    // automatically)
    glVertexAttribPointer(
        attr_coord_id,       // attribute
        2,                   // number of elements per vertex, here (x,y)
        GL_FLOAT,            // the type of each element
        GL_FALSE,            // take our values as-is
        2 * sizeof(GLfloat), // no extra data between each position
        0                    // pointer to the C array
    );
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(attr_coord_id);
  }
};

class GlslUniform {
public:
  const char* uniform_name;
  GLint uniform_loc;

  GlslUniform(const std::string& name) : uniform_name(name.c_str()) {}

  bool getLoc(GLuint program) {
    uniform_loc = glGetUniformLocation(program, uniform_name);
    if (uniform_loc == -1) {
      fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
      return false;
    }
    return true;
  }
};

class UniformLocContainer {
private:
  std::map<std::string, GLint> map;

public:
  void add(const std::string& name, const GLuint& program) {
    GlslUniform uniform(name);
    uniform.getLoc(program);
    map.insert(std::make_pair(name, uniform.uniform_loc));
  }
  const GLint& operator[](const std::string& name) const {
    return map.at(name);
  }
};

#endif

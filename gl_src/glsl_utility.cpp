
#include "glsl_utility.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>

template <GLint target, typename Datatype>
bool OpenGLTexture<target, Datatype>::init(
    const Size& size_, const int& tex_num_, GLenum internal_format_,
    GLenum format, Datatype* pixels, GLint filter_param, GLint wrap_param) {
  CHECK_GL_ERROR();
  this->size = size_;
  this->internal_format = internal_format_;

  // genarate Texture object
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGenTextures(1, &name);
  if (tex_num_ == -1)
    tex_num = name - 1;
  else
    tex_num = GLuint(tex_num_);

  glActiveTexture(GL_TEXTURE0 + tex_num);

  CHECK_GL_ERROR();

  glBindTexture(target, name);
  if (target == GL_TEXTURE_1D) {
    glTexImage1D(target, 0, internal_format, size[0], 0, format,
                 gltype<Datatype>, pixels);
  } else {
    glTexImage2D(target, 0, internal_format, size[0], size[1], 0, format,
                 gltype<Datatype>, pixels);
  }

  CHECK_GL_ERROR();

  f_param = filter_param;
  w_param = wrap_param;
  GLint mag_filter = f_param;
  if (f_param == GL_NEAREST_MIPMAP_NEAREST ||
      f_param == GL_NEAREST_MIPMAP_LINEAR) {
    mag_filter = GL_NEAREST;
    glGenerateMipmap(name);
  } else if (f_param == GL_LINEAR_MIPMAP_NEAREST ||
             f_param == GL_LINEAR_MIPMAP_LINEAR) {
    mag_filter = GL_LINEAR;
    glGenerateMipmap(name);
  }

  CHECK_GL_ERROR();

  // set parameters
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, f_param);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, f_param);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, w_param);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, w_param);

  CHECK_GL_ERROR();

  // texture init.
  glActiveTexture(GL_TEXTURE0);

  return true;
}

template <GLint target, typename Datatype>
bool OpenGLTexture<target, Datatype>::initFrameBuffer() {
  if (Dimention<target> != 2) {
    std::cerr << "texture1d can't call initFrameBuffer()" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  glActiveTexture(GL_TEXTURE0 + name);
  glGenFramebuffers(1, &fbID);
  glBindFramebuffer(GL_FRAMEBUFFER, fbID);
  if (internal_format == GL_DEPTH_COMPONENT) {
    glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, name,
                           0);
  } else {
    // make renfer buffer for depth buffer.
    glGenRenderbuffers(1, &rbID);
    glBindRenderbuffer(GL_RENDERBUFFER_EXT, rbID);
    glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, size[0],
                          size[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, name,
                           0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                              GL_RENDERBUFFER_EXT, rbID);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, fbID);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << fbID << " : " << glCheckFramebufferStatus(GL_FRAMEBUFFER)
              << std::endl;
  }

  CHECK_GL_ERROR();

  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

template <GLint target, typename Datatype>
bool OpenGLTexture<target, Datatype>::bindFB() const {
  if (Dimention<target> != 2) {
    std::cerr << "texture1d can't call bindFB()" << std::endl;
    std::exit(EXIT_FAILURE);
  } else if (fbID == GLuint(-1)) {
    std::cerr << "[error] call initFrameBuffer() before calling bindFB()."
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  //  glActiveTexture(GL_TEXTURE0 + tex_num);
  //  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbID);
  glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbID);
  return true;
}
template <GLint target, typename Datatype>
void OpenGLTexture<target, Datatype>::resetFB() const {
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

template <GLint target, typename Datatype>
bool OpenGLTexture<target, Datatype>::uniform(const GLuint& program,
                                              const char* uniform_name) const {
  GLint loc;
  bool f = getUniLoc(uniform_name, loc, program);

  if (!f) return false;

  glActiveTexture(GL_TEXTURE0 + tex_num);
  glUniform1i(loc, tex_num);

  return true;
}

template <GLint target, typename Datatype>
bool OpenGLTexture<target, Datatype>::subImage(const Size& pos,
                                               const Size& area, int format_,
                                               Datatype* pixels) {
  glActiveTexture(GL_TEXTURE0 + tex_num);

  if (target == GL_TEXTURE_1D) {
    glTexSubImage1D(target, 0, pos[0], area[0], format_, gltype<Datatype>,
                    pixels);
  } else {
    glTexSubImage2D(target, 0, pos[0], pos[1], area[0], area[1], format_,
                    gltype<Datatype>, pixels);
  }
  CHECK_GL_ERROR();

  glActiveTexture(GL_TEXTURE0);
  return true;
}

template <GLint target, typename Datatype>
std::unique_ptr<Datatype[]> OpenGLTexture<target, Datatype>::getPixelData(const GLint& format) const {
  if (fbID == GLuint(-1)) {
    std::cerr << "OpenGLTexture::getPixelData() must be called after initFrameBuffer()!" << std::endl;
    exit(EXIT_FAILURE);
  }
  bindFB();

  auto dst = std::make_unique<Datatype[]>(size[0] * size[1] * 4);
  glReadPixels(0, 0, size[0], size[1], format, gltype<Datatype>, &dst[0]);
  
  return dst;
}

template <GLint target, typename Datatype>
void OpenGLTexture<target, Datatype>::getPixelData(const GLint& format, Datatype* dst) const{
  if (fbID == GLuint(-1)) {
    std::cerr << "OpenGLTexture::getPixelData() must be called after initFrameBuffer()!" << std::endl;
    exit(EXIT_FAILURE);
  }
  bindFB();

  glReadPixels(0, 0, size[0], size[1], format, gltype<Datatype>, dst);
}

template class OpenGLTexture<GL_TEXTURE_RECTANGLE, GLint>;
template class OpenGLTexture<GL_TEXTURE_RECTANGLE, GLfloat>;
template class OpenGLTexture<GL_TEXTURE_RECTANGLE, GLubyte>;
template class OpenGLTexture<GL_TEXTURE_2D, GLint>;
template class OpenGLTexture<GL_TEXTURE_2D, GLfloat>;
template class OpenGLTexture<GL_TEXTURE_2D, GLubyte>;
template class OpenGLTexture<GL_TEXTURE_1D, GLint>;
template class OpenGLTexture<GL_TEXTURE_1D, GLfloat>;
template class OpenGLTexture<GL_TEXTURE_1D, GLubyte>;

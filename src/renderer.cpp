//
//  renderer.cpp
//  GLSLRenderer
//
//  Created by Skatto on 2018/02/24.
//  Copyright © 2018年 Skatto. All rights reserved.
//

#include "renderer.hpp"

#include <cassert>
#include <future>
#include <iostream>
#include <thread>

#include "../gl_src/glsl_utility.h"
#include "fps.h"
#include "intersect.h"
#include "logger.h"

namespace {

constexpr real kPI = 3.1415926535;

float computeBrightMagnification(std::vector<Polygon>* polygons) {
  float max_v = 0.f;

  for (auto& pol : *polygons) {
    if (pol.material != Material::Light && pol.material != Material::DirLight) {
      continue;
    }
    max_v = std::max(std::abs(pol.col[0]), max_v);
    max_v = std::max(std::abs(pol.col[1]), max_v);
    max_v = std::max(std::abs(pol.col[2]), max_v);
  }

  for (auto& pol : *polygons) {
    if (pol.material != Material::Light && pol.material != Material::DirLight) {
      continue;
    }
    pol.col /= max_v;
  }

  return max_v;
}

PTexture2Df setupTriangleTexture(const int& side_len,
                                 const std::vector<Polygon>& pols) {
  if (pols.size() > std::pow(side_len, 2) / 4) {
    return nullptr;
  }

  struct PolygonData {
    Vec ver0;
    real pad0;
    Vec ver1;
    real pad1;
    Vec ver2;
    real pad2;
    Vec color;
    real info;

    PolygonData(const Polygon& pol)
        : ver0(pol.vert[0]),
          ver1(pol.vert[1]),
          ver2(pol.vert[2]),
          color(pol.col) {
      info = GLfloat(pol.material);
    }
  };

  std::vector<GLfloat> pixels(size_t(std::pow(side_len, 2)) * 4);

  for (size_t i = 0; i < pols.size(); i++) {
    reinterpret_cast<PolygonData*>(pixels.data())[i] = PolygonData(pols[i]);
  }

  std::array<int, 2> tex_size = {{side_len, side_len}};

  return std::make_shared<OpenGLTexture<GL_TEXTURE_2D, GLfloat>>(
      tex_size, -1, GL_RGBA16F, GL_RGBA, &pixels[0], GL_NEAREST);
}

bool setupBVHTexture(const int& side_len, const BVH& bvh, PTexture2Df coord_tex,
                     TextureP<GL_TEXTURE_1D, GLint>& info_tex) {
  if (bvh.nodes.size() > std::pow(side_len, 2) / 2) {
    return false;
  }

  struct BBox {
    Vec start;
    Vec end;
    BBox(const BVH::Node& node) : start(node.start), end(node.end) {}
  };

  std::vector<GLfloat> pixels(size_t(std::pow(side_len, 2)) * 3);
  for (size_t i = 0; i < bvh.nodes.size(); i++) {
    reinterpret_cast<BBox*>(pixels.data())[i] = BBox(bvh.nodes[i]);
  }

  std::vector<GLint> ipixels(size_t(side_len * 3));
  for (size_t i = 0; i < bvh.nodes.size(); i++) {
    if (bvh.nodes[i].leaf) {
      ipixels[3 * i + 0] = GLint(bvh.nodes[i].s_idx);
      ipixels[3 * i + 1] = GLint(bvh.nodes[i].e_idx);
    } else {
      ipixels[3 * i + 0] = -1;
      ipixels[3 * i + 1] = -1;
    }
    if (bvh.nodes[i].brother != size_t(-1)) {
      ipixels[3 * i + 2] = GLint(bvh.nodes[i].brother);
    } else {
      ipixels[3 * i + 2] = -1;
    }
  }

  coord_tex->init({{side_len, side_len}}, -1, GL_RGB16F, GL_RGB, &pixels[0],
                  GL_NEAREST);
  info_tex->init({{side_len}}, -1, GL_RGB16I, GL_RGB_INTEGER, &ipixels[0],
                 GL_NEAREST);

  return true;
}

struct Point {
  Vec pos;
  real polygon_id;
  Vec col;
  real pdf;
  Point() {}
  Point(const Vec& position, const size_t& polygon_id_, const Vec& color,
        const real& pdf_)
      : pos(position), polygon_id(polygon_id_), col(color), pdf(pdf_) {}
};

Ray decideRay(const Intersection& isect, const Ray& priv,
              std::uniform_real_distribution<>& coord, std::mt19937& rnd) {
  real phi = 2.f * kPI * real(coord(rnd));
  float costheta = sqrt(coord(rnd));

  Vec u;
  if (std::abs(isect.normal.x) > 0.1f) {
    u = normalize(cross(isect.normal, Vec(0, 1, 0)));
  } else {
    u = normalize(cross(isect.normal, Vec(1, 0, 0)));
  }
  Vec v = normalize(cross(isect.normal, u));

  Ray ray = priv;
  ray.org = isect.point;
  if (isect.material == Material::Normal) {
    ray.dir =
        normalize(u * std::cos(phi) * costheta + v * std::sin(phi) * costheta +
                  isect.normal * std::sqrt(1.f - costheta * costheta));
    ray.pdf *= kPI;
    assert(0 < dot(ray.dir, isect.normal));
  }
  if (isect.material == Material::Light) {
    ray.dir =
        normalize(u * std::cos(phi) * costheta + v * std::sin(phi) * costheta +
                  isect.normal * std::sqrt(1.f - costheta * costheta));
    ray.pdf *= kPI;
    ray.col += isect.col;
  }
  assert(0 < dot(ray.dir, isect.normal));
  return ray;
}

Ray decideLightRay(const Vec& normal, const Vec& point,
                   std::uniform_real_distribution<>& coord, std::mt19937& rnd,
                   const Material& material) {
  real phi = 2.f * kPI * real(coord(rnd));
  float costheta = sqrt(coord(rnd));

  Vec u;
  if (std::abs(normal.x) > 0.1f) {
    u = normalize(cross(normal, Vec(0, 1, 0)));
  } else {
    u = normalize(cross(normal, Vec(1, 0, 0)));
  }
  Vec v = normalize(cross(normal, u));

  Vec r_dir;
  real r_pdf;
  if (material == Material::Light) {
    r_dir =
        normalize(u * std::cos(phi) * costheta + v * std::sin(phi) * costheta +
                  normal * std::sqrt(1.f - costheta * costheta));
    r_pdf = kPI;
    //    if (coord(rnd) < 0.5) {
    //      r_dir *= -1.f;
    //    }
  }
  return Ray(point, r_dir, r_pdf);
}

constexpr int kMAX_DEPTH = 5;
Point makePoint(const Polygon& light, const real& u, const real& v,
                const BVH& bvh, std::uniform_real_distribution<>& coord,
                std::mt19937& rnd) {
  Vec edge0 = light.vert[1] - light.vert[0];
  Vec edge1 = light.vert[2] - light.vert[0];
  Ray ray = decideLightRay(normalize(cross(edge0, edge1)),
                           light.vert[0] + u * edge0 + v * edge1, coord, rnd,
                           light.material);
  ray.pdf *= cross(edge0, edge1).length();
  if (light.material == Material::Light) {
    ray.col = light.col;
  } else if (light.material == Material::DirLight) {
    ray.col = light.col * dot(ray.dir, normalize(cross(edge0, edge1)));
  }
  Intersection isect;

  for (ray.depth = 1; ray.depth < kMAX_DEPTH; ray.depth++) {
    isect = intersectBVH(ray, bvh);
    if (isect.t >= kINF) {
      return Point(Vec(), -1, Vec(), 0.0);
    }
    ray.col *= isect.col;

    ray.col *= -real(dot(ray.dir, isect.normal));  // / (isect.t * isect.t);
    assert(0 < -dot(ray.dir, isect.normal));
    ray = decideRay(isect, ray, coord, rnd);
    ray.col *= real(dot(ray.dir, isect.normal));
    assert(0 < dot(ray.dir, isect.normal));

    if (coord(rnd) > std::pow(0.8, ray.depth)) {
      return Point(isect.point, isect.solid_id, ray.col, ray.pdf);
    }
    ray.pdf /= std::max(1.0, std::pow(0.8, ray.depth));
    // TODO
  }

  const Polygon& pol = bvh.polygons[isect.solid_id];
  Vec norm =
      normalize(cross(pol.vert[1] - pol.vert[0], pol.vert[2] - pol.vert[0]));
  int sgn = dot(norm, isect.normal) > 0 ? 1 : -1;
  return Point(isect.point, isect.solid_id * sgn, ray.col, ray.pdf);
}

void lightTracing(const std::vector<Polygon>& lights, const int& num_sample,
                  const BVH& bvh, std::vector<Point>* dst,
                  std::atomic_bool* finish_flag) {
  std::uniform_int_distribution<> choice(0, int(lights.size() - 1));
  std::uniform_real_distribution<> coord(0, 1.0);
  std::random_device seed_gen;
  std::mt19937 engine(seed_gen());

  dst->resize(num_sample);

#if 0  // Parallel threading
  int num_thread = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  std::atomic_int i;
  i = 0;
  for (int j = 0; j < num_thread; j++) {
    threads.emplace_back([&]() {
      while (i < num_sample) {
        real u = coord(engine), v = coord(engine);
        if (u + v > real(1)) {
          u = 1 - u;
          v = 1 - v;
        }
        Point point =
            makePoint(lights[choice(engine)], u, v, bvh, coord, engine);
        point.pdf *= lights.size();
        // Probability of selecting one of the light is because 1 /
        // lights.size().
        // TODO Make the probability proportional to the area and light
        // intensity.

        if (point.pdf <= 0.f) {
          continue;
        }
        (*dst)[i++] = point;
      }
    });
  }
  for (auto& th : threads) {
    th.join();
  }
#else  // One line
  int i = 0;
  while (i < num_sample) {
    real u = coord(engine), v = coord(engine);
    if (u + v > real(1)) {
      u = 1 - u;
      v = 1 - v;
    }
    Point point = makePoint(lights[choice(engine)], u, v, bvh, coord, engine);
    point.pdf *= lights.size();
    // Probability of selecting one of the light is because 1 / lights.size().
    // TODO Make the probability proportional to the area and light intensity.

    if (point.pdf <= 0.f) {
      continue;
    }
    (*dst)[i++] = point;
    if (length(point.col) > 10) {
      DEBUG_LOG(point.col);
    }
  }
#endif

  *finish_flag = true;
  DEBUG_LOG("lightTracing done");
}

}  // namespace

bool GlslRayTraceRenderer::init() {
  if (!glfwInit()) {
    return false;
  }  // glfw3

  DEBUG_LOG("GLFW version : ", glfwGetVersionString());

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (!r_config.display) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  } else {
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  }

  window = glfwCreateWindow(r_config.width, r_config.height,
                            w_config.title.c_str(), nullptr, nullptr);

  glfwMakeContextCurrent(window);  // choice drawing window
  glfwSwapInterval(1);

  // glew
  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
    printf("Error: %s\n", glewGetErrorString(glew_status));
    glfwDestroyWindow(window);
    glfwTerminate();
    return false;
  }
  CHECK_GL_ERROR();

  DEBUG_LOG("OpenGL version : ", getGLVesion());
  DEBUG_LOG("OpenGL vender : ", getGLVendor());
  DEBUG_LOG("OpenGL renderer : ", getGLRenderer());

  // setup vertex shader and fragment shader
  GLint link_ok = GL_FALSE;
  std::string vs_string =
#include "test.vert"
      ;
  std::string fs_string =
#include "test.frag"
      ;
  vs_id = create_shader_from_src(vs_string.c_str(), GL_VERTEX_SHADER);
  fs_id = create_shader_from_src(fs_string.c_str(), GL_FRAGMENT_SHADER);

  gl_program_id = glCreateProgram();
  glAttachShader(gl_program_id, vs_id);
  glAttachShader(gl_program_id, fs_id);
  glLinkProgram(gl_program_id);
  glGetProgramiv(gl_program_id, GL_LINK_STATUS, &link_ok);
  if (!link_ok) {
    fprintf(stderr, "glLinkProgram:");
    return false;
  }

  glUseProgram(gl_program_id);
  CHECK_GL_ERROR();

  return true;
}
int GlslRayTraceRenderer::start() {
  if (window == nullptr) return -1;

  // attribute
  std::vector<GLfloat> triangle_attribute{
      -1.f, 1.f, -1.f, -1.f, 1.f, -1.f, 1.f, 1.f,
  };
  QuadDrawer quad("coord2d", gl_program_id, triangle_attribute);

  // setup texture for sending polygon data.
  const float bright_mag = computeBrightMagnification(&bvh.polygons);

  auto tri_tex = setupTriangleTexture(tex_side_len, bvh.polygons);
  if (tri_tex == nullptr) {
    std::cerr << "GlslRayTraceRenderer : size of polygons is too big !"
              << std::endl;
    std::cerr << "GlslRayTraceRenderer : "
                 "please edit tex_side_len in constructor."
              << std::endl;
    return -1;
  }

  auto bvh_info_tex = std::make_shared<OpenGLTexture<GL_TEXTURE_1D, GLint>>();
  auto bvh_tex = std::make_shared<OpenGLTexture<GL_TEXTURE_2D, GLfloat>>();
  if (!setupBVHTexture(tex_side_len, bvh, bvh_tex, bvh_info_tex)) {
    std::cerr << "GlslRayTraceRenderer : size of bvh is too big !" << std::endl;
    std::cerr << "GlslRayTraceRenderer : "
                 "please edit tex_side_len in constructor."
              << std::endl;
    return -1;
  }

  // for off screen rendering, setup two textures (and framebuffer).
  OpenGLTexture<GL_TEXTURE_2D, GLfloat> accumulator[2];
  accumulator[0].init({{r_config.width, r_config.height}}, -1, GL_RGBA32F,
                      GL_NEAREST);
  accumulator[1].init({{r_config.width, r_config.height}}, -1, GL_RGBA32F,
                      GL_NEAREST);
  accumulator[0].initFrameBuffer();
  accumulator[1].initFrameBuffer();

  OpenGLTexture<GL_TEXTURE_RECTANGLE, GLfloat> point_tex;
  point_tex.init({{r_config.n_sample_frame * 2, 100}}, -1, GL_RGBA16F);

  UniformLocContainer uni_locs;

  uni_locs.add("brightness", gl_program_id);
  uni_locs.add("TRI_TEX_COL", gl_program_id);
  uni_locs.add("num_tri", gl_program_id);
  uni_locs.add("aspect_ratio", gl_program_id);
  uni_locs.add("rand_seed", gl_program_id);
  uni_locs.add("num_sample", gl_program_id);
  uni_locs.add("gamma", gl_program_id);
  uni_locs.add("onlyDraw", gl_program_id);
  uni_locs.add("bvh_size", gl_program_id);
  uni_locs.add("render_count", gl_program_id);

  FpsCounter fps;
  fps.init();

  const size_t num_points = r_config.n_sample_frame * 20;

  std::vector<Point> points;
  std::atomic_bool finish_flag;
  std::thread light_tracing_th(lightTracing, lights, num_points, bvh, &points,
                               &finish_flag);
  light_tracing_th.join();
  point_tex.subImage({{0, 0}}, {{r_config.n_sample_frame, 100}}, GL_RGBA,
                     reinterpret_cast<GLfloat*>(&points[0]));

  finish_flag = false;
  light_tracing_th =
      std::thread(lightTracing, lights, num_points, bvh, &points, &finish_flag);

  tri_tex->uniform(gl_program_id, "tri_tex");
  bvh_tex->uniform(gl_program_id, "bvh_tex");
  bvh_info_tex->uniform(gl_program_id, "bvh_info_tex");
  point_tex.uniform(gl_program_id, "point_tex");
  glUniform1i(uni_locs["TRI_TEX_COL"], tex_side_len);
  glUniform1i(uni_locs["num_tri"], GLint(bvh.polygons.size()));
  glUniform1i(uni_locs["bvh_size"], GLint(bvh.nodes.size()));
  glUniform1f(uni_locs["aspect_ratio"],
              float(r_config.width) / float(r_config.height));
  glUniform1f(uni_locs["brightness"], bright_mag);
  glUniform1f(uni_locs["gamma"], r_config.gamma);

  // Main Loop
  size_t n = 0;
  int render_count = 0;
  while (!glfwWindowShouldClose(window)) {
    if (size_t(render_count) * r_config.n_sample_frame < num_points) {
      accumulator[(n + 1) % 2].uniform(gl_program_id, "d_tex");
      glUniform4f(uni_locs["rand_seed"], rand_(), rand_(), rand_(), rand_());
      glUniform1i(uni_locs["onlyDraw"], false);
      glUniform1i(uni_locs["num_sample"], r_config.n_sample_frame);
      glUniform1i(uni_locs["render_count"], render_count);

      glViewport(0, 0, r_config.width, r_config.height);

      accumulator[n % 2].bindFB();
      quad.draw();
      glFlush();
      accumulator[0].resetFB();

      n++;
      render_count++;
    } else if (finish_flag) {
      light_tracing_th.join();
      point_tex.subImage({{0, 0}}, {{r_config.n_sample_frame * 2, 100}},
                         GL_RGBA, reinterpret_cast<GLfloat*>(&points[0]));
      render_count = 0;

      finish_flag = false;
      light_tracing_th = std::thread(lightTracing, lights, num_points, bvh,
                                     &points, &finish_flag);
    }

    // display result.
    if (r_config.display) {
      if (w_config.is_retina) {
        glViewport(0, 0, r_config.width * 2, r_config.height * 2);
      }
      accumulator[n % 2].uniform(gl_program_id, "d_tex");
      glUniform1i(uni_locs["onlyDraw"], true);
      glUniform1i(uni_locs["num_sample"], int(n));
      glUseProgram(gl_program_id);

      quad.draw();
      CHECK_GL_ERROR();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
    CHECK_GL_ERROR();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
      glfwSetWindowShouldClose(window, 1);
    }

    // log fps avarage. and compute ray/sec.
    if (-1 != fps.update()) {
      size_t rps =
          size_t(double(fps.ave_fps) * double(r_config.width) *
                 double(r_config.height) * double(r_config.n_sample_frame));
      std::clog << "fps : " << fps.fps << "  rps average : " << rps
                << std::endl;
    }
  }  // Main Loop

  light_tracing_th.join();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

//
//  renderer.hpp
//  GLSLRenderer
//
//  Created by Skatto on 2018/02/24.
//  Copyright © 2018年 Skatto. All rights reserved.
//

#ifndef renderer_hpp20180224
#define renderer_hpp20180224

#include <string>

#include "../gl_src/glsl.h"
#include "common.h"

struct WindowConfig {
  std::string title;
  bool is_retina;
};

struct RenderConfig {
  bool display;
  int width;
  int height;
  float gamma = 1.f;
  int n_sample_frame;
  size_t max_sample;
};

class GlslRayTraceRenderer {
public:
  const RenderConfig r_config;
  const WindowConfig w_config;
  const int tex_side_len;

private:
  GLFWwindow* window = nullptr;

  GLuint gl_program_id;
  GLuint vs_id;
  GLuint fs_id;

  BVH bvh;
  std::vector<Polygon> lights;

public:
  GlslRayTraceRenderer(const RenderConfig& r_config_,
                       const WindowConfig& w_config_,
                       const int& tex_side_len_ = 512)
      : r_config(r_config_), w_config(w_config_), tex_side_len(tex_side_len_) {
    if (!init()) {
      std::cerr << "GlslRayTraceRenderer init failed." << std::endl;
    }
  }
  int start();

  bool setPolygons(const std::vector<Polygon>& polygons_) {
    // setup light array
    for (auto& pol : polygons_) {
      if (pol.material == Material::Light) {
        lights.push_back(pol);
      }
    }
    // make BVH
    return bvh.init(polygons_);
  }

private:
  bool init();
};

#endif /* renderer_hpp20180224 */

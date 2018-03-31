//
//  main.cpp
//  GLSLRenderer
//
//  Created by Skatto on 2018/02/24.
//  Copyright © 2018年 Skatto. All rights reserved.
//

#include <iostream>
#include "renderer.hpp"

int main() {
  std::vector<Polygon> polygons{
      Polygon(Vec(-8.f, -3.f, 3.f), Vec(8.f, -3.f, 3.f), Vec(-8.f, 3.f, 3.f),
              WHITE),
      Polygon(Vec(8.f, -3.f, 3.f), Vec(-8.f, 3.f, 3.f), Vec(8.f, 3.f, 3.f),
              WHITE),
      Polygon(Vec(-8.f, -3.f, -3.f), Vec(8.f, -3.f, -3.f), Vec(-8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(8.f, -3.f, -3.f), Vec(-8.f, 3.f, -3.f), Vec(8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(-8.f, -3.f, 3.f), Vec(-8.f, -3.f, -3.f), Vec(-8.f, 3.f, 3.f),
              WHITE),
      Polygon(Vec(-8.f, -3.f, -3.f), Vec(-8.f, 3.f, 3.f), Vec(-8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(8.f, -3.f, 3.f), Vec(8.f, -3.f, -3.f), Vec(8.f, 3.f, 3.f),
              WHITE),
      Polygon(Vec(8.f, -3.f, -3.f), Vec(8.f, 3.f, 3.f), Vec(8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(-8.f, 3.f, 3.f), Vec(8.f, 3.f, 3.f), Vec(-8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(8.f, 3.f, 3.f), Vec(-8.f, 3.f, -3.f), Vec(8.f, 3.f, -3.f),
              WHITE),
      Polygon(Vec(-8.f, -3.f, 3.f), Vec(8.f, -3.f, 3.f), Vec(-8.f, -3.f, -3.f),
              WHITE),
      Polygon(Vec(8.f, -3.f, 3.f), Vec(-8.f, -3.f, -3.f), Vec(8.f, -3.f, -3.f),
              WHITE),
      // Polygon(Vec(6, -0.5, -2.9), Vec(6, 0.5, -2.9), Vec(7, 0.5, -2.9),
      // WHITE * 50, MATERIAL_LIGHT),
      // Polygon(Vec(6, -0.5, -2.9), Vec(7, -0.5, -2.9), Vec(7, 0.5,
      // -2.9), WHITE * 50, MATERIAL_LIGHT),
      Polygon(Vec(7, 3, 3), Vec(8, 2, 3), Vec(8, 3, 2), RED * 10,
              Material::Light),
      Polygon(Vec(7, -3, 3), Vec(8, -2, 3), Vec(8, -3, 2), GREEN * 10,
              Material::Light),
      Polygon(Vec(7, 3, -3), Vec(8, 2, -3), Vec(8, 3, -2), BLUE * 10,
              Material::Light),
  };

  WindowConfig window;
  window.is_retina = true;
  window.title = "test";
  RenderConfig render;
  render.width = 300;
  render.height = 300;
  render.gamma = 0.4f;
  render.display = true;
  render.n_sample_frame = 10;
  render.max_sample = 1e10;

  GlslRayTraceRenderer renderer(render, window);

  renderer.setPolygons(polygons);

  renderer.start();

  return 0;
}

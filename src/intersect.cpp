//
//  intersect.cpp
//  NewGlslRenderer
//
//  Created by Shun on 2018/03/31.
//  Copyright © 2018年 Shun. All rights reserved.
//

#include "intersect.h"

namespace {

bool intersectTriangle(const Ray& ray, const size_t& pol_idx,
                       const std::vector<Polygon>& polygons,
                       Intersection* result) {
  const Polygon& polygon = polygons[pol_idx];
  Vec position0 = polygon.vert[0];
  Vec edge0 = polygon.vert[1] - polygon.vert[0];
  Vec edge1 = polygon.vert[2] - polygon.vert[0];

  /* Möller–Trumbore intersection algorithm */
  Vec P = cross(ray.dir, edge1);
  float det = dot(P, edge0);
  if (-kEPS < det && det < kEPS) return false;
  float inv_det = 1.f / det;
  Vec T = ray.org - position0;
  float u = dot(T, P) * inv_det;
  if (u < 0.f || 1.f < u) return false;
  Vec Q = cross(T, edge0);
  float v = dot(ray.dir, Q) * inv_det;
  if (v < 0.f || 1.f < u + v) return false;
  float t = dot(edge1, Q) * inv_det;

  if (kEPS < t && result->t > t) {  // Hit
    result->point = ray.org + ray.dir * t;
    result->t = t;
    result->col = polygon.col;
    result->normal = normalize(cross(edge0, edge1));
    result->material = polygon.material;
    result->solid_id = pol_idx;
    if (dot(result->normal, ray.dir) > 0) {
      result->normal = -1 * result->normal;
    }
    if (polygon.material == Material::Light &&
        polygon.material == Material::DirLight) {
      result->end = true;
    } else {
      result->end = false;
    }
    return true;
  }
  return false;
}

bool intersectBoundingBox(const Ray& ray, const BVH::Node& node) {
  float t_far = kINF, t_near = -kINF;
  for (int i = 0; i < 3; i++) {
    float t1 = (node.start[i] - ray.org[i]) / ray.dir[i];
    float t2 = (node.end[i] - ray.org[i]) / ray.dir[i];
    if (t1 < t2) {
      t_far = std::min(t_far, t2);
      t_near = std::max(t_near, t1);
    } else {
      t_far = std::min(t_far, t1);
      t_near = std::max(t_near, t2);
    }
    if (t_far < t_near) return false;
  }
  return t_far > 0;
}

}  // namespace

bool intersectBVH(const Ray& ray, const BVH& bvh, Intersection* isect) {
  isect->end = false;
  isect->t = kINF;

  size_t node_idx = 0;
  while (true) {
    const BVH::Node& node = bvh.nodes[node_idx];
    if (intersectBoundingBox(ray, node)) {
      if (node.leaf) {
        for (size_t tri_idx = node.s_idx; tri_idx < node.e_idx; tri_idx++) {
          if (intersectTriangle(ray, tri_idx, bvh.polygons, isect)) {
            isect->solid_id = tri_idx;
          }
        }
      }
      node_idx++;
      if (node_idx >= bvh.nodes.size()) break;
    } else {
      if (node.brother == size_t(-1)) {
        break;
      }
      node_idx = node.brother;
    }
  }

  return isect->t < kINF;
}

Intersection intersectBVH(const Ray& ray, const BVH& bvh) {
  Intersection isect;
  intersectBVH(ray, bvh, &isect);
  return isect;
}

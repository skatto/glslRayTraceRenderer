//
//  common.cpp
//  NewGlslRenderer
//
//  Created by Skatto on 2018/03/14.
//  Copyright © 2018年 Skatto. All rights reserved.
//

#include "common.h"

#include <algorithm>
#include <deque>
#include <functional>

#include "logger.h"

namespace {

constexpr int kMAX_POL = 7;

BVH::Node buildBVHNode(const std::vector<Polygon>& polygons,
                       const size_t& start, const size_t& end,
                       const size_t& parent) {
  BVH::Node node;
  node.start = Vec(kINF);
  node.end = Vec(-kINF);
  for (size_t i = start; i < end; i++) {
    for (auto& vert : polygons[i].vert) {
      node.end = max(vert, node.end);
      node.start = min(vert, node.start);
    }
  }
  node.s_idx = start;
  node.e_idx = end;
  node.parent = parent;
  return node;
}

constexpr real getMax(const Polygon& pol, const int& axis) {
  return mymax(pol.vert[0][axis], mymax(pol.vert[1][axis], pol.vert[2][axis]));
}

template <int axis>
constexpr bool comparePolygon(const Polygon& a, const Polygon& b) {
  return (getMax(a, axis) > getMax(b, axis));
}

real surfaceArea(const BVH::Node& a) {
  const Vec l = a.end - a.start;
  return 2 * (l[0] * l[1] + l[1] * l[2] + l[2] * l[0]);
}

// sort pols and decide partitioning index.
size_t decidePartition(const BVH::Node& parent, std::vector<Polygon>* pols) {
  static std::function<decltype(comparePolygon<0>)> compare[3]{
      comparePolygon<0>,
      comparePolygon<1>,
      comparePolygon<2>,
  };

  size_t s_id = parent.s_idx;
  size_t e_id = parent.e_idx;

  real vmin = kINF;
  size_t min_idx = (s_id + e_id) / 2;
  int min_c = 0;
  Vec center = (parent.start + parent.end) / 2;
  for (int c : {0, 1, 2}) {
    std::sort(std::begin(*pols) + s_id, std::begin(*pols) + e_id, compare[c]);
    for (size_t i = s_id + 1; i < e_id; i++) {
      if (getMax(pols->at(i), c) < center[c]) {
        float s = surfaceArea(buildBVHNode(*pols, s_id, i, 0)) +
                  surfaceArea(buildBVHNode(*pols, i, e_id, 0));
        if (s < vmin) {
          vmin = s;
          min_idx = i;
          min_c = c;
        }
        break;
      }
    }
  }
  std::sort(std::begin(*pols) + s_id, std::begin(*pols) + e_id, compare[min_c]);

  return min_idx;
}

} // namespace

bool BVH::init(const std::vector<Polygon>& pols) {
  polygons = pols;
  if (polygons.size() < 1) {
    return false;
  }
  DEBUG_LOG("start building BVH !");

  static std::function<decltype(comparePolygon<0>)> compare[3]{
      comparePolygon<0>,
      comparePolygon<1>,
      comparePolygon<2>,
  };

  std::deque<Node> issue_stack;
  issue_stack.push_back(buildBVHNode(polygons, 0, polygons.size(), -1));

  while (!issue_stack.empty()) {
    Node node = issue_stack.back();
    issue_stack.pop_back();

    if (node.parent == size_t(-1)) {

    } else if (node.parent != nodes.size() - 1) {
      nodes[node.parent + 1].brother = nodes.size();
    }

    if (node.e_idx - node.s_idx <= kMAX_POL) {
      node.leaf = true;
      nodes.emplace_back(node);
      continue;
    }
    size_t start = node.s_idx;
    size_t end = node.e_idx;

    size_t p_idx = decidePartition(node, &polygons);

    issue_stack.emplace_back(buildBVHNode(polygons, p_idx, end, nodes.size()));
    issue_stack.emplace_back(
        buildBVHNode(polygons, start, p_idx, nodes.size()));

    nodes.emplace_back(node);
  }

  for (size_t i = 1; i < nodes.size() - 1; i++) {
    Node& node = nodes[i];
    if (node.brother == size_t(-1)) {
      node.brother = nodes[node.parent].brother;
    }
  }

  DEBUG_LOG("finish building BVH !");
  DEBUG_LOG("BVH size is ", nodes.size());

  return true;
}

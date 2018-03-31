//
//  common.h
//  comic_renderer
//
//  Created by Skatto on 2016/03/22.
//  Copyright © 2016年 shun. All rights reserved.
//

#ifndef common_h_20160322
#define common_h_20160322

#include <cmath>
#include <iostream>
#include <random>
#include <string>

typedef float real;

constexpr real kINF = 99999999999.9f;
constexpr real kEPS = 0.0001f;

struct Vec {
  real x, y, z;
  constexpr Vec(const real& x_, const real& y_, const real& z_)
      : x(x_), y(y_), z(z_) {}
  constexpr Vec(const real& v = 0) : x(v), y(v), z(v) {}

  inline void init() {
    x = 0.f;
    y = 0.f;
    z = 0.f;
  }

  constexpr inline Vec operator+(const Vec& b) const {
    return Vec(x + b.x, y + b.y, z + b.z);
  }
  constexpr inline Vec operator-(const Vec& b) const {
    return Vec(x - b.x, y - b.y, z - b.z);
  }
  constexpr Vec operator*(const real& b) const {
    return Vec(x * b, y * b, z * b);
  }
  constexpr Vec operator*(const Vec& b) const {
    return Vec(x * b.x, y * b.y, z * b.z);
  }
  constexpr Vec operator/(const real b) const {
    real p = 1 / b;
    return Vec(x * p, y * p, z * p);
  }
  inline void operator+=(const Vec& b) {
    x += b.x;
    y += b.y;
    z += b.z;
  }
  inline void operator-=(const Vec& b) {
    x -= b.x;
    y -= b.y;
    z -= b.z;
  }
  inline void operator*=(const real& b) {
    x *= b;
    y *= b;
    z *= b;
  }
  inline void operator*=(const Vec& b) {
    x *= b.x;
    y *= b.y;
    z *= b.z;
  }
  inline void operator/=(const real& b) {
    x /= b;
    y /= b;
    z /= b;
  }

  constexpr real length_squared() const { return x * x + y * y + z * z; }
  inline real length() const {
    return real(std::sqrt(double(length_squared())));
  }

  constexpr real operator[](int i) const { return (&x)[i]; }
  real& operator[](int i) { return (&x)[i]; }
};

inline const std::string to_string(const Vec& v) {
  return " ( " + std::to_string(v.x) + "," + std::to_string(v.y) + "," +
         std::to_string(v.z) + " ) ";
}
inline std::ostream& operator<<(std::ostream& os, const Vec& put) {
  os << to_string(put);
  return os;
}

constexpr real mymax(const real& a, const real& b) { return a > b ? a : b; }
constexpr Vec max(const Vec& a, const Vec& b) {
  return Vec(mymax(a.x, b.x), mymax(a.y, b.y), mymax(a.z, b.z));
}

constexpr real mymin(const real& a, const real& b) { return a < b ? a : b; }
constexpr Vec min(const Vec& a, const Vec& b) {
  return Vec(mymin(a.x, b.x), mymin(a.y, b.y), mymin(a.z, b.z));
}

constexpr real length(const Vec& v) { return v.length(); }

inline Vec operator*(const real& f, const Vec& v) { return v * f; }
inline Vec normalize(const Vec& v) { return v * (real(1) / v.length()); }
inline const Vec multiply(const Vec& v1, const Vec& v2) {
  return Vec(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}
inline real dot(const Vec& v1, const Vec& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
inline const Vec cross(const Vec& v1, const Vec& v2) {
  return Vec((v1.y * v2.z) - (v1.z * v2.y), (v1.z * v2.x) - (v1.x * v2.z),
             (v1.x * v2.y) - (v1.y * v2.x));
}
inline const Vec rotate(const Vec& a, const real& theta, const real& phi,
                        const real& ganma) {
  real sin_t = sinf(theta), cos_t = cosf(theta), sin_p = sinf(phi),
       cos_p = cosf(phi), sin_g = sinf(ganma), cos_g = cosf(ganma);
  return Vec(
      a.x * (cos_t * cos_g * cos_p - sin_t * sin_g) -
          a.y * (cos_t * cos_p * sin_g - sin_t * cos_g) + a.z * cos_t * sin_p,
      a.x * (sin_t * cos_g * cos_p + cos_t * sin_g) -
          a.y * (sin_t * cos_p * sin_g + cos_t * cos_g) + a.z * sin_t * sin_p,
      -sin_p * cos_g * a.x + sin_t * sin_g * a.y + cos_p * a.z);
}

inline void rotate0(Vec& a, const real& alpha, const real& beta,
                    const real& ganma) {
  real sin_a = sinf(alpha), cos_a = cosf(alpha), sin_b = sinf(beta),
       cos_b = cosf(beta), sin_g = sinf(ganma), cos_g = cosf(ganma);
  real x = a.x * (cos_a * cos_b * cos_g - sin_a * sin_g) -
           a.y * (cos_a * cos_b * sin_g + sin_a * cos_g) + a.z * cos_a * sin_b,
       y = a.x * (sin_a * cos_b * cos_g + cos_a * sin_g) -
           a.y * (sin_a * cos_b * sin_g - cos_a * cos_g) + a.z * sin_a * sin_b,
       z = a.x * -sin_b * cos_g + a.y * sin_b * sin_g + a.z * cos_b;
  a.x = x;
  a.y = y;
  a.z = z;
}

typedef Vec vec;
typedef Vec color;

constexpr color RED = color(1, 0, 0);
constexpr color GREEN = color(0, 1, 0);
constexpr color BLUE = color(0, 0, 1);
constexpr color YELLOW = color(1, 1, 0);
constexpr color LIGHT_BLUE = color(0, 1, 1);
constexpr color VIOLET = color(1, 0, 1);
constexpr color WHITE = color(0.9f);
constexpr color BLACK = color();
constexpr color GRAY = color(0.5f);

inline real rand_() { return real(std::rand()) / RAND_MAX; }

enum Material { Light, DirLight, Normal };

struct Ray {
  Vec org;
  Vec dir;
  Vec col;
  int depth;
  real pdf;
  Ray(const Vec& org_, const Vec& dir_, const real& pdf_ = 1)
      : org(org_), dir(dir_), depth(0), pdf(pdf_) {}
};

struct Intersection {
  Vec point;
  Vec normal;
  real t;
  Vec col;
  size_t solid_id;
  bool end;
  Material material;
  Intersection() : point(Vec(kINF)), t(kINF) {}
};

struct Polygon {
  Vec vert[3];
  color col;
  Material material;
  constexpr Polygon(const Vec& x, const Vec& y, const Vec& z, const color& col_,
                    const Material& material_ = Material::Normal)
      : vert{x, y, z}, col(col_), material(material_) {}
};

class BVH {
public:
  struct Node {
    Vec start, end;
    size_t s_idx = size_t(-1), e_idx = size_t(-1);
    size_t brother = size_t(-1);  // index of brother
    size_t parent = size_t(-1);
    bool leaf = false;
  };
  std::vector<Node> nodes;
  std::vector<Polygon> polygons;

public:
  BVH() {}

  bool init(const std::vector<Polygon>& polygons_);
};

#endif /* common_h */

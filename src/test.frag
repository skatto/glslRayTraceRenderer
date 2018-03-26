R"(
#version 330

#define kINF 100000000
#define kZERO 0.0001
#define kPI 3.1415926535

uniform int TRI_TEX_COL;

const vec3 camera_dir = vec3(1, 0, 0);
const vec3 camera_pos = vec3(-3, 0, 0);

const vec2 screen_size = vec2(640, 480);

uniform int num_sample;

uniform float aspect_ratio;

uniform vec4 rand_seed;

uniform sampler2D tri_tex;
uniform int num_tri;

uniform sampler2D bvh_tex;
uniform isampler1D bvh_info_tex;
uniform int bvh_size;

uniform bool onlyDraw;
uniform sampler2D d_tex;
uniform float brightness;
uniform float gamma;

in vec2 position;
out vec4 FragColor;

struct Intersection {
  vec3 point;
  vec3 normal;
  int pol_id;
  float t;
  vec3 col;
  int material;
};

struct Ray {
  vec3 org;
  vec3 dir;
  vec3 col;
};

struct Solid {
  int type;

  int reftype;
  vec3 color;
  bool light;
};

struct Seed {
  vec2 co[2];
};

Seed seed;

float rand() {
  vec2 buf;
  buf.x = fract(sin(dot(seed.co[0] ,vec2(12.9898,78.233))) * 43758.5453);
  buf.y = fract(sin(dot(seed.co[1] ,vec2(62.4293,29.845))) * 71354.8973);
  seed.co[1] = seed.co[0];
  seed.co[0] = buf;
  return buf.x;
}

void intersectTriangle(const Ray ray, const int tri_idx, inout Intersection result) {
  int col_idx = tri_idx % TRI_TEX_COL;
  int row_idx = tri_idx / TRI_TEX_COL;
  float per_tex = 1.0 / TRI_TEX_COL;
  vec3 position0 = texture(tri_tex, vec2(4*col_idx+0, row_idx) * per_tex).xyz;
  vec3 edge0 = texture(tri_tex, vec2(4*col_idx+1, row_idx) * per_tex).xyz - position0;
  vec3 edge1 = texture(tri_tex, vec2(4*col_idx+2, row_idx) * per_tex).xyz - position0;

  /* Möller–Trumbore intersection algorithm */
  vec3 P = cross(ray.dir, edge1);
  float det = dot(P, edge0);
  if(-kZERO < det && det < kZERO) return;
  float inv_det = 1.0 / det;
  vec3 T = ray.org - position0;
  float u = dot(T, P) * inv_det;
  if(u < 0.0 || 1.0 < u) return;
  vec3 Q = cross(T, edge0);
  float v = dot(ray.dir, Q) * inv_det;
  if(v < 0.0 || 1.0 < u + v) return;
  float t = dot(edge1, Q) * inv_det;

  if(kZERO < t && result.t > t){ // Hit
    result.point = ray.org + ray.dir * t;
    result.t = t;
    vec4 data = texture(tri_tex, vec2(4*col_idx+3, row_idx) * per_tex);
    result.col = data.xyz;
    result.normal = normalize(cross(edge0, edge1));
    result.pol_id = tri_idx;
    if (dot(result.normal, ray.dir) > 0) {
      result.normal = -result.normal;
    }
    result.material = int(data.w);
  }
}

bool intersectBoundingBox(const Ray ray, const int bb_idx) {
  int col_idx = bb_idx % TRI_TEX_COL;
  int row_idx = bb_idx / TRI_TEX_COL;
  float per_tex = 1.0 / TRI_TEX_COL;
  
  vec3 start = texture(bvh_tex, vec2(2*col_idx+0, row_idx) * per_tex).xyz;
  vec3 end = texture(bvh_tex, vec2(2*col_idx+1, row_idx) * per_tex).xyz;
 
  float t_far = kINF, t_near = -kINF;
  for(int i = 0; i < 3; i++){
    float t1 = (start[i] - ray.org[i]) / ray.dir[i];
    float t2 = (end[i] - ray.org[i]) / ray.dir[i];
    if(t1 < t2){
      t_far = min(t_far, t2);
      t_near = max(t_near, t1);
    }else{
      t_far = min(t_far, t1);
      t_near = max(t_near, t2);
    }
    if(t_far < t_near) return false;
  }
  return t_far > 0;
}

Intersection intersectBVH(const Ray ray) {
  Intersection isect;
  isect.t = kINF;
 
  int node_idx = 0;
  while(true) {
    ivec3 info = texture(bvh_info_tex, float(node_idx) / TRI_TEX_COL).xyz;
    
    if (intersectBoundingBox(ray, node_idx)) {
      if (info.x != -1) {
        for(int tri_idx = info.x; tri_idx < info.y; tri_idx++){
          intersectTriangle(ray, tri_idx, isect);
        }
      }
      node_idx++;
      if(node_idx >= bvh_size) break;
    }
    else {
      if (info.z == -1) {
        break;
      }
      node_idx = info.z;
    }
  }
  return isect;
}

Intersection intersect(const Ray ray) {
  Intersection isect;
  isect.t = kINF;
  for (int i = 0; i < num_tri; i++) {
    intersectTriangle(ray, i, isect);
  }
  return isect;
}

Ray decideRay(const vec3 normal, const vec3 point, const vec3 color, inout float pdf) {
  Ray ray;

  float phi = 2 * kPI * rand();
  float costheta = sqrt(rand());

  vec3 u;
  if (abs(normal.x) > kZERO) {
    u = normalize(cross(normal, vec3(0, 1, 0)));
  } else {
    u = normalize(cross(normal, vec3(1, 0, 0)));
  }
  vec3 v = normalize(cross(normal, u));
  ray = Ray(point, normalize(u * cos(phi) * costheta +
                             v * sin(phi) * costheta +
                             normal * sqrt(1.0 - costheta * costheta)), color);
  pdf *= kPI;
  return ray;
}

/*
vec3 toLight(const vec3 point, inout float pdf) {
  vec3 color = vec3(0);
//  int light_sam = int(num_tri + floor(rand() * num_light));
  float per_tex = 1.0 / TRI_TEX_COL;
  for (int light_sam = 0; light_sam < num_light; light_sam++) {
    int col_idx = light_sam % TRI_TEX_COL;
    int row_idx = light_sam / TRI_TEX_COL;

    float u = rand();
    float v = rand();
    
    if (u + v > 1) {
      u = 1 - u;
      v = 1 - v;
    }

    vec3 position0 = texture(tri_tex, vec2(4*col_idx+0, row_idx) * per_tex).xyz * mag;
    vec3 edge0 = texture(tri_tex, vec2(4*col_idx+1, row_idx) * per_tex).xyz * mag - position0;
    vec3 edge1 = texture(tri_tex, vec2(4*col_idx+2, row_idx) * per_tex).xyz * mag - position0;
    vec3 pos = edge0 * u + edge1 * v + position0;

    Ray to_l = Ray(point, normalize(pos - point), vec3(1));

    Intersection isect = intersect(to_l);
    if (isect.pol_id == light_sam){
      color += isect.col * -dot(isect.normal, to_l.dir);
      pdf *= length(pos - point) * kPI;
    }
  }

  return color / num_light;
}
*/

vec3 renderRay(Ray ray, out float pdf) {
  pdf = 1.f;
  ray.col = vec3(1);
  int n = 1;
  while(true) {
    Intersection result = intersectBVH(ray);
    if (result.t == kINF) {
      return vec3(0, 0, 0);
    }
    else if (result.material == 0) {  // material == Light
      ray.col *= result.col;
      return ray.col;
    }
    else if (result.material == 1) {  // material == DirLight
      ray.col *= result.col * -dot(result.normal, ray.dir);
      return ray.col;
    }
    if (n > 5 || rand() > pow(0.6, n-1)) {
      return vec3(0);
    }
    pdf *= pow(0.6, n - 1);
 
    ray.col *= result.col;
    ray = decideRay(result.normal, result.point, ray.col, pdf);

    n += 1;
  }
}

void main() {
  if (onlyDraw) {
    vec4 col = texture(d_tex, position);
    col = clamp(brightness * col / num_sample, vec4(0), vec4(1));
    FragColor = vec4(pow(col.xyz, vec3(gamma)), 1);
    return;
  }
 
  vec3 c_x = normalize(cross(camera_dir, vec3(0, 0, 1)));
  vec3 c_y = normalize(cross(camera_dir, c_x));

  seed.co[0] = rand_seed.xy * fract(sin(position.xy) * 1000);
  seed.co[1] = rand_seed.zw * fract(cos(position.yx) * 1000);

  vec3 color = vec3(0);
  for (int i = 0; i < num_sample; i++) {
    vec3 ray_d = normalize(c_x * (position.x - 0.5 + rand() / screen_size.x) * aspect_ratio +
                           c_y * (position.y - 0.5 + rand() / screen_size.y) +
                           camera_dir);
    Ray ray = Ray(camera_pos, ray_d, vec3(1));

    float pdf;
    vec3 col = renderRay(ray, pdf);
    color += col / pdf;
  }
  color /= num_sample;
  
  FragColor = vec4(texture(d_tex, position).xyz + color, 1);
}
)"

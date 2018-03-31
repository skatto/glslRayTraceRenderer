R"(
#version 330

#define kINF 100000000
#define kEPS 0.0001
#define kPI 3.1415926535

uniform int TRI_TEX_COL;

const vec3 camera_dir = vec3(1, 0, 0);
const vec3 camera_pos = vec3(-3, 0, 0);

const vec2 screen_size = vec2(640, 480);

uniform int num_sample;
uniform int render_count;

uniform float aspect_ratio;

uniform vec4 rand_seed;

uniform sampler2D tri_tex;
uniform int num_tri;

uniform sampler2D bvh_tex;
uniform isampler1D bvh_info_tex;
uniform int bvh_size;

uniform sampler2DRect point_tex;

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
  bool face;  // 裏表
};

struct Ray {
  vec3 org;
  vec3 dir;
  vec3 col;
};

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
  if(-kEPS < det && det < kEPS) return;
  float inv_det = 1.0 / det;
  vec3 T = ray.org - position0;
  float u = dot(T, P) * inv_det;
  if(u < 0.0 || 1.0 < u) return;
  vec3 Q = cross(T, edge0);
  float v = dot(ray.dir, Q) * inv_det;
  if(v < 0.0 || 1.0 < u + v) return;
  float t = dot(edge1, Q) * inv_det;

  if(kEPS < t && result.t > t){ // Hit
    result.point = ray.org + ray.dir * t;
    result.t = t;
    vec4 data = texture(tri_tex, vec2(4*col_idx+3, row_idx) * per_tex);
    result.col = data.xyz;
    result.normal = normalize(cross(edge0, edge1));
    result.pol_id = tri_idx;
    if (dot(result.normal, ray.dir) > 0) {
      result.normal = -result.normal;
      result.face = true;
    }
    else {
      result.face = false;
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

vec3 renderRay(Ray ray) {
  ray.col = vec3(1);
  int n = 1;

  Intersection isect = intersectBVH(ray);
  if (isect.t == kINF) {
    return vec3(0, 0, 0);
  }
  else if (isect.material == 0) {  // material == Light
    ray.col *= isect.col;
    return ray.col;
  }
  else if (isect.material == 1) {  // material == DirLight
    ray.col *= isect.col * -dot(isect.normal, ray.dir);
    return ray.col;
  }

  vec3 color = vec3(0);

  for (int i = 0; i < num_sample; i++) {
    vec4 pos = texture(point_tex, vec2(2 * i + 0, render_count));
    if (isect.pol_id == int(abs(pos.w))) {
      continue;
    }

    if (dot(pos.xyz - isect.point, isect.normal) <= kEPS) {
//      return vec3(1,1,0);
      continue;
    }

    ray = Ray(isect.point, normalize(pos.xyz - isect.point), isect.col);
    Intersection result = intersectBVH(ray);

    if (result.t == kINF) {
      continue;
    }
    else if (result.pol_id != int(abs(pos.w))) {
      continue;
    }
//    else if (result.face && sign(pos.w) > 0) {
//      continue;
//    }
//    else if (!result.face && sign(pos.w) < 0) {
//      continue;
//    }

    vec4 col = texture(point_tex, vec2(2 * i + 1, render_count));
    col.xyz *= result.col / kPI / kPI;

    color += max(vec3(0), col.xyz * dot(ray.dir, isect.normal) * -dot(result.normal, ray.dir) * col.w / (result.t * result.t));
  }

  return color / num_sample;
}

void main() {
  if (onlyDraw) {
    vec4 col = texture(d_tex, position);
//    if (col.x < 0) {
//      FragColor = vec4(0, 1, 0, 1);
//      return;
//    }if (col.y < 0) {
//      FragColor = vec4(0, 1, 0, 1);
//      return;
//    }if (col.z < 0.3) {
//      FragColor = vec4(0, 1, 0, 1);
//      return;
//    }
    col = clamp(col / num_sample, vec4(0), vec4(1));
    FragColor = vec4(pow(col.xyz, vec3(gamma)), 1);
    return;
  }
 
  vec3 c_x = normalize(cross(camera_dir, vec3(0, 0, -1)));
  vec3 c_y = normalize(cross(camera_dir, c_x));

  vec3 ray_d = normalize(c_x * (position.x - 0.5 + rand_seed.x / screen_size.x) * aspect_ratio +
                         c_y * (position.y - 0.5 + rand_seed.y / screen_size.y) +
                         camera_dir);
  vec3 col = renderRay(Ray(camera_pos, ray_d, vec3(1)));
  
  FragColor = vec4(texture(d_tex, position).xyz + col, 1);
//  FragColor = vec4(col, 1);
}
)"

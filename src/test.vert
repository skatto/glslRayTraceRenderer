R"(
#version 330

layout (location = 0) in vec2 coord2d;

out vec2 position;
out vec3 color;

void main(void) {
  gl_Position = vec4(coord2d, 0.0, 1.0);
  position = (coord2d+vec2(1,1)) / 2;
}
)"

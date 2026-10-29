#version 450

layout(location = 0) out vec2 frag_position;

const vec2 positions[3] = vec2[](
    vec2(-1, -1),
    vec2(3, -1),
    vec2(-1, 3)
  );

void main() {
  frag_position = positions[gl_VertexIndex];
  gl_Position = vec4(frag_position, 0.0, 1.0);
}

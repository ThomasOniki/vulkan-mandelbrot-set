#version 450

layout(location = 0) in vec2 frag_position;
layout(location = 0) out vec4 out_color;

// (a+bi) * (c+di)
// = a*(c+di) + bi*(c+di)
// = ac + adi + bic + bidi
// = ac + adi + bic - bd
// = ac - bd + adi + bic
// = (ac - bd) + (ad + bc)i
vec2 complex_mul(vec2 a, vec2 b) {
  return vec2((a.x * b.x - a.y * b.y), (a.x * b.y + a.y * b.x));
}
vec2 complex_add(vec2 a, vec2 b) {
  return vec2(a.x + b.x, a.y + b.y);
}

float square(float linear) {
  return linear * linear * linear * linear;
}

void main() {
  const uint iter_count = 100;
  const vec2 p1 = vec2(-2, -2);
  const vec2 p2 = vec2(2, 2);
  const vec3 c1 = vec3(0, 0, 0);
  const vec3 c2 = vec3(1, 1, 1);

  const float big = 100000000000000000.0;

  const float x_norm = (frag_position.x + 1) / 2;
  const float y_norm = (frag_position.y + 1) / 2;
  const float x_diff = p2.x - p1.x;
  const float y_diff = p2.y - p1.y;
  const vec2 c = vec2(p1.x + x_diff * x_norm, p1.y + y_diff * y_norm);

  vec2 z = vec2(0.0, 0.0);
  float color = 0;
  for (uint i = 0; i < iter_count; ++i) {
    z = complex_add(complex_mul(z, z), c);
    if (abs(z.x) > big || abs(z.y) > big) {
      color = square(1.0 - float(i) / iter_count);
      break;
    }
  }
  out_color = vec4(
      c1.r + (c2.r - c1.r) * color,
      c1.g + (c2.g - c1.g) * color,
      c1.b + (c2.b - c1.b) * color,
      1.0
    );
}

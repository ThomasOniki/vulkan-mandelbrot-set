struct VSOutput {
  float4 position : SV_Position;
};

static const float2 positions[] = {
  { -0.5, -0.5 },
  { 0.5, -0.5 },
  { -0.5, 0.5 },
};

[shader("vertex")]
VSOutput vertex_shader(uint vertex_index: SV_VertexID) {
  VSOutput out;
  out.position = float4(positions[vertex_index], 0, 0);
  return out;
}

[shader("fragment")]
float4 fragment_shader(VSOutput) : SV_Target { return float4(1, 0, 0, 0); }


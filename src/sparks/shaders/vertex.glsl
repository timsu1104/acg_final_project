

struct Vertex {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 tex_coord;
};

Vertex GetVertex(uint index) {
  uint offset = index * 11;
  Vertex vertex;
  vertex.position =
      vec3(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
  vertex.normal =
      vec3(vertices[offset + 3], vertices[offset + 4], vertices[offset + 5]);
  vertex.tangent =
      vec3(vertices[offset + 6], vertices[offset + 7], vertices[offset + 8]);
  vertex.tex_coord = vec2(vertices[offset + 9], vertices[offset + 10]);
  return vertex;
}

float GetArea(uint idx) {
  float area_ = 0;
  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint v_offset_up, i_offset_up;
  if (idx == object_infos.length() - 1) {
    v_offset_up = vertices.length();
    i_offset_up = indices.length();
  } else {
    v_offset_up = object_infos[idx + 1].vertex_offset;
    i_offset_up = object_infos[idx + 1].index_offset;
  }
  for (uint i = i_offset; i < i_offset_up; i += 3) {
    uint j = i + 1, k = i + 2;
    Vertex v0 = GetVertex(v_offset+indices[i]);
    Vertex v1 = GetVertex(v_offset+indices[j]);
    Vertex v2 = GetVertex(v_offset+indices[k]);
    float area = length(cross(v1.position - v0.position, v2.position - v0.position)) * 0.5f;
    area_ += area;
  }
  return area_;
}

vec3 sample_mesh(uint idx, float total_area) {
  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint v_offset_up, i_offset_up;
  uint sample_index = 0;
  float sampled_float = RandomFloat();
  float u = RandomFloat();
  float v = RandomFloat();

  if (idx == object_infos.length() - 1) {
    v_offset_up = vertices.length();
    i_offset_up = indices.length();
  } else {
    v_offset_up = object_infos[idx + 1].vertex_offset;
    i_offset_up = object_infos[idx + 1].index_offset;
  }
  for (uint i = i_offset; i < i_offset_up; i += 3) {
    uint j = i + 1, k = i + 2;
    Vertex v0 = GetVertex(v_offset+indices[i]);
    Vertex v1 = GetVertex(v_offset+indices[j]);
    Vertex v2 = GetVertex(v_offset+indices[k]);
    float area = length(cross(v1.position - v0.position, v2.position - v0.position)) * 0.5;
    float prob = area / total_area;
    if (sampled_float >= 0 && sampled_float < prob) {
      return v0.position * (1.0 - u) + v1.position * (u * (1.0 - v)) + v2.position * (u * v);
    }
    sampled_float -= prob;
  }
  return vec3(0.0);
}
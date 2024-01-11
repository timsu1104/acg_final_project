

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
    float area = length(cross(v1.position - v0.position, v2.position - v0.position)) * 0.5;
    area_ += area;
  }
  return area_;
}

float SignedVolumeOfTriangle(vec3 p1, vec3 p2, vec3 p3) {
    float v321 = p3.x*p2.y*p1.z;
    float v231 = p2.x*p3.y*p1.z;
    float v312 = p3.x*p1.y*p2.z;
    float v132 = p1.x*p3.y*p2.z;
    float v213 = p2.x*p1.y*p3.z;
    float v123 = p1.x*p2.y*p3.z;
    return (-v321 + v231 + v312 - v132 - v213 + v123) / 6.0;
}

float GetVolume(uint idx) {
  float volume_ = 0;
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
    volume_ += SignedVolumeOfTriangle(v0.position, v1.position, v2.position);
  }
  return volume_;
}

vec3 sample_mesh(uint idx, float total_area) {

  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint i_offset_up;
  float sampled_float = RandomFloat() * total_area;
  float u = RandomFloat();
  float v = RandomFloat();

  if (idx == object_infos.length() - 1) {
    i_offset_up = indices.length();
  } else {
    i_offset_up = object_infos[idx + 1].index_offset;
  }
  for (uint i = i_offset; i < i_offset_up; i += 3) {
    uint j = i + 1, k = i + 2;
    Vertex v0 = GetVertex(v_offset+indices[i]);
    Vertex v1 = GetVertex(v_offset+indices[j]);
    Vertex v2 = GetVertex(v_offset+indices[k]);
    float area = length(cross(v1.position - v0.position, v2.position - v0.position)) * 0.5;
    if (sampled_float <= area || i >= i_offset_up - 3) {
      return v0.position * (1.0 - u) + v1.position * (u * (1.0 - v)) + v2.position * (u * v);
    }
    sampled_float -= area;
  }
  // Unreachable
  return vec3(0.0);
}

vec3 sample_volume(uint idx) {
  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint i_offset_up = idx == object_infos.length() - 1 ? indices.length() : object_infos[idx + 1].index_offset;
  vec3 min_pos=vec3(1e9), max_pos=vec3(-1e9);
  for (uint i = i_offset; i < i_offset_up; i += 3) {
    uint j = i + 1, k = i + 2;
    Vertex v0 = GetVertex(v_offset+indices[i]);
    Vertex v1 = GetVertex(v_offset+indices[j]);
    Vertex v2 = GetVertex(v_offset+indices[k]);
    min_pos = min(v0.position, min_pos);
    min_pos = min(v1.position, min_pos);
    min_pos = min(v2.position, min_pos);
    max_pos = max(v0.position, max_pos);
    max_pos = max(v1.position, max_pos);
    max_pos = max(v2.position, max_pos);
  }
  return min_pos + (max_pos - min_pos) * vec3(RandomFloat(), RandomFloat(), RandomFloat());
}

vec3 GetCenter(uint idx) {
  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint i_offset_up = idx == object_infos.length() - 1 ? indices.length() : object_infos[idx + 1].index_offset;
  vec3 min_pos=vec3(1e9), max_pos=vec3(-1e9);
  for (uint i = i_offset; i < i_offset_up; i += 3) {
    uint j = i + 1, k = i + 2;
    Vertex v0 = GetVertex(v_offset+indices[i]);
    Vertex v1 = GetVertex(v_offset+indices[j]);
    Vertex v2 = GetVertex(v_offset+indices[k]);
    min_pos = min(v0.position, min_pos);
    min_pos = min(v1.position, min_pos);
    min_pos = min(v2.position, min_pos);
    max_pos = max(v0.position, max_pos);
    max_pos = max(v1.position, max_pos);
    max_pos = max(v2.position, max_pos);
  }
  return (min_pos + max_pos) * 0.5;
}
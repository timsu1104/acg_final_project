struct HitRecord {
  int hit_entity_id;
  vec3 position;
  vec3 normal;
  vec3 geometry_normal;
  vec3 tangent;
  vec2 tex_coord;
  bool front_face;

  vec3 base_color;
  vec3 emission;
  float emission_strength;
  float alpha;
  uint material_type;
};

HitRecord GetHitRecord(RayPayload ray_payload, vec3 origin, vec3 direction) {
  HitRecord hit_record;
  ObjectInfo object_info = object_infos[ray_payload.object_id];
  Vertex v0 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 0]);
  Vertex v1 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 1]);
  Vertex v2 = GetVertex(
      object_info.vertex_offset +
      indices[object_info.index_offset + ray_payload.primitive_id * 3 + 2]);
  hit_record.hit_entity_id = int(ray_payload.object_id);

  mat3 object_to_world = mat3(ray_payload.object_to_world);
  hit_record.position = ray_payload.object_to_world *
                        vec4(mat3(v0.position, v1.position, v2.position) *
                                 ray_payload.barycentric,
                             1.0);

  hit_record.normal = normalize(transpose(inverse(object_to_world)) *
                                mat3(v0.normal, v1.normal, v2.normal) *
                                ray_payload.barycentric);
  hit_record.geometry_normal =
      normalize(transpose(inverse(object_to_world)) *
                cross(v1.position - v0.position, v2.position - v0.position));
  hit_record.tangent =
      normalize(object_to_world * mat3(v0.tangent, v1.tangent, v2.tangent) *
                ray_payload.barycentric);
  hit_record.tex_coord = mat3x2(v0.tex_coord, v1.tex_coord, v2.tex_coord) *
                         ray_payload.barycentric;

  Material mat = materials[hit_record.hit_entity_id];
  hit_record.base_color =
      mat.albedo_color *
      texture(texture_samplers[mat.albedo_texture_id], hit_record.tex_coord)
          .xyz;
  hit_record.emission = mat.emission;
  hit_record.emission_strength = mat.emission_strength;
  hit_record.alpha = mat.alpha;
  hit_record.material_type = mat.material_type;

  if (dot(hit_record.geometry_normal, hit_record.normal) < 0.0) {
    hit_record.geometry_normal = -hit_record.geometry_normal;
  }

  hit_record.front_face = true;
  if (dot(direction, hit_record.geometry_normal) > 0.0) {
    hit_record.front_face = false;
    hit_record.geometry_normal = -hit_record.geometry_normal;
    hit_record.normal = -hit_record.normal;
    hit_record.tangent = -hit_record.tangent;
  }

  return hit_record;
}

HitRecord TraceRayMesh(int idx, vec3 origin, vec3 direction, float t_min, bool max_flag) {
  HitRecord hit_record;
  hit_record.hit_entity_id = -1;
  float result = -1.0;

  uint v_offset = object_infos[idx].vertex_offset;
  uint i_offset = object_infos[idx].index_offset;
  uint i_offset_up;

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

    mat3 A = mat3(v1.position - v0.position, v2.position - v0.position, -direction);
    if (abs(determinant(A)) < 1e-9) {
      continue;
    }
    A = inverse(A);
    vec3 uvt = A * (origin - v0.position);
    float t = uvt.z;
    if (t < t_min) {
      continue;
    }
    if (result > 0.0) {
      if (!max_flag && t > result) {
        continue;
      } else if (max_flag && t < result) {
        continue;
      }
    }
    float u = uvt.x;
    float v = uvt.y;
    float w = 1.0 - u - v;
    vec3 position = origin + t * direction;
    if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) {
      result = t;
      vec3 geometry_normal = normalize(
          cross(v2.position - v0.position, v1.position - v0.position));
      if (dot(geometry_normal, direction) < 0.0) {
        hit_record.hit_entity_id = idx;
        hit_record.position = position;
        hit_record.geometry_normal = geometry_normal;
        hit_record.normal = v0.normal * w + v1.normal * u + v2.normal * v;
        hit_record.tangent =
            v0.tangent * w + v1.tangent * u + v2.tangent * v;
        hit_record.tex_coord =
            v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
        hit_record.front_face = true;
      } else {
        hit_record.hit_entity_id = idx;
        hit_record.position = position;
        hit_record.geometry_normal = -geometry_normal;
        hit_record.normal = -(v0.normal * w + v1.normal * u + v2.normal * v);
        hit_record.tangent =
            -(v0.tangent * w + v1.tangent * u + v2.tangent * v);
        hit_record.tex_coord =
            v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
        hit_record.front_face = false;
      }
    }
  }
  Material mat = materials[hit_record.hit_entity_id];
  hit_record.base_color =
      mat.albedo_color *
      texture(texture_samplers[mat.albedo_texture_id], hit_record.tex_coord)
          .xyz;
  hit_record.emission = mat.emission;
  hit_record.emission_strength = mat.emission_strength;
  hit_record.alpha = mat.alpha;
  hit_record.material_type = mat.material_type;

  return hit_record;
}
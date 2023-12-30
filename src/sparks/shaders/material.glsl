
struct Material {
  vec3 albedo_color; // 0
  int albedo_texture_id; // 12
  vec3 emission; // 16
  int normal_map_id; // 28
  float emission_strength; // 32
  float alpha; // 36
  float density; // 40
  uint material_type; // 44
};

#define MATERIAL_TYPE_LAMBERTIAN 0
#define MATERIAL_TYPE_SPECULAR 1
#define MATERIAL_TYPE_TRANSMISSIVE 2
#define MATERIAL_TYPE_PRINCIPLED 3
#define MATERIAL_TYPE_EMISSION 4
#define MATERIAL_TYPE_ISOTROPIC 5

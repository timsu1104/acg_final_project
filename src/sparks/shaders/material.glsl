
struct Material {
  vec3 albedo_color; // 0
  int albedo_texture_id; // 12
  vec3 emission; // 16
  int normal_map_id; // 28
  vec3 attenuation; // 32
  float emission_strength; // 44
  float alpha; // 48
  int belong_id; // 52
  uint material_type; // 56
};

#define MATERIAL_TYPE_LAMBERTIAN 0
#define MATERIAL_TYPE_SPECULAR 1
#define MATERIAL_TYPE_TRANSMISSIVE 2
#define MATERIAL_TYPE_PRINCIPLED 3
#define MATERIAL_TYPE_EMISSION 4
#define MATERIAL_TYPE_VOLUME 5
#define MATERIAL_TYPE_MICROFACET 6
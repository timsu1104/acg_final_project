
struct Material {
  vec3 albedo_color; // 0
  int albedo_texture_id; // 12
  vec3 emission; // 16
  float emission_strength; // 28
  float alpha; // 32
  float density; // 36
  uint material_type; // 40
};

#define MATERIAL_TYPE_LAMBERTIAN 0
#define MATERIAL_TYPE_SPECULAR 1
#define MATERIAL_TYPE_TRANSMISSIVE 2
#define MATERIAL_TYPE_PRINCIPLED 3
#define MATERIAL_TYPE_EMISSION 4
#define MATERIAL_TYPE_ISOTROPIC 5

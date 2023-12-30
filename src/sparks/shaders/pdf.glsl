struct Onb {
    vec3 _u, _v, _w;
};

struct CosineHemispherePdf {
    Onb uvw;
};

struct LightPdf {
    int index;
    float area;
    vec3 normal_;
};

struct MixturePdf {
    LightPdf light_;
    CosineHemispherePdf cosine_;
    float prob;
};

Onb Onb_from_z(vec3 z_dir) {
  Onb ret;
  ret._w = normalize(z_dir);
  ret._u = normalize(cross(z_dir, (abs(z_dir.x) > 0.1 ? vec3(0.0, 1.0, 0.0): vec3(1.0, 0.0, 0.0))));
  ret._v = normalize(cross(z_dir, ret._u));
  return ret;
}

vec3 local(Onb uvw, float x, float y, float z) {
  return x * uvw._u + y * uvw._v + z * uvw._w;
}

vec3 Generate_Cos(CosineHemispherePdf pdf, vec3 origin) {
  float u1 = RandomFloat();
  float u2 = RandomFloat();
  float z_r = sqrt(1.0 - u1);
  float z = sqrt(u1);
  float phi = 2.0 * PI * u2;
  vec3 ret = local(pdf.uvw, z_r * cos(phi), z_r * sin(phi), z);
  return ret;
}

float Value_Cos(CosineHemispherePdf pdf, vec3 direction) {
  float cos_theta = dot(pdf.uvw._w, direction);
  return cos_theta < 0.0 ? 0.0 : cos_theta / PI;
}

vec3 Generate_Light(LightPdf pdf, vec3 origin) {
  vec3 light_sample = sample_mesh(pdf.index, pdf.area);
  if (dot(light_sample - origin, pdf.normal_) > 0.0) {
    return normalize(light_sample - origin);
  } else {
    return vec3(0.0);
  }
}

float Value_Light(LightPdf pdf, vec3 origin, vec3 direction) {
  HitRecord light_hit_rec = TraceRayMesh(pdf.index, origin, direction, 1e-3, false);
  float dist = distance(light_hit_rec.position, origin);
  float cosine = abs(dot(direction, light_hit_rec.normal)) / direction.length();
  return (dist * dist) / (cosine * pdf.area);
}

vec3 Generate_Mix(MixturePdf pdf, vec3 origin) {
  float u = RandomFloat();
  if (u < pdf.prob) {
    return Generate_Light(pdf.light_, origin);
  } else {
    return Generate_Cos(pdf.cosine_, origin);
  }
}

float Value_Mix(MixturePdf pdf, vec3 origin, vec3 direction) {
  float light_pdf = Value_Light(pdf.light_, origin, direction);
  float brdf_pdf = Value_Cos(pdf.cosine_, direction);
  return pdf.prob * light_pdf + (1.0 - pdf.prob) * brdf_pdf;
}
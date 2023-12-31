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

struct VolumeLightPdf {
    int index;
    float volume;
    float t;
    vec3 normal_;
};

struct MixturePdf {
    LightPdf light_[6];
    VolumeLightPdf volume_[2];
    CosineHemispherePdf cosine_;
    float prob_light, prob_volume, prob_cosine;
    int light_num, volume_num;
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

vec3 Generate_Volume(inout VolumeLightPdf pdf, vec3 origin) {
  vec3 light_sample = sample_volume(pdf.index);
  if (dot(light_sample - origin, pdf.normal_) > 0.0) {
    pdf.t = distance(light_sample, origin);
    return normalize(light_sample - origin);
  } else {
    return vec3(0.0);
  }
}

float Value_Volume(VolumeLightPdf pdf, vec3 origin, vec3 direction) {
  HitRecord light_hit_rec = TraceRayMesh(pdf.index, origin, direction, 1e-3, false);
  HitRecord far_light_hit_rec = TraceRayMesh(pdf.index, origin, direction, 1e-3, true);
  if (light_hit_rec.position == far_light_hit_rec.position) {
    light_hit_rec = TraceRayMesh(pdf.index, origin, -direction, 1e-3, false);
  }
  float dist = distance(light_hit_rec.position, far_light_hit_rec.position);
  return (pdf.t * pdf.t * dist) / (pdf.volume);
}

vec3 Generate_Mix(MixturePdf pdf, vec3 origin) {
  float u = RandomFloat();
  if (u < pdf.prob_light) {
    uint sel_light_idx = RandomInt(pdf.light_num);
    return Generate_Light(pdf.light_[sel_light_idx], origin);
  } else if (u < pdf.prob_light + pdf.prob_volume) {
    uint sel_vol_idx = RandomInt(pdf.volume_num);
    return Generate_Volume(pdf.volume_[sel_vol_idx], origin);
  } else {
    return Generate_Cos(pdf.cosine_, origin);
  }
}

float Value_Mix(MixturePdf pdf, vec3 origin, vec3 direction) {
  float light_pdf = 0;
  for (int i=0;i<pdf.light_num;i++) {
    light_pdf += Value_Light(pdf.light_[i], origin, direction);
  }
  light_pdf /= pdf.light_num;
  float volume_pdf = 0;
  for (int i=0;i<pdf.volume_num;i++) {
    volume_pdf += Value_Volume(pdf.volume_[i], origin, direction);
  }
  volume_pdf /= pdf.volume_num;
  float brdf_pdf = Value_Cos(pdf.cosine_, direction);
  return pdf.prob_light * light_pdf + pdf.prob_volume * volume_pdf + pdf.prob_cosine * brdf_pdf;
}

float DistributionGGX(vec3 n, vec3 h, float roughness) {
  float a = roughness * roughness, a2 = a * a;
  float nh = max(dot(n, h), 0.0f), nh2 = nh * nh;
  float div = (nh2 * (a2 - 1.0) + 1.0);
  div = PI * div * div;
  return a2 / max(div, 1e-5);
}

float geometrySchlickGGX(float nv, float k) {
  float div = nv * (1.0 - k) + k;
  return nv / div;
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float nv = max(dot(n, v), 0.0);
  float nl = max(dot(n, l), 0.0);
  float ggx2 = geometrySchlickGGX(nv, k);
  float ggx1 = geometrySchlickGGX(nl, k);
  return ggx1 * ggx2;
}

void fresnel(vec3 I, vec3 N, float ior, out float kr) {
  float cosi = dot(I, N);
  float etai = 1;
  float etat = ior;
  if (cosi > 0) {
    float t = etai;
    etai = etat;
    etat = t;
  }
  float sint = etai / etat * sqrt(max(0, 1 - cosi * cosi));
  if (sint >= 1) {
    kr = 1.0;
  }
  else {
    float cost = sqrt(max(0, 1 - sint * sint));
    cosi = abs(cosi);
    float rs = ((etat * cosi) - (etai * cosi)) / ((etat * cosi) + (etai * cost));
    float rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    kr = (rs * rs) + (rp * rp) / 2;
  }
}
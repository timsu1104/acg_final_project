struct Onb {
    vec3 _u, _v, _w;
};

struct CosineHemispherePdf {
    Onb uvw;
};

struct LightPdf {
    uint index;
    vec3 normal_;
};

struct MixturePdf {
    LightPdf light_;
    CosineHemispherePdf cosine_;
    float prob;
};

vec3 Generate_Light(LightPdf pdf, vec3 origin) {
  vec3 sample = sample_mesh(pdf.index);
  if (dot(sample - origin, pdf.normal_) > 0.0) {
    return normalize(sample - origin);
  } else {
    return vec3(0.0);
  }
}

float Value_Light(LightPdf pdf, vec3 origin, vec3 direction) {
  TraceRay(origin, direction);
  int idx = hit_record.hit_entity_id;
  if (idx == -1 || material[idx].material_type != MATERIAL_TYPE_EMISSION) {
    return 0.0;
  }
  float area = GetArea(idx);
  auto dist = distance(hit_record.position, origin);
  auto cosine = abs(dot(direction, hit_record.normal)) / direction.length();
  return (dist * dist) / (cosine * area);
}

vec3 Generate_Mix(MixturePdf pdf, vec3 origin) {
  if (RandomFloat() < pdf.prob) {
    return Generate_Light(pdf.light_, origin);
  } else {
    return Generate_Cos(pdf.cosine_, origin);
  }
}

float Value_Mix(MixturePdf pdf, vec3 origin, vec3 direction) {
  return pdf.prob * Value_Light(pdf.light_, origin, direction) +
         (1.0 - pdf.prob) * Value_Cos(pdf.cosine_, direction);
}
#include "sparks/assets/pdf.h"

#include "sparks/assets/material.h"
#include "sparks/assets/scene.h"
#include "sparks/util/util.h"
#include "vulkan/vulkan_core.h"

namespace sparks {
Onb::Onb() {
  _u = glm::vec3(1, 0, 0);
  _v = glm::vec3(0, 1, 0);
  _w = glm::vec3(0, 0, 1);
}
Onb::Onb(glm::vec3 z_dir) {
  _w = glm::normalize(z_dir);
  _u = glm::normalize(
      glm::cross(_w, (std::abs(_w.x) > 0.1 ? glm::vec3(0.0, 1.0, 0.0)
                                           : glm::vec3(1.0, 0.0, 0.0))));
  _v = glm::normalize(glm::cross(_w, _u));
}
glm::vec3 Onb::u() const {
  return _u;
}
glm::vec3 Onb::v() const {
  return _v;
}
glm::vec3 Onb::w() const {
  return _w;
}
glm::vec3 Onb::local(float x, float y, float z) const {
  return x * _u + y * _v + z * _w;
}
glm::vec3 Onb::local(glm::vec3 t) const {
  return t.x * _u + t.y * _v + t.z * _w;
}

UniformSpherePdf::UniformSpherePdf(glm::vec3 normal){
  uvw = Onb(normal);
}
glm::vec3 UniformSpherePdf::Generate(glm::vec3 origin, std::mt19937 &rd) const {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float theta = dist(rd) * 2.0f * PI;
  float phi = dist(rd) * PI;
  return uvw.local(std::cos(theta) * std::sin(phi),
                   std::sin(theta) * std::sin(phi), std::cos(phi));
}

float UniformSpherePdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  return 0.25f * INV_PI;
}

UniformHemispherePdf::UniformHemispherePdf(glm::vec3 normal) {
  uvw = Onb(normal);
}
glm::vec3 UniformHemispherePdf::Generate(glm::vec3 origin,
                                         std::mt19937 &rd) const {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float u1 = dist(rd);
  float u2 = dist(rd);
   double z_r = u1;
   double z = sqrt(1.0 - u1 * u1);
   double phi = 2.0 * PI * u2;
   return uvw.local(z_r * std::cos(phi), z_r * std::sin(phi), z);
}

float UniformHemispherePdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  return 0.5f * INV_PI;
}

LightPdf::LightPdf(glm::vec3 normal, const Scene *scene) : scene_(scene), normal_(normal) {
  normal_ = normal;
  light_ = NULL;
  for (auto &entity : scene->GetEntities()) {
    const Model* model = entity.GetModel();
    material = &entity.GetMaterial();
    if (material->material_type == MATERIAL_TYPE_EMISSION) {
      light_ = model;
      break;
    }
  }
}

glm::vec3 LightPdf::Generate(glm::vec3 origin, std::mt19937 &rd) const {
  if (light_ == NULL) return glm::vec3{0.0f};
  auto sample = light_->SamplingPoint(rd);
  if (glm::dot(sample - origin, normal_) > 0.0f) {
    return glm::normalize(sample - origin);
  } else {
    return glm::vec3{0.0f};
  }
}

float LightPdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  HitRecord light_hit_record;
  float light_t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &light_hit_record);
  if (light_t == -1.0f || light_hit_record.hit_entity_id == -1 || scene_->GetEntity(light_hit_record.hit_entity_id).GetMaterial().material_type != MATERIAL_TYPE_EMISSION) {
    return 0.0f;
  }
  float area = light_->GetArea();
  auto distance = glm::length(light_hit_record.position - origin);
  auto cosine = fabs(glm::dot(direction, light_hit_record.normal)) / direction.length();
  return (distance * distance) / (cosine * area);
}


VolumeLightPdf::VolumeLightPdf(glm::vec3 normal, const Scene *scene) : scene_(scene), normal_(normal) {
  normal_ = normal;
  light_ = NULL;
  for (auto &entity : scene->GetEntities()) {
    const Model* model = entity.GetModel();
    material = &entity.GetMaterial();
    if (material->material_type == MATERIAL_TYPE_VOLUME) {
      light_ = model;
      break;
    }
  }
}

glm::vec3 VolumeLightPdf::Generate(glm::vec3 origin, std::mt19937 &rd) const {
  if (light_ == NULL) return glm::vec3{0.0f};
  auto sample = light_->SamplingPoint(rd);
  if (glm::dot(sample - origin, normal_) > 0.0f) {
    return glm::normalize(sample - origin);
  } else {
    return glm::vec3{0.0f};
  }
}

float VolumeLightPdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  HitRecord light_hit_record;
  float light_t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &light_hit_record);
  if (light_t == -1.0f || light_hit_record.hit_entity_id == -1 || scene_->GetEntity(light_hit_record.hit_entity_id).GetMaterial().material_type != MATERIAL_TYPE_EMISSION) {
    return 0.0f;
  }
  float area = light_->GetArea();
  auto distance = glm::length(light_hit_record.position - origin);
  auto cosine = fabs(glm::dot(direction, light_hit_record.normal)) / direction.length();
  return (distance * distance) / (cosine * area);
}

CosineHemispherePdf::CosineHemispherePdf(glm::vec3 normal) {
  uvw = Onb(normal);
}
glm::vec3 CosineHemispherePdf::Generate(glm::vec3 origin,
                                        std::mt19937 &rd) const {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float u1 = dist(rd);
  float u2 = dist(rd);
  double z_r = sqrt(1.0 - u1);
  double z = sqrt(u1);
  double phi = 2.0 * PI * u2;
  return uvw.local(z_r * std::cos(phi),
                   z_r * std::sin(phi), z);
}

float CosineHemispherePdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  float cos_theta = glm::dot(uvw.w(), direction);
  return cos_theta < 0 ? 0 : cos_theta / PI;
}

MixturePdf::MixturePdf(Pdf* p1, Pdf* p2, float prob1){
  pdfList.resize(2);
  probList.resize(2);
  pdfList[0] = p1;
  probList[0] = prob1;
  pdfList[1] = p2;
  probList[1] = 1.0f;
}

MixturePdf::MixturePdf(std::vector<Pdf*> list, std::vector<float> weight)
    : pdfList(list){
  probList = weight;
  float sum = 0;
  for (int i = 0; i < probList.size(); ++i)
    sum += probList[i];
  for (int i = 0; i < probList.size(); ++i)
    probList[i] /= sum;
  for (int i = 1; i < probList.size(); ++i)
    probList[i] += probList[i-1];
  probList[probList.size() - 1] = 1.0f;
}

MixturePdf::MixturePdf(std::vector<Pdf*> list) : pdfList(list) {
  probList.resize(list.size());
  for (int i = 0; i < probList.size(); ++i)
    probList[i] = (i + 1) * 1.0f / probList.size();
  probList[probList.size() - 1] = 1.0f;
}

glm::vec3 MixturePdf::Generate(glm::vec3 origin, std::mt19937 &rd) const {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  float samp = dist(rd);
  int pdfNo = std::lower_bound(probList.begin(), probList.end(), samp) -
              probList.begin();
  return pdfList[pdfNo]->Generate(origin, rd);
}

float MixturePdf::Value(glm::vec3 origin, glm::vec3 direction) const {
  float result = probList[0] * pdfList[0]->Value(origin, direction);
  for (int i = 1; i < probList.size(); ++i) {
    result +=
        (probList[i] - probList[i - 1]) * pdfList[i]->Value(origin, direction);
  }
  return result;
}
}  // namespace sparks
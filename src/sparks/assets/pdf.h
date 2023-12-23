#pragma once
#include <random>
#include "sparks/assets/material.h"
#include "sparks/assets/model.h"
#include "sparks/assets/scene.h"
#include "sparks/assets/util.h"
#include "sparks/assets/vertex.h"

namespace sparks {
class Onb {
 public:
  Onb();
  Onb(glm::vec3 z_dir);
  glm::vec3 u() const;
  glm::vec3 v() const;
  glm::vec3 w() const;
  glm::vec3 local(float x, float y, float z) const;
  glm::vec3 local(glm::vec3 t) const;
 private:
  glm::vec3 _u, _v, _w;
};

class Pdf {
 public:
  virtual ~Pdf() = default;
  // Sample a direction based on the origin and a random_generator
  virtual glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const {
    return glm::vec3(1, 0, 0);
 }
  // Return the probability of sampling the direction based on the origin
  virtual float Value(glm::vec3 origin, glm::vec3 direction) const {
    return 1.0f;
  }
};

class UniformSpherePdf : public Pdf {
 public:
  UniformSpherePdf(glm::vec3 normal);
  glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const override;
  float Value(glm::vec3 origin, const glm::vec3 direction) const override;

 private:
  Onb uvw;
};

class UniformHemispherePdf : public Pdf {
 public:
  UniformHemispherePdf(glm::vec3 normal);
  glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const override;
  float Value(glm::vec3 origin, const glm::vec3 direction) const override;

 private:
  Onb uvw;
};

class LightPdf : public Pdf {
 public:
  // Collect all light sources in the scene
  LightPdf(glm::vec3 normal, const Scene *scene);
  glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const override;
  float Value(glm::vec3 origin, const glm::vec3 direction) const override;
  float Area() const {return light_->GetArea();}
  const Material& GetMaterial() const {return *material;}

 private:
  const Scene* scene_;
  const Model* light_{nullptr};
  const Material* material;
  glm::vec3 normal_;
};

class CosineHemispherePdf : public Pdf {
 public:
  CosineHemispherePdf(glm::vec3 normal);
  glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const override;
  float Value(glm::vec3 origin, glm::vec3 direction) const override;

 private:
  Onb uvw;
};

class MixturePdf : public Pdf {
 public:
  MixturePdf(Pdf* p1, Pdf* p2, float prob1);
  MixturePdf(std::vector<Pdf*> list, std::vector<float> prob);
  MixturePdf(std::vector<Pdf*> list);
  glm::vec3 Generate(glm::vec3 origin, std::mt19937 &rd) const override;
  float Value(glm::vec3 origin, glm::vec3 direction) const override;

 private:
  std::vector<Pdf*> pdfList;
  std::vector<float> probList;
};
}  // namespace sparks
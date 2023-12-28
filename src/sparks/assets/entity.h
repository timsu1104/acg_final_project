#pragma once
#include "memory"
#include "sparks/assets/material.h"
#include "sparks/assets/mesh.h"
#include "sparks/assets/model.h"

namespace sparks {
class Entity {
 public:
  template <class ModelType>
  Entity(const ModelType &model,
         const Material &material,
         const glm::mat4 &transform = glm::mat4{1.0f},
         const glm::vec3 &vel = glm::vec3{0.0f},
         const glm::vec3 &ang_vel = glm::vec3{0.0f}) {
    model_ = std::make_unique<ModelType>(model);
    material_ = material;
    transform_ = transform;
    vel_ = vel;
    ang_vel_ = ang_vel;
    ang_vel_ = ang_vel / 180.0f * glm::pi<float>();
    name_ = model_->GetDefaultEntityName();
  }

  template <class ModelType>
  Entity(const ModelType &model,
         const Material &material,
         const glm::mat4 &transform,
         const std::string &name,
         const glm::vec3 &vel = glm::vec3{0.0f},
         const glm::vec3 &ang_vel = glm::vec3{0.0f}) {
    model_ = std::make_unique<ModelType>(model);
    material_ = material;
    transform_ = transform;
    name_ = name;
    vel_ = vel;
    ang_vel_ = ang_vel / 180.0f * glm::pi<float>();
  }
  [[nodiscard]] const Model *GetModel() const;
  [[nodiscard]] glm::mat4 &UpdateTransformMatrix(uint32_t ms);
  [[nodiscard]] glm::mat4 &GetTransformMatrix();
  [[nodiscard]] const glm::mat4 &GetTransformMatrix() const;
  [[nodiscard]] Material &GetMaterial();
  [[nodiscard]] const Material &GetMaterial() const;
  [[nodiscard]] const std::string &GetName() const;

 private:
  std::unique_ptr<Model> model_;
  Material material_{};
  glm::mat4 transform_{1.0f};
  glm::vec3 vel_{0.0f}, ang_vel_{0.0f};
  std::string name_;
  uint32_t refresh_time_{3000};
};
}  // namespace sparks

#include "sparks/assets/entity.h"
#include "glm/gtc/matrix_transform.hpp"

namespace sparks {

const Model *Entity::GetModel() const {
  return model_.get();
}

glm::mat4 &Entity::UpdateTransformMatrix(uint32_t ms) {
  if (ms < refresh_time_) {
    refresh_time_ -= ms;
    return transform_;
  }
  float delta_time = (3000 + ms - refresh_time_) / 90000.0f;
  refresh_time_ = 3000;
  // float delta_time = ms / 1000.0f;
  glm::vec3 translation = glm::vec3(transform_[3]);
  glm::mat3 rotation = glm::mat3(transform_);
  glm::vec3 new_trans = translation + delta_time * vel_;
  glm::mat4 new_rot = glm::mat4(0.0f);
  for (int i = 0; i < 3; i++) {
    new_rot[i] = glm::vec4(rotation[i], 0.0f);
  }
  new_rot[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  new_rot = glm::rotate(new_rot, delta_time * ang_vel_[0], glm::vec3(1.0f, 0.0f, 0.0f));
  new_rot = glm::rotate(new_rot, delta_time * ang_vel_[1], glm::vec3(0.0f, 1.0f, 0.0f));
  new_rot = glm::rotate(new_rot, delta_time * ang_vel_[2], glm::vec3(0.0f, 0.0f, 1.0f));
  transform_ = glm::translate(glm::mat4(1.0f), new_trans) * new_rot;
  return transform_;
}

glm::mat4 &Entity::GetTransformMatrix() {
  return transform_;
}

const glm::mat4 &Entity::GetTransformMatrix() const {
  return transform_;
}

Material &Entity::GetMaterial() {
  return material_;
}

const Material &Entity::GetMaterial() const {
  return material_;
}

const std::string &Entity::GetName() const {
  return name_;
}

}  // namespace sparks

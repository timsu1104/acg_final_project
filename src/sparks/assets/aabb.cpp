#include "sparks/assets/aabb.h"

#include "algorithm"
#include "grassland/grassland.h"

namespace sparks {

AxisAlignedBoundingBox::AxisAlignedBoundingBox(float x_low,
                                               float x_high,
                                               float y_low,
                                               float y_high,
                                               float z_low,
                                               float z_high) {
  this->x_low = x_low;
  this->x_high = x_high;
  this->y_low = y_low;
  this->y_high = y_high;
  this->z_low = z_low;
  this->z_high = z_high;
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const glm::vec3 &position) {
  x_low = position.x;
  x_high = position.x;
  y_low = position.y;
  y_high = position.y;
  z_low = position.z;
  z_high = position.z;
}

bool LessEqual(float x, float y, float eps = 0.02f) {
  return x < y || fabs(x - y) < eps; 
}

inline bool Equal(float x, float y) {
  const float eps = 0.05f;
  return fabs(x - y) < eps; 
}

inline bool LessEqual2(float x, float y) {
  const float eps1 = 0.1f, eps2 = 0.01f;
  return (x < y && fabs(x - y) < eps1) || fabs(x - y) < eps2;
}

int AxisAlignedBoundingBox::CollisionCheck(const AxisAlignedBoundingBox& aabb, glm::vec3 vel, float delta_time) {
  int kx = false, ky = false, kz = false;
  kx = x_low < aabb.x_low ? LessEqual(aabb.x_low, x_high) : LessEqual(x_low, aabb.x_high);
  ky = y_low < aabb.y_low ? LessEqual(aabb.y_low, y_high) : LessEqual(y_low, aabb.y_high);
  kz = z_low < aabb.z_low ? LessEqual(aabb.z_low, z_high) : LessEqual(z_low, aabb.z_high);
  if (!(kx && ky && kz)) {
    return 0;
  }
  if (LessEqual(x_high, aabb.x_low) && LessEqual(aabb.x_low, x_high + vel.x * delta_time)) {
    return 1;
  }
  else if (LessEqual(aabb.x_high, x_low) && LessEqual(x_low + vel.x * delta_time, aabb.x_high)) {
    return 2;
  }
  else if (LessEqual(y_high, aabb.y_low, 0.03f) && LessEqual(aabb.y_low, y_high + vel.y * delta_time, 0.03f)) {
    return 3;
  }
  else if (LessEqual(aabb.y_high, y_low, 0.03f) && LessEqual(y_low + vel.y * delta_time, aabb.y_high, 0.03f)) {
    return 4;
  } 
  else if (LessEqual(z_high, aabb.z_low) && LessEqual(aabb.z_low, z_high + vel.z * delta_time)) {
    return 5;
  }
  else if (LessEqual(aabb.z_high, z_low) && LessEqual(z_low + vel.z * delta_time, aabb.z_high)) {
    return 6;
  }
  return 0;
}

bool AxisAlignedBoundingBox::IsIntersect(const glm::vec3 &origin,
                                         const glm::vec3 &direction,
                                         float t_min,
                                         float t_max) const {
  if (x_low <= origin.x && origin.x <= x_high && y_low <= origin.y &&
      origin.y <= y_high && z_low <= origin.z && origin.z <= z_high) {
    return true;
  }
  float intersection_range_low = t_max * (1.0f + t_min);
  float intersection_range_high = 0.0f;
  float t;
  glm::vec3 intersection;
#define TestIntersection(x, y, z)                                     \
  if (std::abs(direction.x) > 1e-5) {                                 \
    float inv_d = 1.0f / direction.x;                                 \
    t = (x##_low - origin.x) * inv_d;                                 \
    intersection = origin + direction * t;                            \
    if (y##_low <= intersection.y && intersection.y <= y##_high &&    \
        z##_low <= intersection.z && intersection.z <= z##_high) {    \
      intersection_range_low = std::min(intersection_range_low, t);   \
      intersection_range_high = std::max(intersection_range_high, t); \
    }                                                                 \
    t = (x##_high - origin.x) * inv_d;                                \
    intersection = origin + direction * t;                            \
    if (y##_low <= intersection.y && intersection.y <= y##_high &&    \
        z##_low <= intersection.z && intersection.z <= z##_high) {    \
      intersection_range_low = std::min(intersection_range_low, t);   \
      intersection_range_high = std::max(intersection_range_high, t); \
    }                                                                 \
  }
  TestIntersection(x, y, z);
  TestIntersection(z, x, y);
  TestIntersection(y, z, x);
  return intersection_range_high >= t_min && intersection_range_low <= t_max;
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::operator&(
    const AxisAlignedBoundingBox &aabb) const {
  return {std::max(x_low, aabb.x_low), std::min(x_high, aabb.x_high),
          std::max(y_low, aabb.y_low), std::min(y_high, aabb.y_high),
          std::max(z_low, aabb.z_low), std::min(z_high, aabb.z_high)};
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::operator|(
    const AxisAlignedBoundingBox &aabb) const {
  return {std::min(x_low, aabb.x_low), std::max(x_high, aabb.x_high),
          std::min(y_low, aabb.y_low), std::max(y_high, aabb.y_high),
          std::min(z_low, aabb.z_low), std::max(z_high, aabb.z_high)};
}

AxisAlignedBoundingBox &AxisAlignedBoundingBox::operator&=(
    const AxisAlignedBoundingBox &aabb) {
  x_low = std::max(x_low, aabb.x_low);
  x_high = std::min(x_high, aabb.x_high);
  y_low = std::max(y_low, aabb.y_low);
  y_high = std::min(y_high, aabb.y_high);
  z_low = std::max(z_low, aabb.z_low);
  z_high = std::min(z_high, aabb.z_high);
  return *this;
}

AxisAlignedBoundingBox &AxisAlignedBoundingBox::operator|=(
    const AxisAlignedBoundingBox &aabb) {
  x_low = std::min(x_low, aabb.x_low);
  x_high = std::max(x_high, aabb.x_high);
  y_low = std::min(y_low, aabb.y_low);
  y_high = std::max(y_high, aabb.y_high);
  z_low = std::min(z_low, aabb.z_low);
  z_high = std::max(z_high, aabb.z_high);
  return *this;
}

glm::vec3 AxisAlignedBoundingBox::Center() const {
  return glm::vec3((x_low + x_high) * 0.5f, (y_low + y_high) * 0.5f, (z_low + z_high) * 0.5f);
}

glm::vec3 AxisAlignedBoundingBox::Diagonal() const {
  return glm::vec3(x_high - x_low, y_high - y_low, z_high - z_low);
}

int AxisAlignedBoundingBox::maxExtent() const {
  glm::vec3 diag = Diagonal();
  if (diag.x > diag.y && diag.x > diag.z) {
    return 0;
  } else if (diag.y > diag.z) {
    return 1;
  }
  return 2;
}

}  // namespace sparks

#include "sparks/assets/bvh.h"
#include "sparks/assets/material.h"

namespace sparks {
    float BVHNode::TraceRay(const glm::vec3 &origin,
                   const glm::vec3 &direction,
                   float t_min,
                   float t_max,
                   HitRecord *hit_record,
                   bool ignore_isotropic) const {
        float result = -1.0f;
        if (!aabb.IsIntersect(origin, direction, t_min, t_max)) {
            return result;
        }
        if (ignore_isotropic && entity_id != -1 && entity->GetMaterial().material_type == MATERIAL_TYPE_VOLUME) {
            assert(entity_id == 6);
            return result;
        }

        if (entity_id != -1) {
            auto res = entity->GetModel()->TraceRay(origin,
                                                direction,
                                                t_min,
                                                hit_record);
            if (res != -1 && hit_record) {
                hit_record->hit_entity_id = entity_id;
            }
            return res;
        }

        HitRecord left_record, right_record;
        float left_dis = left->TraceRay(origin,
                                       direction,
                                       t_min,
                                       t_max,
                                       hit_record ?
                                       &left_record : nullptr, ignore_isotropic);
        float right_dis = right->TraceRay(origin,
                                       direction,
                                       t_min,
                                       t_max,
                                       hit_record ?
                                       &right_record : nullptr, ignore_isotropic);
        if (left_dis < 0.0f) {
            if (hit_record) {
                *hit_record = right_record;
            }
            return right_dis;
        }
        if (right_dis < 0.0f) {
            if (hit_record) {
                *hit_record = left_record;
            }
            return left_dis;
        }
        if (left_dis < right_dis) {
            if (hit_record) {
                *hit_record = left_record;
            }
            return left_dis;
        } else {
            if (hit_record) {
                *hit_record = right_record;
            }
            return right_dis;
        }
    }
}
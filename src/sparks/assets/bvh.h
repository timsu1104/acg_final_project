#pragma once
#include "sparks/assets/aabb.h"
#include "sparks/assets/mesh.h"
#include "sparks/assets/entity.h"
#include "sparks/assets/hit_record.h"

namespace sparks {

struct BVHNode {
    AxisAlignedBoundingBox aabb{};
    BVHNode* left{nullptr};
    BVHNode* right{nullptr};
    Entity* entity{nullptr};
    int entity_id{-1};
    int splitAxis{0};
    BVHNode() {
        aabb = AxisAlignedBoundingBox();
        left = nullptr;
        right = nullptr;
        entity = nullptr;
        entity_id = -1;
    }
    float TraceRay(const glm::vec3 &origin,
                   const glm::vec3 &direction,
                   float t_min,
                   float t_max,
                   HitRecord *hit_record) const;
};

struct BVHTree {
    BVHNode* root{nullptr};
    BVHTree() {
        root = nullptr;
    }
    BVHTree(BVHNode* root_) {
        root = root_;
    }
    BVHTree operator=(BVHNode* root_) {
        root = root_;
        return *this;
    }
    float TraceRay(const glm::vec3 &origin,
                   const glm::vec3 &direction,
                   float t_min,
                   float t_max,
                   HitRecord *hit_record) const {
        return root->TraceRay(origin, direction, t_min, t_max, hit_record);                
    }
};

}
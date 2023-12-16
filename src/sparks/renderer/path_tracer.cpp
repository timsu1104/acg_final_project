#include "sparks/renderer/path_tracer.h"

#include "sparks/util/util.h"
#include "glm/gtc/random.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace sparks {
PathTracer::PathTracer(const RendererSettings *render_settings,
                       const Scene *scene) {
  render_settings_ = render_settings;
  scene_ = scene;
}

class HemisphereSampler {
public:
    // 生成半球均匀分布的方向
    glm::vec3 Sample(glm::vec3 origin, std::mt19937 rd) const {
        float theta = 2.0 * glm::pi<float>() * RandomFloat(rd), phi = glm::pi<float>() * RandomFloat(rd);

        glm::vec3 direction(
            glm::cos(phi),
            glm::sin(phi) * glm::cos(theta),
            glm::sin(phi) * glm::sin(theta)
        );

        return direction;
    }

    float PDF() const {
        return 1.0 / (2.0 * glm::pi<float>());
    }

private:
    float RandomFloat(std::mt19937& rd) const {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rd);
    }
};

float reflectance(float cosine, float ref_idx) {
  auto r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
  r0 = r0 * r0;
  return r0 + (1 - r0) * glm::pow((1.0f - cosine), 5);
}

glm::vec3 PathTracer::SampleRay(glm::vec3 origin,
                                glm::vec3 direction,
                                int x,
                                int y,
                                int sample) const {
  glm::vec3 throughput{1.0f};
  glm::vec3 radiance{0.0f};
  glm::vec3 l_dir{0.0f};
  HitRecord hit_record;
  const float rate = 0.9;
  const int max_bounce = render_settings_->num_bounces;
  std::mt19937 rd(sample ^ x ^ y ^ std::time(0));
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  for (int i = 0; i < max_bounce; i++) {
    auto t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record);
    if (t > 0.0f) {
      auto &material =
          scene_->GetEntity(hit_record.hit_entity_id).GetMaterial();
      if (material.material_type == MATERIAL_TYPE_EMISSION) {
        radiance += throughput * material.emission * material.emission_strength;
        break;
      }
      
      if (material.material_type == MATERIAL_TYPE_LAMBERTIAN) {
        origin = hit_record.position;
        glm::vec3 albedo = material.albedo_color;
        glm::vec3 normal = hit_record.normal;
        if (glm::dot(normal, direction) > 0.0f) {
          normal = -normal;
        }
        if (material.albedo_texture_id >= 0) {
          albedo *= glm::vec3{
            scene_->GetTextures()[material.albedo_texture_id].Sample(hit_record.tex_coord)
          };
        }
        CosineHemispherePdf sampler(normal);
        direction = sampler.Generate(origin, rd);
        float pdf = sampler.Value(origin, direction);
        float scatter = std::max(0.f, glm::dot(normal, direction) / glm::pi<float>());
        throughput *= albedo * scatter / pdf / rate;
      }
      if (material.material_type == MATERIAL_TYPE_SPECULAR) {
        origin = hit_record.position;
        direction = glm::reflect(direction, hit_record.normal);
        throughput *= material.albedo_color;
      }
      if (material.material_type == MATERIAL_TYPE_TRANSMISSIVE) {
        origin = hit_record.position;
        float ir = 1.33f;
        float refraction_ratio = 1.0f / ir;
        throughput *= material.albedo_color;
        glm::vec3 normal = hit_record.normal;
        if (glm::dot(normal, direction) < 0.0f) {
          normal = -normal;
        }
        float cosine = glm::dot(normal, direction) / (glm::length(normal) * glm::length(direction));
        float sine = glm::sqrt(1.0f - cosine * cosine);
        if (reflectance(cosine, refraction_ratio) > dist(rd)) {
          direction = glm::reflect(direction, normal);
        } else {
          direction = glm::refract(direction, normal, refraction_ratio);
          refraction_ratio = ir;
          for (int k = 0; k < 5; k++) {
            scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record);
            origin = hit_record.position;
            refraction_ratio = 1.0f / refraction_ratio;
            glm::vec3 normal = hit_record.normal;
            if (glm::dot(normal, direction) < 0.0f) {
              normal = -normal;
            }
            cosine = glm::dot(normal, direction) / (glm::length(normal) * glm::length(direction));
            sine = glm::sqrt(1.0f - cosine * cosine);
            bool cannot_refract = refraction_ratio * sine > 1.0f;
            if (cannot_refract || reflectance(cosine, refraction_ratio) > dist(rd)) {
              direction = glm::reflect(direction, normal);
            } else {
              direction = glm::refract(direction, normal, refraction_ratio);
              break;
            }
          }
        }
      }
      if (material.material_type == MATERIAL_TYPE_PRINCIPLED) {
        //TODO
      } 
      // else {
      //   throughput *=
      //       material.albedo_color *
      //       glm::vec3{scene_->GetTextures()[material.albedo_texture_id].Sample(
      //           hit_record.tex_coord)};
      //   origin = hit_record.position;
      //   direction = scene_->GetEnvmapLightDirection();
      //   radiance += throughput * scene_->GetEnvmapMinorColor();
      //   throughput *=
      //       std::max(glm::dot(direction, hit_record.normal), 0.0f) * 2.0f;
      //   if (scene_->TraceRay(origin, direction, 1e-3f, 1e4f, nullptr) < 0.0f) {
      //     radiance += throughput * scene_->GetEnvmapMajorColor();
      //   }
      //   break;
      // }
      if (dist(rd) > rate || glm::max(throughput.x, glm::max(throughput.y, throughput.z)) < 1e-5f) {
        break;
      }
    } else {
      radiance += throughput * glm::vec3{scene_->SampleEnvmap(direction)};
      break;
    }
  }
  radiance = glm::min(radiance, glm::vec3{1});
  return radiance;
}

// glm::vec3 PathTracer::SampleRayRecursive(glm::vec3 origin,
//                                 glm::vec3 direction,
//                                 int x,
//                                 int y,
//                                 int sample,
//                                 int depth) const {    
//     glm::vec3 throughput{1.0f};
//     glm::vec3 radiance{0.0f};

//     if (depth == 0) {
//       return radiance;
//     }

//     HitRecord hit_record;
//     std::mt19937 rd(sample ^ x ^ y ^ std::time(0));
//     float rate = 0.9f;
//     std::uniform_real_distribution<float> dist(0.0f, 1.0f);
//     if (dist(rd) > rate) {
//       return radiance;
//     }
//     auto t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record);
//     if (t > 0.0f) {
//       auto &material = 
//           scene_->GetEntity(hit_record.hit_entity_id).GetMaterial();
//       if (material.material_type == MATERIAL_TYPE_EMISSION) {
//         radiance += throughput * material.emission * material.emission_strength;
//         return radiance;
//       }
//       glm::vec3 l_dir{0.0f}, l_indir{0.0f};
//       if (material.material_type == MATERIAL_TYPE_LAMBERTIAN) {
//         glm::vec3 new_ori = hit_record.position;
//         glm::vec3 normal = hit_record.normal;
//         glm::vec3 albedo = material.albedo_color;
//         if (material.albedo_texture_id >= 0) {
//           albedo *= glm::vec3{
//               scene_->GetTextures()[material.albedo_texture_id].Sample(
//                   hit_record.tex_coord)};
//         }
//         if (glm::dot(normal, direction) > 0.0f) {
//           normal = -normal;
//         }
//         CosineHemispherePdf sampler(normal);
//         glm::vec3 new_dir = sampler.Generate(new_ori, rd);
//         float pdf = sampler.Value(new_ori, new_dir);
//         HitRecord new_rec;
//         auto tt = scene_->TraceRay(new_ori, new_dir, 1e-3f, 1e4f, &new_rec);
//         if (tt > 0.0f) {
//           auto &new_mat = scene_->GetEntity(new_rec.hit_entity_id).GetMaterial();
//           // if (new_mat.material_type != MATERIAL_TYPE_EMISSION) {
//           //   glm::vec3 fr = glm::vec3{1.0f} * std::max(0.0f, glm::dot(direction, normal) / glm::pi<float>());
//           //   float cos = glm::max(0.0f, glm::dot(new_dir, normal));
//           //   l_indir = SampleRayRecursive(new_ori, new_dir, x, y, sample, depth - 1) * f_r
//           // }
//           // glm::vec3 fr = glm::vec3{1.0f} * std::max(0.0f, glm::dot(direction, -normal) / glm::pi<float>());
//           float cos = glm::max(0.0f, glm::dot(new_dir, normal)) / glm::pi<float>();
//           l_indir = albedo * SampleRayRecursive(new_ori, new_dir, x, y, sample, depth - 1) * cos / pdf / rate;
//           return l_dir + l_indir;
//         }
//         else {
//           return l_dir + l_indir;
//         }
//       } else {
//         return radiance;
//       }
//     } else {
//       radiance += throughput * glm::vec3{scene_->SampleEnvmap(direction)};
//       return radiance;
//     }
// }

// glm::vec3 PathTracer::SampleRay(glm::vec3 origin,
//                                 glm::vec3 direction,
//                                 int x,
//                                 int y,
//                                 int sample) const {
                            
//     const int max_bounce = render_settings_->num_bounces;
//     return SampleRayRecursive(origin, direction, x, y, sample, max_bounce);
//   }
}  // namespace sparks

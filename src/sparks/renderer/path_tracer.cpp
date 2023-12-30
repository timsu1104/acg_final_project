#include "sparks/renderer/path_tracer.h"

#include "sparks/assets/hit_record.h"
#include "sparks/assets/material.h"
#include "sparks/assets/pdf.h"
#include "sparks/util/util.h"
#include "glm/gtc/random.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace sparks {

inline static float InScatter(glm::vec3 start, glm::vec3 rd, glm::vec3 lightPos, float d)
{
  // Ack: https://zhuanlan.zhihu.com/p/21425792
    glm::vec3 q = start - lightPos;
    float b = glm::dot(rd, q);
    float c = glm::dot(q, q);
    float iv = 1.0f / sqrt(c - b*b);
    float l = iv * (atan( (d + b) * iv) - atan( b*iv ));

    return l;
}

PathTracer::PathTracer(const RendererSettings *render_settings,
                       const Scene *scene) {
  render_settings_ = render_settings;
  scene_ = scene;
}

class HemisphereSampler {
public:
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
  HitRecord hit_record_iso;
  const float rate = 0.9;
  const int soft_shadow_sample = 5;
  const int max_bounce = render_settings_->num_bounces;
  std::mt19937 rd(sample ^ x ^ y ^ rand());
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  for (int i = 0; i < max_bounce; i++) {
    auto t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record, true);
    auto t_iso = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record_iso, false);
    if (hit_record_iso.hit_entity_id != -1 && scene_->GetEntity(hit_record_iso.hit_entity_id).GetMaterial().material_type != MATERIAL_TYPE_VOLUME && t != t_iso) {
    auto t = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record, true);
    auto t_iso = scene_->TraceRay(origin, direction, 1e-3f, 1e4f, &hit_record_iso, false);
    }
    if (t > 0.0f) {
      auto &material =
          scene_->GetEntity(hit_record.hit_entity_id).GetMaterial();
      if (material.material_type == MATERIAL_TYPE_EMISSION) {
        radiance += throughput * material.emission * material.emission_strength;
        break;
      }

      glm::vec3 albedo = material.albedo_color;
      glm::vec3 normal = hit_record.normal;
      if (glm::dot(normal, direction) > 0.0f) {
        normal = -normal;
      }

      if (t != t_iso) {
        auto &entity = scene_->GetEntity(hit_record_iso.hit_entity_id);
        auto &iso_material = entity.GetMaterial();
        assert(iso_material.material_type == MATERIAL_TYPE_VOLUME);
        float random_float = dist(rd);
        float scatter_distance = - glm::log(random_float) / iso_material.attenuation[rand() % 3];
        HitRecord tmax_hit_record, tmin_hit_record;
        auto model = entity.GetModel();
        float light_t_max = model->TraceRay(origin, direction, 1e-3f, &tmax_hit_record, true);
        float light_t_min = model->TraceRay(origin, direction, -1e4f, &tmin_hit_record, false);
        assert(light_t_max >= light_t_min);
        if (light_t_min <= 0) { // We are inside the object
          light_t_min = 0;
        }
        float distance = glm::distance(tmax_hit_record.position, tmin_hit_record.position);

        if (scatter_distance < distance) {
          origin = origin + scatter_distance * direction;
          direction = glm::sphericalRand(1.0f);

          // Volumetric Lighting
          radiance += throughput * iso_material.emission * iso_material.emission_strength * InScatter(origin, direction, tmin_hit_record.position, distance);
          throughput *= albedo;
          continue;
        }
      }

      if (material.material_type == MATERIAL_TYPE_LAMBERTIAN) {
        
        origin = hit_record.position;
        if (material.albedo_texture_id >= 0) {
          albedo *= glm::vec3{
            scene_->GetTextures()[material.albedo_texture_id].Sample(hit_record.tex_coord)
          };
        }

        // direct light (soft shadow)
        LightPdf direct_light_sampler(normal, scene_);
        glm::vec3 light(0.0f);
        for (int k=0; k < soft_shadow_sample;k++) {
            glm::vec3 light_dir = direct_light_sampler.Generate(origin, rd);
          if (light_dir != glm::zero<glm::vec3>()) {
            auto light_pdf = direct_light_sampler.Value(origin, light_dir);
            if (light_pdf > 0) {
              auto &light_material = direct_light_sampler.GetMaterial();
              auto light_color = light_material.emission * light_material.emission_strength;
              float scatter = std::max(0.f, glm::dot(normal, light_dir) / glm::pi<float>());
              light += 
                throughput
                  * albedo
                  * scatter
                  * light_color
                  / light_pdf;
            }
          }
        }
        l_dir += light / (const float)soft_shadow_sample;

        // Importance Sampling based on Lambertian BRDF
        // CosineHemispherePdf sampler(normal, scene_);
        
        // Importance Sampling based on sampling from light
        // LightPdf sampler(normal, scene_);

        // Multiple Importance Sampling based on Lambertian BRDF and Light sampling
        CosineHemispherePdf brdf_sampler(normal);
        LightPdf light_sampler(normal, scene_);
        MixturePdf sampler(
          &brdf_sampler, 
          &light_sampler,
          0.8f
        );

        direction = sampler.Generate(origin, rd);
        float pdf = sampler.Value(origin, direction);
        float scatter = std::max(0.f, glm::dot(normal, direction) / glm::pi<float>());
        throughput *= albedo * scatter / pdf / rate;
      }
      else if (material.material_type == MATERIAL_TYPE_SPECULAR) {
        origin = hit_record.position;
        direction = glm::reflect(direction, hit_record.normal);
        throughput *= albedo;
      }
      else if (material.material_type == MATERIAL_TYPE_TRANSMISSIVE) {
        origin = hit_record.position;
        float ir = 1.33f;
        float refraction_ratio = 1.0f / ir;
        throughput *= albedo;
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
      else if (material.material_type == MATERIAL_TYPE_PRINCIPLED) {
        origin = hit_record.position;
        // Disney's principled BSDF

        // diffuse
        // f_basediffuse = 

        // sheen

        // metal 

        // clearcoat

        // glass

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
  radiance = glm::min(l_dir + radiance, glm::vec3{1});
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

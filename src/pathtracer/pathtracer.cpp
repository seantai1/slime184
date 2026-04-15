#include "pathtracer.h"

#include "scene/light.h"
#include "scene/sphere.h"
#include "scene/triangle.h"

#include "pathtracer/medium.h"
#include "util/random_util.h"


using namespace CGL::SceneObjects;

namespace CGL {
  PathTracer::PathTracer() {
    gridSampler = new UniformGridSampler2D();
    hemisphereSampler = new UniformHemisphereSampler3D();

    tm_gamma = 2.2f;
    tm_level = 1.0f;
    tm_key = 0.18;
    tm_wht = 5.0f;
  }

  PathTracer::~PathTracer() {
    delete gridSampler;
    delete hemisphereSampler;
  }

  void PathTracer::set_frame_size(size_t width, size_t height) {
    sampleBuffer.resize(width, height);
    sampleCountBuffer.resize(width * height);
  }

  void PathTracer::clear() {
    bvh = NULL;
    scene = NULL;
    camera = NULL;
    sampleBuffer.clear();
    sampleCountBuffer.clear();
    sampleBuffer.resize(0, 0);
    sampleCountBuffer.resize(0, 0);
  }

  void PathTracer::write_to_framebuffer(ImageBuffer &framebuffer, size_t x0,
                                        size_t y0, size_t x1, size_t y1) {
    sampleBuffer.toColor(framebuffer, x0, y0, x1, y1);
  }

  Vector3D
  PathTracer::estimate_direct_lighting_hemisphere(const Ray &r,
                                                  const Intersection &isect) {
    // Estimate the lighting from this intersection coming directly from a light.
    // For this function, sample uniformly in a hemisphere.

    // Note: When comparing Cornel Box (CBxxx.dae) results to importance sampling, you may find the "glow" around the light source is gone.
    // This is totally fine: the area lights in importance sampling has directionality, however in hemisphere sampling we don't model this behaviour.

    // make a coordinate system for a hit point
    // with N aligned with the Z direction.
    Matrix3x3 o2w;
    make_coord_space(o2w, isect.n);
    Matrix3x3 w2o = o2w.T();

    // w_out points towards the source of the ray (e.g.,
    // toward the camera if this is a primary ray)
    const Vector3D hit_p = r.o + r.d * isect.t;
    const Vector3D w_out = w2o * (-r.d);

    // This is the same number of total samples as
    // estimate_direct_lighting_importance (outside of delta lights). We keep the
    // same number of samples for clarity of comparison.
    int num_samples = scene->lights.size() * ns_area_light;
    Vector3D L_out;

    // TODO (Part 3): Write your sampling loop here
    // TODO BEFORE YOU BEGIN
    // UPDATE `est_radiance_global_illumination` to return direct lighting instead of normal shading

    for (int i = 0; i < num_samples; i++) {
      Vector3D wi_obj = hemisphereSampler->get_sample();
      Vector3D wi_world = o2w * wi_obj;
      Ray sample_ray(hit_p, wi_world);
      sample_ray.min_t = EPS_F;
      // Check if ray hits something
      Intersection sample_isect;
      if (bvh->intersect(sample_ray, &sample_isect)) {
        Vector3D Li = sample_isect.bsdf->get_emission(); //inc light
        double p_w = (1/(2*PI)); // choose p(w) to be Uniformly sample hemisphere (lec13) = 1/(2*PI)
        double cos_theta = wi_obj.z; // cos = dot(wi, n) as per note for unit vecs and and n is (0,0,1)
        L_out += isect.bsdf->f(w_out, wi_obj) * Li * cos_theta / p_w;
      }
    }
    L_out = L_out/num_samples;
    return L_out;

  }

  Vector3D
  PathTracer::estimate_direct_lighting_importance(const Ray &r,
                                                  const Intersection &isect) {
    // Estimate the lighting from this intersection coming directly from a light.
    // To implement importance sampling, sample only from lights, not uniformly in
    // a hemisphere.

    // make a coordinate system for a hit point
    // with N aligned with the Z direction.
    Matrix3x3 o2w;
    make_coord_space(o2w, isect.n);
    Matrix3x3 w2o = o2w.T();

    // w_out points towards the source of the ray (e.g.,
    // toward the camera if this is a primary ray)
    const Vector3D hit_p = r.o + r.d * isect.t;
    const Vector3D w_out = w2o * (-r.d);
    Vector3D L_out;

    for (auto light : scene->lights) {
      int num_samples;
      if (light->is_delta_light()) {
        num_samples = 1; //point lights need 1 sample
      } else {
        num_samples = ns_area_light;
      }
      Vector3D L_light;

      for (int i = 0; i < num_samples; i++) {
        Vector3D wi_world;
        double distToLight;
        double pdf;
        Vector3D Li = light->sample_L(hit_p, &wi_world, &distToLight, &pdf);
        if (pdf <= 0) continue;
        Vector3D wi_obj = w2o * wi_world;
        double cos_theta = wi_obj.z;
        if (cos_theta < 0) continue; // if light behind surface
        // check for blockers
        Ray sample_ray(hit_p, wi_world);
        sample_ray.min_t = EPS_F;
        sample_ray.max_t = distToLight - EPS_F;
        Intersection sample_isect;
        // If nothing blocks the light, accumulate (same reflection equation)
        if (!bvh->intersect(sample_ray, &sample_isect)) {
          L_light += isect.bsdf->f(w_out, wi_obj) * Li * cos_theta / pdf;
        }
      }
      L_out += L_light / num_samples;
    }

    return L_out;

  }

  Vector3D PathTracer::zero_bounce_radiance(const Ray &r,
                                            const Intersection &isect) {
    // TODO: Part 3, Task 2
    // Returns the light that results from no bounces of light

    return isect.bsdf->get_emission();


  }

  Vector3D PathTracer::one_bounce_radiance(const Ray &r,
                                           const Intersection &isect) {
    // TODO: Part 3, Task 3
    // Returns either the direct illumination by hemisphere or importance sampling
    // depending on `direct_hemisphere_sample`

    if (direct_hemisphere_sample) {
      return estimate_direct_lighting_hemisphere(r, isect);
    }
    else {
      return estimate_direct_lighting_importance(r, isect);
    }


  }

  Vector3D PathTracer::at_least_one_bounce_radiance(const Ray &r,
                                                    const Intersection &isect) {
    if (r.depth == 0) return Vector3D();

    Matrix3x3 o2w;
    make_coord_space(o2w, isect.n);
    Matrix3x3 w2o = o2w.T();

    Vector3D hit_p = r.o + r.d * isect.t;
    Vector3D w_out = w2o * (-r.d);

    Vector3D L_out = isAccumBounces ? one_bounce_radiance(r, isect) : Vector3D();

    if (r.depth == 1) {
      return isAccumBounces ? L_out : one_bounce_radiance(r, isect);
    }

    Vector3D w_in;
    double pdf = 0.0;
    Vector3D f = isect.bsdf->sample_f(w_out, &w_in, &pdf);

    if (pdf <= 0) return L_out;
    Vector3D w_in_world = o2w * w_in;

    Ray next_ray(hit_p, w_in_world);
    next_ray.min_t = EPS_F;
    next_ray.depth = r.depth - 1;

    const double continue_prob = 0.7;
    const bool continue_path =
        (r.depth == max_ray_depth) || coin_flip(continue_prob);

    if (!continue_path) return L_out;

    double continuation_weight =
        (r.depth == max_ray_depth) ? 1.0 : continue_prob;

    // If the BSDF carries a homogeneous medium and we just refracted INTO
    // the surface (new direction on the opposite side of the normal from
    // where the ray came in), detour through a volumetric random walk.
    HomogeneousMedium* med = isect.bsdf->medium_inside;
    bool entering_medium = (med != nullptr) && (w_in.z < 0.0);

    if (entering_medium) {
      Vector3D L_vol = random_walk_radiance(next_ray, med, /*walk_depth=*/32);
      Vector3D indirect =
          f * L_vol * abs_cos_theta(w_in) / (pdf * continuation_weight);
      L_out += indirect;
      return L_out;
    }

    Intersection next_isect;
    if (bvh->intersect(next_ray, &next_isect)) {
      Vector3D recursive = at_least_one_bounce_radiance(next_ray, next_isect);
      Vector3D indirect =
          f * recursive * abs_cos_theta(w_in) / (pdf * continuation_weight);
      L_out += indirect;
    }

    return L_out;
  }

  Vector3D PathTracer::random_walk_radiance(Ray r,
                                            const HomogeneousMedium* medium,
                                            int walk_depth) {
    if (walk_depth <= 0 || r.depth == 0) return Vector3D();

    Intersection isect;
    if (!bvh->intersect(r, &isect)) {
      // Escaped without hitting a surface. For a closed surface this shouldn't
      // happen; return zero.
      return Vector3D();
    }

    const double st_mean = medium->sigma_t_mean();
    const double xi = random_uniform();
    const double t_hit = isect.t;

    double t_scatter;
    if (st_mean <= 0.0) {
      t_scatter = std::numeric_limits<double>::infinity();
    } else {
      t_scatter = -std::log(std::max(1e-12, 1.0 - xi)) / st_mean;
    }

    if (t_scatter >= t_hit) {
      // Passed through the medium to the bounding surface. Apply per-channel
      // Beer-Lambert transmittance, divide by mean-channel miss pdf, and
      // continue normal surface path tracing at the exit intersection.
      Vector3D Tr = medium->transmittance(t_hit);
      double pdf_miss = (st_mean > 0.0) ? std::exp(-st_mean * t_hit) : 1.0;
      Vector3D weight = Tr / std::max(1e-12, pdf_miss);

      Vector3D L_surf = zero_bounce_radiance(r, isect)
                        + at_least_one_bounce_radiance(r, isect);
      return weight * L_surf;
    }

    // Volumetric scatter event.
    Vector3D p = r.o + r.d * t_scatter;
    Vector3D Tr = medium->transmittance(t_scatter);
    double pdf_scatter = st_mean * std::exp(-st_mean * t_scatter);
    Vector3D scatter_weight = medium->sigma_s * Tr / std::max(1e-12, pdf_scatter);

    Vector3D w_new;
    double pdf_phase;
    medium->sample_phase(-r.d, &w_new, &pdf_phase);
    // isotropic: phase function value f_p = 1/(4pi) = pdf_phase, so f_p/pdf_phase = 1.

    // Russian-roulette termination inside the medium.
    const double cont_prob = 0.9;
    if (!coin_flip(cont_prob)) return Vector3D();

    Ray next(p, w_new);
    next.min_t = EPS_F;
    next.max_t = INF_D;
    next.depth = r.depth;  // medium walk is orthogonal to surface-depth budget

    Vector3D L_next = random_walk_radiance(next, medium, walk_depth - 1);
    return (scatter_weight / cont_prob) * L_next;
  }

  Vector3D PathTracer::est_radiance_global_illumination(const Ray &r) {
    Intersection isect;
    Vector3D L_out;

    // You will extend this in assignment 3-2.
    // If no intersection occurs, we simply return black.
    // This changes if you implement hemispherical lighting for extra credit.

    // The following line of code returns a debug color depending
    // on whether ray intersection with triangles or spheres has
    // been implemented.
    //
    // REMOVE THIS LINE when you are ready to begin Part 3.

    if (!bvh->intersect(r, &isect))
      return envLight ? envLight->sample_dir(r) : L_out;

    // L_out = (isect.t == INF_D) ? debug_shading(r.d) : normal_shading(isect.n);

    //  at_least_one_bounce_radiance -> is all bounced light, emitted radiance L_e emitted radiance
    L_out = zero_bounce_radiance(r, isect) + at_least_one_bounce_radiance(r, isect);

    return L_out;
  }

  void PathTracer::raytrace_pixel(size_t x, size_t y) {
    // TODO (Part 1.2):
    // Make a loop that generates num_samples camera rays and traces them
    // through the scene. Return the average Vector3D.
    // You should call est_radiance_global_illumination in this function.

    // TODO (Part 5):
    // Modify your implementation to include adaptive sampling.
    // Use the command line parameters "samplesPerBatch" and "maxTolerance"
    int num_samples = ns_aa;          // total samples to evaluate
    Vector2D origin = Vector2D(x, y); // bottom left corner of the pixel

    Vector3D radiance(0, 0, 0);
    double s1 = 0.0; // sum of sample illuminances
    double s2 = 0.0; // sum of squared smaple illuminances
    int n = 0; // samples taken so far

    for (int i = 1; i <= num_samples; i++) {
      Vector2D sample = gridSampler->get_sample();
      double normX = (origin.x + sample.x) / sampleBuffer.w;
      double normY = (origin.y + sample.y) / sampleBuffer.h;
      Ray r = camera->generate_ray(normX, normY);
      r.depth = max_ray_depth;
      Vector3D L = est_radiance_global_illumination(r);
      radiance += L;
      double illum = L.illum(); // brightness for adaptive sampling

      s1 += illum; // sum(x_k)
      s2 += illum * illum; // sum(x_k^2)
      n++;

      // check convergence every samplesPerBatch samples
    if (n % samplesPerBatch == 0 && n < num_samples) {
      double mu = s1 / n;
      double variance = (n > 1) ? (s2 - (s1 * s1) / n) / (n - 1) : 0.0;
      variance = max(0.0, variance);
      double sigma = sqrt(variance);
      double I = 1.96 * sigma / sqrt((double) n);
      if (I <= maxTolerance * mu) {
        break;
      }
      }
    }
    radiance /= n;
    sampleBuffer.update_pixel(radiance,x,y);
    sampleCountBuffer[x + y * sampleBuffer.w] = n;
  }

  void PathTracer::autofocus(Vector2D loc) {
    Ray r = camera->generate_ray(loc.x / sampleBuffer.w, loc.y / sampleBuffer.h);
    Intersection isect;

    bvh->intersect(r, &isect);

    camera->focalDistance = isect.t;
  }

} // namespace CGL

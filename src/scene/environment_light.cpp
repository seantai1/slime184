#include "environment_light.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "util/lodepng.h"

namespace CGL { namespace SceneObjects {

  EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
    : envMap(envMap) {
    init();
  }

  EnvironmentLight::~EnvironmentLight() {
    delete[] pdf_envmap;
    delete[] conds_y;
    delete[] marginal_y;
  }

  void EnvironmentLight::init() {
    uint32_t w = envMap->w, h = envMap->h;
    pdf_envmap = new double[w * h];
    conds_y = new double[w * h];
    marginal_y = new double[h];

    std::cout << "[PathTracer] Initializing environment light...";

    std::vector<double> row_sums(h, 0.0);
    double total = 0.0;

    for (uint32_t j = 0; j < h; ++j) {
      double theta = PI * (j + 0.5) / h;
      double sin_theta = std::max(0.0, std::sin(theta));
      for (uint32_t i = 0; i < w; ++i) {
        double luminance =
            std::max(0.0, (double) envMap->data[w * j + i].illum());
        double weight = luminance * sin_theta;
        pdf_envmap[w * j + i] = weight;
        row_sums[j] += weight;
        total += weight;
      }
    }

    if (total <= 0.0) {
      total = 0.0;
      for (uint32_t j = 0; j < h; ++j) {
        double theta = PI * (j + 0.5) / h;
        double sin_theta = std::max(0.0, std::sin(theta));
        row_sums[j] = 0.0;
        for (uint32_t i = 0; i < w; ++i) {
          pdf_envmap[w * j + i] = sin_theta;
          row_sums[j] += sin_theta;
          total += sin_theta;
        }
      }
    }

    double marginal_cdf = 0.0;
    for (uint32_t j = 0; j < h; ++j) {
      double conditional_cdf = 0.0;
      if (row_sums[j] > 0.0) {
        for (uint32_t i = 0; i < w; ++i) {
          conditional_cdf += pdf_envmap[w * j + i] / row_sums[j];
          conds_y[w * j + i] = conditional_cdf;
          pdf_envmap[w * j + i] /= total;
        }
      } else {
        for (uint32_t i = 0; i < w; ++i) {
          conditional_cdf = (i + 1.0) / w;
          conds_y[w * j + i] = conditional_cdf;
          pdf_envmap[w * j + i] = 0.0;
        }
      }

      conds_y[w * j + w - 1] = 1.0;
      marginal_cdf += row_sums[j] / total;
      marginal_y[j] = marginal_cdf;
    }

    marginal_y[h - 1] = 1.0;

    std::cout << "done." << std::endl;
  }

  // Helper functions

  void EnvironmentLight::save_probability_debug() {
    uint32_t w = envMap->w, h = envMap->h;
    uint8_t* img = new uint8_t[4 * w * h];

    for (uint32_t j = 0; j < h; ++j) {
      for (uint32_t i = 0; i < w; ++i) {
        img[4 * (j * w + i) + 3] = 255;
        img[4 * (j * w + i) + 0] = 255 * marginal_y[j];
        img[4 * (j * w + i) + 1] = 255 * conds_y[j * w + i];
        img[4 * (j * w + i) + 2] = 0;
      }
    }

    lodepng::encode("probability_debug.png", img, w, h);
    delete[] img;
  }

  Vector2D EnvironmentLight::theta_phi_to_xy(const Vector2D& theta_phi) const {
    uint32_t w = envMap->w, h = envMap->h;
    double x = theta_phi.y / 2. / PI * w;
    double y = theta_phi.x / PI * h;
    return Vector2D(x, y);
  }

  Vector2D EnvironmentLight::xy_to_theta_phi(const Vector2D& xy) const {
    uint32_t w = envMap->w, h = envMap->h;
    double x = xy.x;
    double y = xy.y;
    double phi = x / w * 2.0 * PI;
    double theta = y / h * PI;
    return Vector2D(theta, phi);
  }

  Vector2D EnvironmentLight::dir_to_theta_phi(const Vector3D dir) const {
    Vector3D unit_dir = dir.unit();
    double theta = acos(unit_dir.y);
    double phi = atan2(-unit_dir.z, unit_dir.x) + PI;
    return Vector2D(theta, phi);
  }

  Vector3D EnvironmentLight::theta_phi_to_dir(const Vector2D& theta_phi) const {
    double theta = theta_phi.x;
    double phi = theta_phi.y;

    double y = cos(theta);
    double x = cos(phi - PI) * sin(theta);
    double z = -sin(phi - PI) * sin(theta);

    return Vector3D(x, y, z);
  }

  Vector3D EnvironmentLight::bilerp(const Vector2D& xy) const {
    double x = std::fmod(xy.x - 0.5, (double) envMap->w);
    if (x < 0.0) x += envMap->w;
    double y = std::max(0.0, std::min((double) envMap->h - 1.0, xy.y - 0.5));

    int x0 = (int) std::floor(x);
    int y0 = (int) std::floor(y);
    int x1 = (x0 + 1) % envMap->w;
    int y1 = std::min(y0 + 1, (int) envMap->h - 1);
    double tx = x - x0;
    double ty = y - y0;

    const Vector3D& c00 = envMap->data[y0 * envMap->w + x0];
    const Vector3D& c10 = envMap->data[y0 * envMap->w + x1];
    const Vector3D& c01 = envMap->data[y1 * envMap->w + x0];
    const Vector3D& c11 = envMap->data[y1 * envMap->w + x1];

    Vector3D cx0 = c00 * (1.0 - tx) + c10 * tx;
    Vector3D cx1 = c01 * (1.0 - tx) + c11 * tx;
    return cx0 * (1.0 - ty) + cx1 * ty;
  }

  double EnvironmentLight::pdf_dir(const Vector3D& dir) const {
    if (envMap->w == 0 || envMap->h == 0) return 0.0;

    Vector2D theta_phi = dir_to_theta_phi(dir);
    Vector2D xy = theta_phi_to_xy(theta_phi);

    int x = (int) std::floor(xy.x);
    int y = (int) std::floor(xy.y);
    x %= (int) envMap->w;
    if (x < 0) x += envMap->w;
    y = std::max(0, std::min((int) envMap->h - 1, y));

    double theta = PI * (y + 0.5) / envMap->h;
    double sin_theta = std::max(1e-8, std::sin(theta));
    double pixel_solid_angle = (2.0 * PI / envMap->w) *
                               (PI / envMap->h) * sin_theta;
    return pdf_envmap[y * envMap->w + x] / pixel_solid_angle;
  }

  double EnvironmentLight::pdf_L(const Vector3D p, const Vector3D wi) const {
    return pdf_dir(wi);
  }

  Vector3D EnvironmentLight::sample_L(const Vector3D p, Vector3D* wi,
    double* distToLight,
    double* pdf) const {
    Vector2D u = sampler_uniform2d.get_sample();
    auto y_it = std::lower_bound(marginal_y, marginal_y + envMap->h, u.y);
    int y = std::min((int) (y_it - marginal_y), (int) envMap->h - 1);

    double* row_begin = conds_y + y * envMap->w;
    auto x_it = std::lower_bound(row_begin, row_begin + envMap->w, u.x);
    int x = std::min((int) (x_it - row_begin), (int) envMap->w - 1);

    Vector2D jitter = sampler_uniform2d.get_sample();
    Vector2D xy((double) x + jitter.x, (double) y + jitter.y);
    *wi = theta_phi_to_dir(xy_to_theta_phi(xy)).unit();
    *distToLight = INF_D;
    *pdf = pdf_dir(*wi);

    return *pdf > 0.0 ? sample_dir(Ray(p, *wi)) : Vector3D();
  }

  Vector3D EnvironmentLight::sample_dir(const Ray& r) const {
    return bilerp(theta_phi_to_xy(dir_to_theta_phi(r.d)));
  }

} // namespace SceneObjects
} // namespace CGL

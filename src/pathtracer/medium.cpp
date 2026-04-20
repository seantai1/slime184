#include "medium.h"

#include <cmath>
#include <algorithm>

#include "CGL/misc.h"
#include "pathtracer/bsdf.h"
#include "util/random_util.h"

namespace CGL {

Vector3D HomogeneousMedium::transmittance(double dist) const {
  Vector3D st = sigma_t();
  return Vector3D(std::exp(-st.x * dist),
                  std::exp(-st.y * dist),
                  std::exp(-st.z * dist));
}

double HomogeneousMedium::sample_distance(double xi, double* pdf_out) const {
  double st = sigma_t_mean();
  if (st <= 0.0) {
    *pdf_out = 1.0;
    return std::numeric_limits<double>::infinity();
  }
  // p(t) = st * exp(-st * t), CDF^-1 = -log(1-xi)/st
  double t = -std::log(std::max(1e-12, 1.0 - xi)) / st;
  *pdf_out = st * std::exp(-st * t);
  return t;
}

void HomogeneousMedium::sample_phase(const Vector3D& wo, Vector3D* wi,
                                     double* pdf) const {
  double u1 = random_uniform();
  double u2 = random_uniform();

  double cos_theta;
  if (std::abs(g) < 1e-3) {
    cos_theta = 1.0 - 2.0 * u1;
  } else {
    double sqr_term = (1.0 - g * g) / (1.0 + g - 2.0 * g * u1);
    cos_theta = -(1.0 + g * g - sqr_term * sqr_term) / (2.0 * g);
    cos_theta = std::max(-1.0, std::min(1.0, cos_theta));
  }

  double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
  double phi = 2.0 * PI * u2;

  Matrix3x3 phase_to_world;
  make_coord_space(phase_to_world, wo.unit());
  *wi = phase_to_world *
        Vector3D(sin_theta * std::cos(phi), sin_theta * std::sin(phi),
                 cos_theta);

  double denom = 1.0 + g * g + 2.0 * g * cos_theta;
  *pdf = (1.0 - g * g) /
         (4.0 * PI * std::max(1e-8, denom * std::sqrt(denom)));
}

}  // namespace CGL

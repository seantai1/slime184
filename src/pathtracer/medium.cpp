#include "medium.h"

#include <cmath>
#include <algorithm>

#include "CGL/misc.h"
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
  // Isotropic: uniform sphere.
  double u1 = random_uniform();
  double u2 = random_uniform();
  double z = 1.0 - 2.0 * u1;           // cos_theta in [-1, 1]
  double r = std::sqrt(std::max(0.0, 1.0 - z * z));
  double phi = 2.0 * PI * u2;
  *wi = Vector3D(r * std::cos(phi), r * std::sin(phi), z);
  *pdf = 1.0 / (4.0 * PI);
}

}  // namespace CGL

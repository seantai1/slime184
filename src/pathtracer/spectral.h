#ifndef CGL_SPECTRAL_H
#define CGL_SPECTRAL_H

#include <algorithm>
#include <cmath>

#include "CGL/vector3D.h"
#include "util/random_util.h"

namespace CGL {
namespace Spectral {

static const double kLambdaMin = 380.0;
static const double kLambdaMax = 720.0;
static const double kLambdaReference = 550.0;

inline double sample_hero_wavelength(double* pdf = nullptr) {
  if (pdf) *pdf = 1.0 / (kLambdaMax - kLambdaMin);
  return kLambdaMin + random_uniform() * (kLambdaMax - kLambdaMin);
}

inline double gaussian(double lambda, double center, double sigma) {
  double x = (lambda - center) / sigma;
  return std::exp(-0.5 * x * x);
}

inline double gaussian_average(double center, double sigma) {
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  double a = (kLambdaMin - center) * inv_sqrt2 / sigma;
  double b = (kLambdaMax - center) * inv_sqrt2 / sigma;
  double integral = sigma * std::sqrt(PI / 2.0) * (std::erf(b) - std::erf(a));
  return integral / (kLambdaMax - kLambdaMin);
}

inline Vector3D wavelength_rgb_response(double lambda_nm) {
  double lambda = std::max(kLambdaMin, std::min(kLambdaMax, lambda_nm));

  double r = gaussian(lambda, 610.0, 45.0);
  double g = gaussian(lambda, 545.0, 35.0);
  double b = gaussian(lambda, 460.0, 30.0);

  static const double avg_r = gaussian_average(610.0, 45.0);
  static const double avg_g = gaussian_average(545.0, 35.0);
  static const double avg_b = gaussian_average(460.0, 30.0);

  return Vector3D(r / avg_r, g / avg_g, b / avg_b);
}

inline Vector3D hero_sample_to_rgb(const Vector3D& radiance,
                                   double lambda_nm) {
  return radiance * wavelength_rgb_response(lambda_nm);
}

inline double cauchy_ior(double base_ior, double cauchy_b,
                         double lambda_nm) {
  double lambda_um = std::max(kLambdaMin, std::min(kLambdaMax, lambda_nm)) *
                     1e-3;
  double reference_um = kLambdaReference * 1e-3;
  return base_ior + cauchy_b *
                        (1.0 / (lambda_um * lambda_um) -
                         1.0 / (reference_um * reference_um));
}

}  // namespace Spectral
}  // namespace CGL

#endif  // CGL_SPECTRAL_H

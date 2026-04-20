#ifndef CGL_MEDIUM_H
#define CGL_MEDIUM_H

#include "CGL/vector3D.h"

namespace CGL {

class HomogeneousMedium {
 public:
  HomogeneousMedium(const Vector3D& sigma_a, const Vector3D& sigma_s, double g)
      : sigma_a(sigma_a), sigma_s(sigma_s), g(g) {}

  Vector3D sigma_a;
  Vector3D sigma_s;
  double g;

  Vector3D sigma_t() const { return sigma_a + sigma_s; }

  double sigma_t_mean() const {
    Vector3D st = sigma_t();
    return (st.x + st.y + st.z) / 3.0;
  }

  Vector3D albedo() const {
    Vector3D st = sigma_t();
    return Vector3D(
        st.x > 0 ? sigma_s.x / st.x : 0.0,
        st.y > 0 ? sigma_s.y / st.y : 0.0,
        st.z > 0 ? sigma_s.z / st.z : 0.0);
  }

  // Beer-Lambert transmittance per-channel.
  Vector3D transmittance(double dist) const;

  // Sample a free-flight distance via the mean sigma_t channel.
  // pdf_out receives the scalar pdf used for weighting.
  double sample_distance(double xi, double* pdf_out) const;

  // Henyey-Greenstein phase sample. Falls back to isotropic when g == 0.
  void sample_phase(const Vector3D& wo, Vector3D* wi, double* pdf) const;
};

}  // namespace CGL

#endif  // CGL_MEDIUM_H

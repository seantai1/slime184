#include "bsdf.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "application/visual_debugger.h"
#include "pathtracer/spectral.h"
#include "util/random_util.h"

using std::max;
using std::min;
using std::swap;

namespace CGL {

namespace {

double sellmeier_ior(double lambda_nm, const Vector3D& b, const Vector3D& c,
                     double fallback_ior) {
  if (lambda_nm <= 0.0) return fallback_ior;

  double lambda_um = std::max(Spectral::kLambdaMin,
                              std::min(Spectral::kLambdaMax, lambda_nm)) *
                     1e-3;
  double lambda2 = lambda_um * lambda_um;
  double n2 = 1.0;

  for (int i = 0; i < 3; i++) {
    double denom = lambda2 - c[i];
    if (std::abs(denom) < 1e-8) continue;
    n2 += b[i] * lambda2 / denom;
  }

  return n2 > 1.0 ? std::sqrt(n2) : fallback_ior;
}

double cauchy_ior(double lambda_nm, double a, double b, double fallback_ior) {
  if (lambda_nm <= 0.0) return fallback_ior;
  double lambda_um = std::max(Spectral::kLambdaMin,
                              std::min(Spectral::kLambdaMax, lambda_nm)) *
                     1e-3;
  return std::max(1.0, a + b / (lambda_um * lambda_um));
}

}  // namespace

// Mirror BSDF //

Vector3D MirrorBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

Vector3D MirrorBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf,
                              double wavelength_nm) {
  reflect(wo, wi);
  *pdf = 1.0;
  return reflectance / abs_cos_theta(*wi);
}

void MirrorBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Mirror BSDF"))
  {
    DragDouble3("Reflectance", &reflectance[0], 0.005);
    ImGui::TreePop();
  }
}

// Microfacet BSDF //

double MicrofacetBSDF::G(const Vector3D wo, const Vector3D wi) {
  return 1.0 / (1.0 + Lambda(wi) + Lambda(wo));
}

double MicrofacetBSDF::D(const Vector3D h) {
  // TODO: proj3-2, part 3
  // Compute Beckmann normal distribution function (NDF) here.
  // You will need the roughness alpha.
  
  return 1.0;
}

Vector3D MicrofacetBSDF::F(const Vector3D wi) {
  // TODO: proj3-2, part 3
  // Compute Fresnel term for reflection on dielectric-conductor interface.
  // You will need both eta and etaK, both of which are Vector3D.

  double cosTheta = cos_theta(wi);
  
  return Vector3D();
}

Vector3D MicrofacetBSDF::f(const Vector3D wo, const Vector3D wi) {
  // TODO: proj3-2, part 3
  // Implement microfacet model here.

  return Vector3D();
}

Vector3D MicrofacetBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf,
                                  double wavelength_nm) {
  // TODO: proj3-2, part 3
  // *Importance* sample Beckmann normal distribution function (NDF) here.
  // Note: You should fill in the sampled direction *wi and the corresponding *pdf,
  //       and return the sampled BRDF value.



  *wi = cosineHemisphereSampler.get_sample(pdf);

  return MicrofacetBSDF::f(wo, *wi);
}

void MicrofacetBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Micofacet BSDF"))
  {
    DragDouble3("eta", &eta[0], 0.005);
    DragDouble3("K", &k[0], 0.005);
    DragDouble("alpha", &alpha, 0.005);
    ImGui::TreePop();
  }
}

// Refraction BSDF //

Vector3D RefractionBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

double RefractionBSDF::ior_at_wavelength(double wavelength_nm) const {
  if (use_sellmeier) {
    return sellmeier_ior(wavelength_nm, sellmeier_b, sellmeier_c, ior);
  }
  if (use_cauchy) {
    return cauchy_ior(wavelength_nm, cauchy_a, cauchy_b, ior);
  }
  return ior;
}

void RefractionBSDF::set_sellmeier_coefficients(const Vector3D& b,
                                                const Vector3D& c) {
  sellmeier_b = b;
  sellmeier_c = c;
  use_sellmeier = true;
  use_cauchy = false;
}

void RefractionBSDF::set_cauchy_coefficients(double a, double b) {
  cauchy_a = a;
  cauchy_b = b;
  use_cauchy = true;
  use_sellmeier = false;
}

Vector3D RefractionBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf,
                                  double wavelength_nm) {
  double eta_ior = ior_at_wavelength(wavelength_nm);
  if (!refract(wo, wi, eta_ior)) {
    *pdf = 0.0;
    return Vector3D();
  }
  *pdf = 1.0;
  double eta = (wo.z > 0) ? 1.0 / eta_ior : eta_ior;
  return transmittance / abs_cos_theta(*wi) / (eta * eta);
}

void RefractionBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Refraction BSDF"))
  {
    DragDouble3("Transmittance", &transmittance[0], 0.005);
    DragDouble("ior", &ior, 0.005);
    ImGui::TreePop();
  }
}

// Glass BSDF //

Vector3D GlassBSDF::f(const Vector3D wo, const Vector3D wi) {
  return Vector3D();
}

double GlassBSDF::ior_at_wavelength(double wavelength_nm) const {
  if (use_sellmeier) {
    return sellmeier_ior(wavelength_nm, sellmeier_b, sellmeier_c, ior);
  }
  if (use_cauchy) {
    return cauchy_ior(wavelength_nm, cauchy_a, cauchy_b, ior);
  }
  return ior;
}

void GlassBSDF::set_sellmeier_coefficients(const Vector3D& b,
                                           const Vector3D& c) {
  sellmeier_b = b;
  sellmeier_c = c;
  use_sellmeier = true;
  use_cauchy = false;
}

void GlassBSDF::set_cauchy_coefficients(double a, double b) {
  cauchy_a = a;
  cauchy_b = b;
  use_cauchy = true;
  use_sellmeier = false;
}

Vector3D GlassBSDF::sample_f(const Vector3D wo, Vector3D* wi, double* pdf,
                             double wavelength_nm) {
  double eta_ior = ior_at_wavelength(wavelength_nm);
  if (!refract(wo, wi, eta_ior)) {
    // total internal reflection: must reflect
    reflect(wo, wi);
    *pdf = 1.0;
    return reflectance / abs_cos_theta(*wi);
  }

  // Schlick's Fresnel approximation
  double R0 = (1.0 - eta_ior) / (1.0 + eta_ior);
  R0 = R0 * R0;
  double cos_i = abs_cos_theta(wo);
  double R = R0 + (1.0 - R0) * pow(1.0 - cos_i, 5.0);

  if (coin_flip(R)) {
    reflect(wo, wi);
    *pdf = R;
    return R * reflectance / abs_cos_theta(*wi);
  } else {
    // wi already set by refract() above
    *pdf = 1.0 - R;
    double eta = (wo.z > 0) ? 1.0 / eta_ior : eta_ior;
    return (1.0 - R) * transmittance / abs_cos_theta(*wi) / (eta * eta);
  }
}

void GlassBSDF::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Refraction BSDF"))
  {
    DragDouble3("Reflectance", &reflectance[0], 0.005);
    DragDouble3("Transmittance", &transmittance[0], 0.005);
    DragDouble("ior", &ior, 0.005);
    ImGui::TreePop();
  }
}

void BSDF::reflect(const Vector3D wo, Vector3D* wi) {
  *wi = Vector3D(-wo.x, -wo.y, wo.z);
}

bool BSDF::refract(const Vector3D wo, Vector3D* wi, double ior) {
  // eta = n_entering / n_exiting (from wo's side into the other side)
  double eta = (wo.z > 0) ? 1.0 / ior : ior;
  double cos2_t = 1.0 - eta * eta * (1.0 - wo.z * wo.z);
  if (cos2_t < 0.0) return false;
  double cos_t = sqrt(cos2_t);
  wi->x = -eta * wo.x;
  wi->y = -eta * wo.y;
  wi->z = (wo.z > 0) ? -cos_t : cos_t;
  return true;
}

} // namespace CGL

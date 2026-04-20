#include "light.h"

#include <cmath>
#include <iostream>

#include "pathtracer/sampler.h"

namespace CGL { namespace SceneObjects {

// Directional Light //

DirectionalLight::DirectionalLight(const Vector3D rad,
                                   const Vector3D lightDir)
    : radiance(rad) {
  dirToLight = -lightDir.unit();
}

Vector3D DirectionalLight::sample_L(const Vector3D p, Vector3D* wi,
                                    double* distToLight, double* pdf) const {
  *wi = dirToLight;
  *distToLight = INF_D;
  *pdf = 1.0;
  return radiance;
}

// Infinite Hemisphere Light //

InfiniteHemisphereLight::InfiniteHemisphereLight(const Vector3D rad)
    : radiance(rad) {
  sampleToWorld[0] = Vector3D(1,  0,  0);
  sampleToWorld[1] = Vector3D(0,  0, -1);
  sampleToWorld[2] = Vector3D(0,  1,  0);
}

Vector3D InfiniteHemisphereLight::sample_L(const Vector3D p, Vector3D* wi,
                                           double* distToLight,
                                           double* pdf) const {
  Vector3D dir = sampler.get_sample();
  *wi = sampleToWorld* dir;
  *distToLight = INF_D;
  *pdf = 1.0 / (2.0 * PI);
  return radiance;
}

double InfiniteHemisphereLight::pdf_L(const Vector3D p,
                                      const Vector3D wi) const {
  return wi.y >= 0.0 ? 1.0 / (2.0 * PI) : 0.0;
}

// Point Light //

PointLight::PointLight(const Vector3D rad, const Vector3D pos) : 
  radiance(rad), position(pos) { }

Vector3D PointLight::sample_L(const Vector3D p, Vector3D* wi,
                             double* distToLight,
                             double* pdf) const {
  Vector3D d = position - p;
  *wi = d.unit();
  *distToLight = d.norm();
  *pdf = 1.0;
  return radiance;
}


// Spot Light //

SpotLight::SpotLight(const Vector3D rad, const Vector3D pos,
                     const Vector3D dir, double angle) {

}

Vector3D SpotLight::sample_L(const Vector3D p, Vector3D* wi,
                             double* distToLight, double* pdf) const {
  return Vector3D();
}


// Area Light //

AreaLight::AreaLight(const Vector3D rad, 
                     const Vector3D pos,   const Vector3D dir, 
                     const Vector3D dim_x, const Vector3D dim_y)
  : radiance(rad), position(pos), direction(dir),
    dim_x(dim_x), dim_y(dim_y), area(dim_x.norm() * dim_y.norm()) { }

Vector3D AreaLight::sample_L(const Vector3D p, Vector3D* wi, 
                             double* distToLight, double* pdf) const {

  Vector2D sample = sampler.get_sample() - Vector2D(0.5f, 0.5f);
  Vector3D d = position + sample.x * dim_x + sample.y * dim_y - p;
  double sqDist = d.norm2();
  double dist = sqrt(sqDist);
  *wi = d / dist;
  double cosTheta = dot(-(*wi), direction.unit());
  *distToLight = dist;
  if (cosTheta <= 0.0) {
    *pdf = 0.0;
    return Vector3D();
  }
  *pdf = sqDist / (area * cosTheta);
  return radiance;
};

double AreaLight::pdf_L(const Vector3D p, const Vector3D wi) const {
  Vector3D n = direction.unit();
  double denom = dot(wi, n);
  if (std::abs(denom) < 1e-8) return 0.0;

  double t = dot(position - p, n) / denom;
  if (t <= 0.0) return 0.0;

  Vector3D hit = p + t * wi;
  Vector3D rel = hit - position;
  double u = dot(rel, dim_x) / dim_x.norm2();
  double v = dot(rel, dim_y) / dim_y.norm2();
  if (std::abs(u) > 0.5 || std::abs(v) > 0.5) return 0.0;

  double cosTheta = dot(-wi, n);
  if (cosTheta <= 0.0) return 0.0;

  double sqDist = (hit - p).norm2();
  return sqDist / (area * cosTheta);
}


// Sphere Light //

SphereLight::SphereLight(const Vector3D rad, const SphereObject* sphere) {

}

Vector3D SphereLight::sample_L(const Vector3D p, Vector3D* wi, 
                               double* distToLight, double* pdf) const {

  return Vector3D();
}

// Mesh Light

MeshLight::MeshLight(const Vector3D rad, const Mesh* mesh) {

}

Vector3D MeshLight::sample_L(const Vector3D p, Vector3D* wi, 
                             double* distToLight, double* pdf) const {
  return Vector3D();
}

} // namespace SceneObjects
} // namespace CGL

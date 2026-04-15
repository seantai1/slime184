#include "sphere.h"

#include <cmath>

#include "pathtracer/bsdf.h"
#include "util/sphere_drawing.h"

namespace CGL {
namespace SceneObjects {

bool Sphere::test(const Ray &r, double &t1, double &t2) const {

  // TODO (Part 1.4):
  // Implement ray - sphere intersection test.
  // Return true if there are intersections and writing the
  // smaller of the two intersection times in t1 and the larger in t2.

  double a = dot(r.d, r.d);
  double b = 2.0 * dot(r.o - o, r.d);
  double c = dot(r.o - o, r.o - o) - r2;
  double discriminant = b * b - 4.0 * a * c;
  if (discriminant < 0) return false;
  t1 = (-b - sqrt(discriminant)) / (2.0 * a);
  t2 = (-b + sqrt(discriminant)) / (2.0 * a);
  return true;
}

bool Sphere::has_intersection(const Ray &r) const {

  // TODO (Part 1.4):
  // Implement ray - sphere intersection.
  // Note that you might want to use the the Sphere::test helper here.

  double t1, t2;
  if (!test(r, t1, t2)) return false;

  // Pick closest t in range clipped
  if (t1 >= r.min_t && t1 <= r.max_t) { 
    r.max_t = t1; return true; 
  }
  if (t2 >= r.min_t && t2 <= r.max_t) { 
    r.max_t = t2; return true; 
  }
  return false;
}

bool Sphere::intersect(const Ray &r, Intersection *i) const {

  // TODO (Part 1.4):
  // Implement ray - sphere intersection.
  // Note again that you might want to use the the Sphere::test helper here.
  // When an intersection takes place, the Intersection data should be updated
  // correspondingly.

  double t1, t2;
  if (!test(r, t1, t2)) return false;

  double t;
  if (t1 >= r.min_t && t1 <= r.max_t) {
    t = t1;
  }
  else if (t2 >= r.min_t && t2 <= r.max_t) {
    t = t2;
  }
  else return false;

  r.max_t = t;
  Vector3D hit_point = r.o + t * r.d;
  i->t = t;
  i->n = normal(hit_point);
  i->primitive = this;
  i->bsdf = get_bsdf();
  return true;
}

void Sphere::draw(const Color &c, float alpha) const {
  Misc::draw_sphere_opengl(o, r, c);
}

void Sphere::drawOutline(const Color &c, float alpha) const {
  // Misc::draw_sphere_opengl(o, r, c);
}

} // namespace SceneObjects
} // namespace CGL

#include "bvh.h"

#include "CGL/CGL.h"
#include "triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CGL {
namespace SceneObjects {

BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
                   size_t max_leaf_size) {

  primitives = std::vector<Primitive *>(_primitives);
  root = construct_bvh(primitives.begin(), primitives.end(), max_leaf_size);
}

BVHAccel::~BVHAccel() {
  if (root)
    delete root;
  primitives.clear();
}

BBox BVHAccel::get_bbox() const { return root->bb; }

void BVHAccel::draw(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->draw(c, alpha);
    }
  } else {
    draw(node->l, c, alpha);
    draw(node->r, c, alpha);
  }
}

void BVHAccel::drawOutline(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->drawOutline(c, alpha);
    }
  } else {
    drawOutline(node->l, c, alpha);
    drawOutline(node->r, c, alpha);
  }
}

BVHNode *BVHAccel::construct_bvh(std::vector<Primitive *>::iterator start,
                                 std::vector<Primitive *>::iterator end,
                                 size_t max_leaf_size) {

  // TODO (Part 2.1):
  // Construct a BVH from the given vector of primitives and maximum leaf
  // size configuration. The starter code build a BVH aggregate with a
  // single leaf node (which is also the root) that encloses all the
  // primitives.


  BBox bbox;

  for (auto p = start; p != end; p++) {
    BBox bb = (*p)->get_bbox();
    bbox.expand(bb);
  }
  size_t count = end-start;
  BVHNode *node = new BVHNode(bbox);
  //leaf base case
  if (count <= max_leaf_size) {
    node->start = start;
    node->end = end;
    return node;
  }

  // split point for most benefit
  int axis = 0;
  if (bbox.extent.y >= bbox.extent.x && bbox.extent.y >= bbox.extent.z) {
    axis = 1;
  }
  else if (bbox.extent.z >= bbox.extent.x && bbox.extent.z >= bbox.extent.y) {
    axis = 2;
  }

  // avg centroid for left right collections
  double avg = 0;
  for (auto p = start; p != end; p++) {
    avg += (*p)->get_bbox().centroid()[axis];
  }
  avg /= count;

  // split left right
  auto mid = start;
  for (auto p = start; p != end; p++) {
    if ((*p)->get_bbox().centroid()[axis] < avg) {
      std::iter_swap(p, mid); //moves primitives with centroid < avg to the front
      mid++;
    }
  } // mid is an iterator to the split point

  // edge case, if all are on one side, need to just split in half
  if (mid == start || mid == end) {
    mid = start + count / 2;
  }

  node->l = construct_bvh(start, mid, max_leaf_size);
  node->r = construct_bvh(mid, end, max_leaf_size);
  return node;


}

bool BVHAccel::has_intersection(const Ray &ray, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.
  // Take note that this function has a short-circuit that the
  // Intersection version cannot, since it returns as soon as it finds
  // a hit, it doesn't actually have to find the closest hit.

  double t0 = ray.min_t;
  double t1 = ray.max_t;
  if (!node->bb.intersect(ray, t0, t1)) return false;
  //leaf
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      total_isects++;
      if ((*p)->has_intersection(ray)) {
        return true;
      }
    }
    return false;
  }

  bool hit1 = has_intersection(ray, node->l);
  bool hit2 = has_intersection(ray, node->r);
  return hit1 || hit2;
}

bool BVHAccel::intersect(const Ray &ray, Intersection *i, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.

  double t0 = ray.min_t;
  double t1 = ray.max_t;
  if (!node->bb.intersect(ray, t0, t1)) return false;
  //leaf
  if (node->isLeaf()) {
    bool hit = false;
    for (auto p = node->start; p != node->end; p++) {
      total_isects++;
      hit = (*p)->intersect(ray, i) || hit;
    }
    return hit;
  }
  bool hit1 = intersect(ray, i, node->l);
  bool hit2 = intersect(ray, i, node->r);
  return hit1 || hit2;

}

} // namespace SceneObjects
} // namespace CGL

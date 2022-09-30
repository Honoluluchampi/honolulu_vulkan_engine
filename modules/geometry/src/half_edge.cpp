#include <geometry/half_edge.hpp>

namespace hnll::geometry {

// vertex
void vertex::update_normal(const vec3& new_face_normal)
{
  auto tmp = normal_ * face_count_ + new_face_normal;
  face_count_++;
  normal_ = (tmp / face_count_).normalized();
}

half_edge_key calc_half_edge_key(const s_ptr<vertex>& v0, const s_ptr<vertex>& v1)
{
  half_edge_key id0 = v0->id_, id1 = v1->id_;
  if (id0 > id1) std::swap(id0, id1);
  return (id0 | (id1 << 32));
}

void mesh_model::associate_half_edge_pair(const s_ptr<half_edge> &he)
{
  auto hash_key = calc_half_edge_key(he->get_vertex(), he->get_next()->get_vertex());
  // check if those hash_key already have a value
  if (auto pair = half_edge_map_.find(hash_key); pair != half_edge_map_.end()) {
    // if the pair has added to the map, associate with it
    he->set_pair(half_edge_map_[hash_key]);
    half_edge_map_[hash_key]->set_pair(he);
  } else {
    // if the pair has not added to the map
    half_edge_map_[hash_key] = he;
  }
}

void mesh_model::add_face(s_ptr<vertex> &v0, s_ptr<vertex> &v1, s_ptr<vertex> &v2)
{
  // create new half_edge
  std::array<s_ptr<half_edge>, 3> hes;
  hes[0] = half_edge::create(v0);
  hes[1] = half_edge::create(v1);
  hes[2] = half_edge::create(v2);
  // half_edge circle
  hes[0]->set_next(hes[1]); hes[0]->set_prev(hes[2]);
  hes[1]->set_next(hes[2]); hes[0]->set_prev(hes[0]);
  hes[2]->set_next(hes[0]); hes[0]->set_prev(hes[1]);
  // register to the hash_table
  for (int i = 0; i < 3; i++) {
    associate_half_edge_pair(hes[i]);
  }
  // new face
  auto fc = face::create(hes[0]);
  // calc face normal
  fc->normal_ = ((v1->position_ - v0->position_).cross(v2->position_ - v0->position_)).normalized();
  // register to each owner
  faces_.push_back(fc);
  hes[0]->set_face(fc);
  hes[1]->set_face(fc);
  hes[2]->set_face(fc);
  // vertex normal
  v0->update_normal(fc->normal_);
  v1->update_normal(fc->normal_);
  v2->update_normal(fc->normal_);
}

} // namespace hnll::geometry
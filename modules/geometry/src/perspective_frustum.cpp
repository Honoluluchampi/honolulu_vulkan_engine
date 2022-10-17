// hnll
#include <geometry/perspective_frustum.hpp>
#include <utils/utils.hpp>

namespace hnll::geometry {

s_ptr<perspective_frustum> perspective_frustum::create(double fov_x, double fov_y, double near_z, double far_z)
{
 auto frustum = std::make_shared<perspective_frustum>(fov_x, fov_y, near_z, far_z);
 return frustum;
}

perspective_frustum::perspective_frustum(double fov_x, double fov_y, double near_z, double far_z)
{
  fov_x_  = fov_x;
  fov_y_  = fov_y;
  near_z_ = near_z;
  far_z_  = far_z;

  // init planes
  near_   = std::make_unique<plane>(vec3(0.f, 0.f, near_z), vec3(0.f, 0.f, 1.f));
  far_    = std::make_unique<plane>(vec3(0.f, 0.f, far_z),  vec3(0.f, 0.f, -1.f));
  left_   = std::make_unique<plane>(vec3(0.f, 0.f, 0.f), vec3( std::cos(fov_x_ / 2.f), 0.f, std::sin(fov_x_ / 2.f)));
  right_  = std::make_unique<plane>(vec3(0.f, 0.f, 0.f), vec3(-std::cos(fov_x_ / 2.f), 0.f, std::sin(fov_x_ / 2.f)));
  top_    = std::make_unique<plane>(vec3(0.f, 0.f, 0.f), vec3(0.f,  std::cos(fov_y_ / 2.f), std::sin(fov_y_ / 2.f)));
  bottom_ = std::make_unique<plane>(vec3(0.f, 0.f, 0.f), vec3(0.f, -std::cos(fov_y_ / 2.f), std::sin(fov_y_ / 2.f)));

  // compute default frustum points
  auto vertical   = fov_y / 2.f;
  auto horizontal = fov_x / 2.f;
  auto z_unit     = near_->normal;
  default_points_[0] = z_unit + vertical * vec3(0, -1, 0) + horizontal * vec3(-1, 0, 0);
  default_points_[1] = z_unit + vertical * vec3(0, -1, 0) + horizontal * vec3( 1, 0, 0);
  default_points_[2] = z_unit + vertical * vec3(0,  1, 0) + horizontal * vec3( 1, 0, 0);
  default_points_[3] = z_unit + vertical * vec3(0,  1, 0) + horizontal * vec3(-1, 0, 0);
}

void perspective_frustum::update_planes(const utils::transform &tf)
{
  const auto& translation = tf.get_translation_ref();
  const auto  rotate_mat  = tf.rotate_mat3();

  // TODO : eigenize and delete
  vec3 tl = { translation.x, translation.y, translation.z };
  // update
  near_->point   = rotate_mat * vec3(0.f, 0.f, near_z_) + tl;
  far_->point    = rotate_mat * vec3(0.f, 0.f, far_z_)  + tl;
  left_->point   = tl;
  right_->point  = tl;
  top_->point    = tl;
  bottom_->point = tl;

  // default normal * rotate_mat
  // TODO : use symmetry
  near_->normal   = rotate_mat * vec3(0.f, 0.f, 1.f);
  far_->normal    = rotate_mat * vec3(0.f, 0.f, -1.f);
  left_->normal   = rotate_mat * vec3(std::cos(fov_x_ / 2.f), 0.f, std::sin(fov_x_ / 2.f));
  right_->normal  = rotate_mat * vec3(-std::cos(fov_x_ / 2.f), 0.f, std::sin(fov_x_ / 2.f));
  top_->normal    = rotate_mat * vec3(0.f, std::cos(fov_y_ / 2.f), std::sin(fov_y_ / 2.f));
  bottom_->normal = rotate_mat * vec3(0.f, -std::cos(fov_y_ / 2.f), std::sin(fov_y_ / 2.f));
}

} // namespace hnll::geometry
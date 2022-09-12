// hnll
#include <game/engine.hpp>
#include <game/components/mesh_component.hpp>
#include <physics/bounding_volumes/bounding_sphere.hpp>
#include <game/actors/default_camera.hpp>

class rigid_ball : public hnll::game::actor
{
  public:
    rigid_ball(const Eigen::Vector3d& center_point, double radius) : hnll::game::actor(){}

    static s_ptr<rigid_ball> create(const Eigen::Vector3d& center_point, double radius)
    {
      // create ball actor and its mesh
      auto ball = std::make_shared<rigid_ball>(center_point, radius);
      auto ball_mesh = hnll::game::engine::get_mesh_model_sp("sphere");
      auto ball_mesh_vertex_position_list = ball_mesh->get_vertex_position_list();
      auto ball_mesh_comp = hnll::game::mesh_component::create(ball, std::move(ball_mesh));

      // create bounding_sphere
      ball->bounding_sphere_ = hnll::physics::bounding_sphere::create_bounding_sphere
          (hnll::physics::ctor_type::RITTER, ball_mesh_vertex_position_list);

      ball->set_translation(glm::vec3{center_point.x(), center_point.y(), center_point.z()});
      ball->velocity_ = {0.f, 0.f, 0.f};
      // register the ball to the engine
      hnll::game::engine::add_actor(ball);
      return ball;
    };

    void update_actor(float dt) override
    {
      position_ += velocity_ * dt;
      velocity_.y += gravity_ * dt;
      // bound
      if (position_.y > 0.f) {
        position_.y = -position_.y;
        velocity_.y = -velocity_.y * restitution_;
      }
      this->set_translation(position_);
    }

  private:
    hnll::physics::bounding_sphere bounding_sphere_;
    glm::vec3 position_;
    glm::vec3 velocity_;
    double gravity_ = 20.f;
    double restitution_ = 1;
};

class falling_ball_app : public hnll::game::engine
{
  public:
    falling_ball_app() : hnll::game::engine("falling ball")
    {
      camera_up_->set_translation(glm::vec3{0.f, 0.f, -20.f});
      auto ball = rigid_ball::create({0.f, -20.f, 0.f}, 1.f);
    }
};

int main()
{
  falling_ball_app app{};
  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
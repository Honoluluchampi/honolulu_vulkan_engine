// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>
#include <game/actors/point_light_manager.hpp>
#include <game/actors/default_camera.hpp>
#include <game/components/mesh_component.hpp>
#include <physics/collision_info.hpp>
#include <physics/collision_detector.hpp>
#include <physics/engine.hpp>
#include <graphics/meshlet_model.hpp>

// lib
#include <imgui.h>

// std
#include <filesystem>
#include <iostream>
#include <typeinfo>

namespace hnll::game {

constexpr float MAX_FPS = 60.0f;
constexpr float MAX_DT = 0.05f;

// static members
actor_map             engine::active_actor_map_{};
actor_map             engine::pending_actor_map_{};
std::vector<actor_id> engine::dead_actor_ids_{};
mesh_model_map        engine::mesh_model_map_;
meshlet_model_map     engine::meshlet_model_map_;
shading_system_map    engine::shading_system_map_;


// glfw
GLFWwindow* engine::glfw_window_;
std::vector<u_ptr<std::function<void(GLFWwindow*, int, int, int)>>> engine::glfw_mouse_button_callbacks_{};

engine::engine(const char* window_name) : graphics_engine_(std::make_unique<hnll::graphics::engine>(window_name))
{
  set_glfw_window(); // ?

#ifndef IMGUI_DISABLED
  gui_engine_ = std::make_unique<hnll::gui::engine>
    (graphics_engine_->get_window(), graphics_engine_->get_device());
  // configure dependency between renderers
  graphics_engine_->get_renderer().set_next_renderer(gui_engine_->renderer_p());
#endif

  init_actors();
  load_data();

  // glfw
  set_glfw_mouse_button_callbacks();
}

void engine::run()
{
  current_time_ = std::chrono::system_clock::now();
  while (!glfwWindowShouldClose(glfw_window_))
  {
    glfwPollEvents();
    process_input();
    update();
    // TODO : implement as physics engine
    re_update_actors();

    render();
  }
  graphics_engine_->wait_idle();
  cleanup();
}

void engine::process_input()
{
}

void engine::update()
{
  is_updating_ = true;

  float dt;
  std::chrono::system_clock::time_point new_time;
  // calc dt
//  do {
  new_time = std::chrono::system_clock::now();

  dt = std::chrono::duration<float, std::chrono::seconds::period>(new_time - current_time_).count();
//  } while(dt < 1.0f / MAX_FPS);

  dt = std::min(dt, MAX_DT);

  for (auto& kv : active_actor_map_) {
    const auto id = kv.first;
    auto& actor = kv.second;
    if(actor->get_actor_state() == actor::state::ACTIVE)
      actor->update(dt);
    // check if the actor is dead
    if (actor->get_actor_state() == actor::state::DEAD) {
      dead_actor_ids_.emplace_back(id);
    }
  }

  // engine specific update
  update_game(dt);

  // camera
  camera_up_->update(dt);
  light_manager_up_->update(dt);

  current_time_ = new_time;
  is_updating_ = false;

  // activate pending actor
  for (auto& pend : pending_actor_map_) {
    if(pend.second->is_renderable())
      graphics_engine_->set_renderable_component(pend.second->get_renderable_component_sp());
    active_actor_map_.emplace(pend.first, std::move(pend.second));
  }
  pending_actor_map_.clear();
  // clear all the dead actors
  for (const auto& id : dead_actor_ids_) {
    if (active_actor_map_[id]->is_renderable())
      graphics_engine_->remove_renderable_component_without_owner(
        active_actor_map_[id]->get_renderable_component_sp()->get_render_type(), id);
    active_actor_map_.erase(id);
  }
  dead_actor_ids_.clear();
}


// physics
void engine::re_update_actors()
{
  physics_engine_->re_update();
}

void engine::render()
{
  viewer_info_  = camera_up_->get_viewer_info();
  graphics_engine_->render(viewer_info_, frustum_info_);

  auto& renderer = graphics_engine_->get_renderer();
  // execute shading_systems
  {
    if (auto command_buffer = renderer.begin_frame()) {
      int frame_index = renderer.get_frame_index();

      utils::frame_info frame_info {
        frame_index,
        command_buffer,
        graphics_engine_->get_global_descriptor_set(frame_index),
      };

      // update ubo
      auto& ubo = graphics_engine_->get_global_ubo();
      ubo.projection   = viewer_info_.projection;
      ubo.view         = viewer_info_.view;
      ubo.inverse_view = viewer_info_.inverse_view;
      graphics_engine_->update_ubo(frame_index);

      // rendering
      renderer.begin_swap_chain_render_pass(command_buffer, HVE_RENDER_PASS_ID);

      for (auto& system : shading_system_map_) {
        system.second->render(frame_info);
      }

      renderer.end_swap_chain_render_pass(command_buffer);
      renderer.end_frame();
    }
  }

  #ifndef IMGUI_DISABLED
  if (!hnll::graphics::renderer::swap_chain_recreated_){
    gui_engine_->begin_imgui();
    update_gui();
    gui_engine_->render();
  }
  #endif
}

#ifndef IMGUI_DISABLED
void engine::update_gui()
{
  // some general imgui upgrade
  update_game_gui();
  for (auto& kv : active_actor_map_) {
  const id_t& id = kv.first;
  auto& actor = kv.second;
  actor->update_gui();
  }
}
#endif

void engine::init_actors()
{
  // hge actors
  camera_up_ = std::make_shared<default_camera>(*graphics_engine_);
  
  // TODO : configure priorities of actors, then update light manager after all light comp
  light_manager_up_ = std::make_shared<point_light_manager>(graphics_engine_->get_global_ubo());

}

void engine::load_data()
{
  // load raw mesh data
  load_mesh_models();
  load_meshlet_models();
  // load_actor();
}

// use filenames as the key of the map
// TODO : add models by adding folders or files
void engine::load_mesh_models()
{
  for (const auto& path : utils::loading_directories) {
    for (const auto& file : std::filesystem::directory_iterator(path)) {
      auto filename = std::string(file.path());
      auto length = filename.size() - path.size();
      auto key = filename.substr(path.size() + 1, length);
      auto mesh_model = hnll::graphics::mesh_model::create_from_file(get_graphics_device(), key);
      mesh_model_map_.emplace(key, std::move(mesh_model));
    }
  }
}

void engine::load_meshlet_models()
{
  for (const auto& path : utils::loading_directories) {
    for (const auto& file : std::filesystem::directory_iterator(path)) {
      auto filename = std::string(file.path());
      auto length = filename.size() - path.size();
      auto key = filename.substr(path.size() + 1, length);
      auto meshlet_model = hnll::graphics::meshlet_model::create_from_file(get_graphics_device(), key);
      std::cout << key << " :" << std::endl << "\tmeshlet count : " << meshlet_model->get_meshlets_count() << std::endl;
      meshlet_model_map_.emplace(key, std::move(meshlet_model));
    }
  }
}

// actors should be created as shared_ptr
void engine::add_actor(const s_ptr<actor>& actor)
{ pending_actor_map_.emplace(actor->get_id(), actor); }

void engine::add_shading_system(u_ptr<hnll::game::shading_system> &&system)
{ shading_system_map_[static_cast<uint32_t>(system->get_rendering_type())] = std::move(system); }

void engine::remove_actor(id_t id)
{
  pending_actor_map_.erase(id);
  active_actor_map_.erase(id);
}

void engine::load_actor()
{
  auto smooth_vase = actor::create();
  auto smooth_vase_model_comp = mesh_component::create(smooth_vase, "smooth_sphere.obj");
  smooth_vase->set_translation(glm::vec3{0.f, 0.f, 3.f});

  auto flat_vase = actor::create();
  auto flat_vase_model_comp = mesh_component::create(flat_vase, "light_bunny.obj");
  flat_vase->set_translation(glm::vec3{0.5f, 0.5f, 0.f});
  flat_vase->set_scale(glm::vec3{3.f, 1.5f, 3.f});
  
  auto floor = actor::create();
  auto floor_model_comp = mesh_component::create(floor, "plane.obj");
  floor->set_translation(glm::vec3{0.f, 0.5f, 0.f});
  floor->set_scale(glm::vec3{3.f, 1.5f, 3.f});

  float light_intensity = 4.f;
  std::vector<glm::vec3> positions;
  float position_radius = 4.f;
  for (int i = 0; i < 6; i++) {
    positions.push_back({position_radius * std::sin(M_PI/3.f * i), -2.f, position_radius * std::cos(M_PI/3.f * i)});
  }
  positions.push_back({0.f, position_radius, 0.f});
  positions.push_back({0.f, -position_radius, 0.f});

  for (const auto& position : positions) {
    auto light = hnll::game::actor::create();
    auto light_component = hnll::game::point_light_component::create(light, light_intensity, 0.f);
    light_component->set_color(glm::vec3(0.f, 1.f, 0.3f));
    light_component->set_radius(0.5f);
    add_point_light(light, light_component);
    light->set_translation(position);
  }
}

void engine::add_point_light(s_ptr<actor>& owner, s_ptr<point_light_component>& light_comp)
{
  // shared by three actor 
  owner->set_renderable_component(light_comp);
  light_manager_up_->add_light_comp(light_comp);
} 

void engine::add_point_light_without_owner(const s_ptr<point_light_component>& light_comp)
{
  // path to the renderer
  graphics_engine_->set_renderable_component(light_comp);
  // path to the manager
  light_manager_up_->add_light_comp(light_comp);
}

void engine::remove_point_light_without_owner(component_id id)
{
  graphics_engine_->remove_renderable_component_without_owner(render_type::POINT_LIGHT, id);
  light_manager_up_->remove_light_comp(id);
}


void engine::cleanup()
{
  active_actor_map_.clear();
  pending_actor_map_.clear();
  dead_actor_ids_.clear();
  mesh_model_map_.clear();
  meshlet_model_map_.clear();
  hnll::graphics::renderer::cleanup_swap_chain();
}

// glfw
void engine::set_glfw_mouse_button_callbacks()
{
  glfwSetMouseButtonCallback(glfw_window_, glfw_mouse_button_callback);
}

void engine::add_glfw_mouse_button_callback(u_ptr<std::function<void(GLFWwindow* window, int button, int action, int mods)>>&& func)
{
  glfw_mouse_button_callbacks_.emplace_back(std::move(func));
  set_glfw_mouse_button_callbacks();
}

void engine::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  for (const auto& func : glfw_mouse_button_callbacks_)
    func->operator()(window, button, action, mods);
  
#ifndef IMGUI_DISABLED
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

void engine::set_frustum_info(utils::frustum_info &&_frustum_info)
{
  frustum_info_ = std::move(_frustum_info);
}

graphics::meshlet_model& engine::get_meshlet_model(std::string model_name)
{ return *meshlet_model_map_[model_name]; }

//actor& engine::get_active_actor(actor_id id)
//{ return *active_actor_map_[id]; }


} // namespace hnll::game

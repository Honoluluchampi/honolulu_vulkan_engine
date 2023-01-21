// hnll
#include <game/shading_systems/meshlet_shading_system.hpp>
#include <game/components/meshlet_component.hpp>
#include <utils/common_using.hpp>
#include <graphics/meshlet_model.hpp>
#include <graphics/swap_chain.hpp>


namespace hnll::game {

struct meshlet_push_constant
{
  mat4 model_matrix  = mat4::Identity();
  mat4 normal_matrix = mat4::Identity();
};

u_ptr<meshlet_shading_system> meshlet_shading_system::create(graphics::device &device)
{ return std::make_unique<meshlet_shading_system>(device); }

void meshlet_shading_system::setup_task_desc()
{
  task_desc_sets_ = graphics::descriptor_set::create(device_);

  // pool
  task_desc_sets_->create_pool(
    graphics::swap_chain::MAX_FRAMES_IN_FLIGHT,
    graphics::swap_chain::MAX_FRAMES_IN_FLIGHT,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
  );

  // layout
  task_desc_sets_->add_layout(VK_SHADER_STAGE_TASK_BIT_NV);

  // buffer
  for (int i = 0; i < graphics::swap_chain::MAX_FRAMES_IN_FLIGHT; i++) {
    // buffer has no actual data at this point
    auto new_buffer = graphics::buffer::create(
      device_,
      sizeof(utils::frustum_info),
      1,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      nullptr
    );
    task_desc_sets_->add_buffer(std::move(new_buffer));
  }

  task_desc_sets_->build_sets();
}

meshlet_shading_system::meshlet_shading_system(graphics::device &device)
  : shading_system(device, utils::shading_type::MESHLET)
{
  setup_task_desc();

  // prepare desc set layouts
  std::vector<VkDescriptorSetLayout> desc_set_layouts;
  // global
  desc_set_layouts.emplace_back(get_global_desc_set_layout());
  // task desc
  desc_set_layouts.emplace_back(task_desc_sets_->get_layout());
  // meshlet
  auto mesh_descs = graphics::meshlet_model::default_desc_set_layouts(device_);
  for (auto&& layout : mesh_descs) {
    desc_set_layouts.emplace_back(std::move(layout->get_descriptor_set_layout()));
  }

  pipeline_layout_ = create_pipeline_layout<meshlet_push_constant>(
    static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT),
    desc_set_layouts
  );

  auto pipeline_config_info = graphics::pipeline::default_pipeline_config_info();
  pipeline_config_info.binding_descriptions.clear();
  pipeline_config_info.attribute_descriptions.clear();

  pipeline_ = create_pipeline(
    pipeline_layout_,
    shading_system::get_default_render_pass(),
    "/modules/graphics/shader/spv/",
    { "simple_meshlet.task.glsl.spv", "simple_meshlet.mesh.glsl.spv", "simple_meshlet.frag.glsl.spv" },
    { VK_SHADER_STAGE_TASK_BIT_NV, VK_SHADER_STAGE_MESH_BIT_NV, VK_SHADER_STAGE_FRAGMENT_BIT },
    pipeline_config_info
  );
}

void meshlet_shading_system::render(const utils::frame_info &frame_info)
{
  auto command_buffer = frame_info.command_buffer;
  pipeline_->bind(command_buffer);

  for (auto& target : render_target_map_) {
    auto obj = dynamic_cast<meshlet_component *>(&target.second);

    meshlet_push_constant push{};
    push.model_matrix  = obj->get_transform().mat4().cast<float>();
    push.normal_matrix = obj->get_transform().normal_matrix().cast<float>();

    // task desc set update

    vkCmdPushConstants(
      command_buffer,
      pipeline_layout_,
      VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(meshlet_push_constant),
      &push
    );

    // update task desc set
    size_t index = frame_info.frame_index;
    task_desc_sets_->write_to_buffer(index, (void *) &frame_info.view_frustum);
    task_desc_sets_->flush_buffer(index);

    obj->bind_and_draw(
      command_buffer,
      { frame_info.global_descriptor_set, task_desc_sets_->get_set(frame_info.frame_index) },
      pipeline_layout_
    );
  }
}

} // namespace hnll::game
// hnll
#include <graphics/pipeline.hpp>
#include <graphics/mesh_model.hpp>

// std
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace hnll::graphics {

// what kind of geometry will be drawn from the vertices (topoloby) and
// if primitive restart should be enabled
void pipeline_config_info::create_input_assembly_info()
{
  input_assembly_info.sType = 
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
}

void pipeline_config_info::create_viewport_info()
{

  viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // by enabling a GPU feature in logical device creation,
  // its possible to use multiple viewports
  viewport_info.viewportCount = 1;
  viewport_info.pViewports = nullptr;
  viewport_info.scissorCount = 1;
  viewport_info.pScissors = nullptr;
}

void pipeline_config_info::create_rasterization_info()
{
  rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // using this requires enabling a GPU feature
  rasterization_info.depthClampEnable = VK_FALSE;
  // if rasterizationInfo_mDiscardEnable is set to VK_TRUE, then geometry never passes
  // through the rasterization_info stage, basically disables any output to the frame_buffer
  rasterization_info.rasterizerDiscardEnable = VK_FALSE;
  // how fragments are generated for geometry
  // using any mode other than fill requires GPU feature
  rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_info.lineWidth = 1.0f;
  // rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_info.cullMode = VK_CULL_MODE_NONE;
  rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  // consider this when shadow mapping is necessary
  rasterization_info.depthBiasEnable = VK_FALSE;
  rasterization_info.depthBiasConstantFactor = 0.0f;
  rasterization_info.depthBiasClamp = 0.0f;
  rasterization_info.depthBiasSlopeFactor = 0.0f;
}

// used for anti-aliasing
void pipeline_config_info::create_multi_sample_state()
{
  multi_sample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multi_sample_info.sampleShadingEnable = VK_FALSE;
  multi_sample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multi_sample_info.minSampleShading = 1.0f;
  multi_sample_info.pSampleMask = nullptr;
  multi_sample_info.alphaToCoverageEnable = VK_FALSE;
  multi_sample_info.alphaToOneEnable = VK_FALSE;
}

// color blending for alpha blending
void pipeline_config_info::create_color_blend_attachment()
{
  // per framebuffer struct
  // in contrast, VkPipelineColorBlendStateCreateInfo is global color blending settings
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; 
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional 
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
  color_blend_attachment.dstColorBlendFactor =VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; 
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; 
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void pipeline_config_info::create_color_blend_state()
{
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE; 
  color_blend_info.logicOp = VK_LOGIC_OP_COPY; // Optional 
  color_blend_info.attachmentCount = 1; 
  color_blend_info.pAttachments = &color_blend_attachment; 
  color_blend_info.blendConstants[0] = 0.0f; // Optional 
  color_blend_info.blendConstants[1] = 0.0f; // Optional
  color_blend_info.blendConstants[2] = 0.0f; // Optional 
  color_blend_info.blendConstants[3] = 0.0f; // Optional
}

void pipeline_config_info::create_depth_stencil_state()
{
  depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_info.depthTestEnable = VK_TRUE;
  depth_stencil_info.depthWriteEnable = VK_TRUE;
  depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
  depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_info.minDepthBounds = 0.0f;  // Optional
  depth_stencil_info.maxDepthBounds = 1.0f;  // Optional
  depth_stencil_info.stencilTestEnable = VK_FALSE;
  depth_stencil_info.front = {};  // Optional
  depth_stencil_info.back = {};   // Optional
}

// a limited amount of the state can be actually be changed without recreating the pipeline
void pipeline_config_info::create_dynamic_state()
{
  dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size()); 
  dynamic_state_info.pDynamicStates = dynamic_state_enables.data();
  dynamic_state_info.flags = 0;
}


pipeline::pipeline(
  device &device,
  const std::string &vertex_file_path,
  const std::string &fragment_file_path,
  const pipeline_config_info &config_info) : device_(device)
{ create_graphics_pipeline(vertex_file_path, fragment_file_path, config_info); }

pipeline::~pipeline()
{
  vkDestroyShaderModule(device_.get_device(), vertex_shader_module_, nullptr);
  vkDestroyShaderModule(device_.get_device(), fragment_shader_module_, nullptr);
  vkDestroyPipeline(device_.get_device(), graphics_pipeline_, nullptr);
}

std::vector<char> pipeline::read_file(const std::string& filepath)
{
  // construct and open
  // immidiately read as binary
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);

  if (!file.is_open())
    throw std::runtime_error("failed to open file: " + filepath);

  size_t file_size = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();

  return buffer;
}

void pipeline::create_graphics_pipeline(
    const std::string &vertex_file_path, 
    const std::string &fragment_file_path, 
    const pipeline_config_info &config_info)
{
  auto vertex_code = read_file(vertex_file_path);
  auto fragment_code = read_file(fragment_file_path);

  create_shader_module(vertex_code, &vertex_shader_module_);
  create_shader_module(fragment_code, &fragment_shader_module_);

  VkPipelineShaderStageCreateInfo shader_stages[2] =
    { create_vertex_shader_stage_info(), create_fragment_shader_stage_info() };

  auto vertex_input_info = create_vertex_input_info();

  // accept vertex data
  auto& binding_descriptions = config_info.binding_descriptions;
  auto& attribute_descriptions = config_info.attribute_descriptions; 
  vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
  vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data(); //optional
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); //optional

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  // programable stage count (in this case vertex and shader stage)
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &config_info.input_assembly_info;
  pipeline_info.pViewportState = &config_info.viewport_info;
  pipeline_info.pRasterizationState = &config_info.rasterization_info;
  pipeline_info.pMultisampleState = &config_info.multi_sample_info;
  pipeline_info.pColorBlendState = &config_info.color_blend_info;
  pipeline_info.pDepthStencilState = &config_info.depth_stencil_info;
  pipeline_info.pDynamicState = &config_info.dynamic_state_info;

  pipeline_info.layout = config_info.pipeline_layout;
  pipeline_info.renderPass = config_info.render_pass;
  pipeline_info.subpass = config_info.subpass;

  pipeline_info.basePipelineIndex = -1;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  // its possible to create multiple VkPipeline objects in a single call
  // second parameter means cache objects enables significantly faster creation
  if (vkCreateGraphicsPipelines(device_.get_device(), VK_NULL_HANDLE, 1,
    &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline!");


}

void pipeline::create_shader_module(const std::vector<char>& code, VkShaderModule* shader_module)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  // char to uint32_t
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

  if (vkCreateShaderModule(device_.get_device(), &create_info, nullptr, shader_module) != VK_SUCCESS)
    throw std::runtime_error("failed to create shader module!");
} 


VkPipelineShaderStageCreateInfo pipeline::create_vertex_shader_stage_info()
{
  VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
  vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_shader_stage_info.module = vertex_shader_module_;
  // the function to invoke
  vertex_shader_stage_info.pName = "main";
  return vertex_shader_stage_info;
}

VkPipelineShaderStageCreateInfo pipeline::create_fragment_shader_stage_info()
{
  VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
  fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_shader_stage_info.module = fragment_shader_module_;
  // the function to invoke
  fragment_shader_stage_info.pName = "main";
  return fragment_shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo pipeline::create_vertex_input_info()
{
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // configrate in create_graphics_pipeline()
    // vertex_input_info.vertexAttributeDescriptionCount = 0;
    // vertex_input_info.vertexBindingDescriptionCount = 0;
    // vertex_input_info.pVertexAttributeDescriptions = nullptr;
    // vertex_input_info.pVertexBindingDescriptions = nullptr;

    return vertex_input_info;
}

void pipeline::bind(VkCommandBuffer command_buffer)
{
  // basic drawing commands
  // bind the graphics pipeline
  // the second parameter specifies if the pipeline object is a graphics or compute pipeline or ray tracer
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
}

void pipeline::default_pipeline_config_info(pipeline_config_info &config_info)
{
  config_info.create_input_assembly_info();
  config_info.create_viewport_info();
  config_info.create_rasterization_info();
  config_info.create_multi_sample_state();
  config_info.create_color_blend_attachment();
  config_info.create_color_blend_state();
  config_info.create_depth_stencil_state();
  config_info.create_dynamic_state();

  config_info.binding_descriptions = mesh_model::vertex::get_binding_descriptions();
  config_info.attribute_descriptions = mesh_model::vertex::get_attribute_descriptions();
}

void pipeline::enable_alpha_blending(pipeline_config_info &config_info)
{
  config_info.color_blend_attachment.blendEnable = VK_TRUE;

  config_info.color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
              VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  config_info.color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  config_info.color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  config_info.color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  config_info.color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
  config_info.color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
  config_info.color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}
} // namespace hnll::graphics
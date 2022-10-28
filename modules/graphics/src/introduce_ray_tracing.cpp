// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>

// sub
#include <extensions_vk.hpp>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {

using vec3 = Eigen::Vector3f;
template<typename T> using u_ptr = std::unique_ptr<T>;
template<typename T> using s_ptr = std::shared_ptr<T>;

struct acceleration_structure
{
  VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
  VkDeviceMemory             memory = VK_NULL_HANDLE;
  VkBuffer                   buffer = VK_NULL_HANDLE;
  VkDeviceAddress            device_address = 0;
};

struct ray_tracing_scratch_buffer
{
  VkBuffer        handle = VK_NULL_HANDLE;
  VkDeviceMemory  memory = VK_NULL_HANDLE;
  VkDeviceAddress device_address = 0;
};

VkDeviceAddress get_device_address(VkDevice device, VkBuffer buffer)
{
  VkBufferDeviceAddressInfo buffer_device_info {
    VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    nullptr
  };
  buffer_device_info.buffer = buffer;
  return vkGetBufferDeviceAddress(device, &buffer_device_info);
}

class hello_triangle {
  public:
    hello_triangle()
    {
      window_ = std::make_unique<graphics::window>(1920, 1080, "hello ray tracing triangle");
      device_ = std::make_unique<graphics::device>(*window_, graphics::rendering_type::RAY_TRACING);

      // load all available extensions (of course including ray tracing extensions)
      load_VK_EXTENSIONS(device_->get_instance(), vkGetInstanceProcAddr, device_->get_device(), vkGetDeviceProcAddr);

      create_triangle_as();
    }

    ~hello_triangle()
    {
      destroy_acceleration_structure(*blas_);
      destroy_acceleration_structure(*tlas_);
    }

  private:
    void create_triangle_as()
    {
      create_vertex_buffer();
      create_triangle_blas();
      create_scene_tlas();
    }

    void create_vertex_buffer()
    {
      uint32_t vertex_count = triangle_vertices_.size();
      uint32_t vertex_size = sizeof(triangle_vertices_[0]);
      VkDeviceSize buffer_size = vertex_size * vertex_count;

      // create staging buffer
      graphics::buffer staging_buffer{
          *device_,
          vertex_size,
          vertex_count,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      };
      staging_buffer.map();
      staging_buffer.write_to_buffer((void *) triangle_vertices_.data());

      // setup vertex buffer create info
      VkBufferUsageFlags usage =
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

      // create vertex buffer
      vertex_buffer_ = std::make_unique<graphics::buffer>(
          *device_,
          vertex_size,
          vertex_count,
          usage,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      );
      // write data to the buffer
      device_->copy_buffer(staging_buffer.get_buffer(), vertex_buffer_->get_buffer(), buffer_size);
    }

    void create_triangle_blas()
    {
      // blas build setup

      // get vertex buffer device address
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address {};
      vertex_buffer_device_address.deviceAddress =
          get_device_address(device_->get_device(), vertex_buffer_->get_buffer());

      // geometry
      VkAccelerationStructureGeometryKHR as_geometry {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
      };
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      as_geometry.geometry.triangles.vertexData = vertex_buffer_device_address;
      as_geometry.geometry.triangles.maxVertex = triangle_vertices_.size();
      as_geometry.geometry.triangles.vertexStride = sizeof(vec3);
      as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

      // build geometry info
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      // prefer performance rather than as build
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      // get as size
      uint32_t num_triangles = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(
        device_->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &as_build_geometry_info,
        &num_triangles,
        &as_build_sizes_info
      );

      // build blas (get handle of VkAccelerationStructureKHR)
      blas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create blas
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = blas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device_->get_device(), &as_create_info, nullptr, &blas_->handle);

      // get the device address of blas
      VkAccelerationStructureDeviceAddressInfoKHR as_device_address_info {};
      as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      as_device_address_info.accelerationStructure = blas_->handle;
      blas_->device_address = vkGetAccelerationStructureDeviceAddressKHR(device_->get_device(), &as_device_address_info);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // build blas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = blas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute blas build command (in GPU)
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = num_triangles;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;

      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands();
      vkCmdBuildAccelerationStructuresKHR(
        command, 1, &as_build_geometry_info, as_build_range_infos.data()
      );

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = blas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    u_ptr<acceleration_structure> create_acceleration_structure_buffer
        (VkAccelerationStructureBuildSizesInfoKHR build_size_info)
    {
      auto as = std::make_unique<acceleration_structure>();
      auto usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        build_size_info.accelerationStructureSize,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        as->buffer,
        as->memory
      );

      return as;
    }

    u_ptr<ray_tracing_scratch_buffer> create_scratch_buffer(VkDeviceSize size)
    {
      auto scratch_buffer = std::make_unique<ray_tracing_scratch_buffer>();
      auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratch_buffer->handle,
        scratch_buffer->memory
      );

      scratch_buffer->device_address = get_device_address(device_->get_device(), scratch_buffer->handle);
      return scratch_buffer;
    }

    void create_scene_tlas()
    {
      VkTransformMatrixKHR transform_matrix = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f
      };

      VkAccelerationStructureInstanceKHR as_instance {};
      as_instance.transform = transform_matrix;
      as_instance.instanceCustomIndex = 0;
      as_instance.mask = 0xFF;
      as_instance.instanceShaderBindingTableRecordOffset = 0;
      as_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      as_instance.accelerationStructureReference = blas_->device_address;

      auto device = device_->get_device();
      VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
      VkMemoryPropertyFlags host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      VkDeviceSize buffer_size = sizeof(VkAccelerationStructureInstanceKHR);

      // create instances buffer
      instances_buffer_ = std::make_unique<graphics::buffer>(
        *device_,
        buffer_size,
        1,
        usage,
        host_memory_props
      );

      // write the data to the buffer
      instances_buffer_->map();
      instances_buffer_->write_to_buffer((void *) &as_instance);

      // compute required memory size
      VkDeviceOrHostAddressConstKHR instance_data_device_address {};
      instance_data_device_address.deviceAddress = get_device_address(device_->get_device(), instances_buffer_->get_buffer());

      VkAccelerationStructureGeometryKHR as_geometry {};
      as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
      as_geometry.geometry.instances.data = instance_data_device_address;

      // get size
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      uint32_t primitive_count = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                              &as_build_geometry_info, &primitive_count, &as_build_sizes_info);

      // create tlas
      tlas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create tlas buffer
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = tlas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device, &as_create_info, nullptr, &tlas_->handle);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // set up for building tlas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = tlas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute build command
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = primitive_count;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;
      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands();
      vkCmdBuildAccelerationStructuresKHR(command, 1, &as_build_geometry_info, as_build_range_infos.data());

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = tlas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    void destroy_acceleration_structure(acceleration_structure& as)
    {
      auto device = device_->get_device();
      vkDestroyAccelerationStructureKHR(device, as.handle, nullptr);
      vkFreeMemory(device, as.memory, nullptr);
      vkDestroyBuffer(device, as.buffer, nullptr);
    }

    // ----------------------------------------------------------------------------------------------
    // variables
    u_ptr<graphics::window> window_;
    u_ptr<graphics::device> device_;
    u_ptr<graphics::buffer> vertex_buffer_;
    std::vector<vec3> triangle_vertices_ = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {0.0f,  0.75f, 0.0f}
    };

    // acceleration structure
    u_ptr<acceleration_structure> blas_;
    u_ptr<acceleration_structure> tlas_;
    u_ptr<graphics::buffer> instances_buffer_;
};
}

int main() {
  hnll::hello_triangle app;
}

// empty
#include <geometry/mesh_model.hpp>
void hnll::geometry::mesh_model::align_vertex_id() {}

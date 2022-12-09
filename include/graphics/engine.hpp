#pragma once

// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/renderer.hpp>
#include <graphics/descriptor_set_layout.hpp>
#include <graphics/buffer.hpp>
#include <graphics/rendering_system.hpp>
#include <utils/rendering_utils.hpp>

// std
#include <memory>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <map>

namespace hnll::graphics {

template<class U> using u_ptr = std::unique_ptr<U>;
template<class S> using s_ptr = std::shared_ptr<S>;

class engine
{
  public:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 820;
    static constexpr float MAX_FRAME_TIME = 0.05;

    engine(const char* window_name = "honolulu engine", graphics::rendering_type rendering_type = rendering_type::MESH_SHADING);
    ~engine();

    engine(const engine &) = delete;
    engine &operator= (const engine &) = delete;

    void render(const utils::viewer_info& _viewer_info, utils::frustum_info& _frustum_info);

    inline void wait_idle() { vkDeviceWaitIdle(device_->get_device()); }
    void update_ubo(int frame_index)
    { ubo_buffers_[frame_index]->write_to_buffer(&ubo_); ubo_buffers_[frame_index]->flush(); }

    inline device&     get_device()     { return *device_; }
    inline renderer&   get_renderer()   { return *renderer_; }
    inline swap_chain& get_swap_chain() { return renderer_->get_swap_chain(); }
    inline window&     get_window()     { return *window_; }
    inline global_ubo& get_global_ubo() { return ubo_; }

    inline VkDescriptorSetLayout get_global_desc_set_layout() const { return global_set_layout_->get_descriptor_set_layout(); }
    inline VkDescriptorSet get_global_descriptor_set(int frame_index) { return global_descriptor_sets_[frame_index]; }

    inline GLFWwindow* get_glfw_window() const { return window_->get_glfw_window(); }
    
  private:
    void init();

    // construct in impl
    u_ptr<window>   window_;
    u_ptr<device>   device_;
    u_ptr<renderer> renderer_;

    // shared between multiple system
    u_ptr<descriptor_pool>       global_pool_;
    std::vector<u_ptr<buffer>>   ubo_buffers_ {swap_chain::MAX_FRAMES_IN_FLIGHT};
    u_ptr<descriptor_set_layout> global_set_layout_;
    std::vector<VkDescriptorSet> global_descriptor_sets_ {swap_chain::MAX_FRAMES_IN_FLIGHT};

    global_ubo ubo_{};
};

} // namespace hnll::graphics

#pragma once

#include <hve_device.hpp>

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace hve {

class HveDescriptorSetLayout {
 
 public:
   class Builder {
   public:
    Builder(HveDevice &hveDevice) : hveDevice_m{hveDevice} {}

    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
    std::unique_ptr<HveDescriptorSetLayout> build() const;

   private:
    HveDevice &hveDevice_m;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_m{};
  };

  // ctor dtor
  HveDescriptorSetLayout(HveDevice &HveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
  ~HveDescriptorSetLayout();
  HveDescriptorSetLayout(const HveDescriptorSetLayout &) = delete;
  HveDescriptorSetLayout &operator=(const HveDescriptorSetLayout &) = delete;

  VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_m; }

 private:
  HveDevice &hveDevice_m;
  VkDescriptorSetLayout descriptorSetLayout_m;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_m;

  friend class HveDescriptorWriter;
};

class HveDescriptorPool {
 public:
  class Builder {
   public:
    Builder(HveDevice &hveDevice) : hveDevice_m{hveDevice} {}

    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<HveDescriptorPool> build() const;

   private:
    HveDevice &hveDevice_m;
    std::vector<VkDescriptorPoolSize> poolSizes_m{};
    uint32_t maxSets_m = 1000;
    VkDescriptorPoolCreateFlags poolFlags_m = 0;
  };

  HveDescriptorPool(
      HveDevice &hveDevice,
      uint32_t maxSets,
      VkDescriptorPoolCreateFlags poolFlags,
      const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~HveDescriptorPool();
  HveDescriptorPool(const HveDescriptorPool &) = delete;
  HveDescriptorPool &operator=(const HveDescriptorPool &) = delete;

  bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

  void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

  void resetPool();

 private:
  HveDevice &hveDevice_m;
  VkDescriptorPool descriptorPool_m;

  friend class HveDescriptorWriter;
};

class HveDescriptorWriter {
 public:
  HveDescriptorWriter(HveDescriptorSetLayout &setLayout, HveDescriptorPool &pool);

  HveDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
  HveDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);

 private:
  HveDescriptorSetLayout &setLayout_m;
  HveDescriptorPool &pool_m;
  std::vector<VkWriteDescriptorSet> writes_m;
};

}  // namespace Hve
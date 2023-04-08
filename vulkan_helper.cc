#include "vulkan_helper.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_enums.hpp>

namespace detail {

void PrintProperties(const vk::PhysicalDeviceProperties& properties) {
  std::cout << "Device name : " << properties.deviceName << std::endl;
  std::cout << "Device ID : " << properties.deviceID << std::endl;
  std::cout << "api Version : " << properties.apiVersion << std::endl;
  auto api_version = properties.apiVersion;
  std::cout << "Vulkan version : " << VK_VERSION_MAJOR(api_version) << "."
            << VK_VERSION_MINOR(api_version) << "."
            << VK_VERSION_PATCH(api_version) << std::endl;
  vk::PhysicalDeviceLimits deviceLimits = properties.limits;
  uint32_t memory_size_limit = deviceLimits.maxComputeSharedMemorySize / 1024;
  std::cout << "Max compute shared memory size : " << memory_size_limit << "KB"
            << std::endl;
}
}  // namespace detail

bool VulkanHelper::InitializeContext() {
  vk::ApplicationInfo app_info{"ComputeShader", 1, nullptr, 0,
                               VK_API_VERSION_1_0};
  //vk::InstanceCreateInfo instance_create_info{vk::InstanceCreateFlags(),
  //                                            &app_info, nullptr, nullptr};
  vk::InstanceCreateInfo instance_create_info{vk::InstanceCreateFlags(),
                                              &app_info, nullptr, nullptr};

  instance_create_info.setFlags(
      vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
  std::vector<const char *> requiredExtensions;
  requiredExtensions.emplace_back(
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  instance_create_info.setPEnabledExtensionNames(requiredExtensions);

  vk::Instance instance = vk::createInstance(instance_create_info);

  std::vector<vk::PhysicalDevice> physical_devices =
      instance.enumeratePhysicalDevices();
  if (physical_devices.empty()) {
    std::printf("There is no Graphical Device\n");
    return false;
  }
  for (vk::PhysicalDevice& device : physical_devices) {
    detail::PrintProperties(device.getProperties());
  }

  // std::printf("Choose the default device\n");

  physical_device_ = physical_devices[0];

  std::vector<vk::QueueFamilyProperties> queue_family_properties =
      physical_device_.getQueueFamilyProperties();
  auto properties_iterator =
      std::find_if(queue_family_properties.begin(),
                   queue_family_properties.end(), [](auto&& properties) {
                     return properties.queueFlags & vk::QueueFlagBits::eCompute;
                   });
  if (properties_iterator == queue_family_properties.end()) {
    std::printf("Device can't find compute queue\n");
    return false;
  }
  compute_queue_family_index_ =
      std::distance(queue_family_properties.begin(), properties_iterator);
  float queue_priority = 1.0f;
  vk::DeviceQueueCreateInfo device_queue_create_info(
      vk::DeviceQueueCreateFlags(), compute_queue_family_index_, 1,
      &queue_priority);

  vk::DeviceCreateInfo device_info(vk::DeviceCreateFlags(),
                                   device_queue_create_info);

  // Create Logic Device
  device_ = physical_device_.createDevice(device_info);

  // create Descriptor set
  vk::DescriptorPoolSize descriptor_pool_size(
      vk::DescriptorType::eStorageBuffer, 1);
  vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
      vk::DescriptorPoolCreateFlags(), 1, descriptor_pool_size);
  descriptor_pool_ = device_.createDescriptorPool(descriptor_pool_create_info);

  vk::CommandPoolCreateInfo CommandPoolCreateInfo(vk::CommandPoolCreateFlags(),
                                                  compute_queue_family_index_);
  vk::CommandPool CommandPool =
      device_.createCommandPool(CommandPoolCreateInfo);

  vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
      CommandPool,                       // Command Pool
      vk::CommandBufferLevel::ePrimary,  // Level
      1);                                // Num Command Buffers
  const std::vector<vk::CommandBuffer> CmdBuffers =
      device_.allocateCommandBuffers(CommandBufferAllocInfo);
  if (CmdBuffers.empty()) {
    std::printf("Create Command Buffer Fails\n");
    return false;
  }
  cmd_buffer_ = CmdBuffers.front();
  return true;
}

BufferWrap VulkanHelper::MallocGPUMemory(std::size_t sizes) {
  vk::BufferCreateInfo bufferCreateInfo{vk::BufferCreateFlags(),
                                        sizes,
                                        vk::BufferUsageFlagBits::eStorageBuffer,
                                        vk::SharingMode::eExclusive,
                                        1,
                                        &compute_queue_family_index_};

  vk::Buffer buffer = device_.createBuffer(bufferCreateInfo);

  vk::MemoryRequirements buffer_requirements =
      device_.getBufferMemoryRequirements(buffer);

  vk::PhysicalDeviceMemoryProperties memory_properties =
      physical_device_.getMemoryProperties();

  uint32_t memory_type_index = uint32_t(~0);
  for (uint32_t index = 0; index < memory_properties.memoryTypeCount; index++) {
    vk::MemoryType memory_type = memory_properties.memoryTypes[index];
    auto required_properties = vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent;
    if ((memory_type.propertyFlags & required_properties) ==
        required_properties) {
      memory_type_index = index;
      break;
    }
  }

  vk::MemoryAllocateInfo memory_allocate_info{buffer_requirements.size,
                                              memory_type_index};

  vk::DeviceMemory buffer_memory = device_.allocateMemory(memory_allocate_info);

  // buffer offset from memory
  device_.bindBufferMemory(buffer, buffer_memory, 0);

  return {buffer, buffer_memory, sizes};
}

vk::Pipeline VulkanHelper::BuildComputeShaderSPIV(
    const std::string& binary_path, std::size_t argument_size) {
  std::ifstream ifs(binary_path, std::ifstream::binary);
  std::filebuf* pbuf = ifs.rdbuf();
  std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
  pbuf->pubseekpos(0, ifs.in);

  std::vector<char> buffer(size);
  pbuf->sgetn(buffer.data(), size);
  ifs.close();
  vk::ShaderModuleCreateInfo shader_module_create_info{
      vk::ShaderModuleCreateFlagBits(), size, (uint32_t*)(buffer.data())};

  vk::ShaderModule shader_module;

  vk::Result result = device_.createShaderModule(&shader_module_create_info,
                                                 nullptr, &shader_module);

  std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_binding;
  for (uint32_t i = 0; i < argument_size; i++) {
    descriptor_set_layout_binding.push_back(
        {i, vk::DescriptorType::eStorageBuffer, 1,
         vk::ShaderStageFlagBits::eCompute, nullptr});
  }
  vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
      vk::DescriptorSetLayoutCreateFlagBits(), descriptor_set_layout_binding};

  descriptor_set_layout_ =
      device_.createDescriptorSetLayout(descriptor_set_layout_create_info);
  vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(descriptor_pool_, 1,
                                                       &descriptor_set_layout_);

  const std::vector<vk::DescriptorSet> descriptor_sets =
      device_.allocateDescriptorSets(DescriptorSetAllocInfo);

  if (descriptor_sets.empty()) {
    std::printf("Descriptor Set Empty\n");
    // return false;
    // return;
  }
  vk::DescriptorSet descriptor_set_ = descriptor_sets[0];
  vk::PipelineLayoutCreateInfo pipeline_layout_create_info{
      vk::PipelineLayoutCreateFlags(), descriptor_set_layout_};

  pipeline_layout_ = device_.createPipelineLayout(pipeline_layout_create_info);
  vk::PipelineCache pipeline_cache =
      device_.createPipelineCache(vk::PipelineCacheCreateInfo());

  vk::PipelineShaderStageCreateInfo pipeline_shader_create_info{
      vk::PipelineShaderStageCreateFlagBits(),
      vk::ShaderStageFlagBits::eCompute, shader_module, "main", nullptr};

  vk::ComputePipelineCreateInfo compute_pipeline_create_info{
      vk::PipelineCreateFlags(), pipeline_shader_create_info, pipeline_layout_};

  vk::ResultValue<vk::Pipeline> res_pipeline = device_.createComputePipeline(pipeline_cache,
                                       compute_pipeline_create_info);

  return res_pipeline.value; 
}

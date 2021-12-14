#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vulkan_helper.hpp"

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

char* ReadBinaryFile(const char* filename, size_t* psize) {
  long int size;
  size_t retval;
  void* shader_code;

  FILE* fp = fopen(filename, "rb");
  assert(fp != NULL);
  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  shader_code = malloc(size);
  retval = fread(shader_code, size, 1, fp);
  *psize = size;
  return (char*)shader_code;
}
vk::ShaderModule LoadShader(const char* file_name, VkDevice device,
                            VkShaderStageFlagBits stage) {
  size_t size = 0;
  const char* shader_code = ReadBinaryFile(file_name, &size);
  assert(size > 0);
  VkShaderModule shader_module;
  VkShaderModuleCreateInfo module_create_info;
  VkResult err;

  module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  module_create_info.pNext = NULL;
  module_create_info.codeSize = size;
  module_create_info.pCode = (uint32_t*)shader_code;
  module_create_info.flags = 0;

  err = vkCreateShaderModule(device, &module_create_info, nullptr,
                             &shader_module);
  return shader_module;
}

VkPipelineShaderStageCreateInfo LoadShader(std::string file_name,
                                           vk::Device device,
                                           VkShaderModule& shader_module,
                                           VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shader_stage_create_info{};
  shader_stage_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_create_info.stage = stage;

  shader_stage_create_info.module =
      LoadShader(file_name.c_str(), device, stage);
  shader_stage_create_info.pName = "main";
  shader_module = shader_stage_create_info.module;
  return shader_stage_create_info;
}
int main(int argc, char** argv) {
    /*
  vk::ApplicationInfo AppInfo{"Super Compute Shader", 1, nullptr, 0,
                              VK_API_VERSION_1_2};

  vk::InstanceCreateInfo InstanceCreateinfo{vk::InstanceCreateFlags(), &AppInfo,
                                            nullptr, nullptr};

  vk::Instance instance = vk::createInstance(InstanceCreateinfo);
  // Get GPU hardware info

  vk::PhysicalDevice physicalDevice =
      instance.enumeratePhysicalDevices().front();
  vk::PhysicalDeviceProperties deviceProperties =
      physicalDevice.getProperties();
  PrintProperties(deviceProperties);

  // retrieve gpu's queue info

  std::vector<vk::QueueFamilyProperties> queue_family_properties =
      physicalDevice.getQueueFamilyProperties();

  auto properties_iterator =
      std::find_if(queue_family_properties.begin(),
                   queue_family_properties.end(), [](auto&& properties) {
                     return properties.queueFlags & vk::QueueFlagBits::eCompute;
                   });

  const uint32_t compute_family_queue_index =
      std::distance(queue_family_properties.begin(), properties_iterator);
  float queue_priority = 1.0f;

  vk::DeviceQueueCreateInfo device_queue_create_info(
      vk::DeviceQueueCreateFlags(), compute_family_queue_index, 1,
      &queue_priority);

  // create device instance using info retrieved from physical device
  vk::DeviceCreateInfo device_info(vk::DeviceCreateFlags(),
                                   device_queue_create_info);

  vk::Device device = physicalDevice.createDevice(device_info);

  // creating buffers
  //
  const uint32_t num_elements = 16;
  const uint32_t buffer_size = num_elements * sizeof(int32_t);
  // create Storage Buffer
  vk::BufferCreateInfo bufferCreateInfo{vk::BufferCreateFlags(),
                                        buffer_size,
                                        vk::BufferUsageFlagBits::eStorageBuffer,
                                        vk::SharingMode::eExclusive,
                                        1,
                                        &compute_family_queue_index};

  vk::Buffer out_buffer = device.createBuffer(bufferCreateInfo);

  vk::MemoryRequirements out_buffer_requirements =
      device.getBufferMemoryRequirements(out_buffer);

  vk::PhysicalDeviceMemoryProperties memory_properties =
      physicalDevice.getMemoryProperties();

  uint32_t memoryTypeIndex = uint32_t(~0);
  vk::DeviceSize memoryHeapSize = uint32_t(~0);

  for (uint32_t current_memory_type_index = 0;
       current_memory_type_index < memory_properties.memoryTypeCount;
       ++current_memory_type_index) {
    vk::MemoryType memory_type =
        memory_properties.memoryTypes[current_memory_type_index];
    if ((vk::MemoryPropertyFlagBits::eHostVisible &
         memory_type.propertyFlags) &&
        (vk::MemoryPropertyFlagBits::eHostCoherent &
         memory_type.propertyFlags)) {
      memoryHeapSize =
          memory_properties.memoryHeaps[memory_type.heapIndex].size;
      memoryTypeIndex = current_memory_type_index;
      break;
    }
  }

  std::cout << "Memory Type Index : " << memoryTypeIndex << std::endl;
  std::cout << "Memory Heap Size : " << memoryHeapSize / 1024 / 1024 / 1024
            << "GB" << std::endl;

  
  vk::MemoryAllocateInfo out_buffer_memory_allocate_outfo(
      out_buffer_requirements.size, memoryTypeIndex);

  vk::DeviceMemory out_buffer_memory =
      device.allocateMemory(out_buffer_memory_allocate_outfo);

  device.bindBufferMemory(out_buffer, out_buffer_memory, 0);

  

  // There are related to the shader resource interface of the shader source code.
  const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding =
      {
          {0, vk::DescriptorType::eStorageBuffer, 1,
        vk::ShaderStageFlagBits::eCompute, nullptr}
        // binding 0 is a Storage Buffer, descriptor array size is 1
        // visible to Compute Stage
       };

  vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
      vk::DescriptorSetLayoutCreateFlagBits(), DescriptorSetLayoutBinding);

  vk::DescriptorSetLayout DescriptorSetLayout =
      device.createDescriptorSetLayout(descriptor_set_layout_create_info);
  vk::PipelineLayoutCreateInfo pipeline_layout_create_info{
      vk::PipelineLayoutCreateFlags(), DescriptorSetLayout};
  vk::PipelineLayout pipeline_layout =
      device.createPipelineLayout(pipeline_layout_create_info);
  vk::PipelineCache pipeline_cache =
      device.createPipelineCache(vk::PipelineCacheCreateInfo());

  VkShaderModule shader_module;

  vk::PipelineShaderStageCreateInfo pipeline_shader_create_info = LoadShader(
      "./comp.spv", device, shader_module, VK_SHADER_STAGE_COMPUTE_BIT);

  vk::ComputePipelineCreateInfo compute_pipeline_create_info(
      vk::PipelineCreateFlags(), pipeline_shader_create_info, pipeline_layout);

  vk::Pipeline compute_pipeline = device.createComputePipeline(
      pipeline_cache, compute_pipeline_create_info);
  // update argument data
  vk::DescriptorPoolSize descriptor_pool_size(
      vk::DescriptorType::eStorageBuffer, 1);
  vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
      vk::DescriptorPoolCreateFlags(), 1, descriptor_pool_size);
  vk::DescriptorPool descriptor_pool =
      device.createDescriptorPool(descriptor_pool_create_info);

  vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(descriptor_pool, 1,
                                                       &DescriptorSetLayout);
  const std::vector<vk::DescriptorSet> DescriptorSets =
      device.allocateDescriptorSets(DescriptorSetAllocInfo);
  vk::DescriptorSet DescriptorSet = DescriptorSets.front();

  vk::DescriptorBufferInfo OutBufferInfo(out_buffer, 0,
                                         buffer_size);

  const std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
      {DescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
       &OutBufferInfo, nullptr},
  };
  device.updateDescriptorSets(WriteDescriptorSets, {});

  //  executate data
  vk::CommandPoolCreateInfo CommandPoolCreateInfo(vk::CommandPoolCreateFlags(),
                                                  compute_family_queue_index);
  vk::CommandPool CommandPool = device.createCommandPool(CommandPoolCreateInfo);

  vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
      CommandPool,                       // Command Pool
      vk::CommandBufferLevel::ePrimary,  // Level
      1);                                // Num Command Buffers
  const std::vector<vk::CommandBuffer> CmdBuffers =
      device.allocateCommandBuffers(CommandBufferAllocInfo);
  vk::CommandBuffer CmdBuffer = CmdBuffers.front();

  vk::CommandBufferBeginInfo CmdBufferBeginInfo(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  CmdBuffer.begin(CmdBufferBeginInfo);
  CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline);
  CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,  // Bind point
                               pipeline_layout,   // Pipeline Layout
                               0,                // First descriptor set
                               {DescriptorSet},  // List of descriptor sets
                               {});              // Dynamic offsets
  CmdBuffer.dispatch(num_elements, 1, 1);
  CmdBuffer.end();

  vk::Queue Queue = device.getQueue(compute_family_queue_index, 0);
  vk::Fence Fence = device.createFence(vk::FenceCreateInfo());

  vk::SubmitInfo SubmitInfo(0,            // Num Wait Semaphores
                            nullptr,      // Wait Semaphores
                            nullptr,      // Pipeline Stage Flags
                            1,            // Num Command Buffers
                            &CmdBuffer);  // List of command buffers
  Queue.submit({SubmitInfo}, Fence);
  device.waitForFences({Fence},        // List of fences
                       true,           // Wait All
                       uint64_t(-1));  // Timeout

  int32_t* OutBufferPtr =
      static_cast<int32_t*>(device.mapMemory(out_buffer_memory, 0, buffer_size));
  for (uint32_t I = 0; I < num_elements; ++I) {
    std::cout << OutBufferPtr[I] << " ";
  }
  std::cout << std::endl;
  device.unmapMemory(out_buffer_memory);
*/

  VulkanHelper helper;
  helper.InitializeContext();
  auto ptr = helper.MallocGPUMemory(32 * sizeof(int32_t));
  int32_t input[32];
  for (int i = 0; i < 32; i++) {
      input[i] = i + 10;
  }
  auto output_ptr = helper.MallocGPUMemory(32 * sizeof(int32_t));

  helper.CopyMemory(ptr, input, 32 * sizeof(int32_t));
  //vk::Pipeline compute_pipeline_2 = helper.BuildComputeShaderSPIV("./comp.spv", 2);

  helper.ExecuteProgram("./comp.spv", 32, 1, 1, ptr, output_ptr);

  int32_t output[32];
  helper.CopyMemory(output, output_ptr, 32 * sizeof(int32_t));
  for (int i = 0; i < 32; i++) {
      std::cout << output[i] << " ";
  }
  std::cout << std::endl;
  return 0;
}
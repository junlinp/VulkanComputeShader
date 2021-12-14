#ifndef VULKAN_HELPER_HPP_

#include "vulkan/vulkan.hpp"
#include <map>
#include "iostream"

struct BufferWrap {
    vk::Buffer buffer_;
    vk::DeviceMemory device_memory_;
    std::size_t size_;
};

class VulkanHelper {
 public:
  bool InitializeContext();
  BufferWrap MallocGPUMemory(std::size_t sizes);

  // TODO (junlinp@qq.com):
  // It seems that the argument_num can be optimized and left out.
  vk::Pipeline BuildComputeShaderSPIV(const std::string& binary_path,
                                      std::size_t argument_num);

bool CopyMemory(void* dest, BufferWrap& sour, std::size_t size) {
    void* device_ptr = device_.mapMemory(sour.device_memory_, 0, size);
    std::memcpy(dest, device_ptr, size);
    device_.unmapMemory(sour.device_memory_);
    return true;
};

bool CopyMemory(BufferWrap& dest, void* sour, std::size_t size) {
    
    void* device_ptr = device_.mapMemory(dest.device_memory_, 0, size);
    //char* c_sour = static_cast<char*>(sour);
    //char* c_device = static_cast<char*>(device_ptr);

    std::memcpy(device_ptr, sour, size);
    device_.unmapMemory(dest.device_memory_);
    return true;
};
  // FreeGPUMemory();
  // May using the Template meta Programing for executa
  // Args must be type
  template <typename... Args>
  bool ExecuteProgram(const std::string& binary_path, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z, Args... args) {
      // bind args
        //static_assert(std::is_convertible_v<BufferWrap, args> && ...);
        if (compute_pipeline_cache_.find(binary_path) == compute_pipeline_cache_.end()) {
            compute_pipeline_cache_[binary_path] = BuildComputeShaderSPIV(binary_path, sizeof...(args));
        }

        vk::Pipeline compute_pipeline = compute_pipeline_cache_[binary_path];

        std::vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;

        (descriptor_buffer_infos.push_back({args.buffer_, 0, args.size_}), ...);

        std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {};
        uint32_t index = 0;
        for (vk::DescriptorBufferInfo& info : descriptor_buffer_infos) {
            WriteDescriptorSets.push_back(
                                        {descriptor_set_, index++, 0, 1,
                                        vk::DescriptorType::eStorageBuffer,
                                        nullptr, &info, nullptr}
            );
        }

        device_.updateDescriptorSets(WriteDescriptorSets, {});

        

        vk::CommandBufferBeginInfo CmdBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd_buffer_.begin(CmdBufferBeginInfo);
        cmd_buffer_.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline);

        cmd_buffer_.bindDescriptorSets(vk::PipelineBindPoint::eCompute,  // Bind point
                                pipeline_layout_,   // Pipeline Layout
                               0,                // First descriptor set
                               {descriptor_set_},  // List of descriptor sets
                               {});              // Dynamic offsets
                    
        cmd_buffer_.dispatch(group_count_x, group_count_y, group_count_z);
        cmd_buffer_.end();

        vk::Queue Queue = device_.getQueue(compute_queue_family_index_, 0);
        vk::Fence Fence = device_.createFence(vk::FenceCreateInfo());

        vk::SubmitInfo SubmitInfo(0,            // Num Wait Semaphores
                            nullptr,      // Wait Semaphores
                            nullptr,      // Pipeline Stage Flags
                            1,            // Num Command Buffers
                            &cmd_buffer_);  // List of command buffers
        Queue.submit({SubmitInfo}, Fence);
        vk::Result wait_result = device_.waitForFences({Fence},        // List of fences
                       true,           // Wait All
                       uint64_t(-1));  // Timeout
        
        // How to check out the result
        return true;
  }

 private:
  vk::PhysicalDevice physical_device_;
  vk::Device device_;
  uint32_t compute_queue_family_index_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::PipelineLayout pipeline_layout_;
  std::map<std::string, vk::Pipeline> compute_pipeline_cache_;
  vk::DescriptorSet descriptor_set_;
  vk::CommandBuffer cmd_buffer_;
  vk::DescriptorPool descriptor_pool_;
};
#endif  // VULKAN_HELPER_HPP_
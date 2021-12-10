#ifndef VULKAN_HELPER_HPP_

#include "vulkan/vulkan.hpp"
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
  bool ExecuteProgram(vk::Pipeline& compute_pipeline, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z, Args... args) {
      // bind args
    std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {};
        //static_assert(std::is_convertible_v<BufferWrap, args> && ...);

        vk::DescriptorPoolSize descriptor_pool_size(
            vk::DescriptorType::eStorageBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlags(), 1, descriptor_pool_size);
        vk::DescriptorPool descriptor_pool =
            device_.createDescriptorPool(descriptor_pool_create_info);

        vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(
            descriptor_pool, 1, &descriptor_set_layout_);
        const std::vector<vk::DescriptorSet> descriptor_sets =
            device_.allocateDescriptorSets(DescriptorSetAllocInfo);

        if (descriptor_sets.empty()) {
            std::printf("Descriptor Set Empty\n");
            return false;
        }
        vk::DescriptorSet descriptor_set = descriptor_sets[0];

        std::vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;

        (descriptor_buffer_infos.push_back({args.buffer_, 0, args.size_}), ...);

        for (vk::DescriptorBufferInfo& info : descriptor_buffer_infos) {
            WriteDescriptorSets.push_back(
                                        {descriptor_set, 1, 0, 1,
                                        vk::DescriptorType::eStorageBuffer,
                                        nullptr, &info, nullptr}
            );
        }

        device_.updateDescriptorSets(WriteDescriptorSets, {});
        std::cout << "Update Descriptor Sets" << std::endl;

        vk::CommandPoolCreateInfo CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), compute_queue_family_index_);
        vk::CommandPool CommandPool = device_.createCommandPool(CommandPoolCreateInfo);

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
        vk::CommandBuffer CmdBuffer = CmdBuffers.front();

        vk::CommandBufferBeginInfo CmdBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        //
        std::cout << "Before Command Begin " << std::endl;
        CmdBuffer.begin(CmdBufferBeginInfo);
        CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline);

        CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,  // Bind point
                                pipeline_layout_,   // Pipeline Layout
                               0,                // First descriptor set
                               {descriptor_set},  // List of descriptor sets
                               {});              // Dynamic offsets
                    
        CmdBuffer.dispatch(group_count_x, group_count_y, group_count_z);
        CmdBuffer.end();

        vk::Queue Queue = device_.getQueue(compute_queue_family_index_, 0);
        vk::Fence Fence = device_.createFence(vk::FenceCreateInfo());

        vk::SubmitInfo SubmitInfo(0,            // Num Wait Semaphores
                            nullptr,      // Wait Semaphores
                            nullptr,      // Pipeline Stage Flags
                            1,            // Num Command Buffers
                            &CmdBuffer);  // List of command buffers
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
};
#endif  // VULKAN_HELPER_HPP_
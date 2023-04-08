#include "gtest/gtest.h"
#include <cmath>
#include "vulkan_helper.hpp"

template<typename T>
T Distance(T* a, T* b) {
    T reduce{0};
    for(int i = 0; i < 128; i++) {
        T temp = a[i] - b[i];
        reduce += temp * temp;
    }
    return reduce;
}

void DistanceGPU(int* a, std::size_t a_size, int* b, std::size_t b_size, int* result_matrix) {
    VulkanHelper vulkan;
    vulkan.InitializeContext();

    auto device_a = vulkan.MallocGPUMemory(a_size * 128 * sizeof(int));
    auto device_b = vulkan.MallocGPUMemory(b_size * 128 * sizeof(int));
    auto device_c = vulkan.MallocGPUMemory(a_size * b_size * sizeof(int));
    auto device_size = vulkan.MallocGPUMemory(sizeof(int));
    int sizes_ = b_size;
    vulkan.CopyMemory(device_a, a, sizeof(int) * a_size * 128);
    vulkan.CopyMemory(device_b, b, sizeof(int) * b_size * 128);
    vulkan.CopyMemory(device_size, &sizes_, sizeof(int));

    //auto pipeline = vulkan.BuildComputeShaderSPIV("./match.spv", 4);

    vulkan.ExecuteProgram("./distance.spv", a_size / 32, b_size / 32, 1, device_a, device_b, device_c, device_size);

    vulkan.CopyMemory(result_matrix, device_c, sizeof(int) * a_size * b_size);
}

class MatchMatrixTest : public ::testing::Test {
    protected:
        void SetUp() override {
            lhs_size = 1024 * 8;
            rhs_size = 1024 * 8;

            lhs_ptr = new int[lhs_size * 128];
            rhs_ptr = new int[rhs_size * 128];
            for (int i = 0; i < lhs_size * 128; i++) {
                lhs_ptr[i] = rand() % 3;
            }

            for (int i = 0; i < rhs_size * 128; i++) {
                rhs_ptr[i] = rand() % 5;
            }
            result_matrix = new int[lhs_size * rhs_size];

            for (int row = 0; row < lhs_size; row++) {
                for (int col = 0; col < rhs_size; col++) {
                    result_matrix[row * rhs_size + col] = 0;
                    for (int i = 0; i < 128; i++) {
                        int temp = lhs_ptr[row * 128 + i] - rhs_ptr[col * 128 + i];
                        result_matrix[row * rhs_size + col] += temp * temp;
                    }
                }
            }
        }

        void TearDown() override {
            delete[] lhs_ptr;
            delete[] rhs_ptr;
            delete[] result_matrix;
        }

        int* lhs_ptr;
        int lhs_size;
        int* rhs_ptr;
        int rhs_size;
        int* result_matrix;
};
/*
TEST_F(MatchMatrixTest, CPU) {
    for (int row = 0; row < lhs_size; row++) {
        for (int col = 0; col < rhs_size; col++) {
            int expect = result_matrix[row * rhs_size + col];
            int target = Distance(&lhs_ptr[row * 128], &rhs_ptr[col * 128]);

            EXPECT_EQ(expect, target);
        }
    }
}
*/
#include <chrono>
TEST_F(MatchMatrixTest, GPU_Vulkan) {
    int* matrix = new int[lhs_size * rhs_size];

    auto start = std::chrono::high_resolution_clock::now();
    DistanceGPU(lhs_ptr, lhs_size, rhs_ptr, rhs_size, matrix);
    auto end = std::chrono::high_resolution_clock::now();

    for (int row = 0; row < lhs_size; row++) {
        for (int col = 0; col < rhs_size; col++) {
            int expect = result_matrix[row * rhs_size + col];
            int target =  matrix[row * rhs_size + col];
            EXPECT_EQ(expect, target);
        }
    }
    delete[] matrix;
    std::cout << (end - start).count() / 1000 / 1000 << "ms" << std::endl;
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
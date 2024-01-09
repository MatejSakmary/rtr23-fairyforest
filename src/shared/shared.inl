#include "../backend/backend.inl"
#define COMPUTE_TEST_X_THREADS 64
BUFFER_REF(4)
TestBuffer
{
    f32vec4 color;
};

struct TrianglePC
{
    VkDeviceAddress test_buffer_address;
};

BUFFER_REF(4)
ComputeTestBuffer
{
    u32 value;
};

struct ComputeTestPC
{
    VkDeviceAddress compute_test_buffer;
};
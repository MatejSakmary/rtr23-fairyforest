#include "../backend/backend.inl"

BUFFER_REF(4) TestBuffer
{
    f32vec4 color;
};

struct TrianglePC
{
    VkDeviceAddress test_buffer_address;
};
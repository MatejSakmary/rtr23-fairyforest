#include "features.hpp"

namespace ff
{
    void PhysicalDeviceFeatureTable::initialize()
    {
        this->features = {
            .robustBufferAccess = VK_FALSE,
            .fullDrawIndexUint32 = VK_FALSE,
            .imageCubeArray = VK_TRUE,
            .independentBlend = VK_TRUE,
            .geometryShader = VK_FALSE,
            .tessellationShader = VK_TRUE,
            .sampleRateShading = VK_FALSE,
            .dualSrcBlend = VK_FALSE,
            .logicOp = VK_FALSE,
            .multiDrawIndirect = VK_TRUE, // Very useful for gpu driven rendering
            .drawIndirectFirstInstance = VK_FALSE,
            .depthClamp = VK_TRUE, // NOTE(msakmary) need self for bikeshed if breaks ping me
            .depthBiasClamp = VK_FALSE,
            .fillModeNonSolid = VK_TRUE,
            .depthBounds = VK_FALSE,
            .wideLines = VK_TRUE,
            .largePoints = VK_FALSE,
            .alphaToOne = VK_FALSE,
            .multiViewport = VK_FALSE,
            .samplerAnisotropy = VK_TRUE, // Allows for anisotropic filtering.
            .textureCompressionETC2 = VK_FALSE,
            .textureCompressionASTC_LDR = VK_FALSE,
            .textureCompressionBC = VK_FALSE,
            .occlusionQueryPrecise = VK_FALSE,
            .pipelineStatisticsQuery = VK_FALSE,
            .vertexPipelineStoresAndAtomics = VK_FALSE,
            .fragmentStoresAndAtomics = VK_TRUE,
            .shaderTessellationAndGeometryPointSize = VK_FALSE,
            .shaderImageGatherExtended = VK_FALSE,
            .shaderStorageImageExtendedFormats = VK_FALSE,
            .shaderStorageImageMultisample = VK_TRUE,            // Useful for software vrs.
            .shaderStorageImageReadWithoutFormat = VK_TRUE,      
            .shaderStorageImageWriteWithoutFormat = VK_TRUE,     
            .shaderUniformBufferArrayDynamicIndexing = VK_FALSE, // This is superseded by descriptor indexing.
            .shaderSampledImageArrayDynamicIndexing = VK_FALSE,  // This is superseded by descriptor indexing.
            .shaderStorageBufferArrayDynamicIndexing = VK_FALSE, // This is superseded by descriptor indexing.
            .shaderStorageImageArrayDynamicIndexing = VK_FALSE,  // This is superseded by descriptor indexing.
            .shaderClipDistance = VK_FALSE,
            .shaderCullDistance = VK_FALSE,
            .shaderFloat64 = VK_FALSE,
            .shaderInt64 = VK_TRUE, // Used for buffer device address math.
            .shaderInt16 = VK_FALSE,
            .shaderResourceResidency = VK_FALSE,
            .shaderResourceMinLod = VK_FALSE,
            .sparseBinding = VK_FALSE,
            .sparseResidencyBuffer = VK_FALSE,
            .sparseResidencyImage2D = VK_FALSE,
            .sparseResidencyImage3D = VK_FALSE,
            .sparseResidency2Samples = VK_FALSE,
            .sparseResidency4Samples = VK_FALSE,
            .sparseResidency8Samples = VK_FALSE,
            .sparseResidency16Samples = VK_FALSE,
            .sparseResidencyAliased = VK_FALSE,
            .variableMultisampleRate = VK_FALSE,
            .inheritedQueries = VK_FALSE,
        };
        this->chain = nullptr;
        this->buffer_device_address = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .pNext = chain,
            .bufferDeviceAddress = VK_TRUE,
            .bufferDeviceAddressCaptureReplay = static_cast<VkBool32>(true),
            .bufferDeviceAddressMultiDevice = VK_FALSE,
        };
        this->chain = reinterpret_cast<void *>(&this->buffer_device_address);
        this->descriptor_indexing = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
            .pNext = chain,
            .shaderInputAttachmentArrayDynamicIndexing = VK_FALSE,
            .shaderUniformTexelBufferArrayDynamicIndexing = VK_FALSE,
            .shaderStorageTexelBufferArrayDynamicIndexing = VK_FALSE,
            .shaderUniformBufferArrayNonUniformIndexing = VK_FALSE,
            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
            .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
            .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
            .shaderInputAttachmentArrayNonUniformIndexing = VK_FALSE,
            .shaderUniformTexelBufferArrayNonUniformIndexing = VK_FALSE,
            .shaderStorageTexelBufferArrayNonUniformIndexing = VK_FALSE,
            .descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
            .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE,
            .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
            .descriptorBindingPartiallyBound = VK_TRUE,
            .descriptorBindingVariableDescriptorCount = VK_FALSE,
            .runtimeDescriptorArray = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->descriptor_indexing);
        this->host_query_reset = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES,
            .pNext = this->chain,
            .hostQueryReset = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->host_query_reset);
        this->dynamic_rendering = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .pNext = this->chain,
            .dynamicRendering = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->dynamic_rendering);
        this->sync2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
            .pNext = this->chain,
            .synchronization2 = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->sync2);
        this->timeline_semaphore = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
            .pNext = this->chain,
            .timelineSemaphore = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->timeline_semaphore);
        this->scalar_layout = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
            .pNext = this->chain,
            .scalarBlockLayout = VK_TRUE,
        };
        this->chain = reinterpret_cast<void *>(&this->scalar_layout);
    }

    void PhysicalDeviceExtensionList::initialize()
    {
        this->size = 0;
        this->data[size++] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        this->data[size++] = {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME};
    }
} // namespace ff
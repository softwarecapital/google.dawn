// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DAWNNATIVE_D3D12_DEVICED3D12_H_
#define DAWNNATIVE_D3D12_DEVICED3D12_H_

#include "common/SerialQueue.h"
#include "dawn_native/Device.h"
#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/D3D12Info.h"
#include "dawn_native/d3d12/Forward.h"
#include "dawn_native/d3d12/TextureD3D12.h"

namespace dawn_native { namespace d3d12 {

    class CommandAllocatorManager;
    class PlatformFunctions;
    class ResidencyManager;
    class ResourceAllocatorManager;
    class SamplerHeapCache;
    class ShaderVisibleDescriptorAllocator;
    class StagingDescriptorAllocator;

#define ASSERT_SUCCESS(hr)            \
    do {                              \
        HRESULT succeeded = hr;       \
        ASSERT(SUCCEEDED(succeeded)); \
    } while (0)

    // Definition of backend types
    class Device : public DeviceBase {
      public:
        static ResultOrError<Device*> Create(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device() override;

        MaybeError Initialize();

        ResultOrError<Ref<CommandBufferBase>> CreateCommandBuffer(
            CommandEncoder* encoder,
            const CommandBufferDescriptor* descriptor) override;

        MaybeError TickImpl() override;

        ID3D12Device* GetD3D12Device() const;
        ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
        ID3D12SharingContract* GetSharingContract() const;

        ComPtr<ID3D12CommandSignature> GetDispatchIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndirectSignature() const;
        ComPtr<ID3D12CommandSignature> GetDrawIndexedIndirectSignature() const;

        CommandAllocatorManager* GetCommandAllocatorManager() const;
        ResidencyManager* GetResidencyManager() const;

        const PlatformFunctions* GetFunctions() const;
        ComPtr<IDXGIFactory4> GetFactory() const;
        ComPtr<IDxcLibrary> GetDxcLibrary() const;
        ComPtr<IDxcCompiler> GetDxcCompiler() const;
        ComPtr<IDxcValidator> GetDxcValidator() const;

        ResultOrError<CommandRecordingContext*> GetPendingCommandContext();

        const D3D12DeviceInfo& GetDeviceInfo() const;

        MaybeError NextSerial();
        MaybeError WaitForSerial(ExecutionSerial serial);

        void ReferenceUntilUnused(ComPtr<IUnknown> object);

        MaybeError ExecutePendingCommandContext();

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;

        void CopyFromStagingToBufferImpl(CommandRecordingContext* commandContext,
                                         StagingBufferBase* source,
                                         uint64_t sourceOffset,
                                         BufferBase* destination,
                                         uint64_t destinationOffset,
                                         uint64_t size);

        MaybeError CopyFromStagingToTexture(const StagingBufferBase* source,
                                            const TextureDataLayout& src,
                                            TextureCopy* dst,
                                            const Extent3D& copySizePixels) override;

        ResultOrError<ResourceHeapAllocation> AllocateMemory(
            D3D12_HEAP_TYPE heapType,
            const D3D12_RESOURCE_DESC& resourceDescriptor,
            D3D12_RESOURCE_STATES initialUsage);

        void DeallocateMemory(ResourceHeapAllocation& allocation);

        ShaderVisibleDescriptorAllocator* GetViewShaderVisibleDescriptorAllocator() const;
        ShaderVisibleDescriptorAllocator* GetSamplerShaderVisibleDescriptorAllocator() const;

        // Returns nullptr when descriptor count is zero.
        StagingDescriptorAllocator* GetViewStagingDescriptorAllocator(
            uint32_t descriptorCount) const;

        StagingDescriptorAllocator* GetSamplerStagingDescriptorAllocator(
            uint32_t descriptorCount) const;

        SamplerHeapCache* GetSamplerHeapCache();

        StagingDescriptorAllocator* GetRenderTargetViewAllocator() const;

        StagingDescriptorAllocator* GetDepthStencilViewAllocator() const;

        Ref<TextureBase> CreateExternalTexture(const TextureDescriptor* descriptor,
                                               ComPtr<ID3D12Resource> d3d12Texture,
                                               ExternalMutexSerial acquireMutexKey,
                                               bool isSwapChainTexture,
                                               bool isInitialized);
        ResultOrError<ComPtr<IDXGIKeyedMutex>> CreateKeyedMutexForTexture(
            ID3D12Resource* d3d12Resource);
        void ReleaseKeyedMutexForTexture(ComPtr<IDXGIKeyedMutex> dxgiKeyedMutex);

        void InitTogglesFromDriver();

        uint32_t GetOptimalBytesPerRowAlignment() const override;
        uint64_t GetOptimalBufferToTextureCopyOffsetAlignment() const override;

        float GetTimestampPeriodInNS() const override;

      private:
        using DeviceBase::DeviceBase;

        ResultOrError<Ref<BindGroupBase>> CreateBindGroupImpl(
            const BindGroupDescriptor* descriptor) override;
        ResultOrError<Ref<BindGroupLayoutBase>> CreateBindGroupLayoutImpl(
            const BindGroupLayoutDescriptor* descriptor) override;
        ResultOrError<Ref<BufferBase>> CreateBufferImpl(
            const BufferDescriptor* descriptor) override;
        ResultOrError<Ref<ComputePipelineBase>> CreateComputePipelineImpl(
            const ComputePipelineDescriptor* descriptor) override;
        ResultOrError<Ref<PipelineLayoutBase>> CreatePipelineLayoutImpl(
            const PipelineLayoutDescriptor* descriptor) override;
        ResultOrError<Ref<QuerySetBase>> CreateQuerySetImpl(
            const QuerySetDescriptor* descriptor) override;
        ResultOrError<Ref<RenderPipelineBase>> CreateRenderPipelineImpl(
            const RenderPipelineDescriptor2* descriptor) override;
        ResultOrError<Ref<SamplerBase>> CreateSamplerImpl(
            const SamplerDescriptor* descriptor) override;
        ResultOrError<Ref<ShaderModuleBase>> CreateShaderModuleImpl(
            const ShaderModuleDescriptor* descriptor,
            ShaderModuleParseResult* parseResult) override;
        ResultOrError<Ref<SwapChainBase>> CreateSwapChainImpl(
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<Ref<NewSwapChainBase>> CreateSwapChainImpl(
            Surface* surface,
            NewSwapChainBase* previousSwapChain,
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<Ref<TextureBase>> CreateTextureImpl(
            const TextureDescriptor* descriptor) override;
        ResultOrError<Ref<TextureViewBase>> CreateTextureViewImpl(
            TextureBase* texture,
            const TextureViewDescriptor* descriptor) override;

        void ShutDownImpl() override;
        MaybeError WaitForIdleForDestruction() override;

        MaybeError CheckDebugLayerAndGenerateErrors();

        MaybeError ApplyUseDxcToggle();

        ComPtr<ID3D12Fence> mFence;
        HANDLE mFenceEvent = nullptr;
        ResultOrError<ExecutionSerial> CheckAndUpdateCompletedSerials() override;

        ComPtr<ID3D12Device> mD3d12Device;  // Device is owned by adapter and will not be outlived.
        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12SharingContract> mD3d12SharingContract;

        // 11on12 device and device context corresponding to mCommandQueue
        ComPtr<ID3D11On12Device> mD3d11On12Device;
        ComPtr<ID3D11DeviceContext2> mD3d11On12DeviceContext;

        ComPtr<ID3D12CommandSignature> mDispatchIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndirectSignature;
        ComPtr<ID3D12CommandSignature> mDrawIndexedIndirectSignature;

        CommandRecordingContext mPendingCommands;

        SerialQueue<ExecutionSerial, ComPtr<IUnknown>> mUsedComObjectRefs;

        std::unique_ptr<CommandAllocatorManager> mCommandAllocatorManager;
        std::unique_ptr<ResourceAllocatorManager> mResourceAllocatorManager;
        std::unique_ptr<ResidencyManager> mResidencyManager;

        static constexpr uint32_t kMaxSamplerDescriptorsPerBindGroup =
            3 * kMaxSamplersPerShaderStage;
        static constexpr uint32_t kMaxViewDescriptorsPerBindGroup =
            kMaxBindingsPerPipelineLayout - kMaxSamplerDescriptorsPerBindGroup;

        static constexpr uint32_t kNumSamplerDescriptorAllocators =
            ConstexprLog2Ceil(kMaxSamplerDescriptorsPerBindGroup) + 1;
        static constexpr uint32_t kNumViewDescriptorAllocators =
            ConstexprLog2Ceil(kMaxViewDescriptorsPerBindGroup) + 1;

        // Index corresponds to Log2Ceil(descriptorCount) where descriptorCount is in
        // the range [0, kMaxSamplerDescriptorsPerBindGroup].
        std::array<std::unique_ptr<StagingDescriptorAllocator>, kNumViewDescriptorAllocators + 1>
            mViewAllocators;

        // Index corresponds to Log2Ceil(descriptorCount) where descriptorCount is in
        // the range [0, kMaxViewDescriptorsPerBindGroup].
        std::array<std::unique_ptr<StagingDescriptorAllocator>, kNumSamplerDescriptorAllocators + 1>
            mSamplerAllocators;

        std::unique_ptr<StagingDescriptorAllocator> mRenderTargetViewAllocator;

        std::unique_ptr<StagingDescriptorAllocator> mDepthStencilViewAllocator;

        std::unique_ptr<ShaderVisibleDescriptorAllocator> mViewShaderVisibleDescriptorAllocator;

        std::unique_ptr<ShaderVisibleDescriptorAllocator> mSamplerShaderVisibleDescriptorAllocator;

        // Sampler cache needs to be destroyed before the CPU sampler allocator to ensure the final
        // release is called.
        std::unique_ptr<SamplerHeapCache> mSamplerHeapCache;

        // The number of nanoseconds required for a timestamp query to be incremented by 1
        float mTimestampPeriod = 1.0f;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_DEVICED3D12_H_

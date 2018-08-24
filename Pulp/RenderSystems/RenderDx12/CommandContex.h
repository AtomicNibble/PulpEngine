#pragma once

#include <Containers\Fifo.h>

#include "GpuResource.h"
#include "Allocators\DynamicDescriptorHeap.h"
#include "Allocators\LinearAllocator.h"

#include <Math\XViewPort.h>

X_NAMESPACE_BEGIN(render)

class CommandListManger;
class CommandContext;
class CommandSignature;
class RootSignature;
class GpuBuffer;
class ColorBuffer;
class DepthBuffer;
class StructuredBuffer;
class GraphicsPSO;
class ComputePSO;
class DescriptorAllocatorPool;

struct Param
{
    X_INLINE explicit Param(float32_t f);
    X_INLINE explicit Param(uint32_t u);
    X_INLINE explicit Param(int32_t i);

    X_INLINE void operator=(float32_t f);
    X_INLINE void operator=(uint32_t u);
    X_INLINE void operator=(int32_t i);

    union
    {
        float32_t fval;
        uint32_t uint;
        int32_t sint;
    };
};

class GraphicsContext;
class ComputeContext;

class ContextManager
{
    static const size_t COMMAND_LIST_TYPE_NUM = 4; // num D3D12_COMMAND_LIST_TYPE

public:
    ContextManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
        CommandListManger& cmdListMan, DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan);
    ~ContextManager();

    GraphicsContext* allocateGraphicsContext(void);
    CommandContext* allocateContext(D3D12_COMMAND_LIST_TYPE type);

    void freeContext(CommandContext* pContex);
    void destroyAllContexts(void);

    X_INLINE const CommandListManger& getCmdListMan(void) const;
    X_INLINE CommandListManger& getCmdListMan(void);

private:
    core::MemoryArenaBase* arena_;
    ID3D12Device* pDevice_;
    CommandListManger& cmdListMan_;
    DescriptorAllocatorPool& pool_;
    LinearAllocatorManager& linAllocMan_;

    core::CriticalSection cs_;
    core::Array<CommandContext*> contextPool_[COMMAND_LIST_TYPE_NUM];
    core::Fifo<CommandContext*> availableContexts_[COMMAND_LIST_TYPE_NUM];
};

class CommandContext
{
    X_NO_COPY(CommandContext);
    X_NO_ASSIGN(CommandContext);

    // buffer some resource barries untill we perform a operation that needs them flushed or we reach this count.
    static const size_t MAX_DISTRIPTOR_HEAP_TYPES = 2;
    static const size_t RESOURCE_BARRIER_BUF = 16;
    static const uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES;

public:
    CommandContext(ContextManager& contexMan, core::MemoryArenaBase* arena, ID3D12Device* pDevice,
        DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandContext(void);

    X_INLINE D3D12_COMMAND_LIST_TYPE getType(void) const;

    X_INLINE GraphicsContext& getGraphicsContext(void);
    X_INLINE ComputeContext& getComputeContext(void);

    // Flush existing commands to the GPU but keep the context alive
    uint64_t flush(bool waitForCompletion = false);
    // Flush existing commands and release the current context
    uint64_t finishAndFree(bool waitForCompletion = false);

    // Prepare to render by reserving a command list and command allocator
    void initialize(void);
    void reset(void);

    void copyBuffer(GpuResource& dest, GpuResource& src);
    void copyBufferRegion(GpuResource& dest, size_t destOffset, GpuResource& src, size_t srcOffset, size_t numBytes);
    void copySubresource(GpuResource& dest, uint32_t destSubIndex, GpuResource& src, uint32_t srcSubIndex);

    void readbackTexture2D(GpuResource& readbackBuffer, GpuResource& src, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footPrint);

    X_INLINE void copyBufferRegionRaw(GpuResource& dest, size_t destOffset, ID3D12Resource* pSrc, size_t srcOffset, size_t numBytes);
    X_INLINE void copyResourceRaw(GpuResource& dest, ID3D12Resource* pSrc);

    X_INLINE DynAlloc AllocUploadBuffer(size_t sizeInBytes);

    // used for compute shaders
    void copyCounter(GpuResource& dest, size_t destOffset, StructuredBuffer& src);
    void resetCounter(StructuredBuffer& buf, uint32_t value = 0);

    void writeBuffer(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes);
    void writeBufferUnAligned(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes);
    void fillBuffer(GpuResource& dest, size_t destOffset, Param val, size_t numBytes);

    void transitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
    void beginResourceTransition(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
    void insertUAVBarrier(GpuResource& resource, bool flushImmediate = false);
    void insertAliasBarrier(GpuResource& before, GpuResource& after, bool flushImmediate = false);

    X_INLINE void insertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx);
    X_INLINE void resolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t numQueries);
    void pIXBeginEvent(const wchar_t* pLabel);
    void pIXEndEvent(void);
    void pIXSetMarker(const wchar_t* pLabel);

    X_INLINE void setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeapPtr);
    void setDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE* pType, ID3D12DescriptorHeap** pHeapPtrs);
    X_INLINE void setPredication(ID3D12Resource* pBuffer, uint64_t bufferOffset, D3D12_PREDICATION_OP op);

    X_INLINE void flushResourceBarriers(void);

    template<uint32_t maxSubresources>
    X_INLINE uint64_t updateSubresources(ID3D12Device* pDevice, GpuResource& dest, ID3D12Resource* pIntermediate,
        uint64_t intermediateOffset, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSrcData);

protected:
    void bindDescriptorHeaps(void);

    uint64_t updateSubresources(GpuResource& dest, ID3D12Resource* pIntermediate,
        uint32_t firstSubresource, uint32_t numSubresources, uint64_t requiredSize,
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
        const uint32_t* pNumRows, const uint64_t* pRowSizesInBytes, const D3D12_SUBRESOURCE_DATA* pSrcData);

protected:
    ContextManager& contextManager_;

    ID3D12GraphicsCommandList* pCommandList_;
    ID3D12CommandAllocator* pCurrentAllocator_;

    // graphics
    ID3D12RootSignature* pCurGraphicsRootSignature_;
    ID3D12PipelineState* pCurGraphicsPipelineState_;
    // compute
    ID3D12RootSignature* pCurComputeRootSignature_;
    ID3D12PipelineState* pCurComputePipelineState_;

    DynamicDescriptorHeap dynamicDescriptorHeap_;
    DynamicDescriptorHeap dynamicSamplerDescriptorHeap_;

    LinearAllocator cpuLinearAllocator_;
    LinearAllocator gpuLinearAllocator_;

    uint32_t numBarriersToFlush_;
    D3D12_RESOURCE_BARRIER resourceBarrierBuffer[RESOURCE_BARRIER_BUF];

    // we only allow: SBC_SRV_UAV & SAMPLER
    ID3D12DescriptorHeap* pCurrentDescriptorHeaps_[MAX_DISTRIPTOR_HEAP_TYPES];

    D3D12_COMMAND_LIST_TYPE type_;

#if X_DEBUG
    int32_t contexFreed_;
#endif // !X_DEBUG
};

class GraphicsContext : public CommandContext
{
public:
    //	GraphicsContext();
    ~GraphicsContext() X_OVERRIDE;

    void clearUAV(GpuBuffer& target);
    void clearUAV(ColorBuffer& target);
    void clearColor(const ColorBuffer& target);
    void clearDepth(const DepthBuffer& target);
    void clearStencil(const DepthBuffer& target);
    void clearDepthAndStencil(const DepthBuffer& target);

    X_INLINE void beginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
    X_INLINE void endQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
    X_INLINE void resolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex,
        uint32_t numQueries, ID3D12Resource* pDestinationBuffer, uint64_t destinationBufferOffset);

    void setRootSignature(const RootSignature& rootSig);

    X_INLINE void setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs);
    X_INLINE void setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs, D3D12_CPU_DESCRIPTOR_HANDLE DSV);
    X_INLINE void setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV);
    X_INLINE void setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV);
    X_INLINE void setDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV);

    X_INLINE void setViewport(const XViewPort& vp);
    X_INLINE void setViewport(const D3D12_VIEWPORT& vp);
    X_INLINE void setViewport(float32_t x, float32_t y, float32_t w, float32_t h, float32_t minDepth = 0.0f, float32_t maxDepth = 1.0f);
    X_INLINE void setScissor(const D3D12_RECT& rect);
    X_INLINE void setScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
    X_INLINE void setViewportAndScissor(const XViewPort& vp); // sets viewport and scissor to same.
    X_INLINE void setViewportAndScissor(const XViewPort& vp, const D3D12_RECT& scissorRect);
    X_INLINE void setViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
    X_INLINE void setViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    X_INLINE void setStencilRef(uint32_t stencilRef);
    X_INLINE void setBlendFactor(Color8u blendFactor);
    X_INLINE void setBlendFactor(const Colorf& blendFactor);
    X_INLINE void setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

    void setPipelineState(const GraphicsPSO& PSO);
    void setPipelineState(ID3D12PipelineState* pPso);
    X_INLINE void setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
    X_INLINE void setConstants(uint32_t rootIndex, Param X);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W);
    X_INLINE void setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
    void setDynamicCBV(uint32_t rootIndex, size_t BufferSize, const void* pBufferData);
    void setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset = 0);
    void setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset = 0);
    X_INLINE void setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

    X_INLINE void setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    X_INLINE void setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);

    X_INLINE void setDynamicSamplerDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
    X_INLINE void setDynamicSamplerDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);

    X_INLINE void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
    X_INLINE void setVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
    X_INLINE void setVertexBuffers(uint32_t startSlot, uint32_t count, const D3D12_VERTEX_BUFFER_VIEW* pVBViews);
    void setDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, const void* pVBData);
    void setDynamicIB(size_t indexCount, const uint16_t* pIBData);
    void setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);

    X_INLINE void draw(uint32_t vertexCount, uint32_t vertexStartOffset = 0);
    X_INLINE void drawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0);
    X_INLINE void drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
        uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
    X_INLINE void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
        int32_t baseVertexLocation, uint32_t startInstanceLocation);
    void drawIndirect(CommandSignature& drawIndirectCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:
};

class ComputeContext : public CommandContext
{
public:
    //	ComputeContext();
    ~ComputeContext() X_OVERRIDE;

    void clearUAV(GpuBuffer& target);
    void clearUAV(ColorBuffer& target);

    void setRootSignature(const RootSignature& rootSig);

    void setPipelineState(const ComputePSO& PSO);
    X_INLINE void setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
    X_INLINE void setConstants(uint32_t rootIndex, Param X);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z);
    X_INLINE void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W);
    void setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
    void setDynamicCBV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);
    void setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);
    void setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset = 0);
    void setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset = 0);
    X_INLINE void setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

    X_INLINE void dispatch(size_t groupCountX = 1, size_t groupCountY = 1, size_t groupCountZ = 1);
    X_INLINE void dispatch1D(size_t threadCountX, size_t groupSizeX = 64);
    X_INLINE void dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX = 8, size_t groupSizeY = 8);
    X_INLINE void dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ);
    void dispatchIndirect(CommandSignature& dispatchCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:
};

X_NAMESPACE_END

#include "CommandContex.inl"
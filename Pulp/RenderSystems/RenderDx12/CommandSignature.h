#pragma once

#ifndef X_CMD_SIG_H_
#define X_CMD_SIG_H_

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(render)

class RootSignature;

class IndirectParameter
{
public:
    X_INLINE IndirectParameter();

    X_INLINE void draw(void);
    X_INLINE void drawIndexed(void);
    X_INLINE void dispatch(void);
    X_INLINE void vertexBufferView(uint32_t slot);
    X_INLINE void indexBufferView(void);
    X_INLINE void constant(uint32_t rootParameterIndex, uint32_t destOffsetIn32BitValues,
        uint32_t num32BitValuesToSet);
    X_INLINE void constantBufferView(uint32_t rootParameterIndex);
    X_INLINE void shaderResourceView(uint32_t rootParameterIndex);
    X_INLINE void unorderedAccessView(uint32_t rootParameterIndex);

    X_INLINE D3D12_INDIRECT_ARGUMENT_TYPE getType(void) const;

protected:
    D3D12_INDIRECT_ARGUMENT_DESC indirectParam_;
};

class CommandSignature
{
public:
    CommandSignature(core::MemoryArenaBase* arena, size_t numParams = 0);
    ~CommandSignature();

    void clear(void); // keeps param memory.
    void free(void);
    void reset(size_t numParams);

    X_INLINE IndirectParameter& operator[](size_t idx);
    X_INLINE const IndirectParameter& operator[](size_t idx) const;

    void finalize(ID3D12Device* pDevice, const RootSignature* pRootSignature = nullptr);

    // only valid after finalize.
    X_INLINE ID3D12CommandSignature* getSignature(void) const;

protected:
    core::Array<IndirectParameter> params_;

    ID3D12CommandSignature* pSignature_;
};

X_NAMESPACE_END

#include "CommandSignature.inl"

#endif // !X_CMD_SIG_H_
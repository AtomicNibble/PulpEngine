#pragma once

X_NAMESPACE_BEGIN(render)

struct CD3DX12_RECT : public D3D12_RECT
{
    CD3DX12_RECT() = default;
    explicit CD3DX12_RECT(const D3D12_RECT& o) :
        D3D12_RECT(o)
    {
    }

    explicit CD3DX12_RECT(
        LONG Left,
        LONG Top,
        LONG Right,
        LONG Bottom)
    {
        left = Left;
        top = Top;
        right = Right;
        bottom = Bottom;
    }
    ~CD3DX12_RECT() = default;

    X_INLINE operator const D3D12_RECT&() const
    {
        return *this;
    }
};

//------------------------------------------------------------------------------------------------

struct CD3DX12_BOX : public D3D12_BOX
{
    CD3DX12_BOX() = default;
    explicit CD3DX12_BOX(const D3D12_BOX& o) :
        D3D12_BOX(o)
    {
    }
    explicit CD3DX12_BOX(
        LONG Left,
        LONG Right)
    {
        left = Left;
        top = 0;
        front = 0;
        right = Right;
        bottom = 1;
        back = 1;
    }
    explicit CD3DX12_BOX(
        LONG Left,
        LONG Top,
        LONG Right,
        LONG Bottom)
    {
        left = Left;
        top = Top;
        front = 0;
        right = Right;
        bottom = Bottom;
        back = 1;
    }
    explicit CD3DX12_BOX(
        LONG Left,
        LONG Top,
        LONG Front,
        LONG Right,
        LONG Bottom,
        LONG Back)
    {
        left = Left;
        top = Top;
        front = Front;
        right = Right;
        bottom = Bottom;
        back = Back;
    }
    ~CD3DX12_BOX() = default;

    X_INLINE operator const D3D12_BOX&() const
    {
        return *this;
    }
};

X_INLINE bool operator==(const D3D12_BOX& l, const D3D12_BOX& r)
{
    return l.left == r.left && l.top == r.top && l.front == r.front && l.right == r.right && l.bottom == r.bottom && l.back == r.back;
}

X_INLINE bool operator!=(const D3D12_BOX& l, const D3D12_BOX& r)
{
    return !(l == r);
}

//------------------------------------------------------------------------------------------------

struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION
{
    CD3DX12_TEXTURE_COPY_LOCATION() = default;

    explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION& o) :
        D3D12_TEXTURE_COPY_LOCATION(o)
    {
    }
    explicit CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes)
    {
        pResource = pRes;
    }
    CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& Footprint)
    {
        pResource = pRes;
        Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        PlacedFootprint = Footprint;
    }
    CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes, UINT Sub)
    {
        pResource = pRes;
        Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        SubresourceIndex = Sub;
    }
};

//------------------------------------------------------------------------------------------------

struct CD3D12_SHADER_BYTECODE : public D3D12_SHADER_BYTECODE
{
    CD3D12_SHADER_BYTECODE()
    {
    }

    explicit CD3D12_SHADER_BYTECODE(const D3D12_SHADER_BYTECODE& o) :
        D3D12_SHADER_BYTECODE(o)
    {
    }

    explicit CD3D12_SHADER_BYTECODE(const void* _pShaderBytecode, SIZE_T _BytecodeLength)
    {
        pShaderBytecode = _pShaderBytecode;
        BytecodeLength = _BytecodeLength;
    }

    ~CD3D12_SHADER_BYTECODE()
    {
    }

    operator const D3D12_SHADER_BYTECODE&() const
    {
        return *this;
    }
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
    CD3DX12_HEAP_PROPERTIES()
    {
    }

    explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES& o) :
        D3D12_HEAP_PROPERTIES(o)
    {
    }

    CD3DX12_HEAP_PROPERTIES(
        D3D12_CPU_PAGE_PROPERTY cpuPageProperty,
        D3D12_MEMORY_POOL memoryPoolPreference,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = D3D12_HEAP_TYPE_CUSTOM;
        CPUPageProperty = cpuPageProperty;
        MemoryPoolPreference = memoryPoolPreference;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }

    explicit CD3DX12_HEAP_PROPERTIES(
        D3D12_HEAP_TYPE type,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = type;
        CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }

    operator const D3D12_HEAP_PROPERTIES&() const
    {
        return *this;
    }

    bool IsCPUAccessible() const
    {
        return Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK || (Type == D3D12_HEAP_TYPE_CUSTOM && (CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
    }
};

X_INLINE bool operator==(const D3D12_HEAP_PROPERTIES& l, const D3D12_HEAP_PROPERTIES& r)
{
    return l.Type == r.Type && l.CPUPageProperty == r.CPUPageProperty && l.MemoryPoolPreference == r.MemoryPoolPreference && l.CreationNodeMask == r.CreationNodeMask && l.VisibleNodeMask == r.VisibleNodeMask;
}

X_INLINE bool operator!=(const D3D12_HEAP_PROPERTIES& l, const D3D12_HEAP_PROPERTIES& r)
{
    return !(l == r);
}

//------------------------------------------------------------------------------------------------

struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE()
    {
        ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& o) :
        D3D12_CPU_DESCRIPTOR_HANDLE(o)
    {
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize)
    {
        InitOffsetted(other, offsetScaledByIncrementSize);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        ptr += offsetInDescriptors * descriptorIncrementSize;
        return *this;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize)
    {
        ptr += offsetScaledByIncrementSize;
        return *this;
    }

    bool operator==(const D3D12_CPU_DESCRIPTOR_HANDLE& other) const
    {
        return (ptr == other.ptr);
    }

    bool operator!=(const D3D12_CPU_DESCRIPTOR_HANDLE& other) const
    {
        return (ptr != other.ptr);
    }
    CD3DX12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& other)
    {
        ptr = other.ptr;
        return *this;
    }

    inline void InitOffsetted(const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
    {
        InitOffsetted(*this, base, offsetScaledByIncrementSize);
    }

    inline void InitOffsetted(const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
    }

    static inline void InitOffsetted(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
    {
        handle.ptr = base.ptr + offsetScaledByIncrementSize;
    }

    static inline void InitOffsetted(D3D12_CPU_DESCRIPTOR_HANDLE& handle, const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        handle.ptr = base.ptr + offsetInDescriptors * descriptorIncrementSize;
    }
};

//------------------------------------------------------------------------------------------------

struct CD3DX12_GPU_DESCRIPTOR_HANDLE : public D3D12_GPU_DESCRIPTOR_HANDLE
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE()
    {
        ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }
    explicit CD3DX12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& o) :
        D3D12_GPU_DESCRIPTOR_HANDLE(o)
    {
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize)
    {
        InitOffsetted(other, offsetScaledByIncrementSize);
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        ptr += offsetInDescriptors * descriptorIncrementSize;
        return *this;
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize)
    {
        ptr += offsetScaledByIncrementSize;
        return *this;
    }

    inline bool operator==(const D3D12_GPU_DESCRIPTOR_HANDLE& other) const
    {
        return (ptr == other.ptr);
    }

    inline bool operator!=(const D3D12_GPU_DESCRIPTOR_HANDLE& other) const
    {
        return (ptr != other.ptr);
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE& operator=(const D3D12_GPU_DESCRIPTOR_HANDLE& other)
    {
        ptr = other.ptr;
        return *this;
    }

    inline void InitOffsetted(const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
    {
        InitOffsetted(*this, base, offsetScaledByIncrementSize);
    }

    inline void InitOffsetted(const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
    }

    static inline void InitOffsetted(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize)
    {
        handle.ptr = base.ptr + offsetScaledByIncrementSize;
    }

    static inline void InitOffsetted(D3D12_GPU_DESCRIPTOR_HANDLE& handle, const D3D12_GPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        handle.ptr = base.ptr + offsetInDescriptors * descriptorIncrementSize;
    }
};

X_NAMESPACE_END

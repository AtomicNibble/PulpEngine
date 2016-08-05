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

	X_INLINE operator const D3D12_RECT&() const { return *this; }
};

//------------------------------------------------------------------------------------------------

struct CD3DX12_BOX : public D3D12_BOX
{
	CD3DX12_BOX() = default;
	explicit CD3DX12_BOX(const D3D12_BOX& o) :
		D3D12_BOX(o)
	{}
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

	X_INLINE operator const D3D12_BOX&() const { return *this; }
};

X_INLINE bool operator==(const D3D12_BOX& l, const D3D12_BOX& r)
{
	return l.left == r.left && l.top == r.top && l.front == r.front &&
		l.right == r.right && l.bottom == r.bottom && l.back == r.back;
}

X_INLINE bool operator!=(const D3D12_BOX& l, const D3D12_BOX& r)
{
	return !(l == r);
}


//------------------------------------------------------------------------------------------------

struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION
{
	CD3DX12_TEXTURE_COPY_LOCATION() = default;

	explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION &o) :
		D3D12_TEXTURE_COPY_LOCATION(o)
	{}
	explicit CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource* pRes) { pResource = pRes; }
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

	operator const D3D12_SHADER_BYTECODE&() const {
		return *this;
	}
};

X_NAMESPACE_END

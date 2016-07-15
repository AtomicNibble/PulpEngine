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



X_NAMESPACE_END

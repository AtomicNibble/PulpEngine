#pragma once


#ifndef X_DX10_DEVICE_MAN_H_
#define X_DX10_DEVICE_MAN_H_

X_NAMESPACE_BEGIN(texture)

// a texture loaded to the GPU
class XDeviceTexture
{
public:
	XDeviceTexture() : pD3DTexture(nullptr) {}

	ID3D11Resource *getBaseTexture() {
		return pD3DTexture;
	}
	ID3D11Texture2D* get2DTexture() { return (ID3D11Texture2D*)getBaseTexture(); }
	ID3D11Texture2D *getCubeTexture() { return (ID3D11Texture2D*)getBaseTexture(); }
	ID3D11Texture3D *getVolumeTexture() { return (ID3D11Texture3D*)getBaseTexture(); }

	int32 Release() {
		int32 res = 0;
		if (pD3DTexture) {
			res = pD3DTexture->Release();
			pD3DTexture = nullptr;
		}
		return res;
	}

	void setTexture(ID3D11Resource* pD3DTex) {
		pD3DTexture = pD3DTex;
	}

private:
	ID3D11Resource* pD3DTexture;
};

X_NAMESPACE_END

#endif // !X_DX10_DEVICE_MAN_H_
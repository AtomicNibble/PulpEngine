#include "stdafx.h"



#include "ITexture.h"
#include "IFileSys.h"


// hum how to include the loader code.
// it's part of common Render dll.
// Umm. #yolo

#include <../../tools/ImgLib/ImgLib.h>

X_LINK_LIB("engine_ImgLib")


X_USING_NAMESPACE;

using namespace core;
using namespace texture;

template<class T>
bool LoadValid(Texturefmt::Enum fmt, core::Path<char> path)
{
	IFileSys::fileModeFlags mode;
	mode.Set(IFileSys::fileMode::READ);
	mode.Set(IFileSys::fileMode::RANDOM_ACCESS);


	core::XFileScoped file;
	
	core::Path<char> testFolder("images/");
	testFolder /= path;

	if (!file.openFile(testFolder.c_str(), mode))
	{
		return false;
	}

	T loader;

	const bool canLoad = loader.canLoadFile(path);

	EXPECT_TRUE(canLoad);
	if (!canLoad)
	{
		return false;
	}

	texture::XTextureFile texFile(g_arena);

	if (!loader.loadTexture(file.GetFile(), texFile, g_arena)) {
		X_ERROR("UT", "Failed to load texture");
		return false;
	}

	const bool isValid = texFile.isValid();

	if (!isValid) {
		X_ERROR("UT", "TextureFile is not valid");
		return false;
	}

	EXPECT_EQ(fmt, texFile.getFormat());
	if (fmt != texFile.getFormat()) {
		X_ERROR("UT", "ReturnFmt: %s ExpectedFmt: %s Path<char>: %s",
			Texturefmt::ToString(texFile.getFormat()),
			Texturefmt::ToString(fmt),
			testFolder.c_str());

		return false;
	}

	return true;
}


TEST(DDS, BC7)
{
//	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC7_UNORM, Path<char>("bc7_froth2_d.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC7, Path<char>("bc7_mainudun01_p.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC7, Path<char>("bc7_softsquar01_p.dds")));
}

TEST(DDS, ati2)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::ATI2, Path<char>("test_img_load_ati2.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::ATI2, Path<char>("test_img_load_ati2_nomips.dds")));
}

TEST(DDS, DISABLED_A8R8G8B8)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::A8R8G8B8, Path<char>("test_img_load_A8R8G8B8.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::A8R8G8B8, Path<char>("test_img_load_A8R8G8B8_nomips.dds")));
}

TEST(DDS, B8G8R8A8)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::B8G8R8A8, Path<char>("test_img_load_B8G8R8A8.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::B8G8R8A8, Path<char>("test_img_load_B8G8R8A8_nomips.dds")));
}

TEST(DDS, DISABLED_R8B8G8)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::R8G8B8, Path<char>("test_img_load_R8G8B8.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::R8G8B8, Path<char>("test_img_load_R8G8B8_nomips.dds")));
}

TEST(DDS, dxt1)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC1, Path<char>("test_img_load_dxt1.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC1, Path<char>("test_img_load_dxt1_nomips.dds")));
}

TEST(DDS, dxt3)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC2, Path<char>("test_img_load_dxt3.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC2, Path<char>("test_img_load_dxt3_nomips.dds")));
}

TEST(DDS, dxt5)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC3, Path<char>("test_img_load_dxt5.dds")));
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC3, Path<char>("test_img_load_dxt5_nomips.dds")));
}


TEST(DDS, Cube)
{
	EXPECT_TRUE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::B8G8R8A8, Path<char>("skybox_sky_cube.dds")));
}


TEST(DDS, LoadBroken)
{
	X_LOG0("DDS", "Expect incorrect mip map count");
	EXPECT_FALSE(LoadValid<DDS::XTexLoaderDDS>(Texturefmt::BC3, Path<char>("test_img_load_dxt5_bad_header.dds")));
}

TEST(TGA, Load)
{
	EXPECT_TRUE(LoadValid<TGA::XTexLoaderTGA>(Texturefmt::B8G8R8A8, Path<char>("test_img_load_32.tga")));
	EXPECT_TRUE(LoadValid<TGA::XTexLoaderTGA>(Texturefmt::B8G8R8A8, Path<char>("test_img_load_32_rle.tga")));
}

TEST(JPG, Load)
{
	EXPECT_TRUE(LoadValid<JPG::XTexLoaderJPG>(Texturefmt::R8G8B8, Path<char>("test_img_load.jpg")));
}

TEST(PNG, Load)
{
	EXPECT_TRUE(LoadValid<PNG::XTexLoaderPNG>(Texturefmt::R8G8B8A8, Path<char>("test_img_load_32.png")));
	EXPECT_TRUE(LoadValid<PNG::XTexLoaderPNG>(Texturefmt::R8G8B8, Path<char>("test_img_load_24.png")));
}

TEST(PSD, Load)
{
	EXPECT_TRUE(LoadValid<PSD::XTexLoaderPSD>(Texturefmt::A8R8G8B8, Path<char>("test_img_load.psd")));
}


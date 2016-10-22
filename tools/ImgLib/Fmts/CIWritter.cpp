#include "stdafx.h"
#include "TextureLoaderCI.h"
#include "TextureFile.h"
#include "Util\TextureUtil.h"

#include <IFileSys.h>
#include <ITexture.h>
#include <ICi.h>


#include "Hashing\crc32.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{

	bool XTexLoaderCI::saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
	{
		X_UNUSED(swapArena);

		core::Crc32* pCrc = gEnv->pCore->GetCrc32();

		X_ASSERT_NOT_NULL(pCrc);

		CITexureHeader hdr;
		hdr.fourCC = CI_FOURCC;
		hdr.version = CI_VERSION;
		hdr.format = imgFile.getFormat();
		hdr.Mips = imgFile.getNumMips();
		hdr.Faces = imgFile.getNumFaces();
		hdr.Flags = imgFile.getFlags();
		hdr.Flags.Set(TexFlag::CI_IMG);

		hdr.width = safe_static_cast<uint16_t, int32_t>(imgFile.getWidth());
		hdr.height = safe_static_cast<uint16_t, int32_t>(imgFile.getHeight());

		core::zero_object(hdr.__Unused);

		hdr.FaceSize = Util::dataSize(imgFile.getWidth(), imgFile.getHeight(), hdr.Mips, hdr.format);
		hdr.DataSize = hdr.FaceSize * imgFile.getNumFaces();
		hdr.crc32 = pCrc->Begin();

		X_ASSERT(hdr.FaceSize == imgFile.getFaceSize(), "Face size mismatch")(hdr.FaceSize, imgFile.getFaceSize());

		pCrc->Update(&hdr.Flags, sizeof(hdr.Flags), hdr.crc32);

		// crc
		for (size_t i = 0; i < hdr.Faces; i++)
		{
			pCrc->Update(imgFile.getFace(i), hdr.FaceSize, hdr.crc32);
		}

		hdr.crc32 = pCrc->Finish(hdr.crc32);

		file->writeObj(hdr);

		// write each face.
		for (size_t i = 0; i < hdr.Faces; i++)
		{
			file->write(imgFile.getFace(i), hdr.FaceSize);
		}

		return true;
	}

}

X_NAMESPACE_END
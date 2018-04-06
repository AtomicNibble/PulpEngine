#include "stdafx.h"
#include "CI_Img.h"

#include <IFileSys.h>
#include <ITexture.h>
#include <ICi.h>

#include "Hashing\crc32.h"

#include "XTexture.h"

#include "Threading\JobSystem.h"
#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
    namespace
    {
        struct JobData
        {
            JobData(core::Path<char>& path_, core::ReferenceCountedOwner<XTextureFile>& image_, core::MemoryArenaBase* arena_) :
                path(path_),
                image(image_),
                arena(arena_)
            {
            }

            void release(void)
            {
                X_DELETE(this, arena);
            }

            core::Path<char> path;
            core::ReferenceCountedOwner<XTextureFile> image;
            core::MemoryArenaBase* arena;
        };

        X_DECLARE_JOB_ENTRY(WriteCIJob)
        {
            X_UNUSED(workerIdx);

            JobData* pData = reinterpret_cast<JobData*>(pParam);

            WriteCIImg(pData->path, pData->image.instance());

            X_LOG2("Ci", "created: ^5%s", pData->path.c_str());

            pData->release();
        }

    } // namespace

    bool WriteCIImgAsync(core::Path<char>& path, core::ReferenceCountedOwner<XTextureFile>& image, core::MemoryArenaBase* arena)
    {
        JobData* pData = X_NEW(JobData, arena, "CIJobData")(path, image, arena);

        X_UNUSED(pData);
        X_ASSERT_NOT_IMPLEMENTED();
        //		core::JobDecl job(WriteCIJob, pData);
        //		gEnv->pJobSys->AddJob(job, core::JobPriority::NONE);
        return true;
    }

    bool WriteCIImg(core::Path<char>& path, XTextureFile* image)
    {
        X_ASSERT_NOT_NULL(image);
        core::XFileScoped file;
        core::fileModeFlags mode = core::fileModeFlags::WRITE | core::fileModeFlags::RECREATE;

        CITexureHeader hdr;

        path.setExtension(CI_FILE_EXTENSION);

        gEnv->pFileSys->createDirectoryTree(path.c_str());

        core::Crc32* pCrc = gEnv->pCore->GetCrc32();

        if (file.openFile(path.c_str(), mode)) {
            hdr.fourCC = CI_FOURCC;
            hdr.version = CI_VERSION;
            hdr.format = image->getFormat();
            hdr.Mips = image->getNumMips();
            hdr.Faces = image->getNumFaces();
            hdr.Flags = image->getFlags();

            hdr.width = safe_static_cast<uint16_t, int>(image->getWidth());
            hdr.height = safe_static_cast<uint16_t, int>(image->getHeight());

            core::zero_object(hdr.__Unused);

            hdr.DataSize = XTexture::get_data_size(image->getWidth(), image->getHeight(),
                image->getNumFaces(), hdr.Mips, hdr.format);

            hdr.FaceSize = XTexture::get_data_size(image->getWidth(), image->getHeight(),
                1, hdr.Mips, hdr.format);

            hdr.crc32 = pCrc->Begin();

            pCrc->Update(&hdr.Flags, sizeof(hdr.Flags), hdr.crc32);

            // crc
            for (size_t i = 0; i < hdr.Faces; i++) {
                pCrc->Update(image->pFaces[i], hdr.FaceSize, hdr.crc32);
            }

            hdr.crc32 = pCrc->Finish(hdr.crc32);

            file.writeObj(hdr);

            // write each face.
            for (size_t i = 0; i < hdr.Faces; i++) {
                file.write(image->pFaces[i], hdr.FaceSize);
            }
        }
        return false;
    }

} // namespace CI

X_NAMESPACE_END
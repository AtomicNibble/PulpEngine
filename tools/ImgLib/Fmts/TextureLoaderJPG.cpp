#include "stdafx.h"
#include "TextureLoaderJPG.h"

#include <IFileSys.h>

// #include "jpeg-6b\jpeglib.h"
#include <../../3rdparty/source/jpeg-6b/jpeglib.h>

#include "TextureFile.h"

#if X_DEBUG
X_LINK_LIB("jpegd");
#else
X_LINK_LIB("jpeg");
#endif // !X_DEBUG

X_NAMESPACE_BEGIN(texture)
X_DISABLE_WARNING(4611)
X_DISABLE_WARNING(4324)

namespace JPG
{
    namespace
    {
        static const char* JPG_FILE_EXTENSION = ".jpg";
        static const size_t JPEG_MGR_BUFFER_SIZE = (1 << 11);

        struct jpeg_xfile_src_mgr
        {
            jpeg_source_mgr mgr;
            uint32_t bytes_read;
            core::XFile* file;
            unsigned char buffer[JPEG_MGR_BUFFER_SIZE];
        };

        static void xfile_init_source(j_decompress_ptr cinfo)
        {
            jpeg_xfile_src_mgr* src = reinterpret_cast<jpeg_xfile_src_mgr*>(cinfo->src);
            X_UNUSED(src);
            // files already open since engine filesystem
            // will only give back a file object when it's valid and open.
        }

        static boolean xfile_fill_input_buffer(j_decompress_ptr cinfo)
        {
            jpeg_xfile_src_mgr* src = reinterpret_cast<jpeg_xfile_src_mgr*>(cinfo->src);

            size_t bytes_read;
            bytes_read = src->file->read(src->buffer, JPEG_MGR_BUFFER_SIZE);

            src->mgr.next_input_byte = src->buffer;
            src->mgr.bytes_in_buffer = bytes_read;
            if (0 == src->mgr.bytes_in_buffer) {
                /* The image file is truncated. We insert EOI marker to tell the library to stop processing. */
                src->buffer[0] = (JOCTET)0xFF;
                src->buffer[1] = (JOCTET)JPEG_EOI;
                src->mgr.bytes_in_buffer = 2;
            }
            return TRUE;
        }

        static void xfile_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
        {
            jpeg_xfile_src_mgr* src = reinterpret_cast<jpeg_xfile_src_mgr*>(cinfo->src);

            if (num_bytes <= 0) {
                return;
            }

            if (num_bytes <= safe_static_cast<long, size_t>(src->mgr.bytes_in_buffer)) {
                src->mgr.next_input_byte += static_cast<uintptr_t>(num_bytes);
                src->mgr.bytes_in_buffer -= static_cast<size_t>(num_bytes);
            }
            else {
                size_t skip = num_bytes - src->mgr.bytes_in_buffer;

                cinfo->src->bytes_in_buffer = 0;
                src->file->seek(skip, core::SeekMode::CUR);
            }
        }

        static void xfile_term_source(j_decompress_ptr cinfo)
        {
            X_UNUSED(cinfo);
            // we don't close the file handle.
            // since this reader is called with the open file.
            // code that calls it is responsible for closing the file.
        }

        static void init_file_stream(j_decompress_ptr cinfo, jpeg_xfile_src_mgr* src, core::XFile* file)
        {
            src->file = file;
            src->bytes_read = 0;
            src->mgr.init_source = xfile_init_source;
            src->mgr.fill_input_buffer = xfile_fill_input_buffer;
            src->mgr.skip_input_data = xfile_skip_input_data;
            src->mgr.resync_to_restart = jpeg_resync_to_restart;
            src->mgr.term_source = xfile_term_source;
            src->mgr.bytes_in_buffer = 0;
            src->mgr.next_input_byte = nullptr;
            cinfo->src = reinterpret_cast<jpeg_source_mgr*>(src);
        }

        X_ALIGNED_SYMBOL(struct my_error_mgr, 16)
        {
            struct jpeg_error_mgr pub; 

           X_ALIGNED_SYMBOL(jmp_buf setjmp_buffer, 16); 
        };

        typedef struct my_error_mgr* my_error_ptr;

        static void my_error_exit(j_common_ptr cinfo)
        {
            my_error_ptr myerr = (my_error_ptr)cinfo->err;

            char jpeg_last_error[JMSG_LENGTH_MAX];
            (*cinfo->err->format_message)(cinfo, jpeg_last_error);

            X_ERROR("TextureJPG", jpeg_last_error);

            /* Return control to the setjmp point */
            longjmp(myerr->setjmp_buffer, 1);
        }

        static void output_message(j_common_ptr cinfo)
        {
            char jpeg_msg[JMSG_LENGTH_MAX];

            (*cinfo->err->format_message)(cinfo, jpeg_msg);

            X_ERROR("TextureJPG", jpeg_msg);
        }
    } // namespace

    const char* XTexLoaderJPG::EXTENSION = JPG_FILE_EXTENSION;

    XTexLoaderJPG::XTexLoaderJPG()
    {
    }

    XTexLoaderJPG::~XTexLoaderJPG()
    {
    }

    bool XTexLoaderJPG::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < 4) {
            return false;
        }

        const uint16_t* pData = reinterpret_cast<const uint16_t*>(fileData.data());

        return pData[0] == 0xd8ff && pData[1] == 0xe0ff;
    }

    // ITextureFmt
    const char* XTexLoaderJPG::getExtension(void) const
    {
        return JPG_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderJPG::getSrcFmt(void) const
    {
        return ImgFileFormat::JPG;
    }

    bool XTexLoaderJPG::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(JPG_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderJPG::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderJPG::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_ASSERT_NOT_NULL(file);
        X_UNUSED(swapArena);

        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;
        struct jpeg_xfile_src_mgr file_reader;

        core::zero_object(cinfo);

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        jerr.pub.output_message = output_message;

        X_ASSERT_ALIGNMENT(jerr.setjmp_buffer, 16, 0);

        if (setjmp(jerr.setjmp_buffer)) {
            X_ERROR("TextureJPG", "Failed to process jpg");

            jpeg_destroy_decompress(&cinfo);
            return false;
        }

        jpeg_create_decompress(&cinfo);

        init_file_stream(&cinfo, &file_reader, file);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        // check we can load this.
        if (cinfo.output_height < 1 || cinfo.output_height > TEX_MAX_DIMENSIONS || cinfo.output_width < 1 || cinfo.output_width > TEX_MAX_DIMENSIONS) {
            X_ERROR("TextureJPG", "invalid image dimensions. provided: %ix%i max: %ix%i",
                cinfo.output_height, cinfo.output_width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);

            longjmp(jerr.setjmp_buffer, 1);
        }

        if (!core::bitUtil::IsPowerOfTwo(cinfo.output_height) || !core::bitUtil::IsPowerOfTwo(cinfo.output_width)) {
            X_ERROR("TextureJPG", "invalid image dimensions, must be power of two. provided: %ix%i",
                cinfo.output_height, cinfo.output_width);

            longjmp(jerr.setjmp_buffer, 1);
        }

        uint32_t inflated_size = cinfo.output_width * cinfo.output_height * cinfo.num_components;
        uint32_t row_stride = cinfo.output_width * cinfo.output_components;

        X_UNUSED(inflated_size);

        if (cinfo.out_color_space != JCS_RGB) {
            X_ERROR("TextureJPG", "invalid colorspace, must be RGB");
            longjmp(jerr.setjmp_buffer, 1);
        }

        // just check incase.
        if (cinfo.output_components != 3) {
            X_ERROR("TextureJPG", "invalid output_components. provided: %i expected: 3", cinfo.output_components);
            longjmp(jerr.setjmp_buffer, 1);
        }

        // create the img obj.
        TextureFlags flags;
        flags.Set(TextureFlags::NOMIPS);

        imgFile.setNumFaces(1);
        imgFile.setNumMips(1);
        imgFile.setDepth(1);
        imgFile.setFlags(flags);
        imgFile.setFormat(Texturefmt::R8G8B8);
        imgFile.setType(TextureType::T2D);
        imgFile.setHeigth(safe_static_cast<uint16_t, uint32_t>(cinfo.output_height));
        imgFile.setWidth(safe_static_cast<uint16_t, uint32_t>(cinfo.output_width));
        imgFile.resize();

        uint8_t* pBuffer = imgFile.getFace(0);
        uint8_t* pEnd = pBuffer + imgFile.getFaceSize();

        while (cinfo.output_scanline < cinfo.output_height) {

            uint8_t* rowpointer[1];
            rowpointer[0] = pBuffer;

            jpeg_read_scanlines(&cinfo, rowpointer, 1);

            pBuffer += row_stride;
        }

        X_ASSERT((pEnd - pBuffer) == 0, "Faield to decode jpg")(pEnd, pBuffer, (pEnd - pBuffer));

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

#if X_DEBUG == 1
        uint64_t left = file->remainingBytes();
        X_WARNING_IF(left > 0, "TextureJPG", "potential read fail, bytes left in file: %i", left);
#endif

        return true;
    }
    // ~ITextureFmt

} // namespace JPG

X_NAMESPACE_END
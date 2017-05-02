/*
   LZ5 - Fast LZ compression algorithm
   Header File
   Copyright (C) 2011-2016, Yann Collet.
   Copyright (C) 2016, Przemyslaw Skibinski <inikep@gmail.com>

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
    - LZ5 source repository : https://github.com/inikep/lz5
*/
#ifndef LZ5_DECOMPRESS_H_2983
#define LZ5_DECOMPRESS_H_2983

#if defined (__cplusplus)
extern "C" {
#endif

#include "entropy/mem.h"     /* U32 */


/*^***************************************************************
*  Export parameters
*****************************************************************/
/*
*  LZ5_DLL_EXPORT :
*  Enable exporting of functions when building a Windows DLL
*/
#if defined(LZ5_DLL_EXPORT) && (LZ5_DLL_EXPORT==1)
#  define LZ5DLIB_API __declspec(dllexport)
#elif defined(LZ5_DLL_IMPORT) && (LZ5_DLL_IMPORT==1)
#  define LZ5DLIB_API __declspec(dllimport) /* It isn't required but allows to generate better code, saving a function pointer load from the IAT and an indirect jump.*/
#else
#  define LZ5DLIB_API
#endif


/*-************************************
*  Simple Functions
**************************************/

/*
LZ5_decompress_safe() :
    compressedSize : is the precise full size of the compressed block.
    maxDecompressedSize : is the size of destination buffer, which must be already allocated.
    return : the number of bytes decompressed into destination buffer (necessarily <= maxDecompressedSize)
             If destination buffer is not large enough, decoding will stop and output an error code (<0).
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             This function is protected against buffer overflow exploits, including malicious data packets.
             It never writes outside output buffer, nor reads outside input buffer.
*/
LZ5DLIB_API int LZ5_decompress_safe (const char* source, char* dest, int compressedSize, int maxDecompressedSize);



/*!
LZ5_decompress_safe_partial() :
    This function decompress a compressed block of size 'compressedSize' at position 'source'
    into destination buffer 'dest' of size 'maxDecompressedSize'.
    The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
    reducing decompression time.
    return : the number of bytes decoded in the destination buffer (necessarily <= maxDecompressedSize)
       Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
             Always control how many bytes were decoded.
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
*/
LZ5DLIB_API int LZ5_decompress_safe_partial (const char* source, char* dest, int compressedSize, int targetOutputSize, int maxDecompressedSize);



/*-**********************************************
*  Streaming Decompression Functions
************************************************/
typedef struct {
    const BYTE* externalDict;
    size_t extDictSize;
    const BYTE* prefixEnd;
    size_t prefixSize;
} LZ5_streamDecode_t;

/*
 * LZ5_streamDecode_t
 * information structure to track an LZ5 stream.
 * init this structure content using LZ5_setStreamDecode or memset() before first use !
 *
 * In the context of a DLL (liblz5) please prefer usage of construction methods below.
 * They are more future proof, in case of a change of LZ5_streamDecode_t size in the future.
 * LZ5_createStreamDecode will allocate and initialize an LZ5_streamDecode_t structure
 * LZ5_freeStreamDecode releases its memory.
 */
LZ5DLIB_API LZ5_streamDecode_t* LZ5_createStreamDecode(void);
LZ5DLIB_API int                 LZ5_freeStreamDecode (LZ5_streamDecode_t* LZ5_stream);

/*! LZ5_setStreamDecode() :
 *  Use this function to instruct where to find the dictionary.
 *  Setting a size of 0 is allowed (same effect as reset).
 *  @return : 1 if OK, 0 if error
 */
LZ5DLIB_API int LZ5_setStreamDecode (LZ5_streamDecode_t* LZ5_streamDecode, const char* dictionary, int dictSize);

/*
*_continue() :
    These decoding functions allow decompression of multiple blocks in "streaming" mode.
    Previously decoded blocks *must* remain available at the memory position where they were decoded (up to LZ5_DICT_SIZE)
    In the case of a ring buffers, decoding buffer must be either :
    - Exactly same size as encoding buffer, with same update rule (block boundaries at same positions)
      In which case, the decoding & encoding ring buffer can have any size, including small ones ( < LZ5_DICT_SIZE).
    - Larger than encoding buffer, by a minimum of maxBlockSize more bytes.
      maxBlockSize is implementation dependent. It's the maximum size you intend to compress into a single block.
      In which case, encoding and decoding buffers do not need to be synchronized,
      and encoding ring buffer can have any size, including small ones ( < LZ5_DICT_SIZE).
    - _At least_ LZ5_DICT_SIZE + 8 bytes + maxBlockSize.
      In which case, encoding and decoding buffers do not need to be synchronized,
      and encoding ring buffer can have any size, including larger than decoding buffer.
    Whenever these conditions are not possible, save the last LZ5_DICT_SIZE of decoded data into a safe buffer,
    and indicate where it is saved using LZ5_setStreamDecode()
*/
LZ5DLIB_API int LZ5_decompress_safe_continue (LZ5_streamDecode_t* LZ5_streamDecode, const char* source, char* dest, int compressedSize, int maxDecompressedSize);


/*
Advanced decoding functions :
*_usingDict() :
    These decoding functions work the same as
    a combination of LZ5_setStreamDecode() followed by LZ5_decompress_x_continue()
    They are stand-alone. They don't need nor update an LZ5_streamDecode_t structure.
*/
LZ5DLIB_API int LZ5_decompress_safe_usingDict (const char* source, char* dest, int compressedSize, int maxDecompressedSize, const char* dictStart, int dictSize);


#if defined (__cplusplus)
}
#endif

#endif /* LZ5_DECOMPRESS_H_2983827168210 */

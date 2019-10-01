/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

namespace CompressionCodec
{

    enum class Codec : uint8
    {
        INVALID = static_cast<uint8>(-1),
        ZLIB = 0,
        ZSTD,
        LZ4,
        NUM_CODECS
    };

    static const Codec s_AllCodecs[] = { Codec::ZLIB, Codec::ZSTD, Codec::LZ4 };

    static inline bool CheckMagic(const void* pCompressedData, const uint32 magicNumber, const uint32 magicSkippable)
    {
        
        uint32 compressedMagic = 0;
        //If the address is 4bytes aligned, then it is safe to dereference as a 4byte integral.
        if ((reinterpret_cast<uintptr_t>(pCompressedData) & 0x3) == 0)
        {
            compressedMagic = *reinterpret_cast<const uint32*>(pCompressedData);
        }
        else
        {
            //We should read one byte at a time to avoid alignment issues.
            const uint8* pCompressedBytes = static_cast<const uint8*>(pCompressedData);
            compressedMagic = (uint32)pCompressedBytes[0]         | ((uint32)pCompressedBytes[1] << 8) |
                              ((uint32)pCompressedBytes[2] << 16) | ((uint32)pCompressedBytes[3] << 24);
        }
#if defined(AZ_BIG_ENDIAN)
        //Because LZ4 magic is always stored as LE we need to swap.
        compressedMagic = AZStd::endian_swap(compressedMagic);
#endif
        if (compressedMagic == magicNumber)
        {
            return true;
        }
        if ((compressedMagic & 0xFFFFFFF0) == magicSkippable)
        {
            return true;
        }
        return false;
    }


    static inline bool TestForLZ4Magic(const void* pCompressedData)
    {
        constexpr uint32 lz4MagicNumber = 0x184D2204;
        constexpr uint32 lz4MagicSkippable = 0x184D2A50;
        return CheckMagic(pCompressedData, lz4MagicNumber, lz4MagicSkippable);
    }

    static inline bool TestForZSTDMagic(const void* pCompressedData)
    {
        constexpr uint32 zstdMagicNumber = 0xFD2FB528;
        constexpr uint32 zstdMagicSkippable = 0x184D2A50; //Same as LZ4
        return CheckMagic(pCompressedData, zstdMagicNumber, zstdMagicSkippable);
    }

};

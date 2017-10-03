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
#ifndef GM_COMPRESSOR_INTERFACE_H
#define GM_COMPRESSOR_INTERFACE_H

#include <AzCore/PlatformDef.h>
#include <AzCore/std/functional.h>

namespace GridMate
{
    /**
    * Collection of compression related error codes
    */
    enum class CompressorError
    {
        Ok,                   ///< No error, operation finished successfully
        InsufficientBuffer,   ///< Buffer size is insufficient for the operation to complete, increase the size and try again
        CorruptData           ///< Malformed or hacked packet, potentially security issue
    };

    /*
    * Unique identifier of a given compressor
    */
    using CompressorType = AZ::u32;

    /**
     * Packet data compressor interface
     */
    class Compressor
    {
    public:
        virtual ~Compressor() AZ_DEFAULT_METHOD;

        /*
        * Initialize compressor
        */
        virtual bool Init() = 0;

        /*
        * Unique identifier of a given compressor
        */
        virtual CompressorType GetType() const = 0;

        /*
        * Returns max possible size of uncompressed data chunk needed to fit compressed data in maxCompSize bytes
        */
        virtual size_t GetMaxChunkSize(size_t maxCompSize) = 0;

        /*
        * Returns size of compressed buffer needed to uncompress uncompSize of bytes
        */
        virtual size_t GetMaxCompressedBufferSize(size_t uncompSize) = 0;

        /*
        * Finalizes the stream, and returns composed packet
        * compData should be able to fit at least GetMaxCompressedBufferSize(uncompSize) bytes
        */
        virtual CompressorError Compress(const void* uncompData, size_t uncompSize, void* compData, size_t& compSize) = 0;

        /*
         * Decompress packet
         * uncompData should be able to fit at least GetDecompressedBufferSize(compressedDataSize)
        */
        virtual CompressorError Decompress(const void* compData, size_t compDataSize, void* uncompData, size_t& chunkSize, size_t& uncompSize) = 0;
    };

    /**
    * Abstract factory to instantiate compressors
    * Used by carrier to create compressor
    */
    class CompressionFactory
    {
    public:
        virtual ~CompressionFactory() AZ_DEFAULT_METHOD;

        /*
        * Instantiate new compressor
        */
        virtual Compressor* CreateCompressor() = 0;

        /*
        * Destroy compressor
        */
        virtual void DestroyCompressor(Compressor* compressor) = 0;
    };
}

#endif // GM_COMPRESSOR_INTERFACE_H


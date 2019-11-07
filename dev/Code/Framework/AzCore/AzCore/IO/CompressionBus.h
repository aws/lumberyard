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

#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/RequestPath.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    namespace IO
    {
        union CompressionTag
        {
            uint32_t m_code;
            char m_name[4];
        };

        enum class ConflictResolution : uint8_t
        {
            PreferFile,
            PreferArchive,
            UseArchiveOnly
        };

        struct CompressionInfo;
        using DecompressionFunc = AZStd::function<bool(const CompressionInfo& info, const void* compressed, size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize)>;

        struct CompressionInfo
        {
            CompressionInfo() = default;
            CompressionInfo(const CompressionInfo& rhs) = default;
            CompressionInfo& operator=(const CompressionInfo& rhs) = default;

            CompressionInfo(CompressionInfo&& rhs);
            CompressionInfo& operator=(CompressionInfo&& rhs);

            RequestPath m_archiveFilename; //< Relative path to the archive file.
            DecompressionFunc m_decompressor; //< The function to use to decompress the data.
            CompressionTag m_compressionTag{ 0 }; //< Tag that uniquely identifies the compressor responsible for decompressing the referenced data.
            size_t m_offset = 0; //< Offset into the archive file for the found file.
            size_t m_compressedSize = 0; //< On disk size of the compressed file.
            size_t m_uncompressedSize = 0; //< Size after the file has been decompressed.
            ConflictResolution m_conflictResolution = ConflictResolution::UseArchiveOnly; //< Preferred solution when an archive is found in the archive and as a separate file.
            bool m_isCompressed = false; //< Whether or not the file is compressed. If the file is not compressed, the compressed and uncompressed sizes should match.
        };

        class Compression
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            using MutexType = AZStd::recursive_mutex;
            
            virtual ~Compression() = default;

            virtual void FindCompressionInfo(bool& found, CompressionInfo& info, const AZStd::string_view filename) = 0;
        };

        using CompressionBus = AZ::EBus<Compression>;

        namespace CompressionUtils
        {
            bool FindCompressionInfo(CompressionInfo& info, const AZStd::string_view filename);
        }
    }
}

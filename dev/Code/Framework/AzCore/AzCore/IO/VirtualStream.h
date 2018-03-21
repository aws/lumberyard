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
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/functional.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
namespace IO
{
    /*!
    * \brief Wraps stream around a stream.
    * Used for operations such as deferred reads/writes while returning GetLength() and GetCurPos() as if those reads/writes took place
    */
    class VirtualStream : public GenericStream
    {
    public:
        AZ_CLASS_ALLOCATOR(VirtualStream, SystemAllocator, 0);
        VirtualStream(GenericStream* stream, bool ownStream);
        VirtualStream(HandleType fileHandle, OpenMode mode = OpenMode::Invalid, bool ownHandle = false);
        VirtualStream(const char* path, OpenMode mode);
        ~VirtualStream();

        bool IsOpen() const override;
        bool CanSeek() const override;
        bool CanRead() const override;
        bool CanWrite() const override;
        void Seek(OffsetType bytes, SeekMode mode) override;
        SizeType Read(SizeType bytes, void* oBuffer) override;
        SizeType Write(SizeType bytes, const void* iBuffer) override;
        SizeType GetCurPos() const override;

        SizeType GetLength() const override;
        SizeType ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset) override;
        SizeType WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset) override;
        bool IsCompressed() const override { return m_stream->IsCompressed(); }
        const char* GetFilename() const override;
        OpenMode GetModeFlags() const override;
        bool ReOpen() override;
        void Close() override;

        SizeType VirtualWrite(SizeType bytes); ///< Returns the current virtual write offset with the position of where this write would take place within the file if the write actually occurred.

    protected:
        template<typename Type> using CustomDeleter = AZStd::function<void(Type*)>;
        AZStd::unique_ptr<GenericStream, CustomDeleter<GenericStream>> m_stream;
        SizeType m_virtualWriteOffset;
    };
} // namespace IO
} // namespace AZ

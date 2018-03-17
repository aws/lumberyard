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
#include <AzCore/IO/VirtualStream.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
namespace IO
{
    static void OwnedDeleter(GenericStream* stream)
    {
        delete stream;
    }

    static void NonOwnedDeleter(GenericStream*)
    {
    }

    /*!
    \brief Creates a virtual stream using the supplied GenericStream
    \param stream GenericStream stored inside virtual stream
    \param ownStream if true then the VirtualStream is responsible for deleting the GenericStream
    */
    VirtualStream::VirtualStream(GenericStream* stream, bool ownStream)
        : m_stream(stream, ownStream ? OwnedDeleter : NonOwnedDeleter)
        , m_virtualWriteOffset(0U)
    {
        if (m_stream->IsOpen())
        {
            m_virtualWriteOffset = m_stream->GetCurPos();
        }
    }

    VirtualStream::VirtualStream(HandleType fileHandle, OpenMode mode, bool ownHandle)
        : VirtualStream(aznew FileIOStream(fileHandle, mode, ownHandle), true)
    {
    }

    VirtualStream::VirtualStream(const char* path, OpenMode mode)
        : VirtualStream(aznew FileIOStream(path, mode), true)
    {
    }

    VirtualStream::~VirtualStream()
    {
    }

    bool VirtualStream::IsOpen() const
    {
        return m_stream->IsOpen();
    }

    bool VirtualStream::CanSeek() const
    {
        return m_stream->CanSeek();
    }

    bool VirtualStream::CanRead() const
    {
        return m_stream->CanRead();
    }

    bool VirtualStream::CanWrite() const
    {
        return m_stream->CanWrite();
    }

    void VirtualStream::Seek(OffsetType bytes, SeekMode mode)
    {
        m_stream->Seek(bytes, mode);
    }

    AZ::IO::SizeType VirtualStream::Read(SizeType bytes, void* oBuffer)
    {
        return m_stream->Read(bytes, oBuffer);
    }

    SizeType VirtualStream::Write(SizeType bytes, const void* buffer)
    {
        SizeType bytesWritten = m_stream->Write(bytes, buffer);
        return bytesWritten;
    }

    AZ::IO::SizeType VirtualStream::GetCurPos() const
    {
        return m_stream->GetCurPos();
    }

    SizeType VirtualStream::GetLength() const
    {
        return m_stream->GetLength();
    }

    /*!
    \brief Reads from the underlying stream using the specified offset
    \param bytes Amount of bytes to read
    \param oBuffer Buffer to read bytes into. This buffer must be at least sizeof(@bytes)
    \param offset Offset to read from using the underlying stream
    */
    SizeType VirtualStream::ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset)
    {
        return m_stream->ReadAtOffset(bytes, oBuffer, offset);
    }

    /*!
    \brief Writes to the underlying stream at the specified offset
    \param bytes Amount of bytes to write
    \param iBuffer Buffer containing data to write
    \param offset Offset of the underlying stream to use
    */
    SizeType VirtualStream::WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset)
    {
        return m_stream->WriteAtOffset(bytes, iBuffer, offset);
    }

    const char* VirtualStream::GetFilename() const
    {
        return m_stream->GetFilename();
    }

    OpenMode VirtualStream::GetModeFlags() const
    {
        return m_stream->GetModeFlags();
    }

    bool VirtualStream::ReOpen()
    {
        if (m_stream->ReOpen())
        {
            m_virtualWriteOffset = m_stream->GetCurPos();
            return true;
        }
        return false;
    }

    void VirtualStream::Close()
    {
        m_stream->Close();
    }


    /*!
    \brief Returns the current value of the virtual write offset and updates the virtual write offset to be the itself plus the value of bytes
    \detail The purpose of this function is to simulate an in-order write request when this function is called  and return a seek position
    that can be used to multi-thread actual writes in such a way that the actual writes result in the same order of VirtualWrite request
    \param bytes Number of bytes to add to the virtual write offset
    \return the old value of the virtual write offset that be used to perform the actual write out of order
    */
    SizeType VirtualStream::VirtualWrite(SizeType bytes)
    {
        SizeType m_oldWriteOffset = m_virtualWriteOffset;
        m_virtualWriteOffset = m_oldWriteOffset + bytes;
        return m_oldWriteOffset;
    }
} // namespace IO
} // namespace AZ

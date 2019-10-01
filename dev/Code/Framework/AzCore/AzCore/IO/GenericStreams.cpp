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

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/IOUtils.h>

namespace AZ
{
    namespace IO
    {

        /*!
        \brief Reads from the stream using the specified offset
        \param bytes Amount of bytes to read
        \param oBuffer Buffer to read bytes into. This buffer must be at least sizeof(@bytes)
        \param offset Offset to seek to before reading. If negative no seeking occurs
        */
        SizeType GenericStream::ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset)
        {
            if (offset >= 0)
            {
                Seek(offset, SeekMode::ST_SEEK_BEGIN);
            }
            return Read(bytes, oBuffer);
        }

        /*!
        \brief Writes to the stream using the specified offset
        \param bytes Amount of bytes to write
        \param iBuffer Buffer containing data to write
        \param offset Offset to seek to before writing. If negative no seeking occurs
        */
        SizeType GenericStream::WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset)
        {
            if (offset >= 0)
            {
                Seek(offset, SeekMode::ST_SEEK_BEGIN);
            }
            return Write(bytes, iBuffer);
        }

        SizeType GenericStream::ComputeSeekPosition(OffsetType bytes, SeekMode mode)
        {
            SizeType absBytes = static_cast<SizeType>(abs(bytes));
            if (mode == ST_SEEK_BEGIN)
            {
                if (bytes > 0)
                {
                    return absBytes;
                }
                else
                {
                    return 0;
                }
            }
            else if (mode == ST_SEEK_CUR)
            {
                SizeType curPos = GetCurPos();
                if (bytes > 0)
                {
                    return curPos + absBytes;
                }
                else
                {
                    if (curPos >= absBytes)
                    {
                        return curPos - absBytes;
                    }
                    else
                    {
                        return 0;
                    }
                }
            }
            else // mode == ST_SEEK_END
            {
                SizeType curLen = GetLength();
                if (bytes > 0)
                {
                    return curLen + absBytes;
                }
                else
                {
                    if (curLen >= absBytes)
                    {
                        return curLen - absBytes;
                    }
                    else
                    {
                        return 0;
                    }
                }
            }
        }



        /*
         * StreamerStream
         */
        StreamerStream::StreamerStream(const char* filename, OpenMode, SizeType baseOffset, SizeType fakeLen)
            : m_filename(filename)
            , m_baseOffset(baseOffset)
            , m_curPos(0)
            , m_fakeLen(fakeLen)
        {
        }

        bool StreamerStream::IsOpen() const
        {
            if (m_exists == Availability::NotChecked)
            {
                m_exists = FileIOBase::GetInstance()->Exists(m_filename.c_str()) ?
                    Availability::FileExists : Availability::FileMissing;
            }
            return m_exists == Availability::FileExists;
        }

        bool StreamerStream::CanRead() const
        {
            return true;
        }

        bool StreamerStream::CanWrite() const
        {
            return false;
        }

        void StreamerStream::Seek(OffsetType bytes, SeekMode mode)
        {
            m_curPos = ComputeSeekPosition(bytes, mode);
        }

        SizeType StreamerStream::Read(SizeType bytes, void* oBuffer)
        {
            AZ_Assert(AZ::IO::Streamer::IsReady(), "AZ::IO::Streamer has not been started!");
            
            SizeType len = GetLength();
            SizeType bytesToRead = bytes + m_curPos > len ? len - m_curPos : bytes;
            SizeType bytesRead = AZ::IO::Streamer::Instance().Read(m_filename.c_str(), m_curPos + m_baseOffset, bytesToRead, oBuffer);
            m_curPos += bytesRead;
            
            return bytesRead;
        }

        SizeType StreamerStream::Write(SizeType, const void*)
        {
            AZ_Assert(false, "AZ::IO::Streamer does not support writing.");
            return 0;
        }

        SizeType StreamerStream::GetLength() const
        {
            if (m_fakeLen == static_cast<SizeType>(-1))
            {
                if (!FileIOBase::GetInstance()->Size(m_filename.c_str(), m_fakeLen))
                {
                    m_fakeLen = 0;
                }
            }
            return m_fakeLen;
        }

        /*
         * SystemFileStream
         */
        SystemFileStream::SystemFileStream(SystemFile* file, bool isOwner, SizeType baseOffset, SizeType fakeLen)
            : m_file(file)
            , m_baseOffset(baseOffset)
            , m_curPos(0)
            , m_fakeLen(fakeLen)
            , m_isFileOwner(isOwner)
            , m_mode(OpenMode::Invalid)
        {
            AZ_Assert(file, "file is NULL!");
            Seek(0, ST_SEEK_BEGIN);
        }

        SystemFileStream::~SystemFileStream()
        {
            if (m_file && m_isFileOwner)
            {
                m_file->Close();
            }
        }

        bool SystemFileStream::IsOpen() const
        {
            return m_file && m_file->IsOpen();
        }

        bool SystemFileStream::CanRead() const
        {
            return true;
        }

        bool SystemFileStream::CanWrite() const
        {
            return true;
        }

        bool SystemFileStream::Open(const char* path, OpenMode mode)
        {
            m_mode = mode;
            Close();
            return m_file ? m_file->Open(path, TranslateOpenModeToSystemFileMode( path, mode)) : false;
        }


        void SystemFileStream::Close()
        {
            if (m_file)
            {
                m_file->Close();
            }
        }

        void SystemFileStream::Seek(OffsetType bytes, SeekMode mode)
        {
            m_curPos = ComputeSeekPosition(bytes, mode);
            m_file->Seek(m_baseOffset + m_curPos, AZ::IO::SystemFile::SF_SEEK_BEGIN);
        }

        SizeType SystemFileStream::Read(SizeType bytes, void* oBuffer)
        {
            SizeType len = GetLength();
            SizeType bytesToRead = bytes + m_curPos > len ? len - m_curPos : bytes;
            SizeType bytesRead = m_file->Read(bytesToRead, oBuffer);
            m_curPos += bytesRead;
            return bytesRead;
        }

        SizeType SystemFileStream::Write(SizeType bytes, const void* iBuffer)
        {
            AZ_Assert(m_fakeLen == static_cast<SizeType>(-1) || bytes + m_curPos <= GetLength(), "Length has been restricted by m_fakeLen and you are trying to write past it!");
            SizeType bytesWritten = m_file->Write(iBuffer, bytes);
            m_curPos += bytesWritten;
            return bytesWritten;
        }

        SizeType SystemFileStream::GetLength() const
        {
            return m_fakeLen == static_cast<SizeType>(-1)
                   ? m_file->Length() - m_baseOffset
                   : m_fakeLen;
        }

        const char* SystemFileStream::GetFilename() const
        {
            return m_file->Name();
        }

        bool SystemFileStream::ReOpen()
        {
            AZ_Assert(m_mode != OpenMode::Invalid, "File must have been opened at least once with valid OpenMode flags in order to be reopened");
            return m_file ? Open(m_file->Name(), m_mode) : false;
        }


        /*
         * MemoryStream
         */
        void MemoryStream::Seek(OffsetType bytes, SeekMode mode)
        {
            m_curOffset = static_cast<size_t>(ComputeSeekPosition(bytes, mode));
        }

        SizeType MemoryStream::Read(SizeType bytes, void* oBuffer)
        {
            if (m_curOffset >= GetLength())
            {
                return 0;
            }
            SizeType bytesLeft = GetLength() - m_curOffset;
            if (bytes > bytesLeft)
            {
                bytes = bytesLeft;
            }
            if (bytes)
            {
                memcpy(oBuffer, m_buffer + m_curOffset, static_cast<size_t>(bytes));
                m_curOffset += static_cast<size_t>(bytes);
            }
            return bytes;
        }

        SizeType MemoryStream::Write(SizeType bytes, const void* iBuffer)
        {
            AZ_Assert(m_mode == MSM_READWRITE, "This memory stream is not writable!");
            if (m_curOffset >= m_bufferLen)
            {
                return 0;
            }
            SizeType bytesLeft = m_bufferLen - m_curOffset;
            if (bytes > bytesLeft)
            {
                bytes = bytesLeft;
            }
            if (bytes)
            {
                memcpy(const_cast<char*>(m_buffer) + m_curOffset, iBuffer, static_cast<size_t>(bytes));
                m_curOffset += static_cast<size_t>(bytes);
                m_curLen = AZStd::GetMax(m_curOffset, m_curLen);
            }
            return bytes;
        }

    }   // namespace IO
}   // namespace AZ

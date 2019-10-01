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
#ifndef AZCORE_IO_GENERICSTREAMS_H
#define AZCORE_IO_GENERICSTREAMS_H

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        typedef AZ::u64 SizeType;
        typedef AZ::s64 OffsetType;
        enum class OpenMode : AZ::u32;

        /*
         *
         */
        class GenericStream
        {
        public:
            enum SeekMode
            {
                ST_SEEK_BEGIN,
                ST_SEEK_CUR,
                ST_SEEK_END,
            };

            virtual ~GenericStream() {}

            virtual bool        IsOpen() const = 0;
            virtual bool        CanSeek() const = 0;
            virtual bool        CanRead() const = 0;
            virtual bool        CanWrite() const = 0;
            virtual void        Seek(OffsetType bytes, SeekMode mode) = 0;
            virtual SizeType    Read(SizeType bytes, void* oBuffer) = 0;
            virtual SizeType    Write(SizeType bytes, const void* iBuffer) = 0;
            virtual SizeType    GetCurPos() const = 0;
            virtual SizeType    GetLength() const = 0;
            virtual SizeType    ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset = -1);
            virtual SizeType    WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset = -1);
            virtual bool        IsCompressed() const { return false; }
            virtual const char* GetFilename() const { return ""; }
            virtual OpenMode    GetModeFlags() const { return OpenMode(); }
            virtual bool        ReOpen() { return true; }
            virtual void        Close() {}
        protected:
            SizeType ComputeSeekPosition(OffsetType bytes, SeekMode mode);
        };

        /*
         * Wraps around a disc manager stream
         *
         * WARNING!
         * StreamerStream will not cause file operations to become asynchronous or go faster. Because it needs to act as a stream
         * calling StreamerStream will cause the thread to still be blocked and in order to unlock the thread as quickly as possible
         * the file will always be processed as a panic read by AZ::IO::Streamer, which is disruptive to it's scheduling. As there's
         * also the overhead of calling into a multi-threaded system and because streams in general encourage micro-reads it's
         * generally not advised to use StreamerStream. Instead prefer to use alternative ways such as FileIOStream to read the entire
         * file to a temporary buffer and use MemoryStream to continue processing as a stream.
         */
        class StreamerStream
            : public GenericStream
        {
        public:
            StreamerStream(const char* filename, OpenMode flags, SizeType baseOffset = 0, SizeType fakeLen = static_cast<SizeType>(-1));
            ~StreamerStream() override = default;

            virtual bool        IsOpen() const;
            virtual bool        CanSeek() const                             { return true; }
            virtual bool        CanRead() const override;
            virtual bool        CanWrite() const override;
            virtual void        Seek(OffsetType bytes, SeekMode mode);
            virtual SizeType    Read(SizeType bytes, void* oBuffer);
            virtual SizeType    Write(SizeType bytes, const void* iBuffer);
            virtual SizeType    GetCurPos() const                           { return m_curPos; }
            virtual SizeType    GetLength() const;

        protected:
            enum class Availability
            {
                NotChecked,
                FileExists,
                FileMissing
            };

            AZStd::string m_filename;
            SizeType    m_baseOffset;
            SizeType    m_curPos;
            mutable SizeType m_fakeLen;
            mutable Availability m_exists = Availability::NotChecked;
        };

        /*
         * Wraps around a SystemFile
         */
        class SystemFile;
        class SystemFileStream
            : public GenericStream
        {
        public:
            SystemFileStream(SystemFile* file, bool isOwner, SizeType baseOffset = 0, SizeType fakeLen = static_cast<SizeType>(-1));
            ~SystemFileStream();

            bool Open(const char* path, OpenMode mode);

            void Close() override;

            virtual bool        IsOpen() const;
            virtual bool        CanSeek() const                             { return true; }
            virtual bool        CanRead() const override;
            virtual bool        CanWrite() const override;
            virtual void        Seek(OffsetType bytes, SeekMode mode);
            virtual SizeType    Read(SizeType bytes, void* oBuffer);
            virtual SizeType    Write(SizeType bytes, const void* iBuffer);
            virtual SizeType    GetCurPos() const                           { return m_curPos; }
            virtual SizeType    GetLength() const;
            virtual const char* GetFilename() const override;
            virtual OpenMode    GetModeFlags() const override { return m_mode; }
            virtual bool        ReOpen() override;

        private:
            SystemFile* m_file;
            SizeType    m_baseOffset;
            SizeType    m_curPos;
            SizeType    m_fakeLen;
            bool        m_isFileOwner;
            OpenMode    m_mode;
        };

        /*
         * Wraps around a memory buffer
         */
        class MemoryStream
            : public GenericStream
        {
        public:
            enum MemoryStreamMode
            {
                MSM_READONLY,
                MSM_READWRITE,
            };

            MemoryStream(const void* buffer, size_t bufferLen)
                : m_buffer(reinterpret_cast<const char*>(buffer))
                , m_bufferLen(bufferLen)
                , m_curOffset(0)
                , m_curLen(bufferLen)
                , m_mode(MSM_READONLY)
            {
            }

            MemoryStream(void* buffer, size_t bufferLen, size_t curLen)
                : m_buffer(reinterpret_cast<char*>(buffer))
                , m_bufferLen(bufferLen)
                , m_curOffset(0)
                , m_curLen(curLen)
                , m_mode(MSM_READWRITE)
            {
            }

            virtual bool        IsOpen() const                              { return m_buffer != NULL; }
            virtual bool        CanSeek() const                             { return true; }
            virtual bool        CanRead() const override                    { return true; }
            virtual bool        CanWrite() const override                   { return true; }
            virtual void        Seek(OffsetType bytes, SeekMode mode);
            virtual SizeType    Read(SizeType bytes, void* oBuffer);
            virtual SizeType    Write(SizeType bytes, const void* iBuffer);
            virtual const void* GetData() const                             { return m_buffer; }
            virtual SizeType    GetCurPos() const                           { return m_curOffset; }
            virtual SizeType    GetLength() const                           { return m_curLen; }

        protected:
            const char*         m_buffer;
            size_t              m_bufferLen;
            size_t              m_curOffset;
            size_t              m_curLen;
            MemoryStreamMode    m_mode;
        };
    }   // namespace IO
}   // namespace AZ

#endif  // AZCORE_IO_GENERICSTREAMS_H
#pragma once

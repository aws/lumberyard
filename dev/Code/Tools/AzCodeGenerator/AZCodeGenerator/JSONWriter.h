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

#include "Writer.h"

#include <memory>
#include <cstdlib>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson
{
    using SizeType = std::size_t;
}

// VS gives us make_unique, but on other compilers with -std=c++11, it's omitted.
#ifndef _MSC_VER
namespace std
{
    template <typename T, typename ... Args>
    unique_ptr<T> make_unique(Args&& ... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
    }
}
#endif

#define RAPIDJSON_SKIP_AZCORE_ERROR

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"

//! Configuration option to choose between Writer and PrettyWriter
#define RAPIDJSONWriter PrettyWriter

namespace CodeGenerator
{
    namespace Output
    {
        void Error(const char* format, ...);
    }

    //! A JSON specific implementation of IWriter, depends on rapidjson.
    //! \tparam WriterType: rapidjson::FileWriteStream or rapidjson::StringBuffer
    template <typename WriterType = rapidjson::StringBuffer>
    class JSONWriter
        : public IWriter
    {
    public:

        // Already opened FILE*, or built-in object such as stdout. Works with FileWriteStream only
        explicit JSONWriter(FILE* fileHandle)
            : m_handle(fileHandle)
        {
            Create<WriterType>("");
        }

        //! Construct a JSONWriter object, if WriterType is rapidjson::StringBuffer, filePath is ignored.
        JSONWriter(const std::string& filePath)
            : m_handle(nullptr)
        {
            Create<WriterType>(filePath);
        }

        //! Constructs a Writer<StringBuffer>
        template<class ClassT>
        typename std::enable_if<std::is_same<rapidjson::StringBuffer, ClassT>::value>::type Create(const std::string&)
        {
            m_writer = std::make_unique<rapidjson::Writer<ClassT> >(m_stringBuffer);
        }

        //! Constructs a Writer<FileWriteStream>
        template<class ClassT>
        typename std::enable_if<std::is_same<rapidjson::FileWriteStream, ClassT>::value>::type Create(const std::string& filePath)
        {
            if (!filePath.empty())
            {
                m_handle = fopen(filePath.c_str(), "w+");
                if (!m_handle)
                {
                    Output::Error("Unable to open %s (errno: %d).\n", filePath.c_str(), errno);
                }
            }

            if (m_handle)
            {
                m_stream = std::make_unique<rapidjson::FileWriteStream>(m_handle, m_streamBuffer, sizeof(m_streamBuffer));
                m_writer = std::make_unique<rapidjson::RAPIDJSONWriter<ClassT> >(*m_stream);
            }
        }

        //! Returns the string buffer, only populated if WriterType is rapidjson::StringBuffer
        const rapidjson::StringBuffer& GetStringBuffer() const
        {
            return m_stringBuffer;
        }

        ~JSONWriter()
        {
            if (m_stream)
            {
                m_stream->Flush();
            }

            if (m_handle)
            {
                fclose(m_handle);
            }
        }

        void Begin() override
        {
            if (IsValid())
            {
                m_writer->StartObject();
            }
        }

        void End() override
        {
            if (IsValid())
            {
                m_writer->EndObject();
            }
        }

        void WriteNull() override
        {
            if (IsValid())
            {
                m_writer->Null();
            }
        }

        void WriteBool(bool b) override
        {
            if (IsValid())
            {
                m_writer->Bool(b);
            }
        }

        void WriteInt(int i) override
        {
            if (IsValid())
            {
                m_writer->Int(i);
            }
        }

        void WriteUInt(unsigned u) override
        {
            if (IsValid())
            {
                m_writer->Uint(u);
            }
        }

        void WriteInt64(int64_t i64) override
        {
            if (IsValid())
            {
                m_writer->Int64(i64);
            }
        }

        void WriteUInt64(uint64_t u64) override
        {
            if (IsValid())
            {
                m_writer->Uint64(u64);
            }
        }

        void WriteDouble(double d) override
        {
            if (IsValid())
            {
                m_writer->Double(d);
            }
        }

        void WriteString(const char* str, size_t length, bool copy = false) override
        {
            if (IsValid())
            {
                m_writer->String(str, length, copy);
            }
        }

        void WriteString(const std::string& str, bool copy = false) override
        {
            if (IsValid())
            {
                m_writer->String(str.c_str(), str.size(), copy);
            }
        }

        void BeginArray() override
        {
            if (IsValid())
            {
                m_writer->StartArray();
            }
        }

        void EndArray(size_t count = 0) override
        {
            if (IsValid())
            {
                m_writer->EndArray(count);
            }
        }

        rapidjson::Writer<WriterType>& GetWriter() const { return *m_writer; }

    private:

        bool IsValid() const
        {
            return m_writer != nullptr;
        }

        FILE* m_handle;
        char m_streamBuffer[65536];
        std::unique_ptr<rapidjson::FileWriteStream> m_stream;

        rapidjson::StringBuffer m_stringBuffer;

        std::unique_ptr<rapidjson::Writer<WriterType> > m_writer;
    };
}

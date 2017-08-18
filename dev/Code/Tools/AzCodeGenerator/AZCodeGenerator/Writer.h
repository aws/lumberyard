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

#include <string>

namespace CodeGenerator
{
    //! Interface for writing data to file.
    class IWriter
    {
    public:
        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void WriteNull() = 0;
        virtual void WriteBool(bool b) = 0;
        virtual void WriteInt(int i) = 0;
        virtual void WriteUInt(unsigned u) = 0;
        virtual void WriteInt64(int64_t i64) = 0;
        virtual void WriteUInt64(uint64_t u64) = 0;
        virtual void WriteDouble(double d) = 0;
        virtual void WriteString(const char* str, size_t length, bool copy = false) = 0;
        virtual void WriteString(const std::string& str, bool copy = false) = 0;

        virtual void BeginArray() = 0;
        virtual void EndArray(size_t count = 0) = 0;
    };
}
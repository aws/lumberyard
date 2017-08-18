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

#include "JSONWriter.h"
namespace CodeGenerator
{
    namespace ErrorCodes
    {
        const unsigned int ClangToolInvokationFailure = 0x2;
        const unsigned int InputOutputListMismatch = 0x3;
        const unsigned int EmptyInputList = 0x4;
        const unsigned int ScriptError = 0x7;
    }

    class IWriter;
    class OutputWriter
    {
    public:
        OutputWriter();
        ~OutputWriter() = default;
        void StartOutput();
        void EndOutput();
        void Print(const char* format, ...);
        void Print(const char* format, va_list args);
        void Error(const char* format, ...);
        void Error(const char* format, va_list args);
        void GeneratedFile(const std::string& fileName, bool shouldBeAddedToBuild = false);
        void DependencyFile(const std::string& fileName);

    private:
        JSONWriter<> m_outputWriter;
        int m_stdout;
        int m_stderr;
    };

    namespace Output
    {
        void StartCapturing();
        void FinishCapturingAndPrint();
        void Print(const char* format, ...);
        void Error(const char* format, ...);
        void GeneratedFile(const std::string& fileName, bool shouldBeAddedToBuild = false);
        void DependencyFile(const std::string& fileName);

        class ScopedCapture
        {
        public:
            ScopedCapture();
            ~ScopedCapture();
        };
    }
}
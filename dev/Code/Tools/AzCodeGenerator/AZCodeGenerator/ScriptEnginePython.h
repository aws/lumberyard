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

#define ENABLE_PYTHON 1

#include "ScriptEngine.h"

namespace PythonScripting
{
    /// ScriptEngine implementation that embeds python and sets up the template engine environment.
    class PythonScriptEngine
        : public CodeGenerator::ScriptEngine
    {
#if ENABLE_PYTHON
    public:
        PythonScriptEngine(const char* programName);

        unsigned int Initialize() override;
        unsigned int Invoke(const char* jsonString, const char* inputFileName, const char* inputPath, const char* outputPath) override;
        void Shutdown() override;

        bool HasScripts() const;
        std::list<std::string> GetScripts() const;
    private:
        void InitializePython(char* programName);
        void ExtendPython();

        std::string m_programName;
#endif // #if ENABLE_PYTHON
    };
}
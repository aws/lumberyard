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

namespace CodeGenerator
{
    /// Abstract class to support custom script engines
    class ScriptEngine
    {
    public:

        virtual ~ScriptEngine()
        {
            Shutdown();
        }

        virtual unsigned int Initialize() { return 0; }
        virtual unsigned int Invoke(const char* /*jsonString*/, const char* /*inputFileName*/, const char* /*inputPath*/, const char* /*outputPath*/) { return 0; }

    private:

        virtual void Shutdown() {}
    };
}
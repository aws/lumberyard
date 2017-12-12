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

#include "Output.h"

namespace CodeGenerator
{
    namespace GlobalConfiguration
    {
        using namespace llvm;

        // Globally-accessible settings
        extern bool g_enableVerboseOutput;

        //! Helper class to simplify syntax for setting up clang arguments, supports loading settings from a file.
        //! Usage:
        //! ClangSettings settings;
        //! settings += "-std=c++11";
        class ClangSettings
        {
        public:

            ClangSettings& operator += (const char* arg);

            ClangSettings& operator += (const std::string& arg);

            const std::vector<std::string>& Args() const;
            std::vector<std::string>& Args();

            //! Loads a file containing clang settings, expects one setting per line.
            void Load(const std::string& filename);

        private:
            std::vector<std::string> m_args;
        };
    }
}
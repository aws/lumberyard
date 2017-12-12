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
#include "precompiled.h"
#include "Configuration.h"
#include <fstream>

namespace CodeGenerator
{
    namespace GlobalConfiguration
    {
        ClangSettings& ClangSettings::operator+=(const char* arg)
        {
            m_args.push_back(arg);
            return *this;
        }

        ClangSettings& ClangSettings::operator+=(const std::string& arg)
        {
            m_args.push_back(arg);
            return *this;
        }

        const std::vector<std::string>& ClangSettings::Args() const
        {
            return m_args;
        }

        std::vector<std::string>& ClangSettings::Args()
        {
            return m_args;
        }

        void ClangSettings::Load(const std::string& filename)
        {
            std::ifstream fileHandle(filename);
            if (!fileHandle.is_open())
            {
                Output::Error("Unable to open file %s for reading!\n", filename.c_str());
            }
            else
            {
                std::string readLine;
                while (getline(fileHandle, readLine))
                {
                    m_args.push_back(readLine);
                }
                fileHandle.close();
            }
        }
    }
}
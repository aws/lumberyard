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
#include "Utils.h"
#include <algorithm>
#include <cstring>
#include <AzCore/base.h>

namespace AZ
{
    namespace Test
    {
        bool ContainsParameter(int argc, char** argv, const std::string& param)
        {
            int index = GetParameterIndex(argc, argv, param);
            return index < 0 ? false : true;
        }

        void CopyParameters(int argc, char** target, char** source)
        {
            for (int i = 0; i < argc; i++)
            {
                const size_t dstSize = std::strlen(source[i]) + 1;
                target[i] = new char[dstSize];
                azstrcpy(target[i], dstSize, source[i]);
            }
        }

        int GetParameterIndex(int argc, char** argv, const std::string& param)
        {
            for (int i = 0; i < argc; i++)
            {
                if (param == argv[i])
                {
                    return i;
                }
            }
            return -1;
        }

        std::vector<std::string> GetParameterList(int& argc, char** argv, const std::string& param, bool removeOnReturn)
        {
            std::vector<std::string> parameters;
            int paramIndex = GetParameterIndex(argc, argv, param);
            if (paramIndex > 0)
            {
                int index = paramIndex + 1;
                while (index < argc && !StartsWith(argv[index], "-"))
                {
                    parameters.push_back(std::string(argv[index]));
                    index++;
                }
            }
            if (removeOnReturn)
            {
                RemoveParameters(argc, argv, paramIndex, paramIndex + (int)parameters.size());
            }
            return parameters;
        }

        std::string GetParameterValue(int& argc, char** argv, const std::string& param, bool removeOnReturn)
        {
            std::string value("");
            int index = GetParameterIndex(argc, argv, param);
            // Make sure we have a valid parameter index and value after the parameter index
            if (index > 0 && index < (argc - 1))
            {
                value = argv[index + 1];
                if (removeOnReturn)
                {
                    RemoveParameters(argc, argv, index, index + 1);
                }
            }
            return value;
        }

        void RemoveParameters(int& argc, char** argv, int startIndex, int endIndex)
        {
            // protect against invalid order of parameters
            if (startIndex > endIndex)
            {
                return;
            }

            // constraint to valid range
            endIndex = std::min(endIndex, argc - 1);
            startIndex = std::max(startIndex, 0);

            int numRemoved = 0;
            int i = startIndex;
            int j = endIndex + 1;

            // copy all existing paramters
            while (j < argc)
            {
                argv[i++] = argv[j++];
            }

            // null out all the remaining parameters and count how many
            // were removed simultaneously
            while (i < argc)
            {
                argv[i++] = nullptr;
                ++numRemoved;
            }

            argc -= numRemoved;
        }

        char** SplitCommandLine(int& size, char* const cmdLine)
        {
            std::vector<char*> tokens;
            char* next_token = nullptr;
            char* tok = azstrtok(cmdLine, 0, " ", &next_token);
            while (tok != NULL)
            {
                tokens.push_back(tok);
                tok = azstrtok(NULL, 0, " ", &next_token);
            }
            size = (int)tokens.size();
            char** token_array = new char*[size];
            for (size_t i = 0; i < size; i++)
            {
                const size_t dstSize = std::strlen(tokens[i]) + 1;
                token_array[i] = new char[dstSize];
                azstrcpy(token_array[i], dstSize, tokens[i]);
            }
            return token_array;
        }

        bool EndsWith(const std::string& s, const std::string& ending)
        {
            if (ending.length() > s.length())
            {
                return false;
            }
            return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
        }

        bool StartsWith(const std::string& s, const std::string& beginning)
        {
            if (beginning.length() > s.length())
            {
                return false;
            }
            return std::equal(beginning.begin(), beginning.end(), s.begin());
        }
    }
}

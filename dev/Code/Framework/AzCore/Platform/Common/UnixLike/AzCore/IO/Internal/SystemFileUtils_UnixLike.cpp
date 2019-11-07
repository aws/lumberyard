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

#include "SystemFileUtils_UnixLike.h"

namespace AZ
{
    namespace IO
    {
        namespace Internal
        {
            void FormatAndPeelOffWildCardExtension(const char* sourcePath, char* filePath, char* extensionPath)
            {
                const char* pSrcPath = sourcePath;
                char* pDestPath = filePath;
                unsigned    numFileChars = 0;
                unsigned    numExtensionChars = 0;
                unsigned* pNumDestChars = &numFileChars;
                bool        bIsWildcardExtension = false;
                while (*pSrcPath)
                {
                    char srcChar = *pSrcPath++;

                    // Skip '*' and '.'
                    if ((!bIsWildcardExtension && srcChar != '*') || (bIsWildcardExtension && srcChar != '.' && srcChar != '*'))
                    {
                        unsigned numChars = *pNumDestChars;
                        pDestPath[numChars++] = srcChar;
                        *pNumDestChars = numChars;
                    }
                    // Wild-card extension is separate
                    if (srcChar == '*')
                    {
                        bIsWildcardExtension = true;
                        pDestPath = extensionPath;
                        pNumDestChars = &numExtensionChars;
                    }
                }
                // Close strings
                filePath[numFileChars] = 0;
                extensionPath[numExtensionChars] = 0;
            }
        }
    }
}
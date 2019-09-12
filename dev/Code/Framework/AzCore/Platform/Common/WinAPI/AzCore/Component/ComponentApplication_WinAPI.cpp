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

#include <AzCore/std/algorithm.h>
#include <AzCore/PlatformIncl.h>

namespace AZ
{
    namespace Platform
    {

        void GetExePath(char* exeDirectory, size_t exeDirectorySize, bool& pathIncludesExe)
        {
            pathIncludesExe = true;

            // Platform specific get exe path: http://stackoverflow.com/a/1024937
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms683197(v=vs.85).aspx
            DWORD pathLen = GetModuleFileNameA(nullptr, exeDirectory, static_cast<DWORD>(exeDirectorySize));

            char* exeDirEnd = exeDirectory + pathLen;

            AZStd::replace(exeDirectory, exeDirEnd, '\\', '/');
        }

    }
}
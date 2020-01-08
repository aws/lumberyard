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

#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Android/AndroidEnv.h>

namespace AZ
{
    namespace Utils
    {
        GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize)
        {
            GetExecutablePathReturnType result;
            result.m_pathIncludesFilename = false;
            const char* privateStoragePath  = AZ::Android::AndroidEnv::Get()->GetAppPrivateStoragePath();
            if(exeStorageSize > strlen(privateStoragePath))
            {
                azstrcpy(exeStorageBuffer, exeStorageSize, privateStoragePath);
            }
            else
            {
                result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
            }
            
            return result;
        }

    }
}
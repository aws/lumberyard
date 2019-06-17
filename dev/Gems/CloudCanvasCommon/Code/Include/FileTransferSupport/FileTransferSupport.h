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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

namespace CloudCanvas
{
    namespace FileTransferSupport
    {
        using FileRequestMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

        bool CanWriteToFile(const AZStd::string& localFileName);
        AZStd::string GetResolvedFile(const AZStd::string& destFolder, const AZStd::string& fileName);
        bool CheckWritableMakePath(const AZStd::string& fileName);
        void MakeEndInSlash(AZStd::string& someString);
        void ValidateWritable(const FileRequestMap& requestMap);
        AZStd::string ResolvePath(const char* dirName, bool returnDirOnFailure = false);
        AZStd::string CalculateMD5(const char* relativeFile, bool useDirectAccess = true);
        AZStd::vector<unsigned char> GetMD5Buffer(const char* relativeFile, bool useDirectAccess = false);
        bool IsPak(const AZStd::string& localFileName);
        bool IsManifest(const AZStd::string& localFileName);
    }
}

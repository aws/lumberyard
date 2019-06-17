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

#include <CloudGemDynamicContent_precompiled.h>

#include <FileTransferSupport/FileTransferSupport.h>

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace CloudCanvas
{
    namespace FileTransferSupport
    {
        // Assumes files which aren't there could be created
        bool CanWriteToFile(const AZStd::string& localFileName)
        {
            return AZ::IO::SystemFile::IsWritable(localFileName.c_str()) || !AZ::IO::SystemFile::Exists(localFileName.c_str());
        }

        AZStd::string ResolvePath(const char* dirName, bool returnDirOnFailure)
        {
            char resolvedGameFolder[AZ_MAX_PATH_LEN] = { 0 };
            if (!AZ::IO::LocalFileIO::GetInstance()->ResolvePath(dirName, resolvedGameFolder, AZ_MAX_PATH_LEN))
            {
                return returnDirOnFailure ? dirName : "";
            }
            return resolvedGameFolder;
        }

        AZStd::string GetResolvedFile(const AZStd::string& destFolder, const AZStd::string& fileName)
        {
            AZStd::string writeFolder = ResolvePath(destFolder.c_str(), true);

            if (fileName.length())
            {
                MakeEndInSlash(writeFolder);
            }

            return{ writeFolder + fileName };
        }

        // Checks a file name including path for write permissions.  Will attempt to create the path to the folder if it does not exist.
        bool CheckWritableMakePath(const AZStd::string& fileName)
        {
            auto lastSeparator = fileName.find_last_of('/');
            AZStd::string writeFolder = lastSeparator != AZStd::string::npos ? fileName.substr(0, lastSeparator) : fileName;
            writeFolder = ResolvePath(writeFolder.c_str());
            // If the directory doesn't exist and we create it and can then write to the file, we're ok
            if (!AZ::IO::SystemFile::Exists(writeFolder.c_str()))
            {
                // This call currently returns a failure even when it succeeds

                AZ::IO::LocalFileIO::GetInstance()->CreatePath(writeFolder.c_str());
            }
            return CanWriteToFile(fileName.c_str());
        }

        void MakeEndInSlash(AZStd::string& someString)
        {
            if (someString.length() && someString[someString.length() - 1] != '/')
            {
                someString += '/';
            }
        }

        void ValidateWritable(const FileRequestMap& requestMap) 
        {
            for (auto thisEntry : requestMap)
            {
                if (!CheckWritableMakePath(thisEntry.second))
                {
                    gEnv->pLog->LogAlways("Can't write to %s", thisEntry.second.c_str());
                }
            }
        }

        AZStd::string CalculateMD5(const char* relativeFile, bool useDirectAccess)
        {
            AZStd::vector<unsigned char> hashBuffer = GetMD5Buffer(relativeFile, useDirectAccess);
            AZStd::string returnStr;
            char charHash[3] = { 0, 0, 0 };

            for (int i = 0; i < hashBuffer.size(); ++i)
            {
                sprintf_s(charHash, "%02x", hashBuffer[i]);
                returnStr += charHash;
            }

            return returnStr;
        }

        AZStd::vector<unsigned char> GetMD5Buffer(const char* relativeFile, bool useDirectAccess)
        {
            AZStd::string sanitizedString = ResolvePath(relativeFile);

            // MD5 calculation requires 16 bytes in the dest string
            static const int hashLength = 16;
            AZStd::vector<unsigned char> hashVec;
            hashVec.resize(hashLength);

            gEnv->pCryPak->ComputeMD5(sanitizedString.c_str(), &hashVec[0],0, useDirectAccess);

            return hashVec;
        }

        bool IsPak(const AZStd::string& someString)
        {
            return (azstricmp(PathUtil::GetExt(someString.c_str()), "pak") == 0);
        }

        bool IsManifest(const AZStd::string& someString)
        {
            return (azstricmp(PathUtil::GetExt(someString.c_str()),"json") == 0);
        }
    }
} // namespace CloudCanvas






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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "pch.h"
#include "ScanAndLoadFilesTask.h"
#include <ICryPak.h>
#include <StringUtils.h>
#include "Util/PathUtil.h"

#include "IEditor.h"

namespace CharacterTool
{
    static bool LoadFile(vector<char>* buf, const char* filename)
    {
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return false;
        }

        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
        size_t size = gEnv->pCryPak->FTell(fileHandle);
        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_SET);

        buf->resize(size);
        bool result = gEnv->pCryPak->FRead(&(*buf)[0], size, fileHandle) == size;
        gEnv->pCryPak->FClose(fileHandle);
        return result;
    }

    SScanAndLoadFilesTask::SScanAndLoadFilesTask(const SLookupRule& rule, const char* description, int index)
        : m_rule(rule)
        , m_description(description)
        , m_index(index)
    {
    }

    ETaskResult SScanAndLoadFilesTask::Work()
    {
        const SLookupRule& rule = m_rule;
        vector<string> masks = rule.masks;

        IFileUtil::FileArray files;

        for (size_t j = 0; j < masks.size(); ++j)
        {
            files.clear();
            const string& mask = masks[j];

            if (!GetIEditor()->GetFileUtil()->ScanDirectory("", mask.c_str(), files, true, true))
            {
                return ETaskResult::eTaskResult_Failed;
            }

            m_loadedFiles.reserve(m_loadedFiles.size() + files.size());
            for (IFileUtil::FileDesc& fileFound : files)
            {
                if (!CryStringUtils::MatchWildcard(fileFound.filename.toUtf8().data(), mask.c_str()))
                {
                    continue;
                }

                ScanLoadedFile file;
                file.scannedFile = fileFound.filename.toUtf8().data();
                file.scannedFile.replace("\\", "/");
                file.fromPak = gEnv->pCryPak->IsFileExist(file.scannedFile.c_str(), ICryPak::eFileLocation_InPak);
                file.fromDisk = gEnv->pCryPak->IsFileExist(file.scannedFile.c_str(), ICryPak::eFileLocation_OnDisk);
                m_loadedFiles.push_back(file);
            }
        }

        return eTaskResult_Completed;
    }

    void SScanAndLoadFilesTask::Finalize()
    {
        for (size_t i = 0; i < m_loadedFiles.size(); ++i)
        {
            const ScanLoadedFile& loadedFile = m_loadedFiles[i];

            SignalFileLoaded(loadedFile);
        }

        SignalLoadingFinished(m_index);
    }
}

#include <CharacterTool/ScanAndLoadFilesTask.moc>

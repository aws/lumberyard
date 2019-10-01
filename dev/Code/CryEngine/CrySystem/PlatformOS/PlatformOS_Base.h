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

#include "IPlatformOS.h"
#include <CryListenerSet.h>
#include <IGameFramework.h>
#include <ITimer.h>

class PlatformOS_Base
    : public IPlatformOS
{
protected:
    class CFileFinderCryPak
        : public IFileFinder
    {
    public:
        virtual IFileFinder::EFileState FileExists(const char* path) { return gEnv->pCryPak->IsFileExist(path) ? eFS_FileOrDirectory : eFS_NotExist; }
        virtual intptr_t FindFirst(const char* filePattern, _finddata_t* fd)
        {
            return gEnv->pCryPak->FindFirst(filePattern, fd, 0, true);
        }
        virtual int FindClose(intptr_t handle) { return gEnv->pCryPak->FindClose(handle); }
        virtual int FindNext(intptr_t handle, _finddata_t* fd) { return gEnv->pCryPak->FindNext(handle, fd); }
        virtual void GetMemoryUsage(ICrySizer* pSizer) const { pSizer->Add(*this); }
    private:
        CDebugAllowFileAccess m_allowFileAccess;
    };
};

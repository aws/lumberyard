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

// Description : Interface to the Platform OS


#pragma once

#if defined(AZ_RESTRICTED_PLATFORM)
#else

#include "IPlatformOS.h"
#include <CryListenerSet.h>

#include "PlatformOS_Base.h"

namespace ZipDir
{
    class FileEntryTree;

    class CacheRW;
    TYPEDEF_AUTOPTR(CacheRW);
    typedef CacheRW_AutoPtr CacheRWPtr;
}


class CPlatformOS_PC
    : public PlatformOS_Base
{
public:

    CPlatformOS_PC();

    // ~IPlatformOS

    virtual IFileFinderPtr          GetFileFinder(unsigned int user);
    virtual void                    MountDLCContent(IDLCListener* pCallback, unsigned int user, const uint8 keyData[16]);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual IPlatformOS::ECDP_Start StartUsingCachePaks(const int user, bool* outWritesOccurred);
    virtual IPlatformOS::ECDP_End EndUsingCachePaks(const int user);
    virtual IPlatformOS::ECDP_Open DoesCachePakExist(const char* const filename, const size_t size, unsigned char md5[16]);
    virtual IPlatformOS::ECDP_Open OpenCachePak(const char* const filename, const char* const bindRoot, const size_t size, unsigned char md5[16]);
    virtual IPlatformOS::ECDP_Close CloseCachePak(const char* const filename);
    virtual IPlatformOS::ECDP_Delete DeleteCachePak(const char* const filename);
    virtual IPlatformOS::ECDP_Write WriteCachePak(const char* const filename, const void* const pData, const size_t numBytes);

    virtual IPlatformOS::EZipExtractFail ExtractZips(const char* path);

    // ~IPlatformOS

private:
    IPlatformOS::EZipExtractFail RecurseZipContents(ZipDir::FileEntryTree* pSourceDir, const char* currentPath, ZipDir::CacheRWPtr pCache);
    bool SxmlMissingFromHDD(ZipDir::FileEntryTree* pSourceDir, const char* currentPath, ZipDir::CacheRWPtr pCache);

    bool DecryptAndCheckSigning(const char* pInData, int inDataLen, char** pOutData, int* pOutDataLen, const uint8 key[16]);
    bool UseSteamReadWriter() const;

private:
    int m_cachePakStatus;
    int m_cachePakUser;
};

#endif

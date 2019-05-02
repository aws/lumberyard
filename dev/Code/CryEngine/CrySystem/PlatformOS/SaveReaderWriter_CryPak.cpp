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

// Description : Implementation of the ISaveReader and ISaveWriter
//               interfaces using CryPak

#include <StdAfx.h>

#include "SaveReaderWriter_CryPak.h"
#include "SaveReaderWriter_Memory.h"
#include <ICrypto.h>

static const int INVALID_SEEK = -1;

int TranslateSeekMode(IPlatformOS::ISaveReader::ESeekMode mode)
{
    COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_SET);
    COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_CUR);
    COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_END);

    switch (mode)
    {
    case IPlatformOS::ISaveReader::ESM_BEGIN:
        return SEEK_SET;
    case IPlatformOS::ISaveReader::ESM_CURRENT:
        return SEEK_CUR;
    case IPlatformOS::ISaveReader::ESM_END:
        return SEEK_END;
    default:
    {
        CRY_ASSERT_TRACE(false, ("Unrecognized seek mode %i", static_cast<int>(mode)));
        return INVALID_SEEK;
    }
    }
}

NO_INLINE_WEAK AZ::IO::HandleType CCryPakFile::FOpen(const char* pName, const char* mode, unsigned nFlags)
{
    return gEnv->pCryPak->FOpen(pName, mode, nFlags);
}

NO_INLINE_WEAK int CCryPakFile::FClose(AZ::IO::HandleType fileHandle)
{
    return gEnv->pCryPak->FClose(fileHandle);
}

NO_INLINE_WEAK size_t CCryPakFile::FGetSize(AZ::IO::HandleType fileHandle)
{
    return gEnv->pCryPak->FGetSize(fileHandle);
}

NO_INLINE_WEAK size_t CCryPakFile::FReadRaw(void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle)
{
    return gEnv->pCryPak->FReadRaw(data, length, elems, fileHandle);
}

NO_INLINE_WEAK size_t CCryPakFile::FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle)
{
    return gEnv->pCryPak->FWrite(data, length, elems, fileHandle);
}

NO_INLINE_WEAK size_t CCryPakFile::FSeek(AZ::IO::HandleType fileHandle, long offset, int mode)
{
    return gEnv->pCryPak->FSeek(fileHandle, offset, mode);
}

////////////////////////////////////////////////////////////////////////////

CCryPakFile::CCryPakFile(const char* fileName, const char* szMode)
    : m_fileName(fileName)
    , m_filePos(0)
    , m_eLastError(IPlatformOS::eFOC_Success)
    , m_bWriting(!strchr(szMode, 'r'))
{
    m_fileHandle = FOpen(fileName, szMode, ICryPak::FOPEN_ONDISK);
    if (m_fileHandle == AZ::IO::InvalidHandle)
    {
        m_fileHandle = FOpen(fileName, szMode);
    }

    if (m_fileHandle != AZ::IO::InvalidHandle && !m_bWriting)
    {
        size_t fileSize = FGetSize(m_fileHandle);
        if (fileSize)
        {
            const DynArray<char>* pMagic;
            const DynArray<uint8>* pKey;
            GetISystem()->GetPlatformOS()->GetEncryptionKey(&pMagic, &pKey);

            m_data.resize(fileSize);

            bool isEncrypted = false;
            if (pMagic->size() && fileSize >= pMagic->size())
            {
                std::vector<uint8> magic;
                magic.resize(pMagic->size());
                FReadRaw(&magic[0], 1, magic.size(), m_fileHandle);
                fileSize -= magic.size();
                isEncrypted = 0 == memcmp(&magic[0], &pMagic->front(), magic.size());
                if (isEncrypted)
                {
                    m_data.resize(fileSize);
                }
                else
                {
                    memcpy(&m_data[0], &magic[0], magic.size());
                    m_filePos += magic.size();
                }
            }

            if (fileSize)
            {
                FReadRaw(&m_data[m_filePos], 1, fileSize, m_fileHandle);
            }
            m_filePos = 0;

            if (isEncrypted && fileSize)
            {
                gEnv->pSystem->GetCrypto()->DecryptBuffer(&m_data[0], &m_data[0], fileSize, &pKey->front(), pKey->size());
            }
        }
    }
}

CCryPakFile::~CCryPakFile()
{
    CloseImpl();
}

IPlatformOS::EFileOperationCode CCryPakFile::CloseImpl()
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    if (m_eLastError == IPlatformOS::eFOC_Success)
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            if (m_bWriting)
            {
                if (!m_data.empty())
                {
#ifndef _RELEASE
                    // Default true if CVar doesn't exist
                    ICVar* pUseEncryption = gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_usePlatformSavingAPIEncryption") : NULL;
                    bool bUseEncryption = pUseEncryption && pUseEncryption->GetIVal() != 0;
                    if (bUseEncryption)
#endif
                    {
                        // Write the magic tag
                        const DynArray<char>* pMagic;
                        const DynArray<uint8>* pKey;
                        GetISystem()->GetPlatformOS()->GetEncryptionKey(&pMagic, &pKey);
                        if (!pMagic->size() || pMagic->size() != FWrite(&pMagic->front(), 1, pMagic->size(), m_fileHandle))
                        {
                            m_eLastError = IPlatformOS::eFOC_ErrorWrite;
                        }
                        else
                        {
                            gEnv->pSystem->GetCrypto()->EncryptBuffer(&m_data[0], &m_data[0], m_data.size(), &pKey->front(), pKey->size());
                        }
                    }

                    if (m_data.size() != FWrite(&m_data[0], 1, m_data.size(), m_fileHandle))
                    {
                        m_eLastError = IPlatformOS::eFOC_ErrorWrite;
                    }
                }
            }

            if (FClose(m_fileHandle) != 0)
            {
                m_eLastError = IPlatformOS::eFOC_Failure;
            }
        }
    }
    m_fileHandle = AZ::IO::InvalidHandle;
    m_data.clear();

    return m_eLastError;
}

////////////////////////////////////////////////////////////////////////////

// Update file stats to make sure SaveGame system reads from new info
void UpdateFileStats(const char* fileName)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/SaveReaderWriter_CryPak_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/SaveReaderWriter_CryPak_cpp_provo.inl"
    #endif
#endif
}

////////////////////////////////////////////////////////////////////////////

CSaveReader_CryPak::CSaveReader_CryPak(const char* fileName)
    : CCryPakFile(fileName, "rbx") // x=don't cache full file
{
    m_eLastError = m_fileHandle == AZ::IO::InvalidHandle ? IPlatformOS::eFOC_ErrorOpenRead : IPlatformOS::eFOC_Success;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::Seek(long seek, ESeekMode mode)
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    long pos = (long)m_filePos;

    switch (mode)
    {
    case ESM_BEGIN:
        pos = seek;
        break;
    case ESM_CURRENT:
        pos += seek;
        break;
    case ESM_END:
        pos = m_data.size() + seek;
        break;
    }

    if (pos < 0 || pos > (long)m_data.size())
    {
        return IPlatformOS::eFOC_Failure;
    }

    m_filePos = (size_t)pos;

    return IPlatformOS::eFOC_Success;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::GetFileCursor(long& fileCursor)
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    fileCursor = (long)m_filePos;

    return IPlatformOS::eFOC_Success;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::ReadBytes(void* data, size_t numBytes)
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    size_t readBytes = numBytes;
    if (m_filePos + readBytes > m_data.size())
    {
        readBytes = m_data.size() - m_filePos;
    }

    memcpy(data, &m_data[m_filePos], readBytes);
    m_filePos += readBytes;

    if (numBytes != readBytes)
    {
        m_eLastError = IPlatformOS::eFOC_ErrorRead;
        return m_eLastError;
    }
    return IPlatformOS::eFOC_Success;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::GetNumBytes(size_t& numBytes)
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    numBytes = m_data.size();

    return IPlatformOS::eFOC_Success;
}

void CSaveReader_CryPak::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddContainer(m_data);
}

void CSaveReader_CryPak::TouchFile()
{
    // Touch the file so it's used as the most previous checkpoint
    CloseImpl();
    m_fileHandle = FOpen(m_fileName, "r+b", ICryPak::FOPEN_ONDISK);
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        char c;
        if (1 == FReadRaw(&c, 1, sizeof(c), m_fileHandle))
        {
            FSeek(m_fileHandle, 0, SEEK_SET);
            FWrite(&c, 1, sizeof(c), m_fileHandle);
        }
        FClose(m_fileHandle);
        m_fileHandle = AZ::IO::InvalidHandle;
    }

    UpdateFileStats(m_fileName.c_str());
}

////////////////////////////////////////////////////////////////////////////

CSaveWriter_CryPak::CSaveWriter_CryPak(const char* fileName)
    : CCryPakFile(fileName, "wb")
{
    UpdateFileStats(fileName);

    m_eLastError = m_fileHandle == AZ::IO::InvalidHandle ? IPlatformOS::eFOC_ErrorOpenWrite : IPlatformOS::eFOC_Success;
}

IPlatformOS::EFileOperationCode CSaveWriter_CryPak::AppendBytes(const void* data, size_t length)
{
    CRY_ASSERT(gEnv);
    CRY_ASSERT(gEnv->pCryPak);

    if (m_fileHandle != AZ::IO::InvalidHandle && length != 0)
    {
        size_t newLength = m_data.size() + length;
        if (newLength > m_data.capacity())
        {
            size_t numBlocks = newLength / s_dataBlockSize;
            size_t newCapaticy = (numBlocks + 1) * s_dataBlockSize;
            m_data.reserve(newCapaticy);
        }
        m_data.resize(newLength);
        memcpy(&m_data[m_filePos], data, length);
        m_filePos += length;
    }
    return IPlatformOS::eFOC_Success;
}

void CSaveWriter_CryPak::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddContainer(m_data);
}

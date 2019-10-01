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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_IMAGE_DDSIMAGE_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_IMAGE_DDSIMAGE_H
#pragma once


#include "ImageExtensionHelper.h"
#include "CImage.h"
#include <IMemory.h>

// uncomment to use AZ::IO::Streamer file IO
#define DDS_USE_AZ_STREAMER

#ifdef DDS_USE_AZ_STREAMER
#include <AzCore/IO/Streamer.h>
#endif

#include <AzCore/IO/FileIO.h>

/**
 * An ImageFile subclass for reading DDS files.
 */
namespace DDSSplitted
{
    enum
    {
        etexNumLastMips = 3,
        etexLowerMipMaxSize = (1 << (etexNumLastMips + 2)),   // + 2 means we drop all the mips that are less than 4x4(two mips: 2x2 and 1x1)
    };

    struct DDSDesc
    {
        DDSDesc()
        {
            memset(this, 0, sizeof(*this));
        }

        const char* pName;
        uint32 nBaseOffset;

        uint32 nWidth;
        uint32 nHeight;
        uint32 nDepth;
        uint32 nSides;
        uint32 nMips;
        uint32 nMipsPersistent;
        ETEX_Format eFormat;
        ETEX_TileMode eTileMode;
        uint32 nFlags;
    };

    typedef CryStackStringT<char, 192> TPath;

    struct ChunkInfo
    {
        TPath fileName;
        uint32 nOffsetInFile;
        uint32 nSizeInFile;
        uint32 nMipLevel;
        uint32 nSideDelta;
    };

    struct RequestInfo
    {
        string fileName;
        void* pOut;
        uint32 nOffs;
        uint32 nRead;
    };

    struct FileWrapper
    {
#ifdef DDS_USE_AZ_STREAMER
        FileWrapper(const char* fileName, bool storeInMem)
            : m_bInMem(storeInMem)
            , m_fileName(fileName)
            , m_isValid(false)
            , m_nRawLen(0)
            , m_nSeek(0)
            , m_pRaw(nullptr)
            , m_cleanup(storeInMem)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "DDS FileWrapper AZ Streamer");
            AZ::u64 fileSize = 0;
            AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Size(fileName, fileSize);
            m_nRawLen = (size_t)fileSize; // Cry DDS texture used 32 bits for filesize so cast here
            if (result == AZ::IO::ResultCode::Success)
            {
                m_isValid = true;
                if (m_bInMem)
                {
                    m_pRaw = new char[m_nRawLen];
                    AZ::IO::GetStreamer()->Read(m_fileName, 0, m_nRawLen, (void*)m_pRaw);
                }
            }
        }
#else
        explicit FileWrapper(CCryFile& file)
            : m_bInMem(false)
            , m_pFile(&file) {}
#endif

        ~FileWrapper()
        {
#ifdef DDS_USE_AZ_STREAMER
            if (!m_bInMem && m_isValid)
            {
                AZ::IO::GetStreamer()->DestroyDedicatedCache(m_fileName);
            }
#endif
            if (m_bInMem && m_cleanup && m_pRaw)
            {
                delete[] m_pRaw;
                m_pRaw = nullptr;
            }
        }
        FileWrapper(const void* p, size_t nLen, bool cleanupData = false)
            : m_bInMem(true)
            , m_pRaw((const char*)p)
            , m_nRawLen(nLen)
            , m_nSeek(0)
            , m_cleanup(cleanupData){}

        bool IsValid() const
        {
            return m_bInMem
                 ? m_pRaw != NULL
#ifdef DDS_USE_AZ_STREAMER
                 : m_isValid;
#else
                 : m_pFile != NULL && m_pFile->GetHandle() != AZ::IO::InvalidHandle;
#endif
        }

        size_t GetLength() const
        {
#ifdef DDS_USE_AZ_STREAMER
            return m_nRawLen;
#else
            return m_bInMem
                   ? m_nRawLen
                   : m_pFile->GetLength();
#endif
        }

        size_t ReadRaw(void* p, size_t amount)
        {
            if (m_bInMem)
            {
                amount = min(m_nRawLen - m_nSeek, amount);
                memcpy(p, m_pRaw + m_nSeek, amount);
                m_nSeek += amount;

                return amount;
            }
            else
            {
#ifdef DDS_USE_AZ_STREAMER
                amount = min(m_nRawLen - m_nSeek, amount);
                AZ::u64 amountRead = AZ::IO::GetStreamer()->Read(m_fileName, m_nSeek, amount, p, AZ::IO::ExecuteWhenIdle, AZ::IO::Request::PriorityType::DR_PRIORITY_NORMAL, nullptr, "DDSRead");
                m_nSeek += (size_t)amountRead; // Cry DDS texture used 32 bits for filesize so cast here
                return amountRead;
#else
                return m_pFile->ReadRaw(p, amount);
#endif
            }
        }

        void Seek(size_t offs)
        {
#ifdef DDS_USE_AZ_STREAMER
            m_nSeek = min(offs, m_nRawLen);
#else
            if (m_bInMem)
            {
                m_nSeek = min(offs, m_nRawLen);
            }
            else
            {
                m_pFile->Seek(offs, SEEK_SET);
            }
#endif
        }

        size_t Tell()
        {
#ifdef DDS_USE_AZ_STREAMER
            return m_nSeek;
#else
            if (m_bInMem)
            {
                return m_nSeek;
            }
            else
            {
                return m_pFile->GetPosition();
            }
#endif
        }

        bool m_bInMem;

#ifdef DDS_USE_AZ_STREAMER
        const char* m_fileName;
#else
        CCryFile* m_pFile;
#endif
        struct
        {
#ifdef DDS_USE_AZ_STREAMER
            bool m_isValid;
#endif

            bool m_cleanup;
            const char* m_pRaw;
            size_t m_nRawLen;
            size_t m_nSeek;
        };
    };

    typedef std::vector<ChunkInfo> Chunks;

    TPath& MakeName(TPath& sOut, const char* sOriginalName, const uint32 nChunk, const uint32 nFlags);

    size_t GetFilesToRead(ChunkInfo* pFiles, size_t nFilesCapacity, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip);

    bool SeekToAttachedImage(FileWrapper& file);

    size_t LoadMipRequests(RequestInfo* pReqs, size_t nReqsCap, const DDSDesc& desc, byte* pBuffer, uint32 nStartMip, uint32 nEndMip);
    bool LoadMipsFromRequests(const RequestInfo* pReqs, size_t nReqs);
    bool LoadMips(byte* pBuffer, const DDSDesc& desc, uint32 nStartMip, uint32 nEndMip);

    int GetNumLastMips(const int nWidth, const int nHeight, const int nNumMips, const int nSides, ETEX_Format eTF, const uint32 nFlags);
};

class CImageDDSFile
    : public CImageFile
{
    friend class CImageFile; // For constructor
public:
    CImageDDSFile(const string& filename);
    CImageDDSFile (const string& filename, uint32 nFlags);

    bool Stream(uint32 nFlags, IImageFileStreamCallback* pStreamCallback);

    virtual DDSSplitted::DDSDesc mfGet_DDSDesc() const;

private: // ------------------------------------------------------------------------------
    /// Read the DDS file from the file.
    bool LoadFromFile(DDSSplitted::FileWrapper& file, uint32 nFlags, DDSSplitted::RequestInfo* pConts, size_t& nConts, size_t nContsCap);

    /// Read the DDS file from the file.
    bool Load(const string& filename, uint32 nFlags);

    void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);

    bool SetHeaderFromMemory(byte* pFileStart, byte* pFileAfterHeader, uint32 nFlags);
    int AdjustHeader();

    bool PostLoad();

    void AdjustFirstFileName(uint32& nFlags, const char* pFileName, DDSSplitted::TPath& adjustedFileName);

    CImageExtensionHelper::DDS_HEADER   m_DDSHeader;
    CImageExtensionHelper::DDS_HEADER_DXT10 m_DDSHeaderExtension;

    _smart_ptr<IMemoryBlock> m_pFileMemory;
};

void WriteDDS(const byte* dat, int wdt, int hgt, int dpth, int Size, char* name, ETEX_Format eF, int NumMips, ETEX_Type eTT);

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_IMAGE_DDSIMAGE_H



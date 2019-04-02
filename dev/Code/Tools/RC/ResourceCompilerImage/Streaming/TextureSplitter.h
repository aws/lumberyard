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

// Description : Routine for creation of streaming pak


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_STREAMING_TEXTURESPLITTER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_STREAMING_TEXTURESPLITTER_H
#pragma once


#ifndef MAKEFOURCC
#define NEED_UNDEF_MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                    \
    ((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) | \
     ((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24))
#endif

#include "IConvertor.h"
#include "../../ResourceCompiler/ConvertContext.h"
#include <list>
#include <set>
#include <ImageProperties.h>
#include <ImageExtensionHelper.h>

struct D3DBaseTexture;

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define TEXTURESPLITTER_H_SECTION_DEFINES 1
#define TEXTURESPLITTER_H_SECTION_CTEXTURE 2
#define TEXTURESPLITTER_H_SECTION_SMARTPOINTER 3

#if defined(TOOLS_SUPPORT_XENIA)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_DEFINES
    #include "Xenia/TextureSplitter_h_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_DEFINES
    #include "Provo/TextureSplitter_h_provo.inl"
#endif
#endif

namespace TextureHelper
{
    int TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, const ETEX_Format eTF);
}

class CTextureSplitter;  // Forward declaration, required for following declaration
// This is so that the friend declaration will retain static linkage
static string MakeFileName(const string& filePath, const uint32 nChunk, const uint32 nFlags);

// creates pak file from file with list of resources
class CTextureSplitter
    : public IConvertor
    , public ICompiler
{
private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

protected:
    struct STexture;

    friend string MakeFileName(const string& filePath, const uint32 nChunk, const uint32 nFlags);

    // textures work
    void ParseDDSTexture(const char* fileName, std::vector<STexture>& resources, std::vector<uint8>& fileContent);
    bool BuildDDSChunks(const char* fileName, STexture& resource);
    void AssembleDDSTextureChunk(
        const char* fileName, const STexture& resource,
        const uint32 nChunkNumber, std::vector<uint8>* pOutChunk);

    void AddResourceToAdditionalList(const char* fileName, const std::vector<uint8>& fileContent);

    // load texture content
    bool LoadTexture(const char* fileName, std::vector<uint8>& fileContent);

    // postprocessing after loading(clamping mips etc.)
    void PostLoadProcessTexture(std::vector<uint8>& fileContent);

    // Arguments:
    //   fileContent - input image, must not be NULL
    // applied to attached images recursively after processing
    // necessary to correctly swap all endians according to the current platform
    void ProcessPlatformSpecificConversions(std::vector<STexture>& resourcesOut, byte* fileContent, const size_t fileSize);
    void ProcessPlatformSpecificConversions_Orbis(STexture& resource, DWORD dwSides, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, DWORD dwMips, ETEX_Format format, bool bDXTCompressed, uint32 nBitsPerPixel); // ACCEPTED_USE
    void ProcessPlatformSpecificConversions_Durango(STexture& resource, DWORD dwSides, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, DWORD dwMips, ETEX_Format format, bool bDXTCompressed, uint32 nBitsPerPixel); // ACCEPTED_USE

    // process single texture
    void ProcessResource(
        std::vector<string>* pOutFilenames,
        const char* inFileName, const STexture& resource,
        const uint32 nChunk);

    // file IO framework
    bool SaveFile(const string& sFileName, const void* pBuffer, const size_t nSize, const FILETIME& fileTime);

public:
    // settings to store textures for streaming
    enum
    {
        kNumLastMips = 3
    };

public:
    CTextureSplitter();
    virtual ~CTextureSplitter();

    virtual void Release();

    virtual void BeginProcessing(const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

    virtual void DeInit();
    virtual ICompiler* CreateCompiler();
    virtual const char* GetExt(int index) const;

    // Used to override reported name of the source file.
    void SetOverrideSourceFileName(const string& srcFilename);
    string GetSourceFilename();

    enum ETargetType
    {
        eTT_None,
        eTT_PC,
        eTT_Orbis, // ACCEPTED_USE
        eTT_Durango, // ACCEPTED_USE
    };

protected:

    // settings to store textures for streaming
    enum EHeaderInfo
    {
        ehiNumLastMips = 3,
    };

    // flags for different resource purposes
    enum EStreamableResourceFlags
    {
        eStreamableResourceFlags_DDSHasAdditionalAlpha = 1 << 0,  // indicates that DDS texture has attached alpha A8 texture
        eStreamableResourceFlags_DDSIsAttachedAlpha    = 1 << 1,  // indicates that DDS texture is attached alpha A8 texture
        eStreamableResourceFlags_NoSplit                             = 1 << 2       // indicates that the texture is to be unsplitted
    };

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_CTEXTURE
    #include "Xenia/TextureSplitter_h_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_CTEXTURE
    #include "Provo/TextureSplitter_h_provo.inl"
#endif
#endif

    // structure to store mapping information about interleaved resource
    struct STexture
    {
        struct SSurface
        {
            SSurface()
                : m_dwSide()
                , m_dwStartMip()
                , m_dwMips()
                , m_dwWidth()
                , m_dwHeight()
                , m_dwDepth()
                , m_dwSize()
                , m_pData()
            {
            }

            uint32 m_dwSide;
            uint32 m_dwStartMip;
            uint32 m_dwMips;
            uint32 m_dwWidth;
            uint32 m_dwHeight;
            uint32 m_dwDepth;
            uint64 m_dwSize;
            byte* m_pData;
        };

        typedef std::vector<SSurface> MipVec;
        typedef std::vector<MipVec> SideVec;

        struct SBlock
        {
            const byte* m_pSurfaceData;
            uint64      m_nChunkSize;                                                           // size of chunk
            SBlock()
                :  m_pSurfaceData(NULL)
                , m_nChunkSize(0) { }
        };

        typedef std::vector<SBlock> TBlocks;

        // description of single resource chunk, assembled from multiple blocks
        struct SChunkDesc
        {
            uint8           m_nChunkNumber;                                                     // ordered number of chunk
            TBlocks   m_blocks;
            bool operator <(const SChunkDesc& cd) const { return m_blocks.begin()->m_pSurfaceData > cd.m_blocks.begin()->m_pSurfaceData; }  // decreasing by offset

            SChunkDesc()
                : m_nChunkNumber(0) { }
        };

        uint8                                           m_nFileFlags;           // flags for different resource purposes
        CImageExtensionHelper::DDS_HEADER                               m_ddsHeader;
        CImageExtensionHelper::DDS_HEADER_DXT10                 m_ddsHeaderExtension;
        SideVec                                     m_surfaces;
        std::list<SChunkDesc>           m_chunks;                   // all file chunks

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XENIA)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_SMARTPOINTER
    #include "Xenia/TextureSplitter_h_xenia.inl"
#endif
#if defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION TEXTURESPLITTER_H_SECTION_SMARTPOINTER
    #include "Provo/TextureSplitter_h_provo.inl"
#endif
#endif

        SSurface* TryGetSurface(int nSide, int nMip);
        void AddSurface(const SSurface& surf);

        STexture()
            : m_nFileFlags(0) {}
    };

protected:
    volatile long m_refCount;

    EEndian m_currentEndian;
    ETargetType m_targetType;
    bool m_bTile;

    uint32 m_attachedAlphaOffset;

    ConvertContext m_CC;

    string m_sOverrideSourceFile;
};

#ifdef NEED_UNDEF_MAKEFOURCC
    #undef MAKEFOURCC
#endif // NEED_UNDEF_MAKEFOURCC

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_STREAMING_TEXTURESPLITTER_H

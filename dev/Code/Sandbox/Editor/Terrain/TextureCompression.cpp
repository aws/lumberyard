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

#include "StdAfx.h"
#include "TextureCompression.h"

#ifdef MAKEFOURCC
#undef MAKEFOURCC
#endif
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#include "ImageExtensionHelper.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
QFile* CTextureCompression::m_pFile = NULL;
CryCriticalSection CTextureCompression::m_sFileLock;

CTextureCompression::CTextureCompression(const bool bHighQuality)
    : m_bHighQuality(bHighQuality)
{
}

CTextureCompression::~CTextureCompression()
{
}


//////////////////////////////////////////////////////////////////////////
void CTextureCompression::SaveCompressedMipmapLevel(const void* data, size_t size, void* userData)
{
    assert(m_pFile);

    // without header
    m_pFile->write(reinterpret_cast<const char*>(data), size);
}



//////////////////////////////////////////////////////////////////////////
void CTextureCompression::WriteDDSToFile(QFile& file, unsigned char* dat, int w, int h, int Size, ETEX_Format eSrcF, ETEX_Format eDstF, int NumMips, const bool bHeader, const bool bDither)
{
    if (bHeader)
    {
        DWORD dwMagic;
        CImageExtensionHelper::DDS_HEADER ddsh;
        ZeroStruct(ddsh);

        dwMagic = MAKEFOURCC('D', 'D', 'S', ' ');
        file.write(reinterpret_cast<char*>(&dwMagic), sizeof(DWORD));

        ddsh.dwSize = sizeof(CImageExtensionHelper::DDS_HEADER);
        ddsh.dwWidth = w;
        ddsh.dwHeight = h;
        ddsh.dwMipMapCount = NumMips;
        if (!NumMips)
        {
            ddsh.dwMipMapCount = 1;
        }
        ddsh.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
        ddsh.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
        ddsh.ddspf = DDSFormats::GetDescByFormat(eDstF);

        switch (eDstF)
        {
        case eTF_BC1:
            ddsh.ddspf = DDSFormats::DDSPF_DXT1;
            break;
        case eTF_BC2:
            ddsh.ddspf = DDSFormats::DDSPF_DXT3;
            break;
        case eTF_BC3:
            ddsh.ddspf = DDSFormats::DDSPF_DXT5;
            break;
        case eTF_B5G6R5:
            ddsh.ddspf = DDSFormats::DDSPF_R5G6B5;
            break;
        case eTF_B8G8R8:
        case eTF_L8V8U8:
            ddsh.ddspf = DDSFormats::DDSPF_R8G8B8;
            break;
        case eTF_R8G8B8A8:
            ddsh.ddspf = DDSFormats::DDSPF_A8R8G8B8;
            break;
        case eTF_L8:
            ddsh.ddspf = DDSFormats::DDSPF_L8;
            break;
        case eTF_A8L8:
            ddsh.ddspf = DDSFormats::DDSPF_A8L8;
            break;
        default:
            assert(0);
            return;
        }
        file.write(reinterpret_cast<char*>(&ddsh), sizeof(ddsh));
    }

    byte* data = NULL;

    if (eDstF == eTF_B8G8R8 || eDstF == eTF_L8V8U8)
    {
        data = new byte[Size];
        int n = Size / 3;
        for (int i = 0; i < n; i++)
        {
            data[i * 3 + 0] = dat[i * 3 + 2];
            data[i * 3 + 1] = dat[i * 3 + 1];
            data[i * 3 + 2] = dat[i * 3 + 0];
        }
        file.write(reinterpret_cast<char*>(data), Size);
    }
    else
    if (eDstF == eTF_R8G8B8A8)
    {
        data = new byte[Size];
        int n = Size / 4;
        for (int i = 0; i < n; i++)
        {
            data[i * 4 + 0] = dat[i * 4 + 2];
            data[i * 4 + 1] = dat[i * 4 + 1];
            data[i * 4 + 2] = dat[i * 4 + 0];
            data[i * 4 + 3] = dat[i * 4 + 3];
        }
        file.write(reinterpret_cast<char*>(data), Size);
    }
    else
    if (eDstF == eTF_B5G6R5)        // input B8G8R8X8, output B5G6R5
    {
        data = new byte[Size / 2];
        int n = Size / 4;
        for (int i = 0; i < n; i++)
        {
            ColorB rgba(dat[i * 4 + 0], dat[i * 4 + 1], dat[i * 4 + 2], dat[i * 4 + 3]);

            *(uint16*)(&data[i * 2]) = rgba.pack_rgb565();
        }
        file.write(reinterpret_cast<char*>(data), Size / 2);
    }
    else
    if (eDstF == eTF_B4G4R4A4)      // input B8G8R8X8, output B4G4R4A4
    {
        data = new byte[Size / 2];
        int n = Size / 4;

        if (bDither)
        {
            for (int i = 0; i < n; i++)
            {
                int a = clamp_tpl((int)dat[i * 4 + 0] + (rand() % 16) - 8, 0, 255);
                int b = clamp_tpl((int)dat[i * 4 + 1] + (rand() % 16) - 8, 0, 255);
                int c = clamp_tpl((int)dat[i * 4 + 2] + (rand() % 16) - 8, 0, 255);
                int d = clamp_tpl((int)dat[i * 4 + 3] + (rand() % 16) - 8, 0, 255);

                *(uint16*)(&data[i * 2]) = ColorB(a, b, c, d).pack_argb4444();
            }
        }
        else
        {
            for (int i = 0; i < n; i++)
            {
                ColorB rgba(dat[i * 4 + 0], dat[i * 4 + 1], dat[i * 4 + 2], dat[i * 4 + 3]);

                *(uint16*)(&data[i * 2]) = rgba.pack_argb4444();
            }
        }
        file.write(reinterpret_cast<char*>(data), Size / 2);
    }
    else if (eSrcF == eTF_R8G8B8A8 && eDstF == eTF_L8)
    {
        const int n = Size / 4;
        data = new byte[n];

        for (int i = 0; i < n; i++)
        {
            data[i] = dat[i * 4 + 1];           // copy green
        }
        file.write(reinterpret_cast<char*>(data), n);
    }
    else if (eSrcF == eTF_R8G8B8A8 && eDstF == eTF_A8L8)
    {
        data = new byte[Size / 2];
        const int n = Size / 4;

        for (int i = 0; i < n; i++)
        {
            data[i * 2 + 0] = dat[i * 4 + 1];           // copy green
            data[i * 2 + 1] = dat[i * 4 + 3];           // copy alpha
        }
        file.write(reinterpret_cast<char*>(data), n);
    }
    else if (eSrcF == eTF_R8G8B8A8 && CImageExtensionHelper::IsBlockCompressed(eDstF))
    {
        CRY_ASSERT_MESSAGE(false, "Editor should not try to save compressed textures, that is Resource compilers job, only used uncompressed formats for eTerrainPrimaryTextureFormat and eTerrainSecondaryTextureFormat");
    }
    else
    {
        assert(eSrcF == eDstF);

        file.write(reinterpret_cast<char*>(dat), Size);
    }

    SAFE_DELETE_ARRAY(data);
}

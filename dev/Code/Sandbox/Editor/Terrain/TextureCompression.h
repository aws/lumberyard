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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_TEXTURECOMPRESSION_H
#define CRYINCLUDE_EDITOR_TERRAIN_TEXTURECOMPRESSION_H
#pragma once


#include "Util/CryMemFile.h"                // CCryMemFile

class CTextureCompression
{
public:
    // constructor
    // Arguments:
    //   bHighQuality - true:slower but better quality, false=use hardware instead
    CTextureCompression(const bool bHighQuality);
    // destructor
    virtual ~CTextureCompression();

    void WriteDDSToFile(QFile& toFile, unsigned char* dat, int w, int h, int Size,
        ETEX_Format eSrcF, ETEX_Format eDstF, int NumMips, const bool bHeader, const bool bDither);

private: // ------------------------------------------------------------------

    static void SaveCompressedMipmapLevel(const void* data, size_t size, void* userData);

    bool                                        m_bHighQuality;     // true:slower but better quality, false=use hardware instead
    static QFile*                  m_pFile;
    static CryCriticalSection m_sFileLock;
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_TEXTURECOMPRESSION_H

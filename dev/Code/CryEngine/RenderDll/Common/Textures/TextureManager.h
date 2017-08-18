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

// Description : Common texture manager declarations.


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H
#pragma once


#include "CryName.h"

class CTexture;

class CTextureManager
{
public:

    CTextureManager() {}
    virtual ~CTextureManager();

    void PreloadDefaultTextures();
    void ReleaseDefaultTextures();

    const CTexture* GetDefaultTexture(const string& sTextureName) const;
    const CTexture* GetDefaultTexture(const CCryNameTSCRC& sTextureNameID) const;

private:

    typedef std::map<CCryNameTSCRC, CTexture*> TTextureMap;
    TTextureMap m_DefaultTextures;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_TEXTURES_TEXTUREMANAGER_H

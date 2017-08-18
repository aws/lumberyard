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

#include "TextureManager.h"
#include "Texture.h"
#include "ISystem.h"
#include <AzFramework/Asset/AssetSystemBus.h>

//////////////////////////////////////////////////////////////////////////

CTextureManager::~CTextureManager()
{
    ReleaseDefaultTextures();
}

//////////////////////////////////////////////////////////////////////////

void CTextureManager::PreloadDefaultTextures()
{
#if !defined(NULL_RENDERER)
    if (m_DefaultTextures.size())
    {
        return;
    }

    uint32 nDefaultFlags = FT_DONT_STREAM;

    XmlNodeRef root = GetISystem()->LoadXmlFromFile("EngineAssets/defaulttextures.xml");
    if (root)
    {
        // we loop over this twice.
        // we are looping from the back to the front to make sure that the order of the assets are correct ,when we try to load them the second time.
        for (int i = root->getChildCount() - 1; i >= 0; i--)
        {
            XmlNodeRef entry = root->getChild(i);
            if (!entry->isTag("entry"))
            {
                continue;
            }

            // make an ASYNC request to move it to the top of the queue:
            EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, entry->getContent());
        }

        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef entry = root->getChild(i);
            if (!entry->isTag("entry"))
            {
                continue;
            }

            uint32 nFlags = nDefaultFlags;

            // check attributes to modify the loading flags
            int nNoMips = 0;
            if (entry->getAttr("nomips", nNoMips) && nNoMips)
            {
                nFlags |= FT_NOMIPS;
            }

            // default textures should be compiled synchronously:

            if (!gEnv->pCryPak->IsFileExist(entry->getContent()))
            {
                // make a SYNC request to block until its ready.
                EBUS_EVENT(AzFramework::AssetSystemRequestBus, CompileAssetSync, entry->getContent());
            }

            CTexture* pTexture = CTexture::ForName(entry->getContent(), nFlags, eTF_Unknown);
            if (pTexture)
            {
                CCryNameTSCRC nameID(entry->getContent());
                m_DefaultTextures[nameID] = pTexture;
            }
        }
    }
#endif
}

void CTextureManager::ReleaseDefaultTextures()
{
    int n = 0;
    for (TTextureMap::iterator it = m_DefaultTextures.begin(); it != m_DefaultTextures.end(); ++it)
    {
        SAFE_RELEASE(it->second);
        n++;
    }
    m_DefaultTextures.clear();
}

//////////////////////////////////////////////////////////////////////////

const CTexture* CTextureManager::GetDefaultTexture(
    const string& sTextureName) const
{
    CCryNameTSCRC nameID(sTextureName.c_str());
    return GetDefaultTexture(nameID);
}

const CTexture* CTextureManager::GetDefaultTexture(
    const CCryNameTSCRC& sTextureNameID) const
{
    TTextureMap::const_iterator it = m_DefaultTextures.find(sTextureNameID);
    if (it == m_DefaultTextures.end())
    {
        return 0;
    }

    return it->second;
}

//////////////////////////////////////////////////////////////////////////
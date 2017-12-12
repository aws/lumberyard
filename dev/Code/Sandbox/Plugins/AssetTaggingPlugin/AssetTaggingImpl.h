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

#ifndef CRYINCLUDE_ASSETTAGGINGPLUGIN_ASSETTAGGINGIMPL_H
#define CRYINCLUDE_ASSETTAGGINGPLUGIN_ASSETTAGGINGIMPL_H
#pragma once

#include "Include/IAssetTagging.h"
#include "Include/IEditorClassFactory.h"

class CAssetTaggingImpl
    : public IAssetTagging
    , public IClassDesc
{
public:
    CAssetTaggingImpl(void);
    virtual ~CAssetTaggingImpl(void);

    bool Initialize(const char* localpath);
    bool IsLocal();

    int CreateTag(const char* tag, const char* category);
    int CreateAsset(const char* path, const char* project);
    int CreateProject(const char* project);

    void AddAssetsToTag(const char* tag, const char* category, const char* project, char** assets, int nAssets);
    void RemoveAssetsFromTag(const char* tag, const char* category, const char* project, char** assets, int nAssets);
    void RemoveTagFromAsset(const char* tag, const char* category, const char* project, const char* asset);

    int GetNumTagsForAsset(const char* asset);
    int GetTagsForAsset(char** tags, int nTags, const char* asset);

    int GetTagForAssetInCategory(char* tag, const char* asset, const char* category);

    int GetNumAssetsForTag(const char* tag);
    int GetAssetsForTag(char** assets, int nAssets, const char* tag);
    int GetNumAssetsWithDescription(const char* description);
    int GetAssetsWithDescription(char** assets, int nAssets, const char* description);

    int GetNumTags();
    int GetAllTags(char** tags, int nTags);
    void DestroyTag(const char* tag);

    int GetNumCategories();
    int GetAllCategories(char** categories, int nCategories);

    int GetNumTagsForCategory(const char* category);
    int GetTagsForCategory(const char* category, char** tags, int nTags);

    int TagExists(const char* tag, const char* category);
    int AssetExists(const char* relpath);
    int ProjectExists(const char* project);

    int GetErrorString(char* errorString, int nLen);
    int GetMaxStringLen();

    QString GetProjectName();
    bool GetAssetDescription(const char* relpath, char* description, int nChars);
    void SetAssetDescription(const char* relpath, const char* project, const char* description);
    bool AutoCompleteDescription(const char* partdesc, char* description, int nChars);

    // from IClassDesc
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_ASSET_TAGGING; };
    REFGUID ClassID()
    {
        // {3D534CCD-747D-4065-B336-846C07861235}
        static const GUID guid = {
            0x3d534ccd, 0x747d, 0x4065, { 0xb3, 0x36, 0x84, 0x6c, 0x7, 0x86, 0x12, 0x35 }
        };
        return guid;
    }
    virtual QString ClassName() { return "Asset Tagging"; };
    virtual QString Category() { return "AssetTagging"; };
    virtual void ShowAbout() {};

    // from IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj)
    {
        if (riid == __uuidof(IAssetTagging))
        {
            *ppvObj = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; };
    ULONG STDMETHODCALLTYPE Release()
    {
        if ((--m_ref) == 0)
        {
            delete this;
            return 0;
        }
        else
        {
            return m_ref;
        }
    }

private:
    bool m_initialized;
    ULONG m_ref;
};


#endif // CRYINCLUDE_ASSETTAGGINGPLUGIN_ASSETTAGGINGIMPL_H

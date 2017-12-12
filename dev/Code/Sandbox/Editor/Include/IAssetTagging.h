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

// Description : This interface provide access to the asset tagging functionality.
#ifndef CRYINCLUDE_EDITOR_INCLUDE_IASSETTAGGING_H
#define CRYINCLUDE_EDITOR_INCLUDE_IASSETTAGGING_H
#pragma once

#include "IEditorClassFactory.h" 

struct IAssetTagging
    : public IUnknown
{
    DEFINE_UUID(0xDBB805D2, 0xB302, 0x4B84, 0x89, 0x86, 0x82, 0xC4, 0xA8, 0x06, 0x1C, 0x4D)

    virtual bool Initialize(const char* localpath) = 0;
    virtual bool IsLocal() = 0;

    virtual int CreateTag(const char* tag, const char* category) = 0;
    virtual int CreateAsset(const char* path, const char* project) = 0;
    virtual int CreateProject(const char* project) = 0;

    virtual void AddAssetsToTag(const char* tag, const char* category, const char* project, char** assets, int nAssets) = 0;
    virtual void RemoveAssetsFromTag(const char* tag, const char* category, const char* project, char** assets, int nAssets) = 0;
    virtual void RemoveTagFromAsset(const char* tag, const char* category, const char* project, const char* asset) = 0;

    virtual int GetNumTagsForAsset(const char* asset) = 0;
    virtual int GetTagsForAsset(char** tags, int nTags, const char* asset) = 0;

    virtual int GetTagForAssetInCategory(char* tag, const char* asset, const char* category) = 0;

    virtual int GetNumAssetsForTag(const char* tag) = 0;
    virtual int GetAssetsForTag(char** assets, int nAssets, const char* tag) = 0;
    virtual int GetNumAssetsWithDescription(const char* description) = 0;
    virtual int GetAssetsWithDescription(char** assets, int nAssets, const char* description) = 0;

    virtual int GetNumTags() = 0;
    virtual int GetAllTags(char** tags, int nTags) = 0;
    virtual void DestroyTag(const char* tag) = 0;

    virtual int GetNumCategories() = 0;
    virtual int GetAllCategories(char** categories, int nCategories) = 0;

    virtual int GetNumTagsForCategory(const char* category) = 0;
    virtual int GetTagsForCategory(const char* category, char** tags, int nTags) = 0;

    virtual int TagExists(const char* tag, const char* category) = 0;
    virtual int AssetExists(const char* relpath) = 0;
    virtual int ProjectExists(const char* project) = 0;

    virtual int GetErrorString(char* errorString, int nLen) = 0;
    virtual int GetMaxStringLen() = 0;

    virtual QString GetProjectName() = 0;
    virtual bool GetAssetDescription(const char* relpath, char* description, int nChars) = 0;
    virtual void SetAssetDescription(const char* relpath, const char* project, const char* description) = 0;
    virtual bool AutoCompleteDescription(const char* partdesc, char* description, int nChars) = 0;

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) { return E_NOINTERFACE; };
    virtual ULONG STDMETHODCALLTYPE AddRef() { return 0; };
    virtual ULONG STDMETHODCALLTYPE Release() { return 0; };
    //////////////////////////////////////////////////////////////////////////
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IASSETTAGGING_H

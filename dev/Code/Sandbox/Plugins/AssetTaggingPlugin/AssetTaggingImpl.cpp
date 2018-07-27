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

#include "stdafx.h"
#include "AssetTaggingImpl.h"
#include "AssetTagging.h"

CAssetTaggingImpl::CAssetTaggingImpl(void)
    : m_initialized(FALSE)
    , m_ref(0)
{
}

CAssetTaggingImpl::~CAssetTaggingImpl(void)
{
    if (m_initialized)
    {
        AssetTagging_CloseConnection();
    }

    m_initialized = false;
}

bool CAssetTaggingImpl::Initialize(const char* localpath)
{
    if (AssetTagging_Initialize(localpath))
    {
        m_initialized = true;
    }
    return m_initialized;
}

bool CAssetTaggingImpl::IsLocal()
{
    if (!m_initialized)
    {
        return true;
    }

    return AssetTagging_IsLocal();
}

int CAssetTaggingImpl::CreateTag(const char* tag, const char* category)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_CreateTag(tag, category, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::CreateAsset(const char* path, const char* project)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_CreateAsset(path, project);
}

int CAssetTaggingImpl::CreateProject(const char* project)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_CreateProject(project);
}

void CAssetTaggingImpl::AddAssetsToTag(const char* tag, const char* category, const char* project, char** assets, int nAssets)
{
    if (!m_initialized)
    {
        return;
    }

    AssetTagging_AddAssetsToTag(tag, category, GetProjectName().toUtf8().data(), assets, nAssets);
}

void CAssetTaggingImpl::RemoveAssetsFromTag(const char* tag, const char* category, const char* project, char** assets, int nAssets)
{
    if (!m_initialized)
    {
        return;
    }

    AssetTagging_RemoveAssetsFromTag(tag, category, GetProjectName().toUtf8().data(), assets, nAssets);
}

void CAssetTaggingImpl::RemoveTagFromAsset(const char* tag, const char* category, const char* project, const char* asset)
{
    if (!m_initialized)
    {
        return;
    }

    AssetTagging_RemoveTagFromAsset(tag, category, project, asset);
}

int CAssetTaggingImpl::GetNumTagsForAsset(const char* asset)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetNumTagsForAsset(asset, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetTagsForAsset(char** tags, int nTags, const char* asset)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetTagsForAsset(tags, nTags, asset, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetTagForAssetInCategory(char* tag, const char* asset, const char* category)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetTagForAssetInCategory(tag, asset, category, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetNumAssetsForTag(const char* tag)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetNumAssetsForTag(tag, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetAssetsForTag(char** assets, int nAssets, const char* tag)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetAssetsForTag(assets, nAssets, tag, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetNumAssetsWithDescription(const char* description)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetNumAssetsWithDescription(description);
}

int CAssetTaggingImpl::GetAssetsWithDescription(char** assets, int nAssets, const char* description)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetAssetsWithDescription(assets, nAssets, description);
}

int CAssetTaggingImpl::GetNumTags()
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetNumTags(GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetAllTags(char** tags, int nTags)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetAllTags(tags, nTags, GetProjectName().toUtf8().data());
}

void CAssetTaggingImpl::DestroyTag(const char* tag)
{
    if (!m_initialized)
    {
        return;
    }

    AssetTagging_DestroyTag(tag, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetNumCategories()
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetNumCategories(GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetAllCategories(char** categories, int nCategories)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetAllCategories(GetProjectName().toUtf8().data(), categories, nCategories);
}

int CAssetTaggingImpl::GetNumTagsForCategory(const char* category)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetNumTagsForCategory(category, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::GetTagsForCategory(const char* category, char** tags, int nTags)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetTagsForCategory(category, GetProjectName().toUtf8().data(), tags, nTags);
}

int CAssetTaggingImpl::TagExists(const char* tag, const char* category)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_TagExists(tag, category, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::AssetExists(const char* relpath)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_AssetExists(relpath, GetProjectName().toUtf8().data());
}

int CAssetTaggingImpl::ProjectExists(const char* project)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_ProjectExists(project);
}

int CAssetTaggingImpl::GetErrorString(char* errorString, int nLen)
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_GetErrorString(errorString, nLen);
}

int CAssetTaggingImpl::GetMaxStringLen()
{
    if (!m_initialized)
    {
        return 0;
    }
    return AssetTagging_MaxStringLen();
}

QString CAssetTaggingImpl::GetProjectName()
{
    ICVar* pCvar = gEnv->pConsole->GetCVar("sys_game_folder");
    if (pCvar && pCvar->GetString())
    {
        return QString(pCvar->GetString());
    }

    return QString("unknown");
}

bool CAssetTaggingImpl::GetAssetDescription(const char* relpath, char* description, int nChars)
{
    if (!m_initialized)
    {
        return 0;
    }

    return AssetTagging_GetAssetDescription(relpath, GetProjectName().toUtf8().data(), description, nChars);
}

void CAssetTaggingImpl::SetAssetDescription(const char* relpath, const char* project, const char* description)
{
    if (!m_initialized)
    {
        return;
    }

    return AssetTagging_SetAssetDescription(relpath, project, description);
}

bool CAssetTaggingImpl::AutoCompleteDescription(const char* partdesc, char* description, int nChars)
{
    if (!m_initialized)
    {
        return false;
    }
    return AssetTagging_AutoCompleteDescription(partdesc, description, nChars);
}
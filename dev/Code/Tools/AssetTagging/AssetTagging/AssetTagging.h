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

#ifndef CRYINCLUDE_TOOLS_ASSETTAGGING_ASSETTAGGING_ASSETTAGGING_H
#define CRYINCLUDE_TOOLS_ASSETTAGGING_ASSETTAGGING_ASSETTAGGING_H
#pragma once

#include <AzCore/PlatformDef.h>

#ifdef ASSETTAGGING_EXPORTS
#define ASSETTAGGING_API extern "C" AZ_DLL_EXPORT
#else
#define ASSETTAGGING_API extern "C" AZ_DLL_IMPORT
#endif


ASSETTAGGING_API int AssetTagging_MaxStringLen();

ASSETTAGGING_API bool AssetTagging_Initialize(const char* localpath);

ASSETTAGGING_API bool AssetTagging_IsLocal();

ASSETTAGGING_API void AssetTagging_CloseConnection();


ASSETTAGGING_API int AssetTagging_CreateTag(const char* tag, const char* category, const char* project);

ASSETTAGGING_API int AssetTagging_CreateAsset(const char* path, const char* project);

ASSETTAGGING_API int AssetTagging_CreateProject(const char* project);

ASSETTAGGING_API int AssetTagging_CreateCategory(const char* category, const char* project);


ASSETTAGGING_API void AssetTagging_AddAssetsToTag(const char* tag, const char* category, const char* project, char** assets, int nAssets);

ASSETTAGGING_API void AssetTagging_RemoveAssetsFromTag(const char* tag, const char* category, const char* project, char** assets, int nAssets);

ASSETTAGGING_API void AssetTagging_RemoveTagFromAsset(const char* tag, const char* category, const char* project, const char* asset);


ASSETTAGGING_API void AssetTagging_DestroyTag(const char* tag, const char* project);

ASSETTAGGING_API void AssetTagging_DestroyCategory(const char* category, const char* project);


ASSETTAGGING_API int AssetTagging_GetNumTagsForAsset(const char* asset, const char* project);

ASSETTAGGING_API int AssetTagging_GetTagsForAsset(char** tags, int nTags, const char* asset, const char* project);


ASSETTAGGING_API int AssetTagging_GetTagForAssetInCategory(char* tag, const char* asset, const char* category, const char* project);


ASSETTAGGING_API int AssetTagging_GetNumAssetsForTag(const char* tag, const char* project);

ASSETTAGGING_API int AssetTagging_GetAssetsForTag(char** assets, int nAssets, const char* tag, const char* project);

ASSETTAGGING_API int AssetTagging_GetNumAssetsWithDescription(const char* description);

ASSETTAGGING_API int AssetTagging_GetAssetsWithDescription(char** assets, int nAssets, const char* description);


ASSETTAGGING_API int AssetTagging_GetNumTags(const char* project);

ASSETTAGGING_API int AssetTagging_GetAllTags(char** tags, int nTags, const char* project);


ASSETTAGGING_API int AssetTagging_GetNumCategories(const char* project);

ASSETTAGGING_API int AssetTagging_GetAllCategories(const char* project, char** categories, int nCategories);


ASSETTAGGING_API int AssetTagging_GetNumTagsForCategory(const char* category, const char* project);

ASSETTAGGING_API int AssetTagging_GetTagsForCategory(const char* category, const char* project, char** tags, int nTags);


ASSETTAGGING_API int AssetTagging_GetNumProjects();

ASSETTAGGING_API int AssetTagging_GetProjects(char** projects, int nProjects);


ASSETTAGGING_API int AssetTagging_TagExists(const char* tag, const char* category, const char* project);

ASSETTAGGING_API int AssetTagging_AssetExists(const char* relpath, const char* project);

ASSETTAGGING_API int AssetTagging_ProjectExists(const char* project);

ASSETTAGGING_API int AssetTagging_CategoryExists(const char* category, const char* project);


ASSETTAGGING_API void AssetTagging_UpdateCategoryOrderId(const char* category, int idx, const char* project);


ASSETTAGGING_API int AssetTagging_GetErrorString(char* errorString, int nLen);

ASSETTAGGING_API bool AssetTagging_GetAssetDescription(const char* relpath, const char* project, char* description, int nChars);

ASSETTAGGING_API void AssetTagging_SetAssetDescription(const char* relpath, const char* project, const char* description);

ASSETTAGGING_API bool AssetTagging_AutoCompleteDescription(const char* partdesc, char* description, int nChars);
#endif // CRYINCLUDE_TOOLS_ASSETTAGGING_ASSETTAGGING_ASSETTAGGING_H

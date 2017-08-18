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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_IASSETSEARCHER_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_IASSETSEARCHER_H
#pragma once


struct IAssetSearcher
    : public _i_reference_target_t
{
public:
    typedef uint64 TAssetSearchId;
    static const TAssetSearchId INVALID_ID = 0;
    static const TAssetSearchId FIRST_VALID_ID = 1;

    virtual ~IAssetSearcher() {}

    virtual QString GetName() const = 0;

    virtual bool Accept(const char* asset, int& assetTypeId) const = 0;
    virtual bool Accept(const char varType, int& assetTypeId) const = 0;

    virtual QString GetAssetTypeName(int assetTypeId) = 0;

    virtual bool Exists(const char* asset, int assetTypeId) const = 0;
    virtual bool GetReplacement(QString& replacement, int assetTypeId) = 0;

    virtual void StartSearcher() = 0;
    virtual void StopSearcher() = 0;

    virtual TAssetSearchId AddSearch(const char* asset, int assetTypeId) = 0;
    virtual void CancelSearch(TAssetSearchId id) = 0;
    virtual bool GetResult(TAssetSearchId id, QStringList& result, bool& doneSearching) = 0;
};

#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_IASSETSEARCHER_H

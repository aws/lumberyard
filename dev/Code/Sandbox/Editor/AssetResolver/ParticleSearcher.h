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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_PARTICLESEARCHER_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_PARTICLESEARCHER_H
#pragma once


#include "IAssetSearcher.h"

////////////////////////////////////////////////////////////////////////////
class CParticleSearcher
    : public IAssetSearcher
{
public:
    CParticleSearcher();
    ~CParticleSearcher();

    virtual QString GetName() const { return "ParticleLib"; }

    virtual bool Accept(const char* asset, int& assetTypeId) const;
    virtual bool Accept(const char varType, int& assetTypeId) const;

    virtual QString GetAssetTypeName(int assetTypeId);

    virtual bool Exists(const char* asset, int assetTypeId) const;
    virtual bool GetReplacement(QString& replacement, int assetTypeId);

    virtual void StartSearcher();
    virtual void StopSearcher();

    virtual TAssetSearchId AddSearch(const char* asset, int assetTypeId);
    virtual void CancelSearch(TAssetSearchId id);
    virtual bool GetResult(TAssetSearchId id, QStringList& result, bool& doneSearching);

private:
    typedef QStringList TStringVec;
    typedef std::map< IAssetSearcher::TAssetSearchId, TStringVec > TParticleSearchRequests;

    void FindReplacement(const char* particleName, TStringVec& res);
    void SplitPath(const char* name, TStringVec& path);
    int FindMatching(const TStringVec& search, const TStringVec& sub);
    TAssetSearchId GetNextFreeId() const;

private:
    TParticleSearchRequests m_requests;
};

////////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_PARTICLESEARCHER_H

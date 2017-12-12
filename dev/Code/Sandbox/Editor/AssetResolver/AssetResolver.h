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

#ifndef CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVER_H
#define CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVER_H
#pragma once


#include <functor.h>
#include "Util/Variable.h"
#include "Include/IMissingAssetResolver.h"

class CMissingAssetReport;
struct IAssetSearcher;

class CMissingAssetResolver
    : public IMissingAssetResolver
{
public:
    CMissingAssetResolver();
    ~CMissingAssetResolver();

    virtual void Shutdown() override;
    virtual void StartResolver() override;
    virtual void StopResolver() override;
    virtual void PumpEvents() override;

    virtual uint32 AddResolveRequest(const char* assetStr, const TMissingAssetResolveCallback& request, char varType = IVariable::DT_SIMPLE, bool onlyIfNotExist = true) override;
    virtual void CancelRequest(const TMissingAssetResolveCallback& request, uint32 reportId = 0) override;

    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
    // ~IEditorNotifyListener

private: // those functions should be only accessible to the dialog!
    friend class CMissingAssetDialog;
    friend class MissingAssetModel;
    friend class CMissingAssetMessage;
    virtual void AcceptRequest(uint32 reportId, const char* filename) override;
    virtual void CancelRequest(uint32 reportId) override;

    virtual CMissingAssetReport* GetReport() override { return m_pReport; }

    virtual IAssetSearcher* GetAssetSearcherById(int id) const override;

private:
    IAssetSearcher* GetSearcherAndAssetId(int& searcherId, int& assetId, const char* filename, char varType);

private:
    _smart_ptr<CMissingAssetReport> m_pReport;
    CryCriticalSection m_mutex;
    typedef std::map<int, _smart_ptr<IAssetSearcher> > TSearcher;
    TSearcher m_searcher;
    int cv_popupMissingAssetResolver;
    int ed_MissingAssetResolver;
};

#endif // CRYINCLUDE_EDITOR_ASSETRESOLVER_ASSETRESOLVER_H

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
#ifndef CRYINCLUDE_EDITOR_INCLUDE_IMISSINGASSETRESOLVER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IMISSINGASSETRESOLVER_H
#pragma once


#include <functor.h>
#include "Util/Variable.h"

class CMissingAssetReport;
struct IAssetSearcher;

struct IMissingAssetResolver
    : public IEditorNotifyListener
{
    virtual ~IMissingAssetResolver(){}

    virtual void Shutdown() = 0;

    virtual void StartResolver() = 0;

    virtual void StopResolver() = 0;

    virtual void PumpEvents() = 0;

    virtual uint32 AddResolveRequest(const char* assetStr, const TMissingAssetResolveCallback& request, char varType = IVariable::DT_SIMPLE, bool onlyIfNotExist = true) = 0;

    virtual void CancelRequest(const TMissingAssetResolveCallback& request, uint32 reportId = 0) = 0;

    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent ev) = 0;

protected: // Functions bellow should be only accessible to the dialog!
    friend class CMissingAssetDialog;
    friend class MissingAssetModel;
    friend class CMissingAssetMessage;

    virtual void AcceptRequest(uint32 reportId, const char* filename) = 0;

    virtual void CancelRequest(uint32 reportId) = 0;

    virtual CMissingAssetReport* GetReport() = 0;

    virtual IAssetSearcher* GetAssetSearcherById(int id) const = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IMISSINGASSETRESOLVER_H

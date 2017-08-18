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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCEANALYZERNODES_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCEANALYZERNODES_H
#pragma once


#include "SequencerNode.h"
#include "MannequinBase.h"

#include <QMenu>

class CRootNode
    : public CSequencerNode
{
public:
    CRootNode(CSequencerSequence* sequence, const SControllerDef& controllerDef);
    ~CRootNode();

    virtual int GetParamCount() const;
    virtual bool GetParamInfo(int nIndex, SParamInfo& info) const;

    virtual CSequencerTrack* CreateTrack(ESequencerParamType nParamId);
};



class CScopeNode
    : public CSequencerNode
{
public:
    CScopeNode(CSequencerSequence* sequence, SScopeData* pScopeData, EMannequinEditorMode mode);

    QMenu menuSetContext;

    virtual void InsertMenuOptions(QMenu* menu);

    virtual void ClearMenuOptions(QMenu* menu);

    virtual void OnMenuOption(int menuOption);
    virtual IEntity* GetEntity();
    virtual int GetParamCount() const;
    virtual bool GetParamInfo(int nIndex, SParamInfo& info) const;

    virtual CSequencerTrack* CreateTrack(ESequencerParamType nParamId);

    virtual void UpdateMutedLayerMasks(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask);

private:
    SScopeData* m_pScopeData;
    EMannequinEditorMode m_mode;
};


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCEANALYZERNODES_H

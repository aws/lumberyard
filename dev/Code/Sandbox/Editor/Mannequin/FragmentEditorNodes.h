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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITORNODES_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITORNODES_H
#pragma once


#include "SequencerNode.h"
#include "MannequinBase.h"

class CMannFragmentEditor;

class CFragmentNode
    : public CSequencerNode
{
public:
    CFragmentNode(CSequencerSequence* sequence, SScopeData* pScopeData, CMannFragmentEditor* fragmentEditor);

    virtual void OnChange();
    virtual uint32 GetChangeCount() const;

    virtual ESequencerNodeType GetType() const;

    virtual IEntity* GetEntity();

    virtual void InsertMenuOptions(QMenu* menu);
    virtual void ClearMenuOptions(QMenu* menu);
    virtual void OnMenuOption(int menuOption);

    virtual int GetParamCount() const;
    virtual bool GetParamInfo(int nIndex, SParamInfo& info) const;

    virtual bool CanAddTrackForParameter(ESequencerParamType paramId) const;

    virtual CSequencerTrack* CreateTrack(ESequencerParamType nParamId);

    virtual void UpdateMutedLayerMasks(uint32 mutedAnimLayerMask, uint32 mutedProcLayerMask);

    SScopeData* GetScopeData();

    void SetIsSequenceNode(const bool locked);
    bool IsSequenceNode() const;

    void SetSelection(const bool hasFragment, const SFragmentSelection& fragSelection);
    bool HasFragment() const;
    const SFragmentSelection& GetSelection() const;

protected:
    uint32 GetNumTracksOfParamID(ESequencerParamType paramID) const;

    bool CanAddParamType(ESequencerParamType paramID) const;

private:
    CMannFragmentEditor* m_fragmentEditor;
    SScopeData* m_pScopeData;
    SFragmentSelection m_fragSelection;
    bool m_isSequenceNode;
    bool m_hasFragment;
};




class CFERootNode
    : public CSequencerNode
{
public:
    CFERootNode(CSequencerSequence* sequence, const SControllerDef& controllerDef);
    ~CFERootNode();
    virtual ESequencerNodeType GetType() const;
    virtual int GetParamCount() const;
    virtual bool GetParamInfo(int nIndex, SParamInfo& info) const;
    virtual CSequencerTrack* CreateTrack(ESequencerParamType nParamId);
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_FRAGMENTEDITORNODES_H

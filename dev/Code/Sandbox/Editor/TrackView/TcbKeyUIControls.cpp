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
#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"

//////////////////////////////////////////////////////////////////////////
class CTcbKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_tableTCB;
    CSmartVariableArray mv_tableValue;
    CSmartVariable<float> mv_easeIn;
    CSmartVariable<float> mv_easeOut;
    CSmartVariable<float> mv_tension;
    CSmartVariable<float> mv_cont;
    CSmartVariable<float> mv_bias;

    CSmartVariable<float> mv_x;
    CSmartVariable<float> mv_y;
    CSmartVariable<float> mv_z;
    CSmartVariable<float> mv_w;

    virtual void OnCreateVars()
    {
        mv_easeIn->SetLimits(-1.0f, 1.0f);
        mv_easeOut->SetLimits(-1.0f, 1.0f);
        mv_tension->SetLimits(-1.0f, 1.0f);
        mv_cont->SetLimits(-1.0f, 1.0f);
        mv_bias->SetLimits(-1.0f, 1.0f);

        AddVariable(mv_tableValue, "Key Value");
        AddVariable(mv_tableValue, mv_x, "X");
        AddVariable(mv_tableValue, mv_y, "Y");
        AddVariable(mv_tableValue, mv_z, "Z");

        AddVariable(mv_tableTCB, "TCB Parameters");
        AddVariable(mv_tableTCB, mv_easeIn, "Ease In");
        AddVariable(mv_tableTCB, mv_easeOut, "Ease Out");
        AddVariable(mv_tableTCB, mv_tension, "Tension");
        AddVariable(mv_tableTCB, mv_cont, "Continuity");
        AddVariable(mv_tableTCB, mv_bias, "Bias");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType) const
    {
        switch (trackType)
        {
        case eAnimCurveType_TCBFloat:
        case eAnimCurveType_TCBVector:
        case eAnimCurveType_TCBQuat:
            return true;
        }
        return false;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 0; }

    static const GUID& GetClassID()
    {
        // {B50715F3-9C93-41a9-BDD0-F4C83AF93CE8}
        static const GUID guid = {
            0xb50715f3, 0x9c93, 0x41a9, { 0xbd, 0xd0, 0xf4, 0xc8, 0x3a, 0xf9, 0x3c, 0xe8 }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CTcbKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        EAnimCurveType curveType = keyHandle.GetTrack()->GetCurveType();
        if (curveType == eAnimCurveType_TCBFloat || curveType == eAnimCurveType_TCBVector || curveType == eAnimCurveType_TCBQuat)
        {
            ITcbKey tcbKey;
            keyHandle.GetKey(&tcbKey);

            SyncValue(mv_bias, tcbKey.bias, true);
            SyncValue(mv_cont, tcbKey.cont, true);
            SyncValue(mv_tension, tcbKey.tens, true);
            SyncValue(mv_easeIn, tcbKey.easeto, true);
            SyncValue(mv_easeOut, tcbKey.easefrom, true);

            if (curveType == eAnimCurveType_TCBQuat)
            {
                Quat keyQuat = tcbKey.GetQuat();
                Ang3 angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(keyQuat)));
                SyncValue(mv_x, angles.x, true);
                SyncValue(mv_y, angles.y, true);
                SyncValue(mv_z, angles.z, true);
            }
            else
            {
                SyncValue(mv_x, tcbKey.fval[0], true);
                SyncValue(mv_y, tcbKey.fval[1], true);
                SyncValue(mv_z, tcbKey.fval[2], true);
            }

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CTcbKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        EAnimCurveType curveType = keyHandle.GetTrack()->GetCurveType();
        if (curveType == eAnimCurveType_TCBFloat || curveType == eAnimCurveType_TCBVector || curveType == eAnimCurveType_TCBQuat)
        {
            ITcbKey tcbKey;
            keyHandle.GetKey(&tcbKey);

            SyncValue(mv_bias, tcbKey.bias, false, pVar);
            SyncValue(mv_cont, tcbKey.cont, false, pVar);
            SyncValue(mv_tension, tcbKey.tens, false, pVar);
            SyncValue(mv_easeIn, tcbKey.easeto, false, pVar);
            SyncValue(mv_easeOut, tcbKey.easefrom, false, pVar);

            if (curveType == eAnimCurveType_TCBQuat)
            {
                Ang3 angles;
                angles.x = mv_x;
                angles.y = mv_y;
                angles.z = mv_z;

                Quat keyQuat = Quat::CreateRotationXYZ(DEG2RAD(angles));
                tcbKey.SetQuat(keyQuat);
            }
            else
            {
                SyncValue(mv_x, tcbKey.fval[0], false, pVar);
                SyncValue(mv_y, tcbKey.fval[1], false, pVar);
                SyncValue(mv_z, tcbKey.fval[2], false, pVar);
            }

            keyHandle.SetKey(&tcbKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CTcbKeyUIControls, "TrackView.KeyUI.Tcb", "TrackViewKeyUI");

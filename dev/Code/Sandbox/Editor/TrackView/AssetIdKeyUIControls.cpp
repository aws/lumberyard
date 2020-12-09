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

#include "StdAfx.h"

#include <AzCore/Asset/AssetCommon.h>

#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/AssetKey.h>

#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"

class CAssetIdKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray m_variableTable;
    CSmartVariable<QString> m_assetVariable;

    void OnCreateVars() override
    {
        AZ::Data::AssetId assetId;
        assetId.SetInvalid();
        m_assetVariable->SetUserData(assetId.m_subId);
        m_assetVariable->SetDisplayValue(assetId.m_guid.ToString<AZStd::string>().c_str());

        AddVariable(m_variableTable, "Key Properties");
        AddVariable(m_variableTable, m_assetVariable, "Asset", IVariable::DT_ASSET);
    }

    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const override
    {
        return valueType == AnimValueType::AssetId;
    }

    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    // The priority of this needs to be higher than some of the default UI controls for the default track type
    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {6A73ABBD-999E-4369-BB62-596853E78837}
        static const GUID guid =
        {
            0x6A73ABBD, 0x999E, 0x4369, { 0xBB, 0x62, 0x59, 0x68, 0x53, 0xE7, 0x88, 0x37 }
        };
        return guid;
    }
};

bool CAssetIdKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType() || selectedKeys.GetKeyCount() > 1)
    {
        return false;
    }

    const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);
    if (keyHandle.GetTrack()->GetValueType() != AnimValueType::AssetId)
    {
        return false;
    }

    AZ::IAssetKey assetKey;
    keyHandle.GetKey(&assetKey);
    m_assetVariable->SetDisplayValue(assetKey.m_assetId.m_guid.ToString<AZStd::string>().c_str());
    m_assetVariable->SetUserData(assetKey.m_assetId.m_subId);
    m_assetVariable->SetDescription(assetKey.m_assetTypeName.c_str());

    return true;
}

void CAssetIdKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType() || pVar != m_assetVariable.GetVar())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        if (keyHandle.GetTrack()->GetValueType() != AnimValueType::AssetId)
        {
            continue;
        }

        AZ::IAssetKey assetKey;
        keyHandle.GetKey(&assetKey);
        AZStd::string assetGuid = m_assetVariable->GetDisplayValue().toUtf8().data();
        if (!assetGuid.empty())
        {
            AZ::Uuid guid(assetGuid.c_str(), assetGuid.length());
            AZ::u32 subId = m_assetVariable->GetUserData().value<AZ::u32>();
            assetKey.m_assetId = AZ::Data::AssetId(guid, subId);
        }
        keyHandle.SetKey(&assetKey);
    }
}

REGISTER_QT_CLASS_DESC(CAssetIdKeyUIControls, "TrackView.KeyUI.AssetId", "TrackViewKeyUI");

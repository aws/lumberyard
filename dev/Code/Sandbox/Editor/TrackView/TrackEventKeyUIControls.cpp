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

#include "StdAfx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TVEventsDialog.h"
#include "TrackViewSequence.h"
#include "TrackViewTrack.h"
#include "Maestro/Types/AnimParamType.h"

//////////////////////////////////////////////////////////////////////////
class CTrackEventKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_event;
    CSmartVariable<QString> mv_value;
    CSmartVariable<bool> mv_editEvents;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_event, "Track Event");
        mv_event->SetFlags(mv_event->GetFlags() | IVariable::UI_UNSORTED);
        AddVariable(mv_table, mv_value, "Value");
        AddVariable(mv_table, mv_editEvents, "Edit Track Events...");
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, AnimValueType valueType) const
    {
        return paramType == AnimParamType::TrackEvent;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {F7D002EB-1FEA-46fa-B857-FC2B1B990B7F}
        static const GUID guid =
        {
            0xf7d002eb, 0x1fea, 0x46fa, { 0xb8, 0x57, 0xfc, 0x2b, 0x1b, 0x99, 0xb, 0x7f }
        };
        return guid;
    }

private:
    void OnClickedEventEdit();
};

//////////////////////////////////////////////////////////////////////////
bool CTrackEventKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::TrackEvent)
        {
            mv_event.SetEnumList(NULL);

            // Add <None> for empty, unset event
            mv_event->AddEnumItem(QObject::tr("<None>"), "");

            // Add track events
            if (CAnimationContext* pContext = GetIEditor()->GetAnimation())
            {
                CTrackViewSequence* pSequence = pContext->GetSequence();

                if (pSequence)
                {
                    const int iCount = pSequence->GetTrackEventsCount();
                    for (int i = 0; i < iCount; ++i)
                    {
                        mv_event->AddEnumItem(pSequence->GetTrackEvent(i),
                            pSequence->GetTrackEvent(i));
                    }
                }
            }

            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            mv_event = eventKey.event.c_str();
            mv_value = eventKey.eventValue.c_str();
            mv_editEvents = false;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CTrackEventKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    if (mv_editEvents == true)
    {
        mv_editEvents = false;
        OnClickedEventEdit();
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::TrackEvent)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            QByteArray event, value;
            event = static_cast<QString>(mv_event).toUtf8();
            value = static_cast<QString>(mv_value).toUtf8();

            if (pVar == mv_event.GetVar())
            {
                eventKey.event = event.data();
            }
            if (pVar == mv_value.GetVar())
            {
                eventKey.eventValue = value.data();
            }
            eventKey.animation = "";
            eventKey.duration = 0;

            keyHandle.SetKey(&eventKey);
        }
    }
}

void CTrackEventKeyUIControls::OnClickedEventEdit()
{
    if (CAnimationContext* pContext = GetIEditor()->GetAnimation())
    {
        CTrackViewSequence* pSequence = pContext->GetSequence();

        if (pSequence)
        {
            // Create dialog
            CTVEventsDialog dlg;
            dlg.exec();
            QString event = mv_event;
            mv_event.SetEnumList(NULL);

            const int iCount = pSequence->GetTrackEventsCount();
            for (int i = 0; i < iCount; ++i)
            {
                mv_event->AddEnumItem(pSequence->GetTrackEvent(i),
                    pSequence->GetTrackEvent(i));
            }

            // The step below is necessary to make the event drop-down up-to-date.
            mv_event.GetVar()->EnableNotifyWithoutValueChange(true);
            mv_event = event;
            mv_event.GetVar()->EnableNotifyWithoutValueChange(false);
        }
    }
}

REGISTER_QT_CLASS_DESC(CTrackEventKeyUIControls, "TrackView.KeyUI.TrackEvent", "TrackViewKeyUI");

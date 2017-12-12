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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNKEYPROPERTIESDLGFE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNKEYPROPERTIESDLGFE_H
#pragma once


#include "SequencerKeyPropertiesDlg.h"
//#include "SequencerUtils.h"

//class CMannequinDialog;
//class CFragment;
class IAnimationDatabase;
struct SMannequinContexts;

//////////////////////////////////////////////////////////////////////////
class SequencerKeys;
class CMannKeyPropertiesDlgFE
    : public CSequencerKeyPropertiesDlg
{
    Q_OBJECT
public:
    CMannKeyPropertiesDlgFE(QWidget* parent = nullptr);
    ~CMannKeyPropertiesDlgFE();

    //////////////////////////////////////////////////////////////////////////
    // ISequencerEventsListener interface implementation
    //////////////////////////////////////////////////////////////////////////
    virtual void OnKeySelectionChange();
    //////////////////////////////////////////////////////////////////////////

    void ForceUpdate()
    {
        m_forceUpdate = true;
    }

    void OnUpdate();

    void RedrawProps();

    void RepopulateUI(CSequencerKeyUIControls* controls);

    SMannequinContexts* m_contexts;

public slots:
    void OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void OnTagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void OnTagAdded(const CTagDefinition* tagDef, const QString& name);

protected:
    void OnInitDialog();
    void RepopulateSelectedKeyUI();

    int m_lastPropertyType;
    bool m_forceUpdate;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNKEYPROPERTIESDLGFE_H

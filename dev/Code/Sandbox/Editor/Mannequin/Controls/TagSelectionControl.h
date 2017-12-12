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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TAGSELECTIONCONTROL_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TAGSELECTIONCONTROL_H
#pragma once

#include <ICryMannequin.h>
#include <QDialog>

class ReflectedPropertyControl;

class CTagSelectionControl
    : public QDialog
{
    Q_OBJECT
public:
    explicit CTagSelectionControl(QWidget* parent = nullptr);
    virtual ~CTagSelectionControl();

    void SetTagDef(const CTagDefinition* pTagDef);
    const CTagDefinition* GetTagDef() const;

    CVarBlockPtr GetVarBlock() const;

    TagState GetTagState() const;
    void SetTagState(const TagState tagState);

    typedef Functor1< CTagSelectionControl* > OnTagStateChangeCallback;
    void SetOnTagStateChangeCallback(OnTagStateChangeCallback onTagStateChangeCallback);

protected:
    void Reset();

    void OnInternalVariableChange(IVariable* pVar);
    void OnTagStateChange();

private:
    const CTagDefinition* m_pTagDef;

    CVarBlockPtr m_pVarBlock;
    std::vector< CSmartVariable< bool > > m_tagVarList;
    std::vector< CSmartVariableEnum< int > > m_tagGroupList;

    ReflectedPropertyControl* const m_propertyControl;

    bool m_ignoreInternalVariableChange;

    OnTagStateChangeCallback m_onTagStateChangeCallback;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TAGSELECTIONCONTROL_H

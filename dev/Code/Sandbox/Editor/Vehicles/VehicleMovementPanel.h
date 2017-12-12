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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMOVEMENTPANEL_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMOVEMENTPANEL_H
#pragma once



#include "PropertyCtrlExt.h"
#include "VehicleDialogComponent.h"

class CVehicleEditorDialog;

/*!
* CVehicleMovementPanel
*/
class CVehicleMovementPanel
    : public QWidget
    , public IVehicleDialogComponent
{
    Q_OBJECT
public:
    CVehicleMovementPanel(CVehicleEditorDialog* pDialog);
    virtual ~CVehicleMovementPanel();

    void UpdateVehiclePrototype(CVehiclePrototype* pProt);
    void OnPaneClose(){}

    void OnMovementTypeChange(IVariable* pVar);

private slots:
    void DoOnMovementTypeChange(IVariable* pVar);

protected:
    void ExpandProps(ReflectedPropertyItem* pItem, bool expand = true);


    CVehiclePrototype* m_pVehicle;

    // pointer to main dialog
    CVehicleEditorDialog* m_pDialog;

    // controls
    CPropertyCtrlExt m_propsCtrl;
    CVarBlockPtr m_pMovementBlock;
    CVarBlockPtr m_pPhysicsBlock;
    CVarBlockPtr m_pTypeBlock;

    IVariable::OnSetCallback m_onSetCallback;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMOVEMENTPANEL_H

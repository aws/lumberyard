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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMODIFICATIONDIALOG_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMODIFICATIONDIALOG_H
#pragma once

#include <QDialog>

namespace Ui {
    class CVehicleModificationDialog;
}

class VehicleModificationModel;

class CVehicleModificationDialog
    : public QDialog
{
    Q_OBJECT

public:
    CVehicleModificationDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CVehicleModificationDialog();

    void SetVariable(IVariable* pVar);
    IVariable* GetVariable(){ return m_pVar; }
  
protected:

    void OnSelChanged();
    void OnModNew();
    void OnModClone();
    void OnModDelete();

    void ReloadModList();
    void SelectMod(IVariable* pMod);
    IVariable* GetSelectedVar();

    IVariable* m_pVar;

    int m_propsLeft, m_propsTop;
    
    VehicleModificationModel* m_model;
    QScopedPointer<Ui::CVehicleModificationDialog> ui;
};


#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEMODIFICATIONDIALOG_H

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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPAINTSPANEL_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPAINTSPANEL_H
#pragma once

#include <QWidget>

#include "IDataBaseManager.h"

namespace Ui {
    class VehiclePaintsPanel;
}

class PaintNameModel;

class CVehiclePaintsPanel
    : public QWidget
    , public IDataBaseManagerListener
{
    Q_OBJECT
public:
    CVehiclePaintsPanel(QWidget* parent = nullptr);
    virtual ~CVehiclePaintsPanel();

    void InitPaints(IVariable* pPaints);
    void Clear();

    // IDataBaseManagerListener
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);
    // ~IDataBaseManagerListener

protected:
    void OnAddNewPaint();
    void OnRemoveSelectedPaint();
    void OnAssignMaterialToSelectedPaint();
    void OnApplyPaintToVehicle();
    void OnPaintNameSelectionChanged();

    void OnInitDialog();

    void CreateNewPaint(const QString& paintName);

private:
    bool IsPaintNameSelected() const;
    QString GetSelectedPaintName() const;
    int GetSelectedPaintNameVarIndex() const;
    void RenameSelectedPaintName(const QString& name);
    void AddPaintName(const QString& paintName, int paintVarIndex);

    void HidePaintProperties();
    void ShowPaintPropertiesByName(const QString& paintName);
    void ShowPaintPropertiesByVarIndex(int paintVarIndex);
    void ShowPaintProperties(IVariable* pPaint);

    IVariable* GetPaintVarByName(const QString& paintName);
    IVariable* GetPaintVarByIndex(int index);

    void OnSelectedPaintNameChanged(IVariable* pVar);
    void OnSelectedPaintMaterialChanged(IVariable* pVar);

    void UpdateAssignMaterialButtonState();

private:
    PaintNameModel* m_paintModel;
    IVariable* m_pSelectedPaint;

    QScopedPointer<Ui::VehiclePaintsPanel> m_ui;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPAINTSPANEL_H

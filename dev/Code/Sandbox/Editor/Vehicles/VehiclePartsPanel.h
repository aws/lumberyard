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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPARTSPANEL_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPARTSPANEL_H
#pragma once


#include "PropertyCtrlExt.h"

#include "VehicleDialogComponent.h"

#include <QDialog>

class CVehicleEditorDialog;
class CVehicleHelper;
class CVehiclePart;
class CVehiclePartsPanel;
class CVehicleWeapon;
class CVehicleSeat;

class QTreeWidgetItem;

namespace Ui
{
    class VehiclePartsPanel;
    class WheelMasterDialog;
}

//! CWheelMasterDialog
class CWheelMasterDialog
    : public QDialog
{
    Q_OBJECT
public:
    CWheelMasterDialog(IVariable* pVar, QWidget* pParent = NULL);   // standard constructor
    virtual ~CWheelMasterDialog();


    IVariable* GetVariable(){ return m_pVar; }

    void SetWheels(std::vector<CVehiclePart*>* pWheels){ m_pWheels = pWheels; }

#define NUM_BOXES 9

protected:
    void showEvent(QShowEvent *event) override;

    void OnCheckBoxClicked();
    void OnApplyWheels();
    void OnToggleAll();

    void ApplyCheckBoxes();

    IVariable* m_pVar;

    std::vector<CVehiclePart*>* m_pWheels;

private:
    QScopedPointer<Ui::WheelMasterDialog> m_ui;
    bool m_initialized;
};


/*!
* CVehiclePartsPanel
*/
class CVehiclePartsPanel
    : public QWidget
    , public IVehicleDialogComponent
{
    Q_OBJECT
public:
    CVehiclePartsPanel(CVehicleEditorDialog* pDialog);
    virtual ~CVehiclePartsPanel();

    // VehicleEditorComponent
    void UpdateVehiclePrototype(CVehiclePrototype* pProt);
    void OnPaneClose();
    void NotifyObjectsDeletion(CVehiclePrototype* pProt);

    void UpdateVariables();
    void ReloadTreeCtrl();
    void OnApplyChanges(IVariable* pVar);
    void OnObjectEvent(CBaseObject* object, int event);

    template<typename T>
    void DeleteTreeObjects()
    {
        CUndoSuspend susp;

#ifdef _DEBUG
        DumpPartTreeMap();
#endif

        for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); )
        {
            if (qobject_cast<T>((*it).first))
            {
                // delete tree item, delete editor object, erase from map
                delete (*it).second.item;
                (*it).first->RemoveEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
                m_pObjMan->DeleteObject(it->first);
                m_partToTree.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        GetIEditor()->SetModifiedFlag();
    }

    CVehiclePart* GetPartForHelper(CVehicleHelper* pHelper);

    void ReloadPropsCtrl();
    void OnSetPartClass(IVariable* pVar);

    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void OnItemRClick(const QPoint& point);
    void closeEvent(QCloseEvent* event);

    void OnHelperNew();
    void OnHelperRename();
    void OnHelperRenameDone(QTreeWidgetItem* item);
    void OnSelect(const QModelIndex& index);
    void OnPartNew();
    void OnPartSelect();
    void OnSeatNew();
    void OnDeleteItem();
    void OnPrimaryWeaponNew();
    void OnSecondaryWeaponNew();
    void OnDisplaySeats();
    void OnEditWheelMaster();
    void OnComponentNew();
    void OnScaleHelpers();

    void DeleteObject(CBaseObject* pObj);

    void ExpandProps(ReflectedPropertyItem* pItem, bool expand = true);
    int FillParts(IVariablePtr pData);
    void AddParts(IVariable* pParts, CBaseObject* pParent);
    void FillHelpers(IVariablePtr pData);
    void FillAssetHelpers();
    void RemoveAssetHelpers();
    void FillSeats();
    void FillComps(IVariablePtr pData);

    void ShowAssetHelpers(bool bShow);
    void HideAssetHelpers();

    void ShowVeedHelpers(bool bShow);
    void ShowSeats(bool bShow);
    void ShowWheels(bool bShow);
    void ShowComps(bool bShow);

    CVehicleHelper* CreateHelperObject(const Vec3& pos, const Vec3& dir, const QString& name, bool unique, bool select, bool editLabel, bool isFromAsset, IVariable* pHelperVar = 0);
    void CreateHelpersFromStatObj(IStatObj* pObj, CVehiclePart* pParent);
    CVehicleWeapon* CreateWeapon(int weaponType, CVehicleSeat* pSeat, IVariable* pVar = 0);

    void DeleteSeats();
    CVehiclePart* FindPart(const QString& name);
    void DumpPartTreeMap();

    void ShowObject(TPartToTreeMap::iterator it, bool bShow);

    QTreeWidgetItem* InsertTreeItem(CBaseObject* pObj, CBaseObject* pParent, bool select = false);
    QTreeWidgetItem* InsertTreeItem(CBaseObject* pObj, QTreeWidgetItem* hParent, bool select = false);
    void DeleteTreeItem(TPartToTreeMap::iterator it);

    CVehiclePart* GetMainPart();
    IVariable* GetVariable();
    void GetWheels(std::vector<CVehiclePart*>& wheels);


    CVehiclePrototype* m_pVehicle;

    // pointer to main dialog
    CVehicleEditorDialog* m_pDialog;

    TPartToTreeMap& m_partToTree;

    // controls
    QTreeWidgetItem* m_hVehicle;

    CBaseObject* m_pSelItem;

    CVehiclePart* m_pMainPart;

    IObjectManager* m_pObjMan;

    IVariablePtr m_pRootVar;

    QScopedPointer<Ui::VehiclePartsPanel> m_ui;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPARTSPANEL_H

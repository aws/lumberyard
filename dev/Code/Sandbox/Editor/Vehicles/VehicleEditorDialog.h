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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEEDITORDIALOG_H
#pragma once

#include <list>
#include <type_traits>

#include "VehicleDialogComponent.h"
#include "VehiclePaintsPanel.h"

#include <QMainWindow>

class CVehiclePrototype;
class CVehicleMovementPanel;
class CVehiclePartsPanel;
class CVehicleFXPanel;

class ReflectedPropertyItem;

class QToolButton;

//////////////////////////////////////////////////////////////////////////
//
// Main Dialog for Veed, the Vehicle Editor.
//
//////////////////////////////////////////////////////////////////////////
class CVehicleEditorDialog
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    static void RegisterViewClass();
    static const GUID& GetClassID();

    CVehicleEditorDialog(QWidget* parent = nullptr);
    ~CVehicleEditorDialog();

    void RepositionPropertiesCtrl();

    QWidget* GetTaskPanel() { return m_taskPanel; }
    CVehiclePartsPanel* GetPartsPanel() { return m_partsPanel; }
    CVehicleMovementPanel* GetMovementPanel() { return m_movementPanel; }

    // vehicle logic
    void SetVehiclePrototype(CVehiclePrototype* pProt);
    CVehiclePrototype* GetVehiclePrototype(){ return m_pVehicle; }

    bool ApplyToVehicle(QString filename = QString(), bool mergeFile = true);

    void OnPrototypeEvent(CBaseObject* object, int event);
    void OnEntityEvent(CBaseObject* object, int event);

    template<typename T>
    void GetObjectsByClass(std::vector<CBaseObject*>& objects)
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        GetObjectsByClass(&ObjType::staticMetaObject, objects);
    }
    void GetObjectsByClass(const QMetaObject* pClass, std::vector<CBaseObject*>& objects);

protected:
    void OnInitDialog();
    void OnDestroy();
    void closeEvent(QCloseEvent* event) override;
    void OnFileNew();
    void OnFileOpen();
    void OnFileSave();
    void OnFileSaveAs();

    void OnMovementEdit();
    void OnPartsEdit();
    void OnFXEdit();
    void OnModsEdit();
    void OnPaintsEdit();

    void CreateTaskPanel();

    void OnPropsSelChanged(ReflectedPropertyItem* pItem);
    void DeleteEditorObjects();
    void EnableEditingLinks(bool enable);
    bool OpenVehicle(bool silent = false);

    void OnVehicleEntityEvent(CBaseObject* pObject, int event);

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

private:

    void DestroyVehiclePrototype();

    // data
    CVehiclePrototype* m_pVehicle;

    // dialog stuff
    QWidget* m_taskPanel;
    QDockWidget* m_pTaskDockPane;

    typedef std::list<IVehicleDialogComponent*> TVeedComponent;
    TVeedComponent m_panels;
    CVehicleMovementPanel* m_movementPanel;
    CVehiclePartsPanel* m_partsPanel;
    CVehicleFXPanel* m_FXPanel;
    CVehiclePaintsPanel m_paintsPanel;

    QToolButton* m_buttonSave;
    QToolButton* m_buttonSaveAs;
    QToolButton* m_buttonPartsEdit;
    QToolButton* m_buttonMovementEdit;
    QToolButton* m_buttonSeatsEdit;
    QToolButton* m_buttonModsEdit;
    QToolButton* m_buttonPaintsEdit;


    // Map of library items to tree ctrl.
    TPartToTreeMap m_partToTree;

    friend class CVehiclePartsPanel;
    friend class CVehicleFXPanel;
    friend class CVehicleMovementPanel;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEEDITORDIALOG_H

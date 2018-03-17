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

#include "VehicleEditorDialog.h"
#include "IViewPane.h"
#include "Objects/EntityObject.h"
#include "Util/PakFile.h"
#include "Objects/SelectionGroup.h"

#include "VehiclePrototype.h"
#include "VehicleData.h"
#include "VehicleXMLSaver.h"
#include "VehicleMovementPanel.h"
#include "VehiclePartsPanel.h"
#include "VehicleFXPanel.h"
#include "VehicleHelperObject.h"
#include "VehiclePart.h"
#include "VehicleComp.h"
#include "VehicleModificationDialog.h"
#include "VehiclePaintsPanel.h"
#include "VehicleXMLHelper.h"
#include "QtViewPaneManager.h"
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QtViewPane.h>

#include <QBoxLayout>
#include <QCloseEvent>
#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileInfo>

#include <QtUI/QCollapsibleGroupBox.h>

#include "QtUtil.h"

#define VEED_FILE_FILTER "Vehicle XML Files (*.xml)"

enum
{
    ID_VEED_MOVEMENT_EDIT = 100,
    ID_VEED_PARTS_EDIT,
    ID_VEED_SEATS_EDIT,
    ID_VEED_MODS_EDIT,
    ID_VEED_PAINTS_EDIT
};

static QDockWidget* findDockWidget(QWidget* widget)
{
    if (widget == nullptr)
    {
        return nullptr;
    }
    QDockWidget* w = qobject_cast<QDockWidget*>(widget);
    if (w)
    {
        return w;
    }
    return findDockWidget(widget->parentWidget());
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(200, 200, 600, 500);
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;

    AzToolsFramework::RegisterViewPane<CVehicleEditorDialog>("Vehicle Editor", LyViewPane::CategoryOther, options);
}

const GUID& CVehicleEditorDialog::GetClassID()
{
    // {8780CFDE-5415-4ac8-AFA3-2B95922A0705}
    static const GUID guid = {
        0x8780cfde, 0x5415, 0x4ac8, { 0xaf, 0xa3, 0x2b, 0x95, 0x92, 0x2a, 0x7, 0x5 }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
CVehicleEditorDialog::CVehicleEditorDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_pVehicle(0)
    , m_movementPanel(0)
    , m_partsPanel(0)
    , m_FXPanel(0)
{
    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CVehicleEditorDialog::~CVehicleEditorDialog()
{
    OnDestroy();
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnInitDialog()
{
    setCentralWidget(new QWidget);

    // Add the menu bar
    QMenuBar* pMenuBar = menuBar();
    QMenu* menu = pMenuBar->addMenu(tr("Vehicle"));
    connect(menu->addAction(tr("New")), &QAction::triggered, this, &CVehicleEditorDialog::OnFileNew);
    connect(menu->addAction(tr("Open Selected")), &QAction::triggered, this, &CVehicleEditorDialog::OnFileOpen);
    connect(menu->addAction(tr("Save")), &QAction::triggered, this, &CVehicleEditorDialog::OnFileSave);
    connect(menu->addAction(tr("Save As..")), &QAction::triggered, this, &CVehicleEditorDialog::OnFileSaveAs);

    // init TaskPanel
    CreateTaskPanel();

    QDockWidget* paintsPanelDW = new AzQtComponents::StyledDockWidget;
    addDockWidget(Qt::LeftDockWidgetArea, paintsPanelDW, Qt::Horizontal);
    paintsPanelDW->setWidget(&m_paintsPanel);
    paintsPanelDW->setWindowTitle(tr("Paints"));
    paintsPanelDW->close();

    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CVehicleEditorDialog::OnVehicleEntityEvent));
    GetIEditor()->RegisterNotifyListener(this);
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::CreateTaskPanel()
{
    /////////////////////////////////////////////////////////////////////////
    // Docking Pane for TaskPanel
    m_pTaskDockPane = new AzQtComponents::StyledDockWidget;
    m_pTaskDockPane->setFeatures(QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_pTaskDockPane);

    /////////////////////////////////////////////////////////////////////////
    // Create empty Task Panel
    m_pTaskDockPane->setWidget(m_taskPanel = new QWidget);
    m_taskPanel->setMaximumWidth(180);
    m_taskPanel->setLayout(new QVBoxLayout);

    QToolButton* pItem =  nullptr;
    QCollapsibleGroupBox* pGroup = nullptr;

    //////////////////////
    // Open/Save group
    //////////////////////
    m_taskPanel->layout()->addWidget(pGroup = new QCollapsibleGroupBox(m_taskPanel));
    pGroup->setTitle(tr("Task"));
    pGroup->setLayout(new QVBoxLayout);

    // placeholder
    pGroup->layout()->addItem(new QSpacerItem(0, 19, QSizePolicy::Fixed, QSizePolicy::Fixed));

    pGroup->layout()->addWidget(pItem = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Open Selected"));
    pItem->setToolTip(tr("Open currently selected vehicle"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-11.png")));
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnFileOpen);

    // apply button
    pGroup->layout()->addWidget(pItem = m_buttonSave = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Save"));
    pItem->setToolTip(tr("Save Changes"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-12.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnFileSave);

    pGroup->layout()->addWidget(pItem = m_buttonSaveAs = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Save As..."));
    pItem->setToolTip(tr("Apply Changes"));
    QPixmap empty(16, 16);
    empty.fill(Qt::transparent);
    pItem->setIcon(empty);
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnFileSaveAs);

    //////////////////////
    // Edit group
    //////////////////////
    m_taskPanel->layout()->addWidget(pGroup = new QCollapsibleGroupBox(m_taskPanel));
    pGroup->setTitle(tr("Edit"));
    pGroup->setLayout(new QVBoxLayout);

    // placeholder
    pGroup->layout()->addItem(new QSpacerItem(0, 19, QSizePolicy::Fixed, QSizePolicy::Fixed));

    pGroup->layout()->addWidget(pItem = m_buttonPartsEdit = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Structure"));
    pItem->setToolTip(tr("Edit part structure"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-03.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnPartsEdit);

    pGroup->layout()->addWidget(pItem = m_buttonMovementEdit = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Movement"));
    pItem->setToolTip(tr("Edit Movement"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-06.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnMovementEdit);

    pGroup->layout()->addWidget(pItem = m_buttonSeatsEdit = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Particles"));
    pItem->setToolTip(tr("Edit Particles"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-09.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnFXEdit);

    pGroup->layout()->addWidget(pItem = m_buttonModsEdit = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Modifications"));
    pItem->setToolTip(tr("Edit Modifications"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-14.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnModsEdit);

    pGroup->layout()->addWidget(pItem = m_buttonPaintsEdit = new QToolButton);
    pItem->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pItem->setAutoRaise(true);
    pItem->setText(tr("Paints"));
    pItem->setToolTip(tr("Edit Paints"));
    pItem->setIcon(QIcon(QStringLiteral(":/VehicleDialog/veed_tree-11.png")));
    pItem->setEnabled(false);
    connect(pItem, &QToolButton::clicked, this, &CVehicleEditorDialog::OnPaintsEdit);

    m_taskPanel->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::SetVehiclePrototype(CVehiclePrototype* pProt)
{
    // delete current objects, if present
    DeleteEditorObjects();

    DestroyVehiclePrototype();

    assert (pProt && pProt->GetVariable());
    m_pVehicle = pProt;

    m_pVehicle->AddEventListener(functor(*this, &CVehicleEditorDialog::OnPrototypeEvent));

    if (m_pVehicle->GetCEntity())
    {
        m_pVehicle->GetCEntity()->AddEventListener(functor(*this, &CVehicleEditorDialog::OnEntityEvent));
    }

    for (TVeedComponent::iterator it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        (*it)->UpdateVehiclePrototype(m_pVehicle);
    }

    if (IVariable* pPaints = GetOrCreateChildVar(pProt->GetVariable(), "Paints"))
    {
        m_paintsPanel.InitPaints(pPaints);
    }

    // if no panel open,
    if (m_panels.size() == 0)
    {
        OnPartsEdit();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnPrototypeEvent(CBaseObject* object, int event)
{
    // called upon prototype deletion
    if (event == CBaseObject::ON_DELETE)
    {
        m_pVehicle = 0;
        EnableEditingLinks(false);
    }
    else if (event == CBaseObject::ON_SELECT)
    {
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnEntityEvent(CBaseObject* object, int event)
{
    // called upon deletion of the entity the prototype points to
    if (event == CBaseObject::ON_DELETE)
    {
        // delete prototype
        if (m_pVehicle)
        {
            GetIEditor()->DeleteObject(m_pVehicle);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::EnableEditingLinks(bool enable)
{
    m_buttonSave->setEnabled(enable);
    m_buttonSaveAs->setEnabled(enable);

    m_buttonPartsEdit->setEnabled(enable);
    m_buttonMovementEdit->setEnabled(enable);
    m_buttonSeatsEdit->setEnabled(enable);
    m_buttonModsEdit->setEnabled(enable);
    m_buttonPaintsEdit->setEnabled(enable);
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnMovementEdit()
{
    if (!m_pVehicle)
    {
        return;
    }

    if (!m_movementPanel)
    {
        // open dock
        QDockWidget* panelDW = new AzQtComponents::StyledDockWidget;
        addDockWidget(Qt::LeftDockWidgetArea, panelDW, Qt::Horizontal);
        panelDW->setWindowTitle(tr("Movement"));

        // create panel
        m_movementPanel = new CVehicleMovementPanel(this);
        panelDW->setWidget(m_movementPanel);
        m_panels.push_back(m_movementPanel);

        m_movementPanel->UpdateVehiclePrototype(m_pVehicle);
    }
    else
    {
        findDockWidget(m_movementPanel)->show();
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnPartsEdit()
{
    if (!m_pVehicle)
    {
        return;
    }

    if (!m_partsPanel)
    {
        // open dock
        QDockWidget* panelDW = new AzQtComponents::StyledDockWidget;
        addDockWidget(Qt::LeftDockWidgetArea, panelDW, Qt::Horizontal);
        panelDW->setWindowTitle(tr("Structure"));

        // create panel
        m_partsPanel = new CVehiclePartsPanel(this);
        panelDW->setWidget(m_partsPanel);
        m_panels.push_back(m_partsPanel);

        m_partsPanel->UpdateVehiclePrototype(m_pVehicle);
    }
    else
    {
        findDockWidget(m_partsPanel)->show();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnFXEdit()
{
    if (!m_pVehicle)
    {
        return;
    }

    if (!m_FXPanel)
    {
        // open dock
        QDockWidget* panelDW = new AzQtComponents::StyledDockWidget;
        addDockWidget(Qt::LeftDockWidgetArea, panelDW, Qt::Horizontal);
        panelDW->setWindowTitle(tr("Particles"));

        // create panel
        m_FXPanel = new CVehicleFXPanel(this);
        panelDW->setWidget(m_FXPanel);
        m_panels.push_back(m_FXPanel);

        m_FXPanel->UpdateVehiclePrototype(m_pVehicle);

        //m_taskPanel.FindGroup(1)->FindItem(ID_VEED_PARTS_APPLY)->SetEnabled(true);
    }
    else
    {
        findDockWidget(m_FXPanel)->show();
    }
}


//////////////////////////////////////////////////////////////////////////
bool CVehicleEditorDialog::ApplyToVehicle(QString filename, bool mergeFile)
{
    if (!m_pVehicle)
    {
        return false;
    }

    // save xml
    if (filename.isEmpty())
    {
        filename = m_pVehicle->GetCEntity()->GetEntityClass() + QStringLiteral(".xml");
    }


    //update vehicle name corresponding to chosen filename
    if (IVariable* pName = GetChildVar(m_pVehicle->GetVariable(), "name"))
    {
        pName->Set(Path::GetFileName(filename));
    }

    m_pVehicle->ApplyClonedData();


    /*CPakFile pakFile;
    if (pakFile.Open( VEHICLE_XML_PATH + filename )) // absolute path?
    {
      CString xmlString = node->getXML();
      pakFile.UpdateFile( VEHICLE_XML_PATH + filename, (void*)((const char*)xmlString), xmlString.GetLength() );
    }*/

    QString absDir = Path::GamePathToFullPath(VEHICLE_XML_PATH);
    CFileUtil::CreateDirectory(absDir.toUtf8().data()); // make sure dir exist
    QString targetFile = absDir + filename;
    XmlNodeRef node;

    if (mergeFile)
    {
        const XmlNodeRef& vehicleDef = CVehicleData::GetXMLDef();
        DefinitionTable::ReloadUseReferenceTables(vehicleDef);
        node = VehicleDataMergeAndSave(targetFile.toUtf8().data(), vehicleDef, m_pVehicle->GetVehicleData());
    }
    else
    {
        node = VehicleDataSave(VEHICLE_XML_DEF.toUtf8().data(), m_pVehicle->GetVehicleData());
    }

    if (node != 0 && XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), node, targetFile.toUtf8().data()))
    {
        Log("[CVehicleEditorDialog]: saved to %s.", targetFile.toUtf8().data());
    }
    else
    {
        Log("[CVehicleEditorDialog]: not saved!", targetFile.toUtf8().data());
        return false;
    }

    // update the vehicle prototype
    bool res = m_pVehicle->ReloadEntity();

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::closeEvent(QCloseEvent* event)
{
    DeleteEditorObjects();

    // hide when prototype still existent
    if (!m_pVehicle)
    {
        deleteLater();
    }

    event->accept();
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::DestroyVehiclePrototype()
{
    if (m_pVehicle == NULL)
    {
        return;
    }

    VehicleXml::CleanUp(m_pVehicle->GetVariable());

    m_pVehicle->RemoveEventListener(functor(*this, &CVehicleEditorDialog::OnPrototypeEvent));

    if (m_pVehicle->GetCEntity() != NULL)
    {
        m_pVehicle->GetCEntity()->RemoveEventListener(functor(*this, &CVehicleEditorDialog::OnEntityEvent));
    }

    GetIEditor()->DeleteObject(m_pVehicle);

    m_pVehicle = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnDestroy()
{
    for (TVeedComponent::iterator it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        (*it)->OnPaneClose();
    }

    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CVehicleEditorDialog::OnVehicleEntityEvent));

    DeleteEditorObjects();

    DestroyVehiclePrototype();
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnFileNew()
{
    QMessageBox::information(this, tr("Editor"), tr("To create a new vehicle, place an existing prototype in your level (use\n\"DefaultVehicle\" for a nearly empty template) and select \"Save As\" to\nsave it as new vehicle class. The Editor needs to be restarted to register\nthe new vehicle class."));
}



//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnFileOpen()
{
    OpenVehicle();
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnFileSave()
{
    if (!m_pVehicle)
    {
        Log("No active vehicle prototype, can't save to file");
        return;
    }

    const QString message = tr("This will overwrite the following file:\n\n%1%2.xml\n\nAre you sure?").arg(VEHICLE_XML_PATH).arg(m_pVehicle->GetCEntity()->GetEntityClass());

    if (QMessageBox::question(this, tr("Overwrite Vehicle xml?"), message) == QMessageBox::Yes)
    {
        ApplyToVehicle();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnFileSaveAs()
{
    if (!m_pVehicle)
    {
        Log("No active vehicle prototype");
        return;
    }

    // CString dir = Path::GetPath(filename);
    QString filename("MyVehicle.xml");
    QString path = Path::GamePathToFullPath(VEHICLE_XML_PATH);
    if (CFileUtil::SelectSaveFile(VEED_FILE_FILTER, "xml", path, filename))
    {
        QWaitCursor wait;

        QFileInfo info(filename);
        if (ApplyToVehicle(info.fileName(), false))
        {
            Warning("The new vehicle entity will be registered at next Editor/Game launch.");
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
namespace
{
    void GetObjectsByClassRec(CBaseObject* pObject, const QMetaObject* pClass, std::vector< CBaseObject* >& objectsOut)
    {
        if (pObject == NULL)
        {
            return;
        }

        if (pClass->cast(pObject))
        {
            objectsOut.push_back(pObject);
        }

        for (int i = 0; i < pObject->GetChildCount(); i++)
        {
            CBaseObject* pChildObject = pObject->GetChild(i);
            GetObjectsByClassRec(pChildObject, pClass, objectsOut);
        }
    }
}

void CVehicleEditorDialog::GetObjectsByClass(const QMetaObject* pClass, std::vector<CBaseObject*>& objects)
{
    GetObjectsByClassRec(m_pVehicle, pClass, objects);
}

//////////////////////////////////////////////////////////////////////////////
inline bool SortObjectsByName(CBaseObject* pObject1, CBaseObject* pObject2)
{
    return QString::compare(pObject1->GetName(), pObject2->GetName(), Qt::CaseInsensitive) < 0;
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnPropsSelChanged(ReflectedPropertyItem* pItem)
{
    // dynamically fills selectboxes

    if (!pItem)
    {
        return;
    }

    int varType = pItem->GetVariable()->GetDataType();

    if (varType != IVariable::DT_VEEDHELPER && varType != IVariable::DT_VEEDPART
        && varType != IVariable::DT_VEEDCOMP)
    {
        return;
    }

    std::vector<CBaseObject*> objects;

    switch (varType)
    {
    case IVariable::DT_VEEDHELPER:
        GetObjectsByClass<CVehicleHelper*>(objects);
        // Components can be also specified as helpers...
        GetObjectsByClass<CVehicleComponent*>(objects);
        break;

    case IVariable::DT_VEEDPART:
        GetObjectsByClass<CVehiclePart*>(objects);
        break;

    case IVariable::DT_VEEDCOMP:
        GetObjectsByClass<CVehicleComponent*>(objects);
        break;
    }

    std::sort(objects.begin(), objects.end(), SortObjectsByName);

    // store current value
    QString val;
    pItem->GetVariable()->Get(val);

    CVarEnumList<QString>* list = new CVarEnumList<QString>();
    list->AddItem("", "");

    for (std::vector<CBaseObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        QString name = (*it)->GetName();
        list->AddItem(name, name);
    }

    CVariableEnum<QString>* pEnum = (CVariableEnum<QString>*)pItem->GetVariable();
    pEnum->SetEnumList(list);
    //pEnum->Set(val);
}


//////////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::DeleteEditorObjects()
{
    CUndoSuspend susp;
    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();

    // notify panels of deletion
    for (TVeedComponent::iterator it = m_panels.begin(); it != m_panels.end(); ++it)
    {
        (*it)->NotifyObjectsDeletion(m_pVehicle);
    }

    // gather objects for deletion
    std::list<CBaseObject*> delObjs;

    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        // this function is used for cleanup only and therefore must not delete variables
        if (IVeedObject* pVO = IVeedObject::GetVeedObject(it->first))
        {
            pVO->DeleteVar(false);
        }

        // only mark top-level parts for deletion, they take care of children
        if ((*it).first->GetParent() == m_pVehicle)
        {
            delObjs.push_back(it->first);
        }
    }

    for (std::list<CBaseObject*>::iterator it = delObjs.begin(); it != delObjs.end(); ++it)
    {
        pObjMan->DeleteObject(*it);
    }

    m_partToTree.clear();
    GetIEditor()->SetModifiedFlag();
}


//////////////////////////////////////////////////////////////////////////
bool CVehicleEditorDialog::OpenVehicle(bool silent /*=false*/)
{
    // create prototype for currently selected vehicle
    CSelectionGroup* group = GetIEditor()->GetSelection();

    if (!group || group->IsEmpty())
    {
        if (!silent)
        {
            QMessageBox::warning(this, tr("Editor"), tr("No vehicle selected"));
        }

        return false;
    }

    if (group->GetCount() > 1)
    {
        if (!silent)
        {
            QMessageBox::warning(this, tr("Editor"), tr("error, multiple objects selected."));
        }

        return false;
    }

    bool ok = false;
    QString sFile, sClass;

    CBaseObject* obj = group->GetObject(0);

    if (!obj)
    {
        return false;
    }

    if (!qobject_cast<CEntityObject*>(obj))
    {
        if (!silent)
        {
            Log("Selected object %s is no vehicle entity.", obj->GetName());
        }

        return false;
    }

    CEntityObject* pEnt = (CEntityObject*)obj;
    CEntityScript* pScript = pEnt->GetScript();

    if (!pScript)
    {
        if (!silent)
        {
            Warning("%s: Entity script is NULL!");
        }

        return false;
    }

    sFile = pScript->GetFile();

    if (!sFile.contains("vehiclepool", Qt::CaseInsensitive))
    {
        if (!silent)
        {
            Warning("%s doesn't seem to be a valid vehicle (got script: %s)", pEnt->GetName(), sFile);
        }

        return false;
    }

    // create prototype and set entity on it
    CVehiclePrototype* pProt = (CVehiclePrototype*)GetIEditor()->GetObjectManager()->NewObject("VehiclePrototype", 0, pEnt->GetEntityClass());

    if (!pProt)
    {
        if (!silent)
        {
            Warning("Spawn of prototype failed, please inform Code!");
        }

        return false;
    }

    DestroyVehiclePrototype();

    pProt->SetVehicleEntity(pEnt);
    SetVehiclePrototype(pProt);
    EnableEditingLinks(true);

    return true;
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnVehicleEntityEvent(CBaseObject* pObject, int event)
{
    if (m_pVehicle == NULL)
    {
        return;
    }

    if (pObject != m_pVehicle->GetCEntity())
    {
        return;
    }

    if (event != CBaseObject::ON_PREDELETE)
    {
        return;
    }

    GetIEditor()->CloseView("Vehicle Editor");
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnModsEdit()
{
    CVehicleModificationDialog dlg;

    IVariable* pMods = GetOrCreateChildVar(m_pVehicle->GetVariable(), "Modifications");
    dlg.SetVariable(pMods);

    if (dlg.exec() == QDialog::Accepted)
    {
        // store updated mods
        ReplaceChildVars(dlg.GetVariable(), pMods);
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnPaintsEdit()
{
    if (m_pVehicle == NULL)
    {
        return;
    }

    findDockWidget(&m_paintsPanel)->show();
}

//////////////////////////////////////////////////////////////////////////
void CVehicleEditorDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnCloseScene:
        GetIEditor()->CloseView("Vehicle Editor");
        break;
    }
}

#include <Vehicles/VehicleEditorDialog.moc>
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
#include "ObjectPanel.h"
#include "Objects/Group.h"
#include "Objects/BaseObject.h"
#include "LayersSelectDialog.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"
#include "Objects/ObjectLayerManager.h"

#include <QtUtilWin.h>
#include <QWidget>
#include "QtUI/ColorButton.h"

#include <ui_ObjectPanel.h>


/////////////////////////////////////////////////////////////////////////////
// CObjectPanel dialog

CObjectPanel::SParams::SParams(CBaseObject* pObject)
{
    if (pObject)
    {
        name = pObject->GetName();
        color = pObject->GetColor();
        area = pObject->GetArea();
        if (pObject->GetLayer())
        {
            layer = pObject->GetLayer()->GetName();
        }
        minSpec = pObject->GetMinSpec();
    }
}

CObjectPanel::CObjectPanel(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_ui(new Ui::ObjectPanel)
    , m_obj(nullptr)
    , m_multiSelect(false)
    , m_currentLayerModified(false)
{
    GetIEditor()->GetObjectManager()->GetLayersManager()->AddUpdateListener(functor(*this, &CObjectPanel::OnLayerUpdate));

    OnInitDialog();
}

CObjectPanel::~CObjectPanel()
{
    GetIEditor()->GetObjectManager()->GetLayersManager()->RemoveUpdateListener(functor(*this, &CObjectPanel::OnLayerUpdate));
}

/////////////////////////////////////////////////////////////////////////////
// CObjectPanel message handlers

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::SetMultiSelect(bool bEnable)
{
    m_multiSelect = bEnable;

    if (bEnable)
    {
        m_ui->objectNameEdit->setEnabled(false);
        m_ui->objectAreaSpin->setEnabled(false);
        m_ui->layerNameEdit->setText(QStringLiteral(""));

        m_ui->materialButton->setEnabled(false);
        m_ui->materialButton->setText(QStringLiteral(""));
    }
    else
    {
        m_ui->objectNameEdit->setEnabled(true);
        m_ui->materialButton->setEnabled(true);
    }

    CSelectionGroup* selection = GetIEditor()->GetSelection();
    for (int i = 0; i < selection->GetCount(); i++)
    {
        const auto selectedObject = selection->GetObject(i);
        if (selectedObject->GetGroup() || !selectedObject->SupportsLayers())
        {
            m_ui->layerButton->setEnabled(false);
            m_ui->layerNameEdit->setText(tr("The selection contains entities incompatible with legacy layers"));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::SetParams(CBaseObject* obj, const SParams& params)
{
    if (obj != m_obj && m_obj)
    {
        OnUpdate();
    }
    m_obj = obj;

    m_ui->objectNameEdit->setText(params.name);
    m_ui->objectColorButton->SetColor(params.color);
    m_ui->objectAreaSpin->setValue(params.area);

    if (m_obj && m_obj->GetMaterial())
    {
        //m_ui->materialButton->setEnabled( true );
        m_ui->materialButton->setText(m_obj->GetMaterial()->GetName());
    }
    else
    {
        //m_ui->materialButton->setEnabled( false );
        if (m_obj)
        {
            m_ui->materialButton->setText(tr("<No Custom Material>"));
        }
        else
        {
            m_ui->materialButton->setText(QStringLiteral(""));
        }
    }

    // Find layer from id.
    CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(params.layer);
    if (pLayer)
    {
        QString layerName = pLayer->GetName();
        if (pLayer->IsModified())
        {
            m_currentLayerModified = true;
            layerName += QStringLiteral("*");
        }

        CGroup* pGroup = m_obj->GetGroup();
        if (pGroup)
        {
            QString layerLockedInfo = tr(" (inherited from %1)").arg(pGroup->GetName());
            layerName += layerLockedInfo;
        }
        const bool supportsLayers = (pGroup || !m_obj->SupportsLayers()) ? false : true;
        m_ui->layerButton->setEnabled(supportsLayers);
        m_ui->layerNameEdit->setText(supportsLayers ? layerName: tr("The selection contains entities incompatible with legacy layers"));
    }
    else
    {
        m_ui->layerNameEdit->setText(QStringLiteral(""));
    }

    m_ui->minSpecCombo->setCurrentIndex(params.minSpec);
}

void CObjectPanel::GetParams(SParams& params)
{
    params.name = m_ui->objectNameEdit->text();
    params.color = m_ui->objectColorButton->Color();
    params.area = m_ui->objectAreaSpin->value();
    //params.flatten = m_flatten;
    //params.shared = m_bShared;

    // KDAB This can get more terse once SParams is Qt-based, instead of CString-based
    // KDAB It would be good to just hold a pointer to the CLayer and be done with this
    QString layerName = m_ui->layerNameEdit->text();
    if (m_currentLayerModified && layerName.endsWith("*"))
    {
        layerName.chop(1);
    }
    params.layer = layerName;

    int spec = m_ui->minSpecCombo->currentIndex();
    params.minSpec = spec;
}

void CObjectPanel::OnObjectColor(const QColor& color)
{
    OnUpdate();

    if (m_multiSelect)
    {
        CUndo undo("Set Color");
        // Update shared flags in current selection group.
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            selection->GetObject(i)->ChangeColor(color);
        }
    }
}

void CObjectPanel::OnInitDialog()
{
    m_ui->setupUi(this);

    m_ui->layerButton->setIcon(QIcon(QStringLiteral(":/Panels/ObjectPanel/icon_layers.png")));

    connect(m_ui->objectColorButton, &ColorButton::ColorChanged, this, &CObjectPanel::OnObjectColor);
    connect(m_ui->objectNameEdit, &QLineEdit::returnPressed, this, &CObjectPanel::OnChangeName);
    connect(m_ui->layerButton, &QToolButton::clicked, this, &CObjectPanel::OnBnClickedLayer);
    connect(m_ui->materialButton, &QPushButton::clicked, this, &CObjectPanel::OnBnClickedMaterial);
    connect(m_ui->objectAreaSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &CObjectPanel::OnUpdateArea);
    connect(m_ui->minSpecCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CObjectPanel::OnMinSpecChange);
}

void CObjectPanel::OnUpdate()
{
    if (m_obj)
    {
        m_obj->OnUIUpdate();
        m_obj->UpdateGroup();
    }
}

/*
void CObjectPanel::OnShared()
{
    // TODO: Add your control notification handler code here
    UpdateData(TRUE);
    OnUpdate();

    if (m_multiSelect)
    {
        CUndo undo("Set Shared");
        // Update shared flags in current selection group.
        CSelectionGroup *selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            selection->GetObject(i)->SetShared(m_bShared);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnUpdateName()
{
    //UpdateData(TRUE);
    //OnUpdate();

    / *
    if (m_multiSelect)
    {
        CUndo undo("Set Name");
        // Update shared flags in current selection group.
        CSelectionGroup *selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            selection->GetObject(i)->SetName(m_name);
        }
    }
    * /
}
*/

/*
//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnUpdateFlatten()
{
    UpdateData(TRUE);
    OnUpdate();

    if (m_multiSelect)
    {
        CUndo undo("Set Flatten");
        // Update shared flags in current selection group.
        CSelectionGroup *selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            //OBJFLAG_FLATTEN
            if (m_flatten)
                selection->GetObject(i)->SetFlags( OBJFLAG_FLATTEN );
            else
                selection->GetObject(i)->ClearFlags( OBJFLAG_FLATTEN );
        }
    }
}
*/
//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnUpdateArea(double value)
{
    OnUpdate();

    if (m_multiSelect)
    {
        CUndo undo("Set Area");
        // Update shared flags in current selection group.
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            selection->GetObject(i)->SetArea(value);
        }
    }
}
void CObjectPanel::OnBnClickedLayer()
{
    // Open Layer selection dialog.
    CLayersSelectDialog dlg(mapToGlobal(m_ui->layerButton->geometry().bottomLeft()));

    QString selLayer = m_ui->layerNameEdit->text();
    dlg.SetSelectedLayer(m_ui->layerNameEdit->text());
    if (dlg.exec() == QDialog::Accepted)
    {
        const QString selNewLayer = dlg.GetSelectedLayer();
        if (!selNewLayer.isEmpty() && selNewLayer != selLayer)
        {
            CObjectLayer* pLayer = 0;

            CObjectLayer* pNewLayer = 0;
            pNewLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(selNewLayer);

            if (!selLayer.isEmpty() && (pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(selLayer)))
            {
                pLayer->SetModified();
            }

            selLayer = selNewLayer;
            pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(selLayer);
            if (!pLayer)
            {
                return;
            }
            pLayer->SetModified();
            m_ui->layerNameEdit->setText(selLayer);

            CUndo undo("Set Object Layer");
            if (m_multiSelect)
            {
                // Update shared flags in current selection group.
                CSelectionGroup* selection = GetIEditor()->GetSelection();
                for (int i = 0; i < selection->GetCount(); i++)
                {
                    CBaseObject* pObject = selection->GetObject(i);
                    pObject->SetLayer(pNewLayer);
                    pObject->UpdatePrefab();
                }
            }
            else
            {
                OnUpdate();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnMinSpecChange()
{
    CUndo undo("Change MinSpec");

    if (!m_multiSelect && m_obj)
    {
        m_obj->StoreUndo("Change MinSpec", true);
    }

    OnUpdate();

    if (m_multiSelect)
    {
        SParams params;
        GetParams(params);

        // Update shared data in current selection group.
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            CBaseObject* pSelectedObj = selection->GetObject(i);
            if (pSelectedObj == NULL)
            {
                continue;
            }
            pSelectedObj->StoreUndo("Change MinSpec", true);
            pSelectedObj->SetMinSpec(params.minSpec);
            pSelectedObj->UpdateGroup();
            pSelectedObj->UpdatePrefab();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnChangeName()
{
    CUndo undo("Set Name");

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();

    if (!pObjMan)
    {
        return;
    }

    const QString name = m_ui->objectNameEdit->text();

    if (m_multiSelect)
    {
        // Update shared flags in current selection group.
        CSelectionGroup* selection = GetIEditor()->GetSelection();

        for (int i = 0; i < selection->GetCount(); i++)
        {
            CBaseObject* pObject = selection->GetObject(i);

            if (!pObject)
            {
                continue;
            }

            if (pObjMan->IsDuplicateObjectName(name))
            {
                pObjMan->ShowDuplicationMsgWarning(pObject, name, true);
                return;
            }

            pObjMan->ChangeObjectName(pObject, name);
        }
    }
    else
    {
        if (m_obj)
        {
            if (m_obj->GetName().compare(name) != 0)
            {
                if (pObjMan->IsDuplicateObjectName(name))
                {
                    pObjMan->ShowDuplicationMsgWarning(m_obj, name, true);
                    m_ui->objectNameEdit->setText(m_obj->GetName());
                    return;
                }
            }
            else
            {
                return;
            }
        }

        OnUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPanel::OnBnClickedMaterial()
{
    // Select current material
    if (m_obj)
    {
        if (m_obj->GetMaterial())
        {
            GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_MATERIAL, m_obj->GetMaterial());
        }
        else
        {
            GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_MATERIAL);
            GetIEditor()->GetMaterialManager()->Command_SelectFromObject();
        }
    }
}


void CObjectPanel::OnLayerUpdate(int event, CObjectLayer* pLayer)
{
    switch (event)
    {
    case CObjectLayerManager::ON_LAYER_MODIFY:
    {
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        if (selection->GetCount() == 1 && selection->GetObject(0) == m_obj && m_obj->GetLayer() && m_obj->GetLayer() == pLayer)
        {
            SParams params;
            GetParams(params);
            params.layer = m_obj->GetLayer()->GetName();
            SetParams(m_obj, params);
        }
    }
    break;
    }
}

#include <ObjectPanel.moc>

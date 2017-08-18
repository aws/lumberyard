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
#include "VehiclePaintsPanel.h"
#include <Vehicles/ui_VehiclePaintsPanel.h>

#include "VehicleData.h"

#include "Material/MaterialManager.h"

class PaintNameModel
    : public QAbstractListModel
{
public:
    PaintNameModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_pPaints(nullptr)
    {
    }

    void InitPaints(IVariable* pPaints)
    {
        if (pPaints == m_pPaints)
        {
            return;
        }
        beginResetModel();
        names.clear();
        indexes.clear();
        if (m_pPaints != nullptr)
        {
            QSignalBlocker sb(this);
            for (int i = 0; i < m_pPaints->GetNumVariables(); i++)
            {
                IVariable* pPaint = m_pPaints->GetVariable(i);
                IVariable* pPaintName = GetChildVar(pPaint, "name");
                if (pPaintName != NULL)
                {
                    QString paintName;
                    pPaintName->Get(paintName);

                    AddPaintName(paintName, i);
                }
            }
        }
        endResetModel();
    }

    void AddPaintName(const QString& name, int i = -1)
    {
        if (paints() == nullptr)
        {
            return;
        }
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        if (i == -1)
        {
            IVariablePtr pNewPaint(new CVariableArray());
            pNewPaint->SetName("Paint");

            IVariablePtr pPaintName(new CVariable< QString >());
            pPaintName->SetName("name");
            pPaintName->Set(name);
            pNewPaint->AddVariable(pPaintName);

            IVariablePtr pPaintMaterial(new CVariable< QString >());
            pPaintMaterial->SetName("material");
            pNewPaint->AddVariable(pPaintMaterial);

            m_pPaints->AddVariable(pNewPaint);
            i = m_pPaints->GetNumVariables() - 1;
        }
        names.push_back(name);
        indexes.push_back(i);
        endInsertRows();
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (row < 0 || row + count > rowCount(parent) || parent.isValid())
        {
            return false;
        }

        beginRemoveRows(QModelIndex(), row, row + count - 1);
        for (int r = row + count - 1; r >= row; --r)
        {
            int selectedPaintVarIndex = indexes[r];

            IVariable* pPaintToRemove = m_pPaints->GetVariable(selectedPaintVarIndex);
            m_pPaints->DeleteVariable(pPaintToRemove);
        }
        endRemoveRows();
        return true;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : names.count();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return names[index.row()];
        case Qt::UserRole:
            return indexes[index.row()];
        default:
            return QVariant();
        }
    }

    IVariable* paints()
    {
        return m_pPaints;
    }

private:
    IVariable* m_pPaints;
    QStringList names;
    QVector<int> indexes;
};

//////////////////////////////////////////////////////////////////////////
CVehiclePaintsPanel::CVehiclePaintsPanel(QWidget* parent)
    : QWidget(parent)
    , m_paintModel(new PaintNameModel(this))
    , m_pSelectedPaint(NULL)
    , m_ui(new Ui::VehiclePaintsPanel)
{
    m_ui->setupUi(this);
    m_ui->m_paintProperties->Setup();
    m_ui->m_paintNames->setModel(m_paintModel);
    GetIEditor()->GetMaterialManager()->AddListener(this);

    connect(m_ui->m_buttonPaintsNew, &QPushButton::clicked, this, &CVehiclePaintsPanel::OnAddNewPaint);
    connect(m_ui->m_buttonPaintsRemove, &QPushButton::clicked, this, &CVehiclePaintsPanel::OnRemoveSelectedPaint);
    connect(m_ui->m_applyMaterialButton, &QPushButton::clicked, this, &CVehiclePaintsPanel::OnAssignMaterialToSelectedPaint);
    connect(m_ui->m_applyToVehicleButton, &QPushButton::clicked, this, &CVehiclePaintsPanel::OnApplyPaintToVehicle);
    connect(m_ui->m_paintNames->selectionModel(), &QItemSelectionModel::currentChanged, this, &CVehiclePaintsPanel::OnPaintNameSelectionChanged);

    OnInitDialog();
}


//////////////////////////////////////////////////////////////////////////
CVehiclePaintsPanel::~CVehiclePaintsPanel()
{
    GetIEditor()->GetMaterialManager()->RemoveListener(this);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnInitDialog()
{
    UpdateAssignMaterialButtonState();

    // Hiding the apply to vehicle button as this functionality is not yet implemented.
    m_ui->m_applyToVehicleButton->setVisible(false);
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::InitPaints(IVariable* pPaints)
{
    if (pPaints)
    {
        m_paintModel->InitPaints(pPaints);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Could not initialize paints for vehicle");
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::Clear()
{
    m_paintModel->InitPaints(nullptr);
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::CreateNewPaint(const QString& paintName)
{
    m_paintModel->AddPaintName(paintName);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnAddNewPaint()
{
    CreateNewPaint("NewPaint");
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnRemoveSelectedPaint()
{
    m_paintModel->removeRow(m_ui->m_paintNames->currentIndex().row());
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    // Listening to material browser item selection changes.
    if (event == EDB_ITEM_EVENT_SELECTED)
    {
        UpdateAssignMaterialButtonState();
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnAssignMaterialToSelectedPaint()
{
    if (m_pSelectedPaint == NULL)
    {
        return;
    }

    CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
    assert(pMaterial != NULL);
    if (pMaterial == NULL)
    {
        return;
    }

    IVariable* pMaterialVar = GetChildVar(m_pSelectedPaint, "material");
    if (pMaterialVar == NULL)
    {
        return;
    }

    QString materialName = Path::MakeGamePath(pMaterial->GetFilename());
    pMaterialVar->Set(materialName);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnApplyPaintToVehicle()
{
}


//////////////////////////////////////////////////////////////////////////
bool CVehiclePaintsPanel::IsPaintNameSelected() const
{
    const QModelIndex selectedItemIndex = m_ui->m_paintNames->currentIndex();
    bool validItemIndex = selectedItemIndex.isValid();
    return validItemIndex;
}


//////////////////////////////////////////////////////////////////////////
QString CVehiclePaintsPanel::GetSelectedPaintName() const
{
    assert(IsPaintNameSelected());

    const QModelIndex selectedItemIndex = m_ui->m_paintNames->currentIndex();
    return selectedItemIndex.data().toString();
}


//////////////////////////////////////////////////////////////////////////
int CVehiclePaintsPanel::GetSelectedPaintNameVarIndex() const
{
    assert(IsPaintNameSelected());

    const QModelIndex selectedItemIndex = m_ui->m_paintNames->currentIndex();
    return selectedItemIndex.data(Qt::UserRole).toInt();
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::RenameSelectedPaintName(const QString& name)
{
    if (!IsPaintNameSelected())
    {
        return;
    }

    const QModelIndex selectedItemIndex = m_ui->m_paintNames->currentIndex();
    m_ui->m_paintNames->model()->setData(selectedItemIndex, name);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::AddPaintName(const QString& paintName, int paintVarIndex)
{
    m_paintModel->AddPaintName(paintName, paintVarIndex);
    m_ui->m_paintNames->setCurrentIndex(m_paintModel->index(m_paintModel->rowCount() - 1, 0));
    OnPaintNameSelectionChanged(); // Called manually as SetCurSel doesn't trigger ON_LBN_SELCHANGE
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnPaintNameSelectionChanged()
{
    if (!IsPaintNameSelected())
    {
        HidePaintProperties();
        return;
    }

    int selectedPaintVarIndex = GetSelectedPaintNameVarIndex();
    ShowPaintPropertiesByVarIndex(selectedPaintVarIndex);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::HidePaintProperties()
{
    if (m_pSelectedPaint != NULL)
    {
        IVariable* pSelectedPaintName = GetChildVar(m_pSelectedPaint, "name");
        if (pSelectedPaintName != NULL)
        {
            pSelectedPaintName->RemoveOnSetCallback(functor(*this, &CVehiclePaintsPanel::OnSelectedPaintNameChanged));
        }

        IVariable* pSeletedPaintMaterial = GetChildVar(m_pSelectedPaint, "material");
        if (pSeletedPaintMaterial != NULL)
        {
            pSeletedPaintMaterial->RemoveOnSetCallback(functor(*this, &CVehiclePaintsPanel::OnSelectedPaintMaterialChanged));
        }
    }

    m_ui->m_paintProperties->RemoveAllItems();
    m_pSelectedPaint = NULL;
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::ShowPaintPropertiesByName(const QString& paintName)
{
    IVariable* pPaint = GetPaintVarByName(paintName);
    ShowPaintProperties(pPaint);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::ShowPaintPropertiesByVarIndex(int paintVarIndex)
{
    IVariable* pPaint = GetPaintVarByIndex(paintVarIndex);
    ShowPaintProperties(pPaint);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::ShowPaintProperties(IVariable* pPaint)
{
    assert(pPaint);
    if (pPaint == NULL)
    {
        return;
    }

    HidePaintProperties();
    CVarBlockPtr pVarBlock(new CVarBlock());
    pVarBlock->AddVariable(pPaint);
    m_ui->m_paintProperties->AddVarBlock(pVarBlock);
    m_ui->m_paintProperties->EnableUpdateCallback(true);

    m_pSelectedPaint = pPaint;

    if (m_pSelectedPaint != NULL)
    {
        IVariable* pSelectedPaintName = GetChildVar(m_pSelectedPaint, "name");
        if (pSelectedPaintName != NULL)
        {
            pSelectedPaintName->AddOnSetCallback(functor(*this, &CVehiclePaintsPanel::OnSelectedPaintNameChanged));
        }

        IVariable* pSeletedPaintMaterial = GetChildVar(m_pSelectedPaint, "material");
        if (pSeletedPaintMaterial != NULL)
        {
            pSeletedPaintMaterial->AddOnSetCallback(functor(*this, &CVehiclePaintsPanel::OnSelectedPaintMaterialChanged));
        }
    }
}


//////////////////////////////////////////////////////////////////////////
IVariable* CVehiclePaintsPanel::GetPaintVarByName(const QString& paintName)
{
    if (m_paintModel->paints() == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < m_paintModel->paints()->GetNumVariables(); i++)
    {
        IVariable* pPaint = m_paintModel->paints()->GetVariable(i);
        IVariable* pPaintName = GetChildVar(pPaint, "name");
        if (pPaintName != NULL)
        {
            QString currentPaintName;
            pPaintName->Get(currentPaintName);

            if (paintName.compare(currentPaintName, Qt::CaseInsensitive) == 0)
            {
                return pPaint;
            }
        }
    }

    return NULL;
}


//////////////////////////////////////////////////////////////////////////
IVariable* CVehiclePaintsPanel::GetPaintVarByIndex(int index)
{
    if (m_paintModel->paints() == NULL)
    {
        return NULL;
    }

    if (index < 0 || m_paintModel->paints()->GetNumVariables() <= index)
    {
        return NULL;
    }

    return m_paintModel->paints()->GetVariable(index);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnSelectedPaintNameChanged(IVariable* pVar)
{
    assert(pVar != NULL);

    QString name;
    pVar->Get(name);

    RenameSelectedPaintName(name);
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::OnSelectedPaintMaterialChanged(IVariable* pVar)
{
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePaintsPanel::UpdateAssignMaterialButtonState()
{
    CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
    bool materialSelected = (pMaterial != NULL);

    m_ui->m_applyMaterialButton->setEnabled(materialSelected);
}

#include <Vehicles/VehiclePaintsPanel.moc>
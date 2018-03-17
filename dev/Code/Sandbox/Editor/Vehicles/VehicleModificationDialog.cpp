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

#include "VehicleData.h"
#include "VehicleModificationDialog.h"

#include <Vehicles/ui_VehicleModificationDialog.h>

class VehicleModificationModel
    : public QAbstractListModel
{
public:
    VehicleModificationModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_modVars.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= m_modVars.size())
        {
            return {};
        }

        auto pModVar = m_modVars[index.row()];

        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (IVariable* pName = GetChildVar(pModVar, "name"))
            {
                QString name;
                pName->Get(name);
                return name;
            }
        }

        case Qt::UserRole:
            return qVariantFromValue<IVariable*>(pModVar);
        }

        return {};
    }

    bool removeRows(int row, int count, const QModelIndex& parent = {}) override
    {
        if (parent.isValid())
        {
            return false;
        }

        beginRemoveRows(parent, row, row + count - 1);
        m_modVars.erase(std::begin(m_modVars) + row, std::begin(m_modVars) + row + count);
        endRemoveRows();

        return true;
    }

    void Reload(IVariable* pVar)
    {
        beginResetModel();

        m_modVars.clear();

        for (int i = 0; i < pVar->GetNumVariables(); ++i)
        {
            auto pModVar = pVar->GetVariable(i);
            if (GetChildVar(pModVar, "name") != nullptr)
            {
                m_modVars.push_back(pVar->GetVariable(i));
            }
        }

        endResetModel();
    }

    QModelIndex VariableIndex(IVariable* pModVar)
    {
        auto it = std::find(std::begin(m_modVars), std::end(m_modVars), pModVar);

        if (it == std::end(m_modVars))
        {
            return {};
        }

        return index(std::distance(std::begin(m_modVars), it), 0);
    }

private:
    std::vector<IVariable*> m_modVars;
};

//////////////////////////////////////////////////////////////////////////////
// CVehicleModificationDialog implementation
//////////////////////////////////////////////////////////////////////////////

CVehicleModificationDialog::CVehicleModificationDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_model(new VehicleModificationModel(this))
    , ui(new Ui::CVehicleModificationDialog)
{
    ui->setupUi(this);
    ui->m_propsCtrl->Setup();
    ui->m_modList->setModel(m_model);

    m_pVar = 0;

    connect(ui->m_modList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CVehicleModificationDialog::OnSelChanged);

    connect(ui->m_buttonNew, &QPushButton::clicked, this, &CVehicleModificationDialog::OnModNew);
    connect(ui->m_buttonClone, &QPushButton::clicked, this, &CVehicleModificationDialog::OnModClone);
    connect(ui->m_buttonDelete, &QPushButton::clicked, this, &CVehicleModificationDialog::OnModDelete);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CVehicleModificationDialog::~CVehicleModificationDialog()
{
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::SetVariable(IVariable* pVar)
{
    if (pVar)
    {
        m_pVar = pVar->Clone(true);
    }
    else
    {
        m_pVar = new CVariableArray();
        m_pVar->SetName("Modifications");
    }

    ReloadModList();
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::OnSelChanged()
{
    ui->m_propsCtrl->RemoveAllItems();

    auto idx = ui->m_modList->currentIndex();
    if (idx.isValid())
    {
        IVariable* pModVar = idx.data(Qt::UserRole).value<IVariable*>();

        // update props panel
        _smart_ptr< CVarBlock > block = new CVarBlock();
        block->AddVariable(pModVar);

        ui->m_propsCtrl->AddVarBlock(block);

        ui->m_propsCtrl->ExpandAll();

        if (IVariable* pElems = GetChildVar(pModVar, "Elems"))
        {
            ReflectedPropertyItem* pItem = ui->m_propsCtrl->FindItemByVar(pElems);
            ui->m_propsCtrl->ExpandAllChildren(pItem, false);


            // expand Elems children
            for (int i = 0; i < pItem->GetChildCount(); ++i)
            {
                ReflectedPropertyItem* pElem = pItem->GetChild(i);
                ui->m_propsCtrl->Expand(pElem, true);

                if (IVariable* pId = GetChildVar(pElem->GetVariable(), "idRef"))
                {
                    pId->SetFlags(pId->GetFlags() | IVariable::UI_DISABLED);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::OnModNew()
{
    IVariable* pMod = new CVariableArray();
    pMod->SetName("Modification");

    IVariable* pName = new CVariable<QString>();
    pName->SetName("name");
    pName->Set("new modification");

    pMod->AddVariable(pName);
    m_pVar->AddVariable(pMod);

    ReloadModList();
    SelectMod(pMod);
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::OnModClone()
{
    IVariable* pMod = GetSelectedVar();
    if (!pMod)
    {
        return;
    }

    IVariable* pClone = pMod->Clone(true);
    m_pVar->AddVariable(pClone);

    ReloadModList();
    SelectMod(pClone);
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::OnModDelete()
{
    auto idx = ui->m_modList->currentIndex();

    if (idx.isValid())
    {
        IVariable* pMod = idx.data(Qt::UserRole).value<IVariable*>();
        m_model->removeRow(idx.row(), idx.parent());
        m_pVar->DeleteVariable(pMod);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::ReloadModList()
{
    if (m_pVar)
    {
        m_model->Reload(m_pVar);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehicleModificationDialog::SelectMod(IVariable* pMod)
{
    auto idx = m_model->VariableIndex(pMod);

    if (idx.isValid())
    {
        ui->m_modList->setCurrentIndex(idx);
    }
}

//////////////////////////////////////////////////////////////////////////////
IVariable* CVehicleModificationDialog::GetSelectedVar()
{
    auto idx = ui->m_modList->currentIndex();
    if (idx.isValid())
    {
        return idx.data(Qt::UserRole).value<IVariable*>();
    }

    return nullptr;
}

#include <Vehicles/VehicleModificationDialog.moc>

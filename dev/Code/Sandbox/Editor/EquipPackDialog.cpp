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

// Description : implementation file


#include "StdAfx.h"
#include "EquipPackDialog.h"
#include "ui_EquipPackDialog.h"
#include "EquipPackLib.h"
#include "EquipPack.h"
#include "StringDlg.h"
#include "GameEngine.h"
#include <IEditorGame.h>
#include "UserMessageDefines.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QTreeWidgetItem>
#include <QDebug>
// CEquipPackDialog dialog

CEquipPackDialog::CEquipPackDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CEquipPackDialog)
    , m_updating(false)
{
    ui->setupUi(this);
    m_AmmoPropWnd = ui->PROPERTIES;
    m_OkBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    m_ExportBtn = ui->BTNEXPORT;
    m_AddBtn = ui->BTNADD;
    m_DeleteBtn = ui->BTNDELETE;
    m_RenameBtn = ui->RENAME;
    m_EquipPacksList = ui->EQUIPPACK;
    m_AvailEquipList = ui->EQUIPAVAILLST;
    m_EquipList = ui->EQUIPUSEDLST;
    m_InsertBtn = ui->BTNINSERT;
    m_RemoveBtn = ui->BTNREMOVE;
    m_PrimaryEdit = ui->PRIMARY;

    m_sCurrEquipPack = "";
    m_bEditMode = true;

    m_AmmoPropWnd->Setup();
    OnInitDialog();

    connect(m_EquipPacksList, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CEquipPackDialog::OnCbnSelchangeEquippack);
    connect(m_AvailEquipList, &QListWidget::itemSelectionChanged, this, &CEquipPackDialog::OnLbnSelchangeEquipavaillst);
    connect(m_AddBtn, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedAdd);
    connect(m_DeleteBtn, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedDelete);
    connect(m_RenameBtn, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedRename);
    connect(m_RemoveBtn, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedRemove);
    connect(m_InsertBtn, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedInsert);
    connect(ui->MOVEUP, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedMoveUp);
    connect(ui->MOVEDOWN, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedMoveDown);
    connect(ui->BTNIMPORT, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedImport);
    connect(ui->BTNEXPORT, &QPushButton::clicked, this, &CEquipPackDialog::OnBnClickedExport);
    connect(m_EquipList, &QTreeWidget::itemSelectionChanged, this, &CEquipPackDialog::OnTvnSelchangedEquipusedlst);
    connect(m_EquipList, &QTreeWidget::itemChanged, this, &CEquipPackDialog::OnCheckStateChange);
}

CEquipPackDialog::~CEquipPackDialog()
{
}

void CEquipPackDialog::UpdateEquipPacksList()
{
    m_EquipPacksList->blockSignals(true);
    m_EquipPacksList->clear();
    const TEquipPackMap& mapEquipPacks = GetIEditor()->GetEquipPackLib()->GetEquipmentPacks();
    for (TEquipPackMap::const_iterator It = mapEquipPacks.begin(); It != mapEquipPacks.end(); ++It)
    {
        m_EquipPacksList->addItem(It->first);
    }

    int nCurSel = m_EquipPacksList->findText(m_sCurrEquipPack);
    const int nEquipPacks = GetIEditor()->GetEquipPackLib()->Count();
    if (!nEquipPacks)
    {
        nCurSel = -1;
    }
    else if (nEquipPacks <= nCurSel)
    {
        nCurSel = 0;
    }
    else if (nCurSel == -1)
    {
        nCurSel = 0;
    }
    m_EquipPacksList->setCurrentIndex(nCurSel);
    m_EquipPacksList->blockSignals(false);
    m_DeleteBtn->setEnabled(nCurSel != -1);
    m_RenameBtn->setEnabled(nCurSel != -1);
    UpdateEquipPackParams();
}

void CEquipPackDialog::UpdateEquipPackParams()
{
    if (m_updating)
    {
        return;
    }
    m_updating = true;

    m_AvailEquipList->clear();
    m_EquipList->clear();
    m_AmmoPropWnd->RemoveAllItems();

    int nItem = m_EquipPacksList->currentIndex();
    m_ExportBtn->setEnabled(nItem != -1);
    if (nItem == -1)
    {
        m_sCurrEquipPack = "";
        m_updating = false;
        return;
    }
    m_sCurrEquipPack = m_EquipPacksList->itemText(nItem);
    QString sName = m_sCurrEquipPack;
    CEquipPack* pEquip = GetIEditor()->GetEquipPackLib()->FindEquipPack(sName);
    if (!pEquip)
    {
        m_updating = false;
        return;
    }
    TEquipmentVec& equipVec = pEquip->GetEquip();

    // add in the order equipments appear in the pack
    for (TEquipmentVec::iterator iter = equipVec.begin(), iterEnd = equipVec.end(); iter != iterEnd; ++iter)
    {
        const SEquipment& equip = *iter;

        AddEquipmentListItem(equip);
    }

    // add all other items to available list
    for (TLstStringIt iter = m_lstAvailEquip.begin(), iterEnd = m_lstAvailEquip.end(); iter != iterEnd; ++iter)
    {
        const SEquipment& equip = *iter;
        if (std::find(equipVec.begin(), equipVec.end(), equip) == equipVec.end())
        {
            m_AvailEquipList->addItem(equip.sName);
        }
    }

    UpdatePrimary();

    m_AmmoPropWnd->AddVarBlock(CreateVarBlock(pEquip));
    m_AmmoPropWnd->SetUpdateCallback(functor(*this, &CEquipPackDialog::AmmoUpdateCallback));
    m_AmmoPropWnd->EnableUpdateCallback(true);
    m_AmmoPropWnd->ExpandAllChildren(m_AmmoPropWnd->GetRootItem(), true);

    // update lists
    m_AvailEquipList->setCurrentRow(0);
    OnLbnSelchangeEquipavaillst();
    OnTvnSelchangedEquipusedlst();
    m_updating = false;
}

void CEquipPackDialog::AddEquipmentListItem(const SEquipment& equip)
{
    if (!GetIEditor() ||
        !GetIEditor()->GetGameEngine() ||
        !GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface())
    {
        AZ_Error("Equipment Manager",
            false,
            "This equipment item can't be added without a valid equipment system interface. The equipment system is deprecated, please contact support if you are using it.");
        return;
    }

    QString sSetup = equip.sSetup;
    QTreeWidgetItem* parent = new QTreeWidgetItem(QStringList { equip.sName });
    parent->setFlags(parent->flags() | Qt::ItemIsUserCheckable);
    parent->setCheckState(0, Qt::Unchecked);
    m_EquipList->addTopLevelItem(parent);

    IEquipmentSystemInterface::IEquipmentItemIteratorPtr accessoryIter = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface()->CreateEquipmentAccessoryIterator(equip.sName.toUtf8().data());

    if (accessoryIter)
    {
        IEquipmentSystemInterface::IEquipmentItemIterator::SEquipmentItem accessoryItem;
        while (accessoryIter->Next(accessoryItem))
        {
            QTreeWidgetItem* categoryItem = nullptr;

            if (accessoryItem.type && accessoryItem.type[0])
            {
                for (int i = 0; i < parent->childCount(); i++)
                {
                    categoryItem = parent->child(i);
                    if (categoryItem->text(0) == QString(accessoryItem.type))
                    {
                        break;
                    }
                }

                if (categoryItem == nullptr)
                {
                    categoryItem = new QTreeWidgetItem(parent, QStringList { QString(accessoryItem.type) });
                    categoryItem->setFlags(categoryItem->flags() | Qt::ItemIsUserCheckable);
                    categoryItem->setCheckState(0, Qt::Unchecked);
                }
            }

            QTreeWidgetItem* accessory = new QTreeWidgetItem(categoryItem, QStringList { QString(accessoryItem.name) });
            accessory->setFlags(accessory->flags() | Qt::ItemIsUserCheckable);
            accessory->setCheckState(0, Qt::Unchecked);

            QString formattedName = QString(tr("|%1|")).arg(QtUtil::ToQString(accessoryItem.name));

            if (sSetup.indexOf(formattedName) != -1)
            {
                accessory->setCheckState(0, Qt::Checked);
                categoryItem->setCheckState(0, Qt::Checked);
                parent->setCheckState(0, Qt::Checked);
            }
        }
    }
}

CVarBlockPtr CEquipPackDialog::CreateVarBlock(CEquipPack* pEquip)
{
    CVarBlock* pVarBlock = m_pVarBlock ? m_pVarBlock->Clone(true) : new CVarBlock();

    TAmmoVec notFoundAmmoVec;
    TAmmoVec& ammoVec = pEquip->GetAmmoVec();
    for (TAmmoVec::iterator iter = ammoVec.begin(); iter != ammoVec.end(); ++iter)
    {
        // check if var block contains all variables
        IVariablePtr pVar = pVarBlock->FindVariable(iter->sName.toUtf8().data());
        if (pVar == 0)
        {
            // pVar = new CVariable<int>;
            // pVar->SetFlags(IVariable::UI_DISABLED);
            // pVarBlock->AddVariable(pVar);
            notFoundAmmoVec.push_back(*iter);
        }
        else
        {
            pVar->SetName(iter->sName.toUtf8().data());
            pVar->Set(iter->nAmount);
        }
    }

    if (notFoundAmmoVec.empty() == false)
    {
        QString notFoundString = tr("The following ammo items in pack '%1' could not be found.\r\nDo you want to permanently remove them?\r\n\r\n").arg(pEquip->GetName());
        TAmmoVec::iterator iter = notFoundAmmoVec.begin();
        TAmmoVec::iterator iterEnd = notFoundAmmoVec.end();
        while (iter != iterEnd)
        {
            notFoundString += QString("'%1'\r\n").arg(iter->sName);
            ++iter;
        }

        if (QMessageBox::question(this, tr(""), notFoundString, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            iter = notFoundAmmoVec.begin();
            while (iter != iterEnd)
            {
                const SAmmo& ammo = *iter;
                stl::find_and_erase(ammoVec, ammo);
                ++iter;
            }
            pEquip->SetModified(true);
        }
        else
        {
            // add missing ammos, but disabled
            iter = notFoundAmmoVec.begin();
            while (iter != iterEnd)
            {
                IVariablePtr pVar = pVarBlock->FindVariable(iter->sName.toUtf8().data());
                if (pVar == 0)
                {
                    pVar = new CVariable<int>();
                    pVar->SetFlags(IVariable::UI_DISABLED);
                    pVarBlock->AddVariable(pVar);
                }
                pVar->SetName(iter->sName);
                pVar->Set(iter->nAmount);
                ++iter;
            }
        }
        notFoundAmmoVec.resize(0);
    }

    int nCount = pVarBlock->GetNumVariables();
    for (int i = 0; i < nCount; ++i)
    {
        IVariable* pVar = pVarBlock->GetVariable(i);
        SAmmo ammo;
        ammo.sName = pVar->GetName();
        if (std::find(ammoVec.begin(), ammoVec.end(), ammo) == ammoVec.end())
        {
            pVar->Get(ammo.nAmount);
            pEquip->AddAmmo(ammo);
            notFoundAmmoVec.push_back(ammo);
        }
    }

    if (notFoundAmmoVec.empty() == false)
    {
        QString notFoundString = tr("The following ammo items have been automatically added to pack '%1':\r\n\r\n").arg(pEquip->GetName());
        TAmmoVec::iterator iter = notFoundAmmoVec.begin();
        TAmmoVec::iterator iterEnd = notFoundAmmoVec.end();
        while (iter != iterEnd)
        {
            notFoundString += QString("'%1'\r\n").arg(iter->sName);
            ++iter;
        }
        QMessageBox::warning(this, tr("Equipment-Pack"), notFoundString);
    }
    return pVarBlock;
}

void CEquipPackDialog::UpdatePrimary()
{
    QString name;
    if (m_EquipList->topLevelItemCount() > 0)
    {
        name = m_EquipList->topLevelItem(0)->text(0);
    }
    m_PrimaryEdit->setText(name);
}

void CEquipPackDialog::AmmoUpdateCallback(IVariable* pVar)
{
    CEquipPack* pEquip = GetIEditor()->GetEquipPackLib()->FindEquipPack(m_sCurrEquipPack);
    if (!pEquip)
    {
        return;
    }
    SAmmo ammo;
    ammo.sName = pVar->GetName();
    pVar->Get(ammo.nAmount);
    pEquip->AddAmmo(ammo);
}

SEquipment* CEquipPackDialog::GetEquipment(QString sDesc)
{
    SEquipment Equip;
    Equip.sName = sDesc;
    TLstStringIt It = std::find(m_lstAvailEquip.begin(), m_lstAvailEquip.end(), Equip);
    if (It == m_lstAvailEquip.end())
    {
        return NULL;
    }
    return &(*It);
}


// CEquipPackDialog message handlers

void CEquipPackDialog::OnInitDialog()
{
    m_sCurrEquipPack = "Player_Default";

    m_lstAvailEquip.clear();

    if (!GetIEditor()->GetGameEngine() || !GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface())
    {
        return;
    }

    // Fill available weapons ListBox with enumerated weapons from game.
    IEquipmentSystemInterface::IEquipmentItemIteratorPtr iter = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface()->CreateEquipmentItemIterator("ItemGivable");
    IEquipmentSystemInterface::IEquipmentItemIterator::SEquipmentItem item;

    if (iter)
    {
        SEquipment Equip;
        while (iter->Next(item))
        {
            Equip.sName = item.name;
            Equip.sType = item.type;
            m_lstAvailEquip.push_back(Equip);
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Equipment Pack"), tr("GetAvailableWeaponNames() returned NULL. Unable to retrieve weapons."));
    }

    m_pVarBlock = new CVarBlock();
    iter = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface()->CreateEquipmentItemIterator("Ammo");
    if (iter)
    {
        while (iter->Next(item))
        {
            static const int defaultAmount = 0;
            IVariablePtr pVar = new CVariable<int>();
            pVar->SetName(item.name);
            pVar->Set(defaultAmount);
            m_pVarBlock->AddVariable(pVar);
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Equipment Pack"), tr("GetAvailableWeaponNames() returned NULL. Unable to retrieve weapons."));
    }

    UpdateEquipPacksList();
}

void CEquipPackDialog::OnCbnSelchangeEquippack()
{
    UpdateEquipPackParams();
}

void CEquipPackDialog::OnBnClickedAdd()
{
    QString packName = QInputDialog::getText(0, tr("Add Equipment Pack"), tr("Enter name for new Equipment-Pack"));
    if (packName.isEmpty())
    {
        return;
    }
    if (!GetIEditor()->GetEquipPackLib()->CreateEquipPack(packName))
    {
        QMessageBox::warning(this, tr("Add Equipment Pack"), tr("An Equipment-Pack with this name exists already !"));
    }
    else
    {
        m_sCurrEquipPack = packName;
        UpdateEquipPacksList();
    }
}

void CEquipPackDialog::OnBnClickedDelete()
{
    if (QMessageBox::question(this, tr("Delete equipment"), tr("Are you sure?"), QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }
    QString sName = m_EquipPacksList->currentText();
    if (!GetIEditor()->GetEquipPackLib()->RemoveEquipPack(sName, true))
    {
        QMessageBox::warning(this,  tr("Delete equipment"), tr("Unable to delete Equipment-Pack !"));
    }
    else
    {
        UpdateEquipPacksList();
    }
}

void CEquipPackDialog::OnBnClickedRename()
{
    QString sName = m_EquipPacksList->currentText();
    QString newName = QInputDialog::getText(this, tr("Rename Equipment - Pack"), tr("Enter new name for Equipment - Pack"));
    if (newName.isEmpty())
    {
        return;
    }
    if (!GetIEditor()->GetEquipPackLib()->RenameEquipPack(sName, newName))
    {
        QMessageBox::warning(this, tr("Rename Equipment - Pack"), tr("Unable to rename Equipment-Pack ! Probably the new name is already used."));
    }
    else
    {
        UpdateEquipPacksList();
    }
}

void CEquipPackDialog::OnBnClickedInsert()
{
    int nItem = m_EquipPacksList->currentIndex();
    if (nItem == -1)
    {
        return;
    }
    QString sName = m_EquipPacksList->currentText();
    CEquipPack* pEquipPack = GetIEditor()->GetEquipPackLib()->FindEquipPack(sName);
    if (!pEquipPack)
    {
        return;
    }

    int currSelected = m_AvailEquipList->currentRow();
    sName = m_AvailEquipList->currentItem()->text();
    SEquipment* pEquip = GetEquipment(sName);
    if (!pEquip)
    {
        return;
    }

    pEquipPack->AddEquip(*pEquip);
    AddEquipmentListItem(*pEquip);
    delete m_AvailEquipList->currentItem();

    currSelected = min(currSelected, m_AvailEquipList->count() - 1);
    if (currSelected >= 0)
    {
        m_AvailEquipList->setCurrentRow(currSelected);
    }

    OnLbnSelchangeEquipavaillst();
}

void CEquipPackDialog::OnBnClickedRemove()
{
    int nItem = m_EquipPacksList->currentIndex();
    if (nItem == -1)
    {
        return;
    }
    QString sName = m_EquipPacksList->currentText();
    CEquipPack* pEquipPack = GetIEditor()->GetEquipPackLib()->FindEquipPack(sName);
    if (!pEquipPack)
    {
        return;
    }

    QList<QTreeWidgetItem*> selection = m_EquipList->selectedItems();
    QTreeWidgetItem* selectedItem = selection.isEmpty() ? nullptr : selection.front();

    if (selectedItem != nullptr)
    {
        sName = selectedItem->text(0);
        SEquipment* pEquip = GetEquipment(sName);

        if (!pEquip)
        {
            return;
        }

        pEquipPack->RemoveEquip(*pEquip);

        m_AvailEquipList->addItem(sName);
        delete selectedItem;
    }

    OnLbnSelchangeEquipavaillst();
}

void CEquipPackDialog::StoreUsedEquipmentList()
{
    int nItem = m_EquipPacksList->currentIndex();
    if (nItem == -1)
    {
        return;
    }

    QString sName = m_EquipPacksList->currentText();

    CEquipPack* pEquipPack = GetIEditor()->GetEquipPackLib()->FindEquipPack(sName);
    if (!pEquipPack)
    {
        return;
    }

    pEquipPack->Clear();

    for (int i = 0; i < m_EquipList->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = m_EquipList->topLevelItem(i);

        sName = item->text(0);
        SEquipment* pEquip = GetEquipment(sName);
        if (pEquip)
        {
            SEquipment newEquip;
            newEquip.sName = pEquip->sName;
            newEquip.sType = pEquip->sType;
            QString setupString = QStringLiteral("|");

            if (item->checkState(0) == Qt::Checked)
            {
                for (int i = 0; i < item->childCount(); i++)
                {
                    QTreeWidgetItem* child = item->child(i);
                    bool isChecked = child->checkState(0) == Qt::Checked;

                    if (isChecked)
                    {
                        if (child->childCount())
                        {
                            for (int j = 0; j < child->childCount(); j++)
                            {
                                QTreeWidgetItem* childsChild = child->child(j);
                                if (childsChild->checkState(0) == Qt::Checked)
                                {
                                    setupString += childsChild->text(0) + QStringLiteral("|");
                                    break;
                                }
                            }
                        }
                        else
                        {
                            setupString += child->text(0) + QStringLiteral("|");
                        }
                    }
                }
            }

            if (setupString != QStringLiteral("|"))
            {
                newEquip.sSetup = setupString;
            }

            pEquipPack->AddEquip(newEquip);
        }
    }
}

void CEquipPackDialog::CopyChildren(QTreeWidgetItem* copyToItem, QTreeWidgetItem* copyFromItem)
{
    for (int i = 0; i < copyFromItem->childCount(); i++)
    {
        QTreeWidgetItem* copyFromChild = copyFromItem->child(i);

        QTreeWidgetItem* newChild = new QTreeWidgetItem(copyToItem, QStringList {copyFromChild->text(0)});
        newChild->setFlags(newChild->flags() | Qt::ItemIsUserCheckable);
        newChild->setCheckState(0, copyFromChild->checkState(0));

        if (copyFromChild->childCount())
        {
            CopyChildren(newChild, copyFromChild);
        }

        newChild->setExpanded(copyFromChild->isExpanded());
    }
}

QTreeWidgetItem* CEquipPackDialog::MoveEquipmentListItem(QTreeWidgetItem* existingItem, QTreeWidgetItem* insertAfter)
{
    QTreeWidgetItem* newItem = new QTreeWidgetItem(QStringList { existingItem->text(0) });
    newItem->setFlags(newItem->flags() | Qt::ItemIsUserCheckable);
    newItem->setCheckState(0, existingItem->checkState(0));

    int ndx = m_EquipList->indexOfTopLevelItem(insertAfter);
    if (ndx == m_EquipList->topLevelItemCount() - 1)
    {
        m_EquipList->addTopLevelItem(newItem);
    }
    else
    {
        m_EquipList->insertTopLevelItem(ndx + 1, newItem);
    }

    CopyChildren(newItem, existingItem);

    newItem->setExpanded(existingItem->isExpanded());
    delete existingItem;

    return newItem;
}

void CEquipPackDialog::OnBnClickedMoveUp()
{
    auto selection = m_EquipList->selectedItems();
    if (selection.isEmpty())
    {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();

    int ndx = m_EquipList->indexOfTopLevelItem(selectedItem);
    if (ndx <= 0)
    {
        return; // move should only be available for root level items
    }
    // first one can't be moved up

    QTreeWidgetItem* prevItem = m_EquipList->topLevelItem(ndx - 1);

    MoveEquipmentListItem(prevItem, selectedItem);

    StoreUsedEquipmentList();
    m_EquipList->selectionModel()->clear();
    selectedItem->setSelected(true);
    OnTvnSelchangedEquipusedlst();
    UpdatePrimary();
}

void CEquipPackDialog::OnBnClickedMoveDown()
{
    auto selection = m_EquipList->selectedItems();
    if (selection.isEmpty())
    {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();

    int ndx = m_EquipList->indexOfTopLevelItem(selectedItem);
    if (ndx < 0)
    {
        return; // move should only be available for root level items
    }
    if (ndx >= m_EquipList->topLevelItemCount())
    {
        return; // can't move down last item
    }
    QTreeWidgetItem* nextItem = m_EquipList->topLevelItem(ndx + 1);

    QTreeWidgetItem* newItem = MoveEquipmentListItem(selectedItem, nextItem);

    StoreUsedEquipmentList();
    m_EquipList->selectionModel()->clear();
    newItem->setSelected(true);
    OnTvnSelchangedEquipusedlst();
    UpdatePrimary();
}

void CEquipPackDialog::OnLbnSelchangeEquipavaillst()
{
    m_InsertBtn->setEnabled(m_AvailEquipList->count() != 0);
}

void CEquipPackDialog::OnBnClickedImport()
{
    CAutoDirectoryRestoreFileDialog dialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, "eqp", {}, "Equipment-Pack-Files (*.eqp)", {}, {}, this);
    if (dialog.exec())
    {
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(dialog.selectedFiles().first().toStdString().c_str());
        if (root)
        {
            GetIEditor()->GetEquipPackLib()->Serialize(root, true);
            UpdateEquipPacksList();
        }
    }
}

void CEquipPackDialog::OnBnClickedExport()
{
    CAutoDirectoryRestoreFileDialog dialog(QFileDialog::AcceptSave, QFileDialog::AnyFile, "eqp", {}, "Equipment-Pack-Files (*.eqp)", {}, {}, this);
    if (dialog.exec())
    {
        XmlNodeRef root = XmlHelpers::CreateXmlNode("EquipPackDB");
        GetIEditor()->GetEquipPackLib()->Serialize(root, false);
        XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, dialog.selectedFiles().first().toStdString().c_str());
    }
}

void CEquipPackDialog::OnTvnSelchangedEquipusedlst()
{
    auto selection = m_EquipList->selectedItems();
    QTreeWidgetItem* selectedItem = selection.isEmpty() ? nullptr : selection.front();

    int ndx = m_EquipList->indexOfTopLevelItem(selectedItem);
    bool showButtons = (selectedItem != nullptr) && ndx != -1;

    m_RemoveBtn->setEnabled(showButtons);
    ui->MOVEUP->setEnabled(showButtons && ndx > 0);
    ui->MOVEDOWN->setEnabled(showButtons && ndx < m_EquipList->topLevelItemCount() - 1);
}

void CEquipPackDialog::OnCheckStateChange(QTreeWidgetItem* checkItem, int column)
{
    if (m_updating)
    {
        return;
    }
    m_updating = true;

    bool itemHasChildren = checkItem->childCount();

    if (checkItem->checkState(0) != Qt::Checked)
    {
        // if unchecking a parent, then uncheck all children too
        if (itemHasChildren)
        {
            SetChildCheckState(checkItem, false);
        }
    }
    else if (!itemHasChildren)
    {
        //check it has a category and item parent, and uncheck siblings if true
        QTreeWidgetItem* categoryParent = checkItem->parent();

        if (categoryParent)
        {
            QTreeWidgetItem* itemParent = categoryParent->parent();

            if (itemParent)
            {
                SetChildCheckState(categoryParent, false, checkItem);
            }
        }
        else
        {
            //if item has no parent and no children force unchecked
            checkItem->setCheckState(0, Qt::Unchecked);
        }
    }

    m_EquipList->selectionModel()->clear();
    checkItem->setSelected(true);

    UpdateCheckState(checkItem);
    StoreUsedEquipmentList();

    m_updating = false;
}

void CEquipPackDialog::SetChildCheckState(QTreeWidgetItem* checkItem, bool checkState, QTreeWidgetItem* ignoreItem)
{
    for (int i = 0; i < checkItem->childCount(); i++)
    {
        QTreeWidgetItem* childItem = checkItem->child(i);
        if (childItem != ignoreItem)
        {
            childItem->setCheckState(0, checkState ? Qt::Checked : Qt::Unchecked);

            if (childItem->childCount())
            {
                SetChildCheckState(childItem, checkState);
            }
        }
    }
}

void CEquipPackDialog::UpdateCheckState(QTreeWidgetItem* checkItem)
{
    if (checkItem != nullptr)
    {
        if (checkItem->childCount())
        {
            checkItem->setCheckState(0, HasCheckedChild(checkItem) ? Qt::Checked : Qt::Unchecked);
        }

        UpdateCheckState(checkItem->parent());
    }
}

bool CEquipPackDialog::HasCheckedChild(QTreeWidgetItem* parentItem)
{
    for (int i = 0; i < parentItem->childCount(); i++)
    {
        QTreeWidgetItem* childItem = parentItem->child(i);
        if (childItem->checkState(0) == Qt::Checked)
        {
            return true;
        }
    }

    return false;
}


void CEquipPackDialog::OnBnClickedCancel()
{
    //reload from files to overwrite local changes
    GetIEditor()->GetEquipPackLib()->Reset();
    GetIEditor()->GetEquipPackLib()->LoadLibs(true);
    reject();
}


void CEquipPackDialog::OnBnClickedOk()
{
    StoreUsedEquipmentList();
    GetIEditor()->GetEquipPackLib()->SaveLibs(true);
    accept();
}

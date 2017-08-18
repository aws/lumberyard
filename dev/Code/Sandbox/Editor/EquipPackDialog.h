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

#ifndef CRYINCLUDE_EDITOR_EQUIPPACKDIALOG_H
#define CRYINCLUDE_EDITOR_EQUIPPACKDIALOG_H

#pragma once

#include "EquipPack.h"

#include <QDialog>
#include <QScopedPointer>

struct IVariable;

typedef std::list<SEquipment>   TLstString;
typedef TLstString::iterator    TLstStringIt;

namespace Ui {
    class CEquipPackDialog;
}

class QComboBox;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QToolButton;
class QLineEdit;
class ReflectedPropertyControl;

// CEquipPackDialog dialog
class CEquipPackDialog
    : public QDialog
{
private:
    ReflectedPropertyControl* m_AmmoPropWnd;
    QComboBox* m_EquipPacksList;
    QListWidget* m_AvailEquipList;
    QTreeWidget* m_EquipList;
    QPushButton* m_OkBtn;
    QPushButton* m_ExportBtn;
    QPushButton* m_AddBtn;
    QPushButton* m_DeleteBtn;
    QPushButton* m_RenameBtn;
    QToolButton* m_InsertBtn;
    QToolButton* m_RemoveBtn;
    QLineEdit*   m_PrimaryEdit;

    TLstString m_lstAvailEquip;
    QString m_sCurrEquipPack;
    CVarBlockPtr m_pVarBlock;
    bool m_bEditMode;
    bool m_updating;

    QScopedPointer<Ui::CEquipPackDialog> ui;

private:
    void UpdateEquipPacksList();
    void UpdateEquipPackParams();
    void UpdatePrimary();
    void AmmoUpdateCallback(IVariable* pVar);
    SEquipment* GetEquipment(QString sDesc);
    void StoreUsedEquipmentList();
    QTreeWidgetItem* MoveEquipmentListItem(QTreeWidgetItem* existingItem, QTreeWidgetItem* insertAfter);
    void AddEquipmentListItem(const SEquipment& equip);
    void CopyChildren(QTreeWidgetItem* copyToItem, QTreeWidgetItem* copyFromItem);
    void SetChildCheckState(QTreeWidgetItem* checkItem, bool checkState, QTreeWidgetItem* ignoreItem = NULL);
    bool HasCheckedChild(QTreeWidgetItem* parentItem);
    void UpdateCheckState(QTreeWidgetItem* childItem);
    CVarBlockPtr CreateVarBlock(CEquipPack* pEquip);

public:
    void SetCurrEquipPack(const QString& sValue) { m_sCurrEquipPack = sValue; }
    QString GetCurrEquipPack() const { return m_sCurrEquipPack; }
    void SetEditMode(bool bEditMode) { m_bEditMode = bEditMode; }

public:
    CEquipPackDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CEquipPackDialog();

public:
    void OnInitDialog();
    void OnCbnSelchangeEquippack();
    void OnBnClickedAdd();
    void OnBnClickedDelete();
    void OnBnClickedRename();
    void OnBnClickedInsert();
    void OnBnClickedRemove();
    void OnLbnSelchangeEquipavaillst();
    void OnBnClickedImport();
    void OnBnClickedExport();
    void OnBnClickedMoveUp();
    void OnBnClickedMoveDown();
    void OnBnClickedCancel();
    void OnTvnSelchangedEquipusedlst();
    void OnCheckStateChange(QTreeWidgetItem* item, int column);
    void OnBnClickedOk();
};

#endif // CRYINCLUDE_EDITOR_EQUIPPACKDIALOG_H

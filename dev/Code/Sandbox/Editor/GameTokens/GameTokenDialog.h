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

#ifndef CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENDIALOG_H
#define CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENDIALOG_H
#pragma once

#include "BaseLibraryDialog.h"
#include "Controls/PreviewModelCtrl.h"

#include <qscopedpointer.h>

class CGameTokenItem;
class CGameTokenManager;
class GameTokenTypeChangeComboBox;

class GameTokenModel;
class GameTokenProxyModel;

class QTreeView;
class QValidator;

/** Dialog which hosts entity prototype library.
*/

namespace Ui {
    class GameTokenDialog;
}
class CGameTokenDialog
    : public CBaseLibraryDialog
{
    Q_OBJECT

public:
    CGameTokenDialog(QWidget* pParent);
    ~CGameTokenDialog();

    CGameTokenItem* GetSelectedGameToken();

protected:
    virtual QTreeView* GetTreeCtrl() override;
    virtual AbstractGroupProxyModel* GetProxyModel() override;
    virtual QAbstractItemModel* GetSourceModel() override;
    virtual BaseLibraryDialogModel* GetSourceDialogModel() override;

    virtual void OnUpdateActions() override;

    void OnAddItem();

    void OnTreeRClick(const QModelIndex& index);
    void OnCopy();
    void OnPaste();

    void OnCustomContextMenuRequested(const QPoint& point);
    void OnReportDblClick(const QModelIndex& index);
    void OnReportSelChange();
    void OnTreeWidgetItemChanged(const QModelIndex& index);

    void OnTokenTypeChange();
    void OnTokenValueChange();
    void OnTokenInfoChange();
    void OnTokenLocalOnlyChange();

    //////////////////////////////////////////////////////////////////////////
    // Some functions can be overriden to modify standart functionality.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitToolbar(/*UINT nToolbarResID */);
    virtual void InitItemToolbar();
    virtual void ReloadItems();
    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);

    const char* GetLibraryAssetTypeName() const override;

private:
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    void SetWidgetItemEditable(const QModelIndex& index);
    void InstallValidator(const QString& typeName);
    GameTokenTypeChangeComboBox* InitializeTypeChangeComboBox();
    QModelIndex GetCurrentSelectedTreeWidgetItemRecord();

    CGameTokenManager* m_pGameTokenManager;

    GameTokenModel* m_model;
    GameTokenProxyModel* m_proxyModel;

    bool m_bSkipUpdateItems;
    bool m_bEditingItems;

    QScopedPointer<Ui::GameTokenDialog> ui;

    QValidator* m_floatValidator;
    QValidator* m_intValidator;
    QValidator* m_boolValidator;
    QValidator* m_vec3Validator;
};

#endif // CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENDIALOG_H

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

#ifndef CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGEDITORDIALOG_H
#pragma once

#include "DialogScriptView.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <Util/AbstractGroupProxyModel.h>

#include <QMainWindow>
#include <QDockWidget>
#include <QTextEdit>
#include <QLabel>
#include <QTreeView>

class CDialogManager;
class DialogScriptModel;
class CDialogEditorDialog;
class QTextEdit;
class QCollapsibleGroupBox;
class QAction;
class QMenu;
class ScriptTreeModel;
class ScriptListModel;

#define DIALOG_EDITOR_NAME "Dialog Editor"
#define DIALOG_EDITOR_VER "1.00"

class ScriptTreeView
    : public QTreeView
{
    Q_OBJECT
public:
    explicit ScriptTreeView(CDialogEditorDialog* editor, QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QString GetCurrentGroup() const;
    void setModel(QAbstractItemModel* model) override;
    void SelectScript(CEditorDialogScript*);
protected:
    void contextMenuEvent(QContextMenuEvent* ev) override;
private:
    void updateLabel();
    CDialogEditorDialog* m_editor;
    QMenu* m_menu;
    QLabel* m_label;
};

class ScriptsDock
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    explicit ScriptsDock(CDialogEditorDialog* editor, ScriptTreeModel* scriptModel, QWidget* parent = nullptr);
    QSize sizeHint() const override;
    ScriptTreeView* GetScriptTreeView() const;
    private Q_SLOT:
    void OnSelectionChanged();
Q_SIGNALS:
    void scriptSelected(CEditorDialogScript*);
private:
    ScriptTreeView* m_scriptView;
    ScriptTreeModel* m_scriptTreeModel;
};

class ActionsDock
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    explicit ActionsDock(CDialogEditorDialog* dialog, QWidget* parent = nullptr);
    QSize sizeHint() const override;
private:
    QCollapsibleGroupBox* m_groupBox;
    CDialogEditorDialog* m_dialog;
};

class HelpDock
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    explicit HelpDock(QWidget* parent = nullptr);
    QTextEdit* GetTextEdit() const;
private:
    QTextEdit* m_textEdit;
};

class DescriptionDock
    : public AzQtComponents::StyledDockWidget
{
    Q_OBJECT
public:
    explicit DescriptionDock(QWidget* parent = nullptr);
    QString GetText() const;
    void SetText(const QString&);
Q_SIGNALS:
    void textChanged();
private:
    QTextEdit* m_textEdit;
};

class CDialogEditorDialog
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    enum ESourceControlOp
    {
        ESCM_IMPORT,
        ESCM_CHECKOUT,
        ESCM_UNDO_CHECKOUT,
        ESCM_GETLATEST,
    };

    explicit CDialogEditorDialog(QWidget* parent = nullptr);
    ~CDialogEditorDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID()
    {
        // {6DFCF286-5A36-47eb-A945-0E1A330509D0}
        static const GUID guid = {
            0x6dfcf286, 0x5a36, 0x47eb, { 0xa9, 0x45, 0xe, 0x1a, 0x33, 0x5, 0x9, 0xd0 }
        };
        return guid;
    }

    CDialogManager* GetManager() const;
    bool DoSourceControlOp(CEditorDialogScript * pScript, ESourceControlOp);
    void closeEvent(QCloseEvent* ev) override;

public Q_SLOTS:
    void CreateScript();
    void DeleteScript();
    void RenameScript();
    void ReloadDialogBrowser();
    void OnLocalEdit();

private Q_SLOTS:
    void AddScriptLine();
    void DeleteScriptLine();
    void OnScriptSelected(CEditorDialogScript*);
    void UpdateToolbarActions();
    void UpdateWindowText(CEditorDialogScript* script, bool titleOnly);
    void OnScriptLineModified(bool modified);
    void OnDescChanged();
protected:
    void OnEditorNotifyEvent(EEditorNotifyEvent ev);
    QMenu* createPopupMenu() override;

private:
    bool SetCurrentScript(CEditorDialogScript* pScript, bool bSelectInTree, bool bSaveModified = true, bool bForceUpdate = false);
    bool SaveCurrent();
    QString GetCurrentDescription() const;
    ActionsDock* m_actionDock;
    HelpDock* m_helpDock;
    DescriptionDock* m_descDock;
    DialogScriptModel* m_model;
    DialogScriptView* m_view;
    CDialogManager* m_dialogManager;
    ScriptTreeModel* m_scriptTreeModel;
    ScriptListModel* m_scriptListModel;
    ScriptsDock* m_scriptDock;
    QAction* m_addAction;
    QAction* m_deleteAction;
};

class DescriptionTextEdit
    : public QTextEdit
{
    Q_OBJECT
public:
    explicit DescriptionTextEdit(QWidget* parent = nullptr)
        : QTextEdit(parent)
    {
        setMinimumHeight(10);
        setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    }
    QSize sizeHint() const override { return QSize(1, 30); }
};

class ScriptListModel
    : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        GroupPathRole = Qt::UserRole,
        ScriptRole
    };

    explicit ScriptListModel(CDialogManager* dialogManager, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void Reload();
    void Reload(CEditorDialogScript*);
private:
    CDialogManager* m_dialogManager;
    QList<CEditorDialogScript*> m_items;
};

class ScriptTreeModel
    : public AbstractGroupProxyModel
{
    Q_OBJECT
public:
    explicit ScriptTreeModel(CDialogManager* dialogManager, QWidget* parent = nullptr);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    CEditorDialogScript* GetScriptAt(const QModelIndex& idx) const;
protected:
    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override;
private:
    CDialogManager* m_dialogManager;
};

Q_DECLARE_METATYPE(CEditorDialogScript*)

#endif // CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGEDITORDIALOG_H

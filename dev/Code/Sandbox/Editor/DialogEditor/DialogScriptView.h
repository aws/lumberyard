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

// Description : A report control to edit items


#ifndef CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTVIEW_H
#define CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTVIEW_H
#pragma once

#include <IAudioSystem.h>
#include "DialogManager.h"

#include <QAbstractTableModel>
#include <QTableView>
#include <QSettings>

class QMenu;
class QAction;
class QMouseEvent;
class QLabel;
class CEditorDialogScript;
class DialogScriptModel;
class CDialogEditorDialog;

class DialogScriptView
    : public QTableView
{
    Q_OBJECT
public:
    explicit DialogScriptView(QWidget* parent = nullptr);
    ~DialogScriptView();
    void setModel(QAbstractItemModel* model) override;
    void SetScript(CEditorDialogScript* pScript);

    QString GetCurrentHelpText() const;
    int GetSelectedColumn() const;

    bool IsAllowEdit() const;
    void AllowEdit(bool allow);
    bool IsModified() const;
    void TryDeleteRow(); // Asks for confirmation first
    void SetDialogEditor(CDialogEditorDialog* pDialogEditor);

    bool CanDeleteRow() const;
    bool CanAddRow() const;

    void SetModified(bool);
    void AddNewRow(bool forceEnd);
    void RemoveSelectedRow();

    static void OnAudioTriggerFinished(Audio::SAudioRequestInfo const* const pAudioRequestInfo);

private Q_SLOTS:
    void ToggleColumnVisibility(QAction*);
    void OnSelectedColumnChanged();
    void OnBrowseAudioTrigger(int row);
    void OnBrowseAG(int row);
    void OnBrowseFacial(int row);
protected:
    bool eventFilter(QObject* watched, QEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent* ev) override;

signals:
    void currentColumnChanged();
    void modifiedChanged(bool);
    void CanDeleteRowChanged(); // TODO: Connect these
    void CanAddRowChanged();
    void helpTextChanged(const QString&);

private:
    void UpdateNoItemText();
    void PlayLine(int row);
    void StopSound();
    void InitializeColumnWidths();
    void CreateHeaderPopup();
    void SaveColumnsVisibility();
    int SelectedRow() const;
    QMenu* m_headerPopup;
    QSettings m_settings;
    CDialogEditorDialog* m_editor;
    DialogScriptModel* m_model;
    static Audio::TAudioControlID ms_currentPlayLine;
    Audio::IAudioProxy* m_pIAudioProxy;
    QLabel* m_label;
    bool m_modified;
    bool m_allowEdit;
    friend class CDialogScriptView;
};

class DialogScriptModel
    : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        LineColumn = 0,
        ActorColumn,
        AudioIDColumn,
        AnimationColumn,
        TypeColumn,
        EPColumn,
        SyncColumn,
        FacialExprColumn,
        WeightColumn,
        FadeColumn,
        LookAtColumn,
        StickyColumn,
        DelayColumn,
        DescriptionColumn,
        NumberOfColumns // Keep at end
    };

    explicit DialogScriptModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void SetScript(CEditorDialogScript* script);
    CEditorDialogScript* GetScript() const;
    void InsertAt(int row, const CEditorDialogScript::SScriptLine& record);
    void RemoveAt(int row);
    const CEditorDialogScript::SScriptLine* getLineAt(int row) const;
    void Append(const CEditorDialogScript::SScriptLine& record);
    void Reset();
    CEditorDialogScript* Script() const;
    void SaveToScript();

signals:
    void modelModified();
private:
    CEditorDialogScript* m_script;
    QVector<CEditorDialogScript::SScriptLine> m_rows;
};

#endif // CRYINCLUDE_EDITOR_DIALOGEDITOR_DIALOGSCRIPTVIEW_H

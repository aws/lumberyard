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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H
#define CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H
#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QScopedPointer>

class QMenu;
class ConsoleWidget;
class QFocusEvent;
class QTableView;

namespace Ui {
    class Console;
}

struct ConsoleLine
{
    QString text;
    bool newLine;
};
typedef std::deque<ConsoleLine> Lines;

class ConsoleLineEdit
    : public QLineEdit
{
    Q_OBJECT
public:
    explicit ConsoleLineEdit(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    bool event(QEvent* ev) override;

signals:
    void variableEditorRequested();
    void setWindowTitle(const QString&);

private:
    void DisplayHistory(bool bForward);
    void ResetHistoryIndex();

    QStringList m_history;
    unsigned int m_historyIndex;
    bool m_bReusedHistory;
};

class ConsoleTextEdit
    : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit ConsoleTextEdit(QWidget* parent = nullptr);
    virtual bool event(QEvent* theEvent) override;

private:
    void showContextMenu(const QPoint& pt);

    QScopedPointer<QMenu> m_contextMenu;
};

class ConsoleVariableItemDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ConsoleVariableItemDelegate(QObject* parent = nullptr);

    // Item delegate overrides for creating the custom editor widget and
    // setting/retrieving data to/from it
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void SetVarBlock(CVarBlock* varBlock);

Q_SIGNALS:
    void editInProgress() const;

private:
    CVarBlock* m_varBlock;
};

class ConsoleVariableModel
    : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum CustomRoles
    {
        VariableCustomRole = Qt::UserRole
    };

    explicit ConsoleVariableModel(QObject* parent = nullptr);

    // Table model overrides
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void SetVarBlock(CVarBlock* varBlock);
    void ClearModifiedRows();

private:
    CVarBlock* m_varBlock;
    QList<int> m_modifiedRows;
};

class ConsoleVariableEditor
    : public QWidget
{
    Q_OBJECT
public:
    explicit ConsoleVariableEditor(QWidget* parent = nullptr);

    static void RegisterViewClass();
    void HandleVariableRowUpdated(ICVar* pCVar);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void SetVarBlock(CVarBlock* varBlock);

    QTableView* m_tableView;
    ConsoleVariableModel* m_model;
    ConsoleVariableItemDelegate* m_itemDelegate;
    CVarBlock* m_varBlock;
};

class CConsoleSCB
    : public QWidget
{
    Q_OBJECT
public:
    explicit CConsoleSCB(QWidget* parent = nullptr);
    ~CConsoleSCB();

    static void RegisterViewClass();
    void SetInputFocus();
    void AddToConsole(const QString& text, bool bNewLine);
    void FlushText();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static CConsoleSCB* GetCreatedInstance();

    static void AddToPendingLines(const QString& text, bool bNewLine);        // call this function instead of AddToConsole() until an instance of CConsoleSCB exists to prevent messages from getting lost

public Q_SLOTS:
    void OnStyleSettingsChanged();

private Q_SLOTS:
    void showVariableEditor();

private:
    QScopedPointer<Ui::Console> ui;
    int m_richEditTextLength;

    Lines m_lines;
    static Lines s_pendingLines;

    QList<QColor> m_colorTable;
    SEditorSettings::ConsoleColorTheme m_backgroundTheme;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_CONSOLESCB_H


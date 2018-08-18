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

// Description : For listing available script commands with their descriptions


#include "StdAfx.h"
#include "ScriptHelpDialog.h"
#include "Util/BoostPythonHelpers.h"
#include <ui_ScriptHelpDialog.h>

#include <QVBoxLayout>
#include <QClipboard>
#include <QApplication>
#include <QLineEdit>
#include <QResizeEvent>

#include <array>

HeaderView::HeaderView(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
    , m_commandFilter(new QLineEdit(this))
    , m_moduleFilter(new QLineEdit(this))
{
    // Allow the header sections to be clickable so we can change change the
    // sort order on click
    setSectionsClickable(true);

    connect(this, &QHeaderView::geometriesChanged, this, &HeaderView::repositionLineEdits);
    connect(this, &QHeaderView::sectionMoved, this, &HeaderView::repositionLineEdits);
    connect(this, &QHeaderView::sectionResized, this, &HeaderView::repositionLineEdits);
    connect(m_commandFilter, &QLineEdit::textChanged, this, &HeaderView::commandFilterChanged);
    connect(m_moduleFilter, &QLineEdit::textChanged, this, &HeaderView::moduleFilterChanged);

    // Calculate our height offset to embed our line edits in the header
    const int margins = frameWidth() * 2 + 1;
    m_lineEditHeightOffset = m_commandFilter->sizeHint().height() + margins;
}

QSize HeaderView::sizeHint() const
{
    // Adjust our height to include the line edit offset
    QSize size = QHeaderView::sizeHint();
    size.setHeight(size.height() + m_lineEditHeightOffset);
    return size;
}

void HeaderView::resizeEvent(QResizeEvent* ev)
{
    QHeaderView::resizeEvent(ev);
    repositionLineEdits();
}

void HeaderView::repositionLineEdits()
{
    const int headerHeight = sizeHint().height();
    const int lineEditYPos = headerHeight - m_lineEditHeightOffset;
    const int col0Width = sectionSize(0);
    const int col1Width = sectionSize(1);
    const int adjustment = 2;

    if (col0Width <= adjustment || col1Width <= adjustment)
    {
        return;
    }

    m_commandFilter->setFixedWidth(col0Width - adjustment);
    m_moduleFilter->setFixedWidth(col1Width - adjustment);

    m_commandFilter->move(1, lineEditYPos);
    m_moduleFilter->move(col0Width + 1, lineEditYPos);

    m_commandFilter->show();
    m_moduleFilter->show();
}

ScriptHelpProxyModel::ScriptHelpProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool ScriptHelpProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (!sourceModel())
    {
        return false;
    }

    const QString command = sourceModel()->index(source_row, ScriptHelpModel::ColumnCommand).data(Qt::DisplayRole).toString().toLower();
    const QString module = sourceModel()->index(source_row, ScriptHelpModel::ColumnModule).data(Qt::DisplayRole).toString().toLower();

    return command.contains(m_commandFilter) && module.contains(m_moduleFilter);
}

void ScriptHelpProxyModel::setCommandFilter(const QString& text)
{
    const QString lowerText = text.toLower();
    if (m_commandFilter != lowerText)
    {
        m_commandFilter = lowerText;
        invalidateFilter();
    }
}

void ScriptHelpProxyModel::setModuleFilter(const QString& text)
{
    const QString lowerText = text.toLower();
    if (m_moduleFilter != lowerText)
    {
        m_moduleFilter = lowerText;
        invalidateFilter();
    }
}

ScriptHelpModel::ScriptHelpModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

QVariant ScriptHelpModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount() || index.column() < 0 || index.column() >= ColumnCount)
    {
        return QVariant();
    }

    const int col = index.column();
    const Item& item = m_items[index.row()];

    if (role == Qt::DisplayRole)
    {
        if (col == ColumnCommand)
        {
            return item.command;
        }
        else if (col == ColumnModule)
        {
            return item.module;
        }
        else if (col == ColumnDescription)
        {
            return item.description;
        }
        else if (col == ColumnExample)
        {
            return item.example;
        }
    }

    return QVariant();
}

int ScriptHelpModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return m_items.size();
}

int ScriptHelpModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return ColumnCount;
}

Qt::ItemFlags ScriptHelpModel::flags(const QModelIndex& index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant ScriptHelpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= ColumnCount || orientation == Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        static const QStringList headers = { tr("Command"), tr("Module"), tr("Description"), tr("Example") };
        return headers.at(section);
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return Qt::AlignLeft;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

void ScriptHelpModel::Reload()
{
    beginResetModel();

    CAutoRegisterPythonCommandHelper* pCurrent = CAutoRegisterPythonCommandHelper::s_pFirst;
    while (pCurrent)
    {
        Item item;
        item.command = pCurrent->m_name;
        item.module = QString(CAutoRegisterPythonModuleHelper::s_modules[pCurrent->m_moduleIndex].name.c_str());
        item.description = pCurrent->m_description;
        item.example = pCurrent->m_example;
        m_items.push_back(item);
        pCurrent = pCurrent->m_pNext;
    }

    endResetModel();
}

ScriptTableView::ScriptTableView(QWidget* parent)
    : QTableView(parent)
    , m_model(new ScriptHelpModel(this))
    , m_proxyModel(new ScriptHelpProxyModel(parent))
{
    HeaderView* headerView = new HeaderView(this);
    setHorizontalHeader(headerView); // our header view embeds filter line edits
    QObject::connect(headerView, SIGNAL(sectionPressed(int)), this, SLOT(sortByColumn(int)));

    m_proxyModel->setSourceModel(m_model);
    setModel(m_proxyModel);
    m_model->Reload();
    setSortingEnabled(true);
    horizontalHeader()->setSortIndicatorShown(false);
    horizontalHeader()->setStretchLastSection(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ContiguousSelection); // Not very useful for this dialog, but the MFC code allowed to select many rows

    static const std::array<int, ScriptHelpModel::ColumnCount> colWidths = { { 100, 60, 300, 200 } };
    for (int col = 0; col < ScriptHelpModel::ColumnCount; ++col)
    {
        setColumnWidth(col, colWidths[col]);
    }

    connect(headerView, &HeaderView::commandFilterChanged,
        m_proxyModel, &ScriptHelpProxyModel::setCommandFilter);
    connect(headerView, &HeaderView::moduleFilterChanged,
        m_proxyModel, &ScriptHelpProxyModel::setModuleFilter);
}

CScriptHelpDialog::CScriptHelpDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.reset(new Ui::ScriptDialog);
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Script Help"));
    connect(ui->tableView, &ScriptTableView::doubleClicked, this, &CScriptHelpDialog::OnDoubleClick);
}

void CScriptHelpDialog::OnDoubleClick(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }
    const QString command = index.sibling(index.row(), ScriptHelpModel::ColumnCommand).data(Qt::DisplayRole).toString();
    const QString module = index.sibling(index.row(), ScriptHelpModel::ColumnModule).data(Qt::DisplayRole).toString();
    const QString textForClipboard = module + QLatin1Char( '.' ) + command + "()";

    QApplication::clipboard()->setText(textForClipboard);
    setWindowTitle(QString("Script Help (Copied \"%1\" to clipboard)").arg(textForClipboard));
}

#include <ScriptHelpDialog.moc>

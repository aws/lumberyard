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
#include "AssetResolverDialog.h"
#include "AssetResolver.h"
#include "IAssetSearcher.h"

#include "Util/CryMemFile.h"                    // CCryMemFile

#include <QtUtil.h>

#include <QBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QTreeView>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif

#define ASSET_RESOLVE_DIAG_CLASSNAME "Missing Asset Resolver"

// CErrorReportDialog dialog
#define BITMAP_NOT_RESOLVED 0
#define BITMAP_PENDING 2
#define BITMAP_AUTO_RESOLVED 1

#define COLUMN_ICON        0
#define COLUMN_TYPE        1
#define COLUMN_ORGNAME     2
#define COLUMN_NEWNAME     3
#define COLUMN_STATE       4

class MissingAssetModel
    : public QAbstractTableModel
{
public:
    MissingAssetModel(QObject* parent = 0)
        : QAbstractTableModel(parent)
    {
        m_imageList.push_back(QPixmap(":/error_report_00.png"));
        m_imageList.push_back(QPixmap(":/error_report_01.png"));
        m_imageList.push_back(QPixmap(":/error_report_02.png"));
    }

    bool appendRow(CMissingAssetRecord* record)
    {
        if (std::find(m_records.begin(), m_records.end(), record) != m_records.end())
        {
            return false;
        }
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_records.push_back(record);
        endInsertRows();
        return true;
    }

    void updateRow(CMissingAssetRecord* record)
    {
        const auto it = std::find(m_records.begin(), m_records.end(), record);
        if (it == m_records.end())
        {
            return;
        }
        const int row = it - m_records.begin();
        record->SetUpdated();
        dataChanged(index(row, 0), index(row, columnCount() - 1));
    }

    void removeRow(CMissingAssetRecord* record)
    {
        const auto it = std::find(m_records.begin(), m_records.end(), record);
        if (it == m_records.end())
        {
            return;
        }
        const int row = it - m_records.begin();
        beginRemoveRows(QModelIndex(), row, row);
        m_records.erase(it);
        endRemoveRows();
    }

    void clear()
    {
        if (m_records.empty())
        {
            return;
        }
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_records.clear();
        endRemoveRows();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.isValid())
        {
            return 0;
        }
        return m_records.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.isValid())
        {
            return 0;
        }
        return 5;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        CMissingAssetRecord* record = m_records[index.row()];
        if (role == Qt::UserRole)
        {
            return QVariant::fromValue(record);
        }

        int icon = -1;
        QString state;
        IAssetSearcher* searcher = GetIEditor()->GetMissingAssetResolver()->GetAssetSearcherById(record->searcherId);
        switch (record->state)
        {
        case CMissingAssetRecord::ESTATE_PENDING:
            state = QLatin1String("Searching"); // these strings were not translated in the MFC version either
            icon = BITMAP_PENDING;
            break;
        case CMissingAssetRecord::ESTATE_AUTO_RESOLVED:
            state = QLatin1String("Resolved");
            icon = BITMAP_AUTO_RESOLVED;
            break;
        case CMissingAssetRecord::ESTATE_NOT_RESOLVED:
            state = QLatin1String("Not found");
            icon = BITMAP_NOT_RESOLVED;
            break;
        case CMissingAssetRecord::ESTATE_ACCEPTED:
            state = QLatin1String("Accepted");
            icon = BITMAP_AUTO_RESOLVED;
            break;
        case CMissingAssetRecord::ESTATE_CANCELLED:
            state = QLatin1String("Cancelled");
            icon = BITMAP_AUTO_RESOLVED;
            break;
        default:
            state = QLatin1String("UNDEFINED");
            break;
        }
        if (record->substitutions.size() > 1)
        {
            state = QString::fromLatin1("%1 (%2 Files)").arg(state, record->substitutions.size());
        }

        if (role == Qt::ForegroundRole)
        {
            switch (icon)
            {
            case BITMAP_PENDING:
                return QColor(255, 255, 0);
            case BITMAP_AUTO_RESOLVED:
                return QColor(0, 255, 0);
            case BITMAP_NOT_RESOLVED:
                return QColor(255, 0, 0);
            }
        }

        switch (index.column())
        {
        case COLUMN_ICON:
            if (role == Qt::DecorationRole && icon != -1)
            {
                return m_imageList[icon];
            }
            break;
        case COLUMN_TYPE:
            return searcher ? searcher->GetAssetTypeName(record->assetTypeId) : QStringLiteral("UNDEFINED");
        case COLUMN_ORGNAME:
            return record->orgname;
        case COLUMN_NEWNAME:
            return record->substitutions.empty() ? QString() : record->substitutions.front();
        case COLUMN_STATE:
            return state;
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case COLUMN_ICON:
                return tr("");
            case COLUMN_TYPE:
                return tr("Type");
            case COLUMN_ORGNAME:
                return tr("Original File");
            case COLUMN_NEWNAME:
                return tr("Resolved File");
            case COLUMN_STATE:
                return tr("State");
            }
        }
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
    {
        layoutAboutToBeChanged();
        std::sort(m_records.begin(), m_records.end(), [&](CMissingAssetRecord* lhs, CMissingAssetRecord* rhs)
            {
                const bool less = lessThan(lhs, rhs, column);
                return order == Qt::AscendingOrder ? less : !less;
            });
        layoutChanged();
    }

    bool lessThan(CMissingAssetRecord* lhs, CMissingAssetRecord* rhs, int column) const
    {
        const auto lhsIt = std::find(m_records.begin(), m_records.end(), lhs);
        if (lhsIt == m_records.end())
        {
            return false;
        }
        const int lhsRow = lhsIt - m_records.begin();
        const auto rhsIt = std::find(m_records.begin(), m_records.end(), rhs);
        if (rhsIt == m_records.end())
        {
            return false;
        }
        const int rhsRow = rhsIt - m_records.begin();

        return index(lhsRow, column).data().toString() < index(rhsRow, column).data().toString();
    }

private:
    std::vector<CMissingAssetRecord*> m_records;
    QVector<QPixmap> m_imageList;
};

//////////////////////////////////////////////////////////////////////////
// CMissingAssetDialog
//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog* CMissingAssetDialog::m_instance = 0;

//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog::CMissingAssetDialog()
    : QWidget()
    , m_treeView(new QTreeView(this))
    , m_model(new MissingAssetModel(this))
{
    m_instance = this;
    setLayout(new QHBoxLayout);
    layout()->addWidget(m_treeView);
    layout()->setMargin(0);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSortingEnabled(true);
    m_treeView->header()->setStretchLastSection(true);

    m_treeView->header()->resizeSection(COLUMN_ICON, 18);
    m_treeView->header()->resizeSection(COLUMN_TYPE, 72);
    m_treeView->header()->resizeSection(COLUMN_ORGNAME, 240);
    m_treeView->header()->resizeSection(COLUMN_NEWNAME, 240);
    m_treeView->header()->resizeSection(COLUMN_STATE, 72);

    connect(m_treeView, &QTreeView::doubleClicked, this, &CMissingAssetDialog::OnReportItemDblClick);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &CMissingAssetDialog::OnReportItemRClick);
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog::~CMissingAssetDialog()
{
    Clear();
    m_instance = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CMissingAssetDialog>(ASSET_RESOLVE_DIAG_CLASSNAME, LyViewPane::CategoryOther, options);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Open()
{
    if (!m_instance)
    {
        GetIEditor()->OpenView(ASSET_RESOLVE_DIAG_CLASSNAME);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Close()
{
    if (m_instance)
    {
        m_instance->close();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Update()
{
    if (!m_instance)
    {
        return;
    }

    m_instance->UpdateReport();
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Clear()
{
    m_model->clear();

    CMissingAssetReport* pReport = GetReport();
    CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
    while (CMissingAssetRecord* pRecord = iter.Next())
    {
        pRecord->pRecordMessage = NULL;
    }
    pReport->SetUpdated(false);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::UpdateReport()
{
    CMissingAssetReport* pReport = GetReport();
    if (!pReport || !pReport->NeedUpdate())
    {
        return;
    }

    CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
    while (CMissingAssetRecord* pRecord = iter.Next())
    {
        if (pRecord->pRecordMessage == NULL)
        {
            if (!pRecord->NeedRemove())
            {
                m_model->appendRow(pRecord);
            }
            else
            {
                m_model->removeRow(pRecord);
            }
        }
        else if (pRecord->NeedRemove())
        {
            m_model->removeRow(pRecord);
        }
        else if (pRecord->NeedUpdate())
        {
            m_model->updateRow(pRecord);
        }
    }

    pReport->SetUpdated(true);
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetReport* CMissingAssetDialog::GetReport()
{
    return GetIEditor()->GetMissingAssetResolver()->GetReport();
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::GetSelectedRecords(TRecords& records)
{
    CMissingAssetReport* pReport = GetReport();
    if (!pReport)
    {
        return;
    }

    const QModelIndexList indexes = m_treeView->selectionModel()->selectedRows();
    for (const QModelIndex& index : indexes)
    {
        CMissingAssetRecord* pRecord = index.data(Qt::UserRole).value<CMissingAssetRecord*>();
        if (pRecord)
        {
            records.push_back(pRecord);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnReportItemRClick()
{
    TRecords records;
    GetSelectedRecords(records);

    enum EMenuItem
    {
        eMI_Nothing = 0,
        eMI_OpenBrowser,
        eMI_Accept,
        eMI_Cancel,
        eMI_AcceptAll,
        eMI_CancelAll,
        eMI_AcceptSubstitution,
    };

    QMenu menu;

    const int recordCount = records.size();

    if (recordCount == 1)
    {
        menu.addAction(tr("Find manually"))->setData(eMI_OpenBrowser);
        menu.addSeparator();
        const QStringList& substitutions = records.front()->substitutions;
        if (substitutions.size() > 1)
        {
            int i = 0;
            for (QStringList::const_iterator it = substitutions.begin(); it != substitutions.end(); ++it)
            {
                menu.addAction(tr("Accept: %1").arg(*it))->setData(eMI_AcceptSubstitution + i++);
            }
            menu.addSeparator();
        }
        if (!substitutions.empty())
        {
            menu.addAction(tr("Accept new file"))->setData(eMI_Accept);
        }
        menu.addAction(tr("Cancel file"))->setData(eMI_Cancel);
        menu.addSeparator();
    }
    else if (recordCount > 1)
    {
        menu.addAction(tr("Accept all selected"))->setData(eMI_Accept);
        menu.addAction(tr("Cancel all selected"))->setData(eMI_Cancel);
        menu.addSeparator();
    }

    menu.addAction(tr("Accept all resolved files"))->setData(eMI_AcceptAll);
    menu.addAction(tr("Cancel all resolved files"))->setData(eMI_CancelAll);

    QAction* action = menu.exec(QCursor::pos());
    const int commandId = action ? action->data().toInt() : 0;

    switch (commandId)
    {
    case eMI_OpenBrowser:
        assert(recordCount == 1);
        ResolveRecord(records.front());
        break;
    case eMI_Accept:
        for (TRecords::iterator it = records.begin(); it != records.end(); ++it)
        {
            CMissingAssetRecord* pRecord = *it;
            if (!pRecord->substitutions.empty())
            {
                AcceptRecort(pRecord);
            }
        }
        break;
    case eMI_Cancel:
        for (TRecords::iterator it = records.begin(); it != records.end(); ++it)
        {
            CancelRecort(*it);
        }
        break;
    case eMI_AcceptAll:
        if (CMissingAssetReport* pReport = GetReport())
        {
            CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
            while (CMissingAssetRecord* pRecord = iter.Next())
            {
                if (!pRecord->substitutions.empty())
                {
                    AcceptRecort(pRecord);
                }
            }
        }
        break;
    case eMI_CancelAll:
        if (CMissingAssetReport* pReport = GetReport())
        {
            CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
            while (CMissingAssetRecord* pRecord = iter.Next())
            {
                CancelRecort(pRecord);
            }
        }
        break;
    default:
        if (commandId >= eMI_AcceptSubstitution)
        {
            int substitutionIdx = commandId - eMI_AcceptSubstitution;
            assert(recordCount == 1);
            AcceptRecort(records.front(), substitutionIdx);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnReportItemDblClick(const QModelIndex& index)
{
    ResolveRecord(index.data(Qt::UserRole).value<CMissingAssetRecord*>());
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::AcceptRecort(CMissingAssetRecord* pRecord, int idx)
{
    assert(pRecord->substitutions.size() > idx);
    GetIEditor()->GetMissingAssetResolver()->AcceptRequest(pRecord->id, pRecord->substitutions[idx].toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::CancelRecort(CMissingAssetRecord* pRecord)
{
    GetIEditor()->GetMissingAssetResolver()->CancelRequest(pRecord->id);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::ResolveRecord(CMissingAssetRecord* pRecord)
{
    IAssetSearcher* pSearcher = GetIEditor()->GetMissingAssetResolver()->GetAssetSearcherById(pRecord->searcherId);

    QString fullFileName;
    if (pSearcher && pSearcher->GetReplacement(fullFileName, pRecord->assetTypeId))
    {
        GetIEditor()->GetMissingAssetResolver()->AcceptRequest(pRecord->id, fullFileName.toUtf8().data());
    }
}

#include <AssetResolver/AssetResolverDialog.moc>
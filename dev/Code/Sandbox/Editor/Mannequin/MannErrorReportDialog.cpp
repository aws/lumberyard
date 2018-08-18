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
#include "MannErrorReportDialog.h"
#include "Clipboard.h"
#include "Util/CryMemFile.h"                    // CCryMemFile

#include "Util/Mailer.h"
#include "GameEngine.h"

#include "MannequinDialog.h"

#include "ErrorReportTableModel.h"

#include "IGameFramework.h"
#include "ICryMannequinEditor.h"

#include <Mannequin/ui_MannErrorReportDialog.h>

#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>
#include <QDateTime>

// CMannErrorReportDialog dialog

//////////////////////////////////////////////////////////////////////////
QString CMannErrorRecord::GetErrorText() const
{
    QString str = QString::fromLatin1("[%1]\t%2").arg(count, 2).arg(error.trimmed());

    if (!file.isEmpty())
    {
        str += QStringLiteral("\t") + file;
    }
    else
    {
        str += QStringLiteral("\t ");
    }
    return str;
}

class CMannErrorReportTableModel
    : public AbstractSortModel
{
public:
    enum Column
    {
        ColumnCheckbox,
        ColumnSeverity,
        ColumnFragment,
        ColumnTags,
        ColumnFragmentTo,
        ColumnTagsTo,
        ColumnType,
        ColumnText,
        ColumnFile
    };

    CMannErrorReportTableModel(QObject* parent = nullptr)
        : AbstractSortModel(parent)
        , m_contexts(CMannequinDialog::GetCurrentInstance()->Contexts())
    {
        m_alignments[ColumnFile] = Qt::AlignRight;
        m_imageList.push_back(QPixmap(":/error_report_00.png"));
        m_imageList.push_back(QPixmap(":/error_report_01.png"));
        m_imageList.push_back(QPixmap(":/error_report_02.png"));
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_errorRecords.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 9;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractTableModel::flags(index);
        if (index.column() == ColumnCheckbox)
        {
            f |= Qt::ItemIsUserCheckable;
        }
        return f;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        assert(index.isValid() && index.row() < m_errorRecords.size());
        if (role == Qt::CheckStateRole && index.column() == ColumnCheckbox)
        {
            return m_checkStates[index.row()] ? Qt::Checked : Qt::Unchecked;
        }
        const CMannErrorRecord& record = m_errorRecords[index.row()];
        return data(record, index.column(), role);
    }

    QVariant data(const CMannErrorRecord& record, int column, int role = Qt::DisplayRole) const
    {
        if (role == Qt::DisplayRole)
        {
            QString tagList;
            switch (column)
            {
            case ColumnFragment:
                return record.fragmentID == FRAGMENT_ID_INVALID ? (record.type == SMannequinErrorReport::Blend ? tr("Any") : tr("None")) : QString::fromLatin1(m_contexts->m_controllerDef->m_fragmentIDs.GetTagName(record.fragmentID));
            case ColumnTags:
                MannUtils::FlagsToTagList(tagList, record.tagState, record.fragmentID, *m_contexts->m_controllerDef, "");
                return tagList;
            case ColumnFragmentTo:
                return record.fragmentIDTo == FRAGMENT_ID_INVALID ? tr("Any") : QString::fromLatin1(m_contexts->m_controllerDef->m_fragmentIDs.GetTagName(record.fragmentIDTo));
            case ColumnTagsTo:
                MannUtils::FlagsToTagList(tagList, record.tagStateTo, record.fragmentIDTo, *m_contexts->m_controllerDef, "");
                return tagList;
            case ColumnType:
                return QStringList({ tr("None"), tr("Fragment"), tr("Transition") }).at(record.type);
            case ColumnText:
                return record.error;
            case ColumnFile:
                return record.file;
            default:
                return QString();
            }
        }
        else if (role == Qt::DecorationRole)
        {
            return m_imageList[record.severity];
        }
        else if (role == Qt::UserRole)
        {
            return QVariant::fromValue(const_cast<CMannErrorRecord*>(&record));
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return m_alignments.value(column, Qt::AlignLeft);
        }
        return QVariant();
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (index.column() == ColumnCheckbox && role == Qt::CheckStateRole)
        {
            bool ok;
            bool checked = value.toInt(&ok) == Qt::Checked;
            if (!ok)
            {
                return false;
            }
            m_checkStates[index.row()] = checked;
            emit dataChanged(index, index);
            return true;
        }
        return QAbstractTableModel::setData(index, value, role);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal)
        {
            return QVariant();
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return m_alignments.value(section, Qt::AlignLeft);
        }
        else if (role == Qt::DisplayRole)
        {
            switch (section)
            {
            case ColumnCheckbox:
                return tr("Sel");
            case ColumnSeverity:
                return tr("");
            case ColumnFragment:
                return tr("Frag");
            case ColumnTags:
                return tr("Tags");
            case ColumnFragmentTo:
                return tr("FragTo");
            case ColumnTagsTo:
                return tr("TagsTo");
            case ColumnType:
                return tr("Type");
            case ColumnText:
                return tr("Text");
            case ColumnFile:
                return tr("File");
            default:
                return QString();
            }
        }
        else
        {
            return QVariant();
        }
    }

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override
    {
        if (orientation == Qt::Horizontal && section >= 0 && section < columnCount() && value.canConvert<int>() && role == Qt::TextAlignmentRole)
        {
            m_alignments.insert(section, value.toInt());
            Q_EMIT headerDataChanged(orientation, section, section);
            const int rows = rowCount();
            if (rows > 0)
            {
                Q_EMIT dataChanged(index(0, section), index(rows - 1, section));
            }
            return true;
        }
        return QAbstractTableModel::setHeaderData(section, orientation, value, role);
    }


    void sort(int column, Qt::SortOrder order) override
    {
        layoutAboutToBeChanged();
        std::sort(m_errorRecords.begin(), m_errorRecords.end(), [&](const CMannErrorRecord& lhs, const CMannErrorRecord& rhs)
            {
                return lessThan(lhs, rhs, column);
            });
        if (order == Qt::DescendingOrder)
        {
            std::reverse(m_errorRecords.begin(), m_errorRecords.end());
        }
        layoutChanged();
    }

    bool lessThan(const CMannErrorRecord& lhs, const CMannErrorRecord& rhs, int column) const
    {
        if (column == ColumnSeverity)
        {
            return lhs.severity < rhs.severity;
        }
        const QVariant l = data(lhs, column);
        const QVariant r = data(rhs, column);
        bool ok;
        const int lInt = l.toInt(&ok);
        const int rInt = r.toInt(&ok);
        if (ok)
        {
            return lInt < rInt;
        }
        return l.toString() < r.toString();
    }

    void setErrorRecords(const std::vector<CMannErrorRecord>& records)
    {
        beginResetModel();
        m_errorRecords = records;
        m_checkStates.clear();
        m_checkStates.resize(m_errorRecords.size());
        endResetModel();
    }

private:
    SMannequinContexts* m_contexts;
    QVector<QPixmap> m_imageList;
    QHash<int, int> m_alignments;
    std::vector<CMannErrorRecord> m_errorRecords;
    std::vector<bool> m_checkStates;
};

CMannErrorReportDialog::CMannErrorReportDialog(QWidget* pParent)
    : QWidget(pParent)
    , m_contexts(NULL)
    , m_ui(new Ui::MannErrorReportDialog)
    , m_errorReportModel(new CMannErrorReportTableModel(this))
{
    OnInitDialog();
}

CMannErrorReportDialog::~CMannErrorReportDialog()
{
}

void CMannErrorReportDialog::AddError(const CMannErrorRecord& error)
{
    m_errorRecords.push_back(error);
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::Refresh()
{
    UpdateErrors();
}

//////////////////////////////////////////////////////////////////////////
//void CMannErrorReportDialog::Close()
//{
//  DestroyWindow();
//
//  if (m_instance)
//  {
//      /*
//      CCryMemFile memFile( new BYTE[256], 256, 256 );
//      CArchive ar( &memFile, CArchive::store );
//      m_instance->m_wndReport.SerializeState( ar );
//      ar.Close();
//
//      UINT nSize = (UINT)memFile.GetLength();
//      LPBYTE pbtData = memFile.Detach();
//      CXTRegistryManager regManager;
//      regManager.WriteProfileBinary( "Dialogs\\ErrorReport", "Configuration", pbtData, nSize);
//      if ( pbtData )
//          delete [] pbtData;
//*/
//      m_instance->DestroyWindow();
//  }
//}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::Clear()
{
    m_errorRecords.clear();
    UpdateErrors();
}



// CMannErrorReportDialog message handlers

void CMannErrorReportDialog::OnInitDialog()
{
    m_ui->setupUi(this);

    m_contexts = CMannequinDialog::GetCurrentInstance()->Contexts();

    m_ui->m_wndReport->setModel(m_errorReportModel);

    m_ui->m_wndReport->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->m_wndReport->header()->setSectionsMovable(true);
    m_ui->m_wndReport->viewport()->setMouseTracking(true);
    m_ui->m_wndReport->viewport()->installEventFilter(this);

    m_ui->m_wndReport->header()->setSectionResizeMode(CMannErrorReportTableModel::ColumnCheckbox, QHeaderView::ResizeToContents);
    m_ui->m_wndReport->header()->setSectionResizeMode(CMannErrorReportTableModel::ColumnSeverity, QHeaderView::ResizeToContents);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnFragment, 90);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnTags, 40);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnFragmentTo, 90);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnTagsTo, 40);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnType, 80);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnText, 300);
    m_ui->m_wndReport->header()->resizeSection(CMannErrorReportTableModel::ColumnFragment, 100);

    connect(m_ui->buttonSelectAll, &QPushButton::clicked, this, &CMannErrorReportDialog::OnSelectAll);
    connect(m_ui->buttonClearSelection, &QPushButton::clicked, this, &CMannErrorReportDialog::OnClearSelection);
    connect(m_ui->buttonCheckForErrors, &QPushButton::clicked, this, &CMannErrorReportDialog::OnCheckErrors);
    connect(m_ui->buttonDeleteFragments, &QPushButton::clicked, this, &CMannErrorReportDialog::OnDeleteFragments);

    connect(m_ui->m_wndReport, &QAbstractItemView::clicked, this, &CMannErrorReportDialog::OnReportItemClick);
    connect(m_ui->m_wndReport, &QAbstractItemView::doubleClicked, this, &CMannErrorReportDialog::OnReportItemDblClick);
    connect(m_ui->m_wndReport, &QWidget::customContextMenuRequested, this, &CMannErrorReportDialog::OnReportItemRClick);
    connect(m_ui->m_wndReport->header(), &QWidget::customContextMenuRequested, this, &CMannErrorReportDialog::OnReportColumnRClick);

    m_ui->m_wndReport->AddGroup(CMannErrorReportTableModel::ColumnFile);
}

void CMannErrorReportDialog::UpdateErrors()
{
    ReloadErrors();
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::ReloadErrors()
{
    m_errorReportModel->setErrorRecords(m_errorRecords);
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnSelectObjects()
{
    /*
    CUndo undo("Select Object(s)" );
    // Clear other selection.
    GetIEditor()->ClearSelection();
    POSITION pos = m_errors.GetFirstSelectedItemPosition();
    while (pos)
    {
        int nItem = m_errors.GetNextSelectedItem(pos);
        CErrorRecord *pError = (CErrorRecord*)m_errors.GetItemData( nItem );
        if (pError && pError->pObject)
        {
            // Select this object.
            GetIEditor()->SelectObject( pError->pObject );
        }
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnSelectAll()
{
    for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
    {
        m_errorReportModel->setData(m_errorReportModel->index(i, 0), Qt::CheckStateRole, Qt::Checked);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnClearSelection()
{
    for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
    {
        m_errorReportModel->setData(m_errorReportModel->index(i, 0), Qt::CheckStateRole, Qt::Unchecked);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnCheckErrors()
{
    CMannequinDialog::GetCurrentInstance()->Validate();
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnDeleteFragments()
{
    uint32 numToDelete = 0;
    for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
    {
        if (m_errorReportModel->index(i, 0).data(Qt::CheckStateRole).toInt() == Qt::Checked)
        {
            numToDelete++;
        }
    }

    if (numToDelete > 0)
    {
        const QString msg = tr("Delete all %1 fragments?").arg(numToDelete);
        int nCode = QMessageBox::critical(this, tr("Delete Fragments?"), msg, QMessageBox::Ok | QMessageBox::Cancel);

        if (nCode == QMessageBox::Ok)
        {
            std::vector<CMannErrorRecord*> recordsToDelete;

            for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
            {
                const QModelIndex index = m_errorReportModel->index(i, 0);
                if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                {
                    recordsToDelete.push_back(index.data(Qt::UserRole).value<CMannErrorRecord*>());
                }
            }

            // sort the records to delete in reverse order by optionindex to prevent these indices to
            // become invalid while deleting other options
            std::sort(recordsToDelete.begin(), recordsToDelete.end(), [](const CMannErrorRecord* p1, const CMannErrorRecord* p2)
                {
                    return p1->fragOptionID > p2->fragOptionID;
                });

            int numRecordsToProcess = recordsToDelete.size();

            for (uint32 i = 0; i < numRecordsToProcess; i++)
            {
                CMannErrorRecord& errorRecord = *recordsToDelete[i];
                const uint32 numScopeDatas = m_contexts->m_contextData.size();
                for (uint32 s = 0; s < numScopeDatas; s++)
                {
                    IAnimationDatabase* animDB = m_contexts->m_contextData[s].database;
                    if (animDB && (errorRecord.file == animDB->GetFilename()))
                    {
                        IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
                        mannequinSys.GetMannequinEditorManager()->DeleteFragmentEntry(animDB, errorRecord.fragmentID, errorRecord.tagState, errorRecord.fragOptionID);
                        break;
                    }
                }
            }

            CMannequinDialog::GetCurrentInstance()->Validate();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
#define ID_REMOVE_ITEM  1
#define ID_SORT_ASC     2
#define ID_SORT_DESC        3
#define ID_GROUP_BYTHIS 4
#define ID_SHOW_GROUPBOX        5
#define ID_SHOW_FIELDCHOOSER 6
#define ID_COLUMN_BESTFIT       7
#define ID_COLUMN_ARRANGEBY 100
#define ID_COLUMN_ALIGMENT  200
#define ID_COLUMN_ALIGMENT_LEFT ID_COLUMN_ALIGMENT + 1
#define ID_COLUMN_ALIGMENT_RIGHT    ID_COLUMN_ALIGMENT + 2
#define ID_COLUMN_ALIGMENT_CENTER   ID_COLUMN_ALIGMENT + 3
#define ID_COLUMN_SHOW      500

void CMannErrorReportDialog::OnReportColumnRClick()
{
    QHeaderView* header = m_ui->m_wndReport->header();

    int column = header->logicalIndexAt(m_ui->m_wndReport->mapFromGlobal(QCursor::pos()));
    if (column < 0)
    {
        return;
    }

    QMenu menu;
    QAction* actionSortAscending = menu.addAction(tr("Sort &Ascending"));
    QAction* actionSortDescending = menu.addAction(tr("Sort Des&cending"));
    menu.addSeparator();
    QAction* actionGroupByThis = menu.addAction(tr("&Group by this field"));
    QAction* actionGroupByBox = menu.addAction(tr("Group &by box"));
    menu.addSeparator();
    QAction* actionRemoveItem = menu.addAction(tr("&Remove column"));
    QAction* actionFieldChooser = menu.addAction(tr("Field &Chooser"));
    menu.addSeparator();
    QAction* actionBestFit = menu.addAction(tr("Best &Fit"));

    actionGroupByBox->setCheckable(true);
    actionGroupByBox->setChecked(m_ui->m_wndReport->IsGroupsShown());

    // create arrange by items
    QMenu menuArrange;
    const int nColumnCount = m_errorReportModel->columnCount();
    for (int nColumn = 0; nColumn < nColumnCount; nColumn++)
    {
        if (!m_ui->m_wndReport->header()->isSectionHidden(nColumn))
        {
            const QString sCaption = m_errorReportModel->headerData(nColumn, Qt::Horizontal).toString();
            if (!sCaption.isEmpty())
            {
                menuArrange.addAction(sCaption)->setData(nColumn);
            }
        }
    }

    menuArrange.addSeparator();

    QAction* actionClearGroups = menuArrange.addAction(tr("Clear groups"));
    menuArrange.setTitle(tr("Arrange By"));
    menu.insertMenu(actionSortAscending, &menuArrange);

    // create columns items
    QMenu menuColumns;
    for (int nColumn = 0; nColumn < nColumnCount; nColumn++)
    {
        const QString sCaption = m_errorReportModel->headerData(nColumn, Qt::Horizontal).toString();
        //if (!sCaption.isEmpty())
        QAction* action = menuColumns.addAction(sCaption);
        action->setCheckable(true);
        action->setChecked(!m_ui->m_wndReport->header()->isSectionHidden(nColumn));
    }

    menuColumns.setTitle(tr("Columns"));
    menu.insertMenu(menuArrange.menuAction(), &menuColumns);

    //create Text alignment submenu
    QMenu menuAlign;

    QAction* actionAlignLeft = menuAlign.addAction(tr("Align Left"));
    QAction* actionAlignRight = menuAlign.addAction(tr("Align Right"));
    QAction* actionAlignCenter = menuAlign.addAction(tr("Align Center"));
    actionAlignLeft->setCheckable(true);
    actionAlignRight->setCheckable(true);
    actionAlignCenter->setCheckable(true);

    int nAlignOption = 0;
    const int alignment = m_errorReportModel->headerData(column, Qt::Horizontal, Qt::TextAlignmentRole).toInt();
    actionAlignLeft->setChecked(alignment & Qt::AlignLeft);
    actionAlignRight->setChecked(alignment & Qt::AlignRight);
    actionAlignCenter->setChecked(alignment & Qt::AlignHCenter);

    menuAlign.setTitle(tr("&Alignment"));
    menu.insertMenu(actionBestFit, &menuAlign);

    // track menu
    QAction* nMenuResult = menu.exec(QCursor::pos());

    // arrange by items
    if (menuArrange.actions().contains(nMenuResult))
    {
        // group by item
        if (actionClearGroups == nMenuResult)
        {
            m_ui->m_wndReport->ClearGroups();
        }
        else
        {
            column = nMenuResult->data().toInt();
            m_ui->m_wndReport->ToggleSortOrder(column);
        }
    }

    // process Alignment options
    if (nMenuResult == actionAlignLeft)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignLeft, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignRight)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignRight, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignCenter)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    }

    // process column selection item
    if (menuColumns.actions().contains(nMenuResult))
    {
        m_ui->m_wndReport->header()->setSectionHidden(menuColumns.actions().indexOf(nMenuResult), !nMenuResult->isChecked());
    }


    // other general items
    if (nMenuResult == actionSortAscending || nMenuResult == actionSortDescending)
    {
        m_ui->m_wndReport->sortByColumn(column, nMenuResult == actionSortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
    else if (nMenuResult == actionFieldChooser)
    {
        //OnShowFieldChooser();
    }
    else if (nMenuResult == actionBestFit)
    {
        m_ui->m_wndReport->resizeColumnToContents(column);
    }
    else if (nMenuResult == actionRemoveItem)
    {
        m_ui->m_wndReport->header()->setSectionHidden(column, true);
    }
    // other general items
    else if (nMenuResult == actionGroupByThis)
    {
        m_ui->m_wndReport->AddGroup(column);
        m_ui->m_wndReport->ShowGroups(true);
    }
    else if (nMenuResult == actionGroupByBox)
    {
        m_ui->m_wndReport->ShowGroups(!m_ui->m_wndReport->IsGroupsShown());
    }
}

#define ID_POPUP_COLLAPSEALLGROUPS 1
#define ID_POPUP_EXPANDALLGROUPS 2


//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::CopyToClipboard()
{
    QString str;
    const QModelIndexList selRows = m_ui->m_wndReport->selectionModel()->selectedRows();
    for (const QModelIndex& index : selRows)
    {
        const CMannErrorRecord* pRecord = index.data(Qt::UserRole).value<CMannErrorRecord*>();
        if (pRecord)
        {
            str += pRecord->GetErrorText();
            //if(pRecord->m_pRecord->pObject)
            //  str+=" [Object: " + pRecord->m_pRecord->pObject->GetName() + "]";
            //if(pRecord->m_pRecord->pItem)
            //  str+=" [Material: " + pRecord->m_pRecord->pItem->GetName() + "]";
            str += QStringLiteral("\r\n");
        }
    }
    if (!str.isEmpty())
    {
        QApplication::clipboard()->setText(str);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnReportItemRClick()
{
    const QModelIndex index = m_ui->m_wndReport->indexAt(m_ui->m_wndReport->viewport()->mapFromGlobal(QCursor::pos()));

    if (!index.isValid())
    {
        return;
    }

    if (m_ui->m_wndReport->model()->hasChildren(index))
    {
        QMenu menu;
        menu.addAction(tr("Collapse &All Groups"), m_ui->m_wndReport, SLOT(collapseAll()));
        menu.addAction(tr("E&xpand All Groups"), m_ui->m_wndReport, SLOT(expandAll()));

        // track menu
        menu.exec(QCursor::pos());
    }
    else
    {
        QMenu menu;
        menu.addAction(tr("Select Object(s)")); // TODO: does nothing?
        menu.addAction(tr("Copy Warning(s) To Clipboard"), this, SLOT(CopyToClipboard()));
        menu.addAction(tr("E-mail Error Report"), this, SLOT(SendInMail()));
        menu.addAction(tr("Open in Excel"), this, SLOT(OpenInExcel()));

        // track menu
        menu.exec(QCursor::pos());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::SendInMail()
{
    //if (!m_pErrorReport)
    //  return;

    // Send E-Mail.
    QString textMsg;
    for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
    {
        const CMannErrorRecord* err = m_errorReportModel->index(i, 0).data(Qt::UserRole).value<CMannErrorRecord*>();
        textMsg += err->GetErrorText() + QString::fromLatin1("\n");
    }

    std::vector<const char*> who;
    const QString subject = QString::fromLatin1("Level %1 Error Report").arg(GetIEditor()->GetGameEngine()->GetLevelName());
    CMailer::SendMail(subject.toUtf8().data(), textMsg.toUtf8().data(), who, who, true);
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OpenInExcel()
{
    //if (!m_pErrorReport)
    //  return;

    const QString levelName = Path::GetFileName(GetIEditor()->GetGameEngine()->GetLevelName());

    const QString filename = QString::fromLatin1("ErrorList_%1_%2.csv").arg(levelName).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));

    // Save to temp file.
    QFile f(filename);
    if (f.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
        {
            const CMannErrorRecord* err = m_errorReportModel->index(i, 0).data(Qt::UserRole).value<CMannErrorRecord*>();
            QString text = err->GetErrorText();
            text.replace(',', '.');
            text.replace('\t', ',');
            f.write((text + QString::fromLatin1("\n")).toUtf8().data());
        }
        f.close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
    }
    else
    {
        Warning("Failed to save %s", (const char*)filename.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnReportItemClick(const QModelIndex& index)
{
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space)
    {
        const QModelIndexList selection = m_ui->m_wndReport->selectionModel()->selectedRows();
        bool setState           = false;
        bool queriedState = false;
        foreach(const QModelIndex &index, selection)
        {
            const QModelIndex checkIndex = index.sibling(index.row(), CMannErrorReportTableModel::ColumnCheckbox);
            const Qt::CheckState check = static_cast<Qt::CheckState>(checkIndex.data(Qt::CheckStateRole).toInt());
            m_ui->m_wndReport->model()->setData(checkIndex, check == Qt::Checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
        }
    }

    if (event->key() == Qt::Key_Return)
    {
        /*
        CMannErrorMessageRecord* pRecord = DYNAMIC_DOWNCAST(CMannErrorMessageRecord,m_wndReport.GetFocusedRow()->GetRecord());
        if (pRecord)
        {
            if (pRecord->SetRead())
            {
                m_wndReport.Populate();
            }
        }
        */
    }

    if (event->matches(QKeySequence::Copy))
    {
        CopyToClipboard();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnReportItemDblClick(const QModelIndex& index)
{
    CMannErrorRecord* pError = index.data(Qt::UserRole).value<CMannErrorRecord*>();
    if (pError)
    {
        //if (pError->pObject != NULL)
        //{
        //  CUndo undo("Select Object(s)" );
        //  // Clear other selection.
        //  GetIEditor()->ClearSelection();
        //  // Select this object.
        //  GetIEditor()->SelectObject( pError->pObject );

        //  CViewport *vp = GetIEditor()->GetActiveView();
        //  if (vp)
        //      vp->CenterOnSelection();
        //  bDone = true;
        //}
        //if (pError->pItem != NULL)
        //{
        //  GetIEditor()->OpenDataBaseLibrary( pError->pItem->GetType(),pError->pItem );
        //  bDone = true;
        //}

        if (pError->fragmentID != FRAGMENT_ID_INVALID)
        {
            CMannequinDialog::GetCurrentInstance()->EditFragment(pError->fragmentID, pError->tagState, pError->contextDef->contextID);
        }

        //if(!bDone && GetIEditor()->GetActiveView())
        //{
        //  float x, y, z;
        //  if(GetPositionFromString(pError->error, &x, &y, &z))
        //  {
        //      CViewport *vp = GetIEditor()->GetActiveView();
        //      Matrix34 tm = vp->GetViewTM();
        //      tm.SetTranslation(Vec3(x, y, z));
        //      vp->SetViewTM(tm);
        //  }
        //}
    }
}

/*
//////////////////////////////////////////////////////////////////////////
void CMannErrorReportDialog::OnShowFieldChooser()
{
    CMainFrm* pMainFrm = (CMainFrame*)AfxGetMainWnd();
    if (pMainFrm)
    {
        BOOL bShow = !pMainFrm->m_wndFieldChooser.IsVisible();
        pMainFrm->ShowControlBar(&pMainFrm->m_wndFieldChooser, bShow, FALSE);
    }
}
*/

#include <Mannequin/MannErrorReportDialog.moc>
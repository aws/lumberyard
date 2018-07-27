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
#include "MannFileManager.h"

#include "Util/Mailer.h"
#include "GameEngine.h"

#include "MannequinDialog.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include "IGameFramework.h"
#include "Helper/MannequinFileChangeWriter.h"
#include "Util/AbstractSortModel.h"
#include "QtUtil.h"
#include "QtUtilWin.h"
#include <QMessageBox>
#include <QMenu>
#include <QItemSelectionModel>

#include <ISourceControl.h>


//////////////////////////////////////////////////////////////////////////
enum EFileReportColumn
{
    FILECOLUMN_STATUS,
    FILECOLUMN_FILE,
    FILECOLUMN_TYPE,
};

enum EFileStatusIcon
{
    STATUSICON_OK,
    STATUSICON_READONLY,
    STATUSICON_CHECKEDOUT,
    STATUSICON_PAK
};

class CXTPMannFileRecord
{
public:
    CXTPMannFileRecord(const SFileEntry& fileEntry)
        : m_fileName(fileEntry.filename)
        , m_fileType(fileEntry.typeDesc)
        , m_bCheckedOut(false)
        , m_bInPak(false)
        , m_bReadOnly(false)
    {
        m_fullFileName = Path::GamePathToFullPath(fileEntry.filename);
        UpdateState();
    }

    bool CheckedOut() const { return m_bCheckedOut; }
    bool InPak() const { return m_bInPak; }
    bool ReadOnly() const { return m_bReadOnly; }
    const QString fileName() const { return m_fileName; }
    const QString fileType() const { return m_fileType; }

    int IconIndex() const { return m_iconIndex; }
    QString IconTooltip() const { return m_iconTooltip; }


    void UpdateState()
    {
        int nIcon = STATUSICON_OK;
        uint32 attr = CFileUtil::GetAttributes(m_fullFileName.toUtf8().data());

        m_bCheckedOut = false;
        m_bInPak = false;
        m_bReadOnly = false;

        // Checked out?
        if ((attr & SCC_FILE_ATTRIBUTE_CHECKEDOUT))
        {
            m_bCheckedOut = true;
            nIcon = STATUSICON_CHECKEDOUT;
        }
        // In a pak?
        else if (attr & SCC_FILE_ATTRIBUTE_INPAK)
        {
            m_bInPak = true;
            nIcon = STATUSICON_PAK;
        }
        // Read-only?
        else if (attr & SCC_FILE_ATTRIBUTE_READONLY)
        {
            m_bReadOnly = true;
            nIcon = STATUSICON_READONLY;
        }

        UpdateIcon(nIcon);
    }

    void CheckOut()
    {
        if (m_bCheckedOut || m_bInPak)
        {
            return;
        }

        if (CFileUtil::CheckoutFile(m_fullFileName.toUtf8().data()))
        {
            m_bCheckedOut = true;
            m_bReadOnly = false;
            UpdateIcon(STATUSICON_CHECKEDOUT);
        }
        else
        {
            QString message = QObject::tr("Failed to checkout file \"%1\"").arg(m_fileName);
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Checkout Failed"), message);
        }
    }

private:
    void UpdateIcon(int nIcon)
    {
        m_iconIndex = nIcon;

        switch (nIcon)
        {
        case STATUSICON_OK:
            m_iconTooltip = QObject::tr("Writeable");
            break;
        case STATUSICON_READONLY:
            m_iconTooltip = QObject::tr("Read-Only");
            break;
        case STATUSICON_CHECKEDOUT:
            m_iconTooltip = QObject::tr("Checked Out");
            break;
        case STATUSICON_PAK:
            m_iconTooltip = QObject::tr("In PAK File");
            break;
        }
    }

    QString m_fileName;
    QString m_fileType;
    QString m_fullFileName;
    bool m_bCheckedOut;
    bool m_bInPak;
    bool m_bReadOnly;
    int m_iconIndex;
    QString m_iconTooltip;
};

//////////////////////////////////////////////////////////////////////////
Q_DECLARE_METATYPE(CXTPMannFileRecord*);

class CMannFileManagerTableModel
    : public AbstractSortModel
{
public:
    CMannFileManagerTableModel(QObject* parent = nullptr)
        : AbstractSortModel(parent)
    {
        m_imageList.push_back(QPixmap(":/FragmentBrowser/MannFileManagerImageList_00.png"));
        m_imageList.push_back(QPixmap(":/FragmentBrowser/MannFileManagerImageList_01.png"));
        m_imageList.push_back(QPixmap(":/FragmentBrowser/MannFileManagerImageList_02.png"));
        m_imageList.push_back(QPixmap(":/FragmentBrowser/MannFileManagerImageList_03.png"));
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_fileRecords.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 3;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        assert(index.isValid() && index.row() < m_fileRecords.size());
        const CXTPMannFileRecord& record = m_fileRecords[index.row()];
        return data(record, index.column(), role);
    }

    QVariant data(const CXTPMannFileRecord& record, int column, int role = Qt::DisplayRole) const
    {
        if (role == Qt::DisplayRole)
        {
            switch (column)
            {
            case FILECOLUMN_FILE:
                return record.fileName();
            case FILECOLUMN_TYPE:
                return record.fileType();
            default:
                return QString();
            }
        }
        else if (role == Qt::DecorationRole && column == FILECOLUMN_STATUS)
        {
            return m_imageList[record.IconIndex()];
        }
        else if (role == Qt::ToolTipRole && column == FILECOLUMN_STATUS)
        {
            return record.IconTooltip();
        }
        else if (role == Qt::UserRole)
        {
            return QVariant::fromValue(const_cast<CXTPMannFileRecord*>(&record));
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return m_alignments.value(column, Qt::AlignLeft);
        }
        return QVariant();
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
            case FILECOLUMN_STATUS:
                return QString();
            case FILECOLUMN_FILE:
                return tr("File");
            case FILECOLUMN_TYPE:
                return tr("Type");
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
            emit headerDataChanged(orientation, section, section);
            const int rows = rowCount();
            if (rows > 0)
            {
                emit dataChanged(index(0, section), index(rows - 1, section));
            }
            return true;
        }
        return QAbstractTableModel::setHeaderData(section, orientation, value, role);
    }


    void sort(int column, Qt::SortOrder order) override
    {
        layoutAboutToBeChanged();
        std::sort(m_fileRecords.begin(), m_fileRecords.end(), [&](const CXTPMannFileRecord& lhs, const CXTPMannFileRecord& rhs)
            {
                return lessThan(lhs, rhs, column);
            });
        if (order == Qt::DescendingOrder)
        {
            std::reverse(m_fileRecords.begin(), m_fileRecords.end());
        }
        layoutChanged();
    }

    bool lessThan(const CXTPMannFileRecord& lhs, const CXTPMannFileRecord& rhs, int column) const
    {
        if (column == FILECOLUMN_STATUS)
        {
            return lhs.IconIndex() < lhs.IconIndex();
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

    void UpdateState(int row)
    {
        assert(row < m_fileRecords.size());
        CXTPMannFileRecord& record = m_fileRecords[row];
        record.UpdateState();
        emit dataChanged(index(row, 0), index(row, columnCount() - 1));
    }

    void SetFileRecords(const std::vector<CXTPMannFileRecord>& records)
    {
        beginResetModel();
        m_fileRecords = records;
        endResetModel();
    }

    std::vector<CXTPMannFileRecord> fileRecords() const
    {
        return m_fileRecords;
    }

private:
    SMannequinContexts* m_contexts;
    QVector<QPixmap> m_imageList;
    QHash<int, int> m_alignments;
    std::vector<CXTPMannFileRecord> m_fileRecords;
};



//////////////////////////////////////////////////////////////////////////
CMannFileManager::CMannFileManager(CMannequinFileChangeWriter& fileChangeWriter, bool bChangedFilesMode, QWidget* pParent)
    : QDialog(pParent)
    , m_fileManagerModel(new CMannFileManagerTableModel(this))
    , m_fileChangeWriter(fileChangeWriter)
    , m_bInChangedFileMode(bChangedFilesMode)
    , ui(new Ui::MannFileManager)

{
    m_bSourceControlAvailable = GetIEditor()->IsSourceControlAvailable();

    ui->setupUi(this);
    if (bChangedFilesMode)
    {
        ui->UNDO->hide();
        ui->buttonBox->hide();
    }
    else
    {
        ui->SAVE->hide();
        ui->RELOAD_ALL->hide();
    }

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnInitDialog()
{
    //m_pShowCurrentPreviewFilesOnlyCheckbox.reset( new CInPlaceButton( functor( *this, &CMannFileManager::OnDisplayOnlyCurrentPreviewClicked ) ) );

    //CRect rcCheckbox = rc;
    //rcCheckbox.left = 5;
    //rcCheckbox.top = rcCheckbox.bottom - 20;
    //rcCheckbox.bottom -= 5;
    //rcCheckbox.right = 300;
    //m_pShowCurrentPreviewFilesOnlyCheckbox->Create( "Save only current preview files", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rcCheckbox, this, ID_DISPLAY_ONLY_PREVIEW_FILES );
    //m_pShowCurrentPreviewFilesOnlyCheckbox->SetChecked( m_fileChangeWriter.GetFilterFilesByControllerDef() );

    ui->m_wndReport->setModel(m_fileManagerModel);
    ui->m_wndReport->AddGroup(FILECOLUMN_FILE);
    ui->m_wndReport->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->m_wndReport->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->m_wndReport->header()->resizeSection(FILECOLUMN_FILE, 350);

    connect(ui->m_wndReport, &QAbstractItemView::clicked, this, &CMannFileManager::OnReportItemClick);
    connect(ui->m_wndReport, &QAbstractItemView::doubleClicked, this, &CMannFileManager::OnReportItemDblClick);
    connect(ui->m_wndReport, &QWidget::customContextMenuRequested, this, &CMannFileManager::OnReportItemRClick);
    connect(ui->m_wndReport->header(), &QWidget::customContextMenuRequested, this, &CMannFileManager::OnReportColumnRClick);
    connect(ui->m_wndReport->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannFileManager::OnReportSelChanged);

    connect(ui->CHECK_OUT, &QPushButton::clicked, this, &CMannFileManager::OnCheckOutSelection);
    connect(ui->UNDO, &QPushButton::clicked, this, &CMannFileManager::OnUndoSelection);
    connect(ui->REFRESH, &QPushButton::clicked, this, &CMannFileManager::OnRefresh);
    connect(ui->RELOAD_ALL, &QPushButton::clicked, this, &CMannFileManager::OnReloadAllFiles);
    connect(ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CMannFileManager::OnSaveFiles);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CMannFileManager::reject);

    if (!m_bSourceControlAvailable)
    {
        ui->CHECK_OUT->hide();
    }
}

void CMannFileManager::showEvent(QShowEvent* event)
{
    UpdateFileRecords();
    if (m_fileManagerModel->rowCount() != 0 && !ui->m_wndReport->selectionModel()->hasSelection())
    {
        QModelIndex ndx = m_fileManagerModel->index(0, 0);
        ndx = ui->m_wndReport->mapFromSource(ndx);
        ui->m_wndReport->selectionModel()->select(ndx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::SetButtonStates()
{
    bool bHasSelection = false;
    bool bPartlyReadOnly = false;
    bool bSelectionPartlyInPak = false;

    QSet<CXTPMannFileRecord*> selectedRecords;
    foreach(QModelIndex idx, ui->m_wndReport->selectionModel()->selectedRows()) {
        selectedRecords.insert(idx.data(Qt::UserRole).value<CXTPMannFileRecord*>());
    }

    for (int i = 0; i < m_fileManagerModel->rowCount(); i++)
    {
        const QModelIndex idx = m_fileManagerModel->index(i, FILECOLUMN_FILE);
        CXTPMannFileRecord* pRecord = m_fileManagerModel->data(idx, Qt::UserRole).value<CXTPMannFileRecord*>();
        if (pRecord)
        {
            if (selectedRecords.contains(pRecord))
            {
                bHasSelection = true;
            }

            if (pRecord->ReadOnly())
            {
                bPartlyReadOnly = true;
            }

            if (selectedRecords.contains(pRecord) && pRecord->InPak())
            {
                bSelectionPartlyInPak = true;
            }
        }
    }

    ui->UNDO->setEnabled(bHasSelection);
    ui->CHECK_OUT->setEnabled(bHasSelection && !bSelectionPartlyInPak && m_bSourceControlAvailable);
    ui->SAVE->setEnabled(!bPartlyReadOnly);
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::UpdateFileRecords()
{
    ReloadFileRecords();
    SetButtonStates();
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::ReloadFileRecords()
{
    const size_t modifiedFilesCount = m_fileChangeWriter.GetModifiedFilesCount();
    std::vector<CXTPMannFileRecord> records;
    for (size_t i = 0; i < modifiedFilesCount; ++i)
    {
        const SFileEntry& fileEntry = m_fileChangeWriter.GetModifiedFileEntry(i);

        records.push_back(CXTPMannFileRecord(fileEntry));
    }
    m_fileManagerModel->SetFileRecords(records);
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnCheckOutSelection()
{
    if (!m_bSourceControlAvailable)
    {
        return;
    }

    const QModelIndexList selRows = ui->m_wndReport->selectionModel()->selectedRows();
    foreach (QModelIndex index, selRows)
    {
        CXTPMannFileRecord* pRecord = index.data(Qt::UserRole).value<CXTPMannFileRecord*>();
        if (pRecord && !pRecord->CheckedOut() && !pRecord->InPak())
        {
            pRecord->CheckOut();
            m_fileManagerModel->UpdateState(index.row());
        }
    }

    SetButtonStates();
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnUndoSelection()
{
    std::vector<QString> undoSelection;
    const QModelIndexList selRows = ui->m_wndReport->selectionModel()->selectedRows();
    foreach(QModelIndex index, selRows)
    {
        CXTPMannFileRecord* pRecord = index.data(Qt::UserRole).value<CXTPMannFileRecord*>();
        if (pRecord)
        {
            undoSelection.push_back(pRecord->fileName());
        }
    }

    const uint32 numDels = undoSelection.size();
    for (int i = 0; i < numDels; i++)
    {
        m_fileChangeWriter.UndoModifiedFile(undoSelection[i].toLocal8Bit().data());
    }

    UpdateFileRecords();

    if (m_fileChangeWriter.GetModifiedFilesCount() == 0)
    {
        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReloadAllFiles()
{
    IAnimationDatabaseManager& manager = gEnv->pGame->GetIGameFramework()->GetMannequinInterface().GetAnimationDatabaseManager();
    manager.ReloadAll();

    accept();
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnRefresh()
{
    UpdateFileRecords();

    if (m_fileChangeWriter.GetModifiedFilesCount() == 0)
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnSaveFiles()
{
    QString filesInPaks;
    const QModelIndexList selRows = ui->m_wndReport->selectionModel()->selectedRows();
    foreach(QModelIndex index, selRows)
    {
        CXTPMannFileRecord* pRecord = index.data(Qt::UserRole).value<CXTPMannFileRecord*>();
        if (pRecord && pRecord->InPak())
        {
            filesInPaks += pRecord->fileName() + "\n";
        }
    }

    if (filesInPaks.length() > 0)
    {
        QString message = tr("The following files are being edited directly from the PAK files\n"
                "and will need to be manually merged in Perforce:\n\n%1\n"
                "Would you like to continue?").arg(filesInPaks);
        if (QMessageBox::question(this, tr("Manual Merge Required"), message) != QMessageBox::Yes)
        {
            return;
        }
    }

    m_fileChangeWriter.WriteModifiedFiles();

    accept();
}

//////////////////////////////////////////////////////////////////////////

void CMannFileManager::OnReportColumnRClick()
{
    QHeaderView* header = ui->m_wndReport->header();

    int column = header->logicalIndexAt(ui->m_wndReport->mapFromGlobal(QCursor::pos()));
    if (column < 0)
    {
        return;
    }

    QMenu menu;

    // create main menu items
    QAction* actionSortAscending = menu.addAction(tr("Sort &Ascending"));
    QAction* actionSortDescending = menu.addAction(tr("Sort Des&cending"));
    menu.addSeparator();
    QAction* actionGroupByThis = menu.addAction(tr("&Group by this field"));
    QAction* actionGroupByBox = menu.addAction(tr("Group &by box"));
    menu.addSeparator();
    QAction* actionRemoveItem = menu.addAction(tr("&Remove column"));
    actionRemoveItem->setVisible(1 < ui->m_wndReport->header()->count());

    QAction* actionFieldChooser = menu.addAction(tr("Field &Chooser"));
    menu.addSeparator();
    QAction* actionBestFit = menu.addAction(tr("Best &Fit"));

    actionGroupByBox->setCheckable(true);
    actionGroupByBox->setChecked(ui->m_wndReport->IsGroupsShown());

    // create arrange by items
    QMenu menuArrange;
    const int nColumnCount = m_fileManagerModel->columnCount();
    for (int nColumn = 0; nColumn < nColumnCount; nColumn++)
    {
        if (!ui->m_wndReport->header()->isSectionHidden(nColumn))
        {
            const QString sCaption = m_fileManagerModel->headerData(nColumn, Qt::Horizontal).toString();
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
        const QString sCaption = m_fileManagerModel->headerData(nColumn, Qt::Horizontal).toString();
        //if (!sCaption.isEmpty())
        QAction* action = menuColumns.addAction(sCaption);
        action->setCheckable(true);
        action->setChecked(!ui->m_wndReport->header()->isSectionHidden(nColumn));
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
    const int alignment = m_fileManagerModel->headerData(column, Qt::Horizontal, Qt::TextAlignmentRole).toInt();
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
            ui->m_wndReport->ClearGroups();
        }
        else
        {
            column = nMenuResult->data().toInt();
            ui->m_wndReport->ToggleSortOrder(column);
        }
    }

    // process Alignment options
    if (nMenuResult == actionAlignLeft)
    {
        m_fileManagerModel->setHeaderData(column, Qt::Horizontal, Qt::AlignLeft, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignRight)
    {
        m_fileManagerModel->setHeaderData(column, Qt::Horizontal, Qt::AlignRight, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignCenter)
    {
        m_fileManagerModel->setHeaderData(column, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    }


    // process column selection item
    if (menuColumns.actions().contains(nMenuResult))
    {
        ui->m_wndReport->header()->setSectionHidden(menuColumns.actions().indexOf(nMenuResult), !nMenuResult->isChecked());
    }


    // other general items
    if (nMenuResult == actionRemoveItem)
    {
        ui->m_wndReport->header()->setSectionHidden(column, true);
    }
    if (nMenuResult == actionSortAscending || nMenuResult == actionSortDescending)
    {
        ui->m_wndReport->sortByColumn(column, nMenuResult == actionSortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
    else if (nMenuResult == actionGroupByThis)
    {
        ui->m_wndReport->AddGroup(column);
        ui->m_wndReport->ShowGroups(true);
    }
    else if (nMenuResult == actionGroupByBox)
    {
        ui->m_wndReport->ShowGroups(!ui->m_wndReport->IsGroupsShown());
    }
    else if (nMenuResult == actionFieldChooser)
    {
        //OnShowFieldChooser();
    }
    else if (nMenuResult == actionBestFit)
    {
        ui->m_wndReport->resizeColumnToContents(column);
    }
}

#define ID_POPUP_COLLAPSEALLGROUPS 1
#define ID_POPUP_EXPANDALLGROUPS 2


//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReportItemRClick()
{
    const QModelIndex index = ui->m_wndReport->indexAt(ui->m_wndReport->viewport()->mapFromGlobal(QCursor::pos()));

    if (!index.isValid())
    {
        return;
    }

    if (ui->m_wndReport->model()->hasChildren(index))
    {
        QMenu menu;
        menu.addAction(tr("Collapse &All Groups"), ui->m_wndReport, SLOT(collapseAll()));
        menu.addAction(tr("E&xpand All Groups"), ui->m_wndReport, SLOT(expandAll()));

        // track menu
        menu.exec(QCursor::pos());
    }
    /*  else if (pItemNotify->pItem)
        {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, 0,_T("Select Object(s)") );
        menu.AppendMenu(MF_STRING, 1,_T("Copy Warning(s) To Clipboard") );
        menu.AppendMenu(MF_STRING, 2,_T("E-mail Error Report") );
        menu.AppendMenu(MF_STRING, 3,_T("Open in Excel") );

        // track menu
        int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pItemNotify->pt.x, pItemNotify->pt.y, this, NULL);
        switch (nMenuResult)
        {
        case 1:
        CopyToClipboard();
        break;
        case 2:
        SendInMail();
        break;
        case 3:
        OpenInExcel();
        break;
        }
        }*/
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReportItemClick(const QModelIndex& index)
{
    QRect rect = ui->m_wndReport->visualRect(index);
    rect.moveTopLeft(ui->m_wndReport->viewport()->mapToGlobal(rect.topLeft()));
    const QRect target = fontMetrics().boundingRect(rect, index.data(Qt::TextAlignmentRole).toInt(), index.data().toString());
    OnReportHyperlink(index);

    SetButtonStates();

    /*
    if (pItemNotify->pColumn->GetItemIndex() == COLUMN_CHECK)
    {
        m_wndReport.Populate();
    }
    */
}


//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReportItemDblClick(const QModelIndex& index)
{
    CXTPMannFileRecord* pRecord = index.data(Qt::UserRole).value<CXTPMannFileRecord*>();
    if (pRecord)
    {
        bool bDone = false;
        //CMannFileRecord *pFileRecord = pRecord->m_pRecord;
        //if (pFileRecord)
        //{
        //if (pFileRecord->pObject != NULL)
        //{
        //  CUndo undo("Select Object(s)" );
        //  // Clear other selection.
        //  GetIEditor()->ClearSelection();
        //  // Select this object.
        //  GetIEditor()->SelectObject( pFileRecord->pObject );

        //  CViewport *vp = GetIEditor()->GetActiveView();
        //  if (vp)
        //      vp->CenterOnSelection();
        //  bDone = true;
        //}
        //if (pFileRecord->pItem != NULL)
        //{
        //  GetIEditor()->OpenDataBaseLibrary( pFileRecord->pItem->GetType(),pFileRecord->pItem );
        //  bDone = true;
        //}

        //if (pFileRecord->fragmentID != FRAGMENT_ID_INVALID)
        //{
        //  m_contexts.m_pAnimContext->mannequinDialog->EditFragment(pFileRecord->fragmentID, pFileRecord->tagState, pFileRecord->contextDef->contextID);
        //}

        //if(!bDone && GetIEditor()->GetActiveView())
        //{
        //  float x, y, z;
        //  if(GetPositionFromString(pFileRecord->error, &x, &y, &z))
        //  {
        //      CViewport *vp = GetIEditor()->GetActiveView();
        //      Matrix34 tm = vp->GetViewTM();
        //      tm.SetTranslation(Vec3(x, y, z));
        //      vp->SetViewTM(tm);
        //  }
        //}
        //}
    }

    /*
    XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;

    if (pItemNotify->pRow)
    {
        TRACE(_T("Double Click on row %d\n"),
    pItemNotify->pRow->GetIndex());

    CXTPMannFileRecord* pRecord = DYNAMIC_DOWNCAST(CXTPMannFileRecord, pItemNotify->pRow->GetRecord());
    if (pRecord)
    {
            {
                m_wndReport.Populate();
    }
    }
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReportSelChanged()
{
    SetButtonStates();
}

//////////////////////////////////////////////////////////////////////////
void CMannFileManager::OnReportHyperlink(const QModelIndex& index)
{
    CXTPMannFileRecord* pRecord = index.data(Qt::UserRole).value<CXTPMannFileRecord*>();
    if (pRecord)
    {
        bool bDone = false;
        //CMannFileRecord *pFileRecord = pRecord->m_pRecord;
        //if (pFileRecord && pFileRecord->pObject != NULL)
        //{
        //  CUndo undo("Select Object(s)" );
        //  // Clear other selection.
        //  GetIEditor()->ClearSelection();
        //  // Select this object.
        //  GetIEditor()->SelectObject( pFileRecord->pObject );
        //  bDone = true;
        //}
        //if (pFileRecord && pFileRecord->pItem != NULL)
        //{
        //  GetIEditor()->OpenDataBaseLibrary( pFileRecord->pItem->GetType(),pFileRecord->pItem );
        //  bDone = true;
        //}

        //if(!bDone && pFileRecord && GetIEditor()->GetActiveView())
        //{
        //  float x, y, z;
        //  if(GetPositionFromString(pFileRecord->error, &x, &y, &z))
        //  {
        //      CViewport *vp = GetIEditor()->GetActiveView();
        //      Matrix34 tm = vp->GetViewTM();
        //      tm.SetTranslation(Vec3(x, y, z));
        //      vp->SetViewTM(tm);
        //  }
        //}
    }
}

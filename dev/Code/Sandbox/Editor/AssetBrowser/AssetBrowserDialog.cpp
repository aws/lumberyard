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
#include "AssetBrowserDialog.h"
#include "AssetBrowserCachingOptionsDlg.h"
#include "AssetTypes/Model/AssetModelItem.h"
#include "AssetTypes/Character/AssetCharacterItem.h"
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetItem.h"
#include "AssetBrowser/AssetBrowserMetaTaggingDlg.h"
#include "Objects/EntityObject.h"
#include "AssetBrowserManager.h"
#include "Util/IndexedFiles.h"
#include "Objects/BrushObject.h"
#include "ViewManager.h"
#include "AssetBrowserFiltersDlg.h"
#include "GeneralAssetDbFilterDlg.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include "MainWindow.h"

#include <AssetBrowser/ui_AssetBrowserDialog.h>

#include "QtUtilWin.h"

#include <QtViewPaneManager.h>

#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QToolBar>

#define ASSET_BROWSER_BYTES_TO_MBYTES(nBytes) ((float)nBytes / 1024.0f / 1024.0f)

namespace AssetBrowser
{
    const QColor kListViewOddRowColor = QColor(235, 235, 235);
    const QColor kListViewSelectedRowColor = QColor(215, 215, 0);
    const UINT kSearchEditDialogHeight = 33;
    const UINT kReportCtrl_CacheTimerDelay = 50;
    const char* kNoPresetText = "(No preset)";
    const UINT kOneMegaByte = 1048576;
    const UINT kOneKiloByte = 1024;
};

CAssetBrowserModel::CAssetBrowserModel(CAssetViewer* assetViewer, QObject* parent)
    : QAbstractTableModel(parent)
    , m_pAssetBrowserDlg(nullptr)
    , m_assetViewer(assetViewer)
{
    m_assetViewer->RegisterObserver(this);

    TAssetDatabases visibleAssetDatabases = m_assetViewer->GetDatabases();

    if (visibleAssetDatabases.empty())
    {
        return;
    }

    //
    // merge all fields from all the selected databases
    //
    m_allFields.clear();
    std::set<QString> insertedFieldnames;

    for (size_t i = 1, iCount = visibleAssetDatabases.size(); i < iCount; ++i)
    {
        IAssetItemDatabase::TAssetFields& fields = visibleAssetDatabases[i]->GetAssetFields();

        for (auto iter = fields.begin(), iterEnd = fields.end(); iter != iterEnd; ++iter)
        {
            if (insertedFieldnames.end() == insertedFieldnames.find(iter->m_fieldName))
            {
                m_allFields.push_back(*iter);
                insertedFieldnames.insert(iter->m_fieldName);
            }
        }
    }
}

int CAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
    return (parent.isValid() || m_assetViewer == nullptr) ? 0 : m_assetViewer->GetAssetItems().size();
}

int CAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_allFields.size();
}

QVariant CAssetBrowserModel::data(const QModelIndex& index, int role) const
{
    auto item = m_assetViewer->GetAssetItems().at(index.row());
    if (role == Qt::DisplayRole)
    {
        return CAssetViewer::GetAssetFieldDisplayValue(item, m_allFields[index.column()].m_fieldName.toUtf8().data());
    }
    else
    {
        return QVariant();
    }
}

QVariant CAssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
    {
        return QVariant();
    }
    if (role == Qt::DisplayRole)
    {
        return m_allFields[section].m_displayName;
    }
    else if (role == Qt::SizeHintRole)
    {
        return QSize(m_allFields[section].m_listColumnWidth, 20);
    }
    return QVariant();
}

void CAssetBrowserModel::sort(int column, Qt::SortOrder order)
{
    layoutAboutToBeChanged();
    m_pAssetBrowserDlg->SortAssets(m_allFields[column].m_fieldName.toUtf8().data(), order == Qt::DescendingOrder);
    layoutChanged();
}

void CAssetBrowserModel::OnAssetFilterChanged()
{
    beginResetModel();
    endResetModel();
}

//////////////////////////////////////////////////////////////////////////
void CAssetBrowserDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CAssetBrowserDialog>(LyViewPane::LegacyAssetBrowser, LyViewPane::CategoryOther, options);
    CommandManagerHelper::RegisterCommand(
        GetIEditor()->GetCommandManager(),
        "asset_browser",
        "show_viewport_selection",
        "",
        "",
        functor(Command_ShowViewportSelection));
}

const GUID& CAssetBrowserDialog::GetClassID()
{
    static const GUID guid =
    {
        0x48903b8b, 0x7269, 0x4ffa, { 0x80, 0xef, 0x46, 0xe9, 0xe5, 0x31, 0xce, 0x85 }
    };
    return guid;
}

CAssetBrowserReportControl::CAssetBrowserReportControl(QWidget* parent)
    : QTreeView(parent)
{
    m_pAssetBrowserDlg = NULL;
    setContextMenuPolicy(Qt::CustomContextMenu);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &CAssetBrowserReportControl::OnContextMenu);
    connect(header(), &QWidget::customContextMenuRequested, this, &CAssetBrowserReportControl::OnContextMenu);
    connect(header(), &QHeaderView::sectionMoved, this, &CAssetBrowserReportControl::SaveColumnsToSettings);
    setSortingEnabled(true);
}

void CAssetBrowserReportControl::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAssetBrowserReportControl::OnSelectionChanged);
    const QStringList columnNames = gSettings.sAssetBrowserSettings.sColumnNames.split(QLatin1Char(','));
    const QStringList visibleColumnNames = gSettings.sAssetBrowserSettings.sVisibleColumnNames.split(QLatin1Char(','));
    QMap<int, int> order;
    for (int logicalIndex = 0; logicalIndex < model->columnCount(); ++logicalIndex)
    {
        const int newIndex = columnNames.indexOf(model->headerData(logicalIndex, Qt::Horizontal).toString());
        order[newIndex] = logicalIndex;
    }
    QList<int> visualIndexes = order.keys();
    qSort(visualIndexes);
    for (int visualIndex : visualIndexes)
    {
        header()->moveSection(header()->visualIndex(order[visualIndex]), visualIndex);
    }
    for (int i = 0; i < model->columnCount(); ++i)
    {
        const bool visible = visibleColumnNames.contains(model->headerData(i, Qt::Horizontal).toString());
        header()->setSectionHidden(i, !visible);
    }
}

void CAssetBrowserReportControl::OnSelectionChanged()
{
    assert(m_pAssetBrowserDlg);
    UINT index = 0;

    if (!m_pAssetBrowserDlg)
    {
        return;
    }

    if (!selectionModel()->hasSelection())
    {
        m_pAssetBrowserDlg->m_bSelectAssetsFromListView = true;
        m_pAssetBrowserDlg->GetAssetViewer().DeselectAll();
        return;
    }

    bool bHasItems = !m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems().empty();

    if (bHasItems)
    {
        int pos = currentIndex().row();
        TAssetItems itemsSelected;

        itemsSelected.reserve(selectionModel()->selectedRows().count());

        for (auto index : selectionModel()->selectedRows())
        {
            UINT idx = index.row();

            if (idx >= 0 && idx < m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems().size())
            {
                itemsSelected.push_back(m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems()[idx]);
            }
        }

        m_pAssetBrowserDlg->m_bSelectAssetsFromListView = true;
        m_pAssetBrowserDlg->GetAssetViewer().SelectAssets(itemsSelected);

        index = CLAMP(pos, 0, m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems().size());
    }
    else
    {
        m_pAssetBrowserDlg->m_bSelectAssetsFromListView = true;
        m_pAssetBrowserDlg->GetAssetViewer().DeselectAll(false);
    }

    if (bHasItems && index >= 0 && index < m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems().size())
    {
        m_pAssetBrowserDlg->m_bSelectAssetsFromListView = true;
        m_pAssetBrowserDlg->GetAssetViewer().EnsureAssetVisible(index, true);
    }
}

void CAssetBrowserReportControl::SaveColumnsToSettings()
{
    QStringList columns;

    for (int visualIndex = 0; visualIndex < model()->columnCount(); ++visualIndex)
    {
        const int logicalIndex = header()->logicalIndex(visualIndex);
        if (!header()->isSectionHidden(logicalIndex))
        {
            columns.push_back(model()->headerData(logicalIndex, Qt::Horizontal).toString());
        }
    }

    gSettings.sAssetBrowserSettings.sVisibleColumnNames = columns.join(QLatin1Char(','));

    columns.clear();

    for (int visualIndex = 0; visualIndex < model()->columnCount(); ++visualIndex)
    {
        const int logicalIndex = header()->logicalIndex(visualIndex);
        columns.push_back(model()->headerData(logicalIndex, Qt::Horizontal).toString());
    }

    gSettings.sAssetBrowserSettings.sColumnNames = "";
    gSettings.sAssetBrowserSettings.sColumnNames = columns.join(QLatin1Char(','));

    gSettings.Save();
}

CAssetBrowserDialog* CAssetBrowserDialog::s_pCurrentInstance = NULL;
bool CAssetBrowserDialog::s_bInitialized = false;

CAssetBrowserDialog::CAssetBrowserDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::AssetBrowserDialog)
{
    m_ui->setupUi(this);
    m_ui->m_searchDlg->setFixedHeight(AssetBrowser::kSearchEditDialogHeight);

    m_bSelectAssetsFromListView = false;
    s_pCurrentInstance = this;
    m_pAssetVirtualRecord = NULL;
    m_pReportHeader = NULL;
    m_pDockPaneFilters = NULL;
    m_pDockPaneListView = NULL;
    m_pDockPanePreview = NULL;
    GetIEditor()->RegisterNotifyListener(this);

    OnInitDialog();
}

CAssetBrowserDialog::~CAssetBrowserDialog()
{
    m_ui->m_assetViewer->UnregisterObserver(this);
    GetIEditor()->UnregisterNotifyListener(this);
    s_pCurrentInstance = NULL;

    CViewport* poViewport = GetIEditor()->GetViewManager()->GetViewport("Perspective");

    if (CAssetModelItem::GetAssetModelViewport())
    {
        // no need to delete the viewport since it is auto deleted on window destroy
        CAssetModelItem::SetAssetModelViewport(NULL);
    }

    if (CAssetCharacterItem::GetAssetCharacterPreviewCtrl())
    {
        // no need to delete the viewport since it is auto deleted on window destroy
        CAssetCharacterItem::SetAssetCharacterPreviewCtrl(NULL);
    }
}

void CAssetBrowserDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        return;
    }

    switch (event)
    {
    case eNotify_OnQuit: // Send when the document is about to close.
    {
        GetIEditor()->UnregisterNotifyListener(this);
        m_ui->m_assetViewer->UnregisterObserver(this);
        break;
    }

    case eNotify_OnCloseScene: // Send when the document is about to close.
    case eNotify_OnBeginSceneOpen: // Sent when document is about to be opened.
    case eNotify_OnBeginNewScene: // Sent when the document is begin to be cleared.
    {
        break;
    }

    case eNotify_OnEndSceneOpen: // Sent after document have been opened.
    case eNotify_OnEndNewScene: // Sent after the document have been cleared.
    {
        break;
    }

    // Sent when document is about to be saved.
    case eNotify_OnBeginSceneSave:
    {
        break;
    }

    // Sent after document have been saved.
    case eNotify_OnEndSceneSave:
    {
        break;
    }
    }
}

void CAssetBrowserDialog::Open(
    const char* pFilename,
    const char* pAssetDbName)
{
    if (!s_pCurrentInstance)
    {
        QtViewPaneManager::instance()->OpenPane(LyViewPane::LegacyAssetBrowser);
    }

    if (!s_pCurrentInstance)
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), tr("Could not open asset browser. Try again or contact Sandbox Support."));
        return;
    }

    s_pCurrentInstance->GetAssetFiltersDlg().GetGeneralDlg()->SelectDatabase(pAssetDbName);

    if (strcmp(pFilename, ""))
    {
        IAssetItem* pAsset = s_pCurrentInstance->GetAssetViewer().FindAssetByFullPath(pFilename);

        if (!pAsset)
        {
            if (IResourceCompilerHelper::IsSourceImageFormatSupported(PathUtil::GetExt(pFilename)))
            {
                string strDDS = PathUtil::ReplaceExtension(pFilename, ".dds");
                pAsset = s_pCurrentInstance->GetAssetViewer().FindAssetByFullPath(strDDS);
            }
        }

        s_pCurrentInstance->GetAssetViewer().SelectAsset(pAsset);
        s_pCurrentInstance->GetAssetViewer().EnsureAssetVisible(pAsset, true);
        s_pCurrentInstance->GetAssetViewer().EnsureAssetVisibleAfterBrowserOpened(pAsset);
    }
}

QString CAssetBrowserDialog::GetFirstSelectedFilename()
{
    TAssetItems items;

    m_ui->m_assetViewer->GetSelectedItems(items);

    if (items.empty())
    {
        return "";
    }

    return Path::AddSlash(items[0]->GetRelativePath()) + items[0]->GetFilename();
}

CAssetBrowserDialog* CAssetBrowserDialog::Instance()
{
    return s_pCurrentInstance;
}

void CAssetBrowserDialog::GetSelectedItems(TAssetItems& rcpoSelectedItemArray)
{
    m_ui->m_assetViewer->GetSelectedItems(rcpoSelectedItemArray);
}

void CAssetBrowserDialog::InitToolbar()
{
    // Create Library toolbar.

    m_lstThumbSize = new QComboBox;
    m_lstThumbSize->addItem(tr("64"), 64);
    m_lstThumbSize->addItem(tr("128"), 128);
    m_lstThumbSize->addItem(tr("256"), 256);
    m_lstThumbSize->setFixedSize(64, 16);

    m_ui->toolBar->insertWidget(m_ui->toolBar->actions()[4], m_lstThumbSize);

    m_lstThumbSize->setCurrentIndex(m_lstThumbSize->findData(gSettings.sAssetBrowserSettings.nThumbSize));

    void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
    connect(m_lstThumbSize, currentIndexChanged, this, &CAssetBrowserDialog::OnAssetBrowserChangeThumbSize);
}

void CAssetBrowserDialog::OnInitDialog()
{
    connect(m_ui->actionBuildCache, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserBuildCache);
    connect(m_ui->actionShowList, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserShowList);
    connect(m_ui->actionShowPreview, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserShowPreview);
    connect(m_ui->actionShowFilters, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserShowFilters);
    connect(m_ui->actionSelectInViewport, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserSelectInViewport);
    connect(m_ui->actionSelectFromViewport, &QAction::triggered, this, &CAssetBrowserDialog::OnAssetBrowserFromViewportSelection);

    InitToolbar();
    CreateAssetViewer();

    m_ui->m_statusBar0->setText(tr("No selection"));//Placeholder info.
    m_ui->m_statusBar1->setText(tr("Ready"));//Placeholder info.

    m_ui->m_searchDlg->Search();
    m_assetFiltersDlg->CreateFilterGroupsRollup();
}

void CAssetBrowserDialog::Command_ShowViewportSelection()
{
    CAssetBrowserDialog* pBrowser = FindViewPane<CAssetBrowserDialog>(LyViewPane::LegacyAssetBrowser);

    if (!pBrowser)
    {
        return;
    }

    pBrowser->OnAssetBrowserFromViewportSelection();
}

void CAssetBrowserDialog::addDockWidget(Qt::DockWidgetArea area, QWidget* widget, const QString& title, bool closable)
{
    QDockWidget* w = new AzQtComponents::StyledDockWidget(title);
    w->setObjectName(widget->metaObject()->className());
    widget->setParent(w);
    w->setWidget(widget);
    if (!closable)
    {
        w->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    }
    if (widget->metaObject()->indexOfSlot("slotVisibilityChanged(bool)") != -1)
    {
        connect(w, SIGNAL(visibilityChanged(bool)), widget, SLOT(slotVisibilityChanged(bool)));
    }

    QMainWindow::addDockWidget(area, w);
}

QDockWidget* CAssetBrowserDialog::findDockWidget(QWidget* widget)
{
    if (widget == nullptr)
    {
        return nullptr;
    }
    QDockWidget* w = qobject_cast<QDockWidget*>(widget);
    if (w)
    {
        return w;
    }
    return findDockWidget(widget->parentWidget());
}

void CAssetBrowserDialog::CreateAssetViewer()
{
    m_ui->m_assetViewer->RegisterObserver(this);

    m_ui->m_searchDlg->m_pAssetBrowserDlg = this;
    m_ui->m_searchDlg->m_bForceFilterUsedInLevel = gSettings.sAssetBrowserSettings.bShowUsedInLevel;
    m_ui->m_searchDlg->m_bForceShowFavorites = gSettings.sAssetBrowserSettings.bShowFavorites;
    m_ui->m_searchDlg->m_bForceHideLods = gSettings.sAssetBrowserSettings.bHideLods;

    m_assetListView = new CAssetBrowserReportControl(this);
    m_assetPreviewDlg = new CAssetBrowserPreviewDlg(this);
    m_assetFiltersDlg = new CAssetBrowserFiltersDlg(this);

    addDockWidget(Qt::LeftDockWidgetArea, m_assetPreviewDlg, tr("Asset Preview"));
    addDockWidget(Qt::BottomDockWidgetArea, m_assetListView, tr("Asset List"));
    addDockWidget(Qt::RightDockWidgetArea, m_assetFiltersDlg, tr("Asset Filters"));

    m_ui->m_assetViewer->SetDatabases(CAssetBrowserManager::Instance()->GetAssetDatabases());
    m_assetPreviewDlg->EndPreviewAsset();
    m_assetPreviewDlg->SetAssetPreviewHeaderDlg(nullptr);
    ResetAssetListView();

    m_pDockPanePreview = findDockWidget(m_assetPreviewDlg);
    m_pDockPaneListView = findDockWidget(m_assetListView);
    m_pDockPaneFilters = findDockWidget(m_assetFiltersDlg);

    connect(m_pDockPanePreview, &QDockWidget::visibilityChanged, this, &CAssetBrowserDialog::OnUpdateAssetBrowserShowPreview);
    connect(m_pDockPaneListView, &QDockWidget::visibilityChanged, this, &CAssetBrowserDialog::OnUpdateAssetBrowserShowList);
    connect(m_pDockPaneFilters, &QDockWidget::visibilityChanged, this, &CAssetBrowserDialog::OnUpdateAssetBrowserShowFilters);
}

void CAssetBrowserDialog::SortAssets(const char* pFieldname, bool bDescending)
{
    m_ui->m_assetViewer->SortAssets(pFieldname, bDescending);
}

void CAssetBrowserDialog::ResetAssetListView()
{
    m_assetListModel = new CAssetBrowserModel(m_ui->m_assetViewer);
    m_assetListView->m_pAssetBrowserDlg = this;
    m_assetListModel->m_pAssetBrowserDlg = this;
    m_assetListView->setModel(m_assetListModel);
    m_assetListView->header()->resizeSections(QHeaderView::ResizeToContents);
}

CAssetViewer& CAssetBrowserDialog::GetAssetViewer()
{
    return *m_ui->m_assetViewer;
}

CAssetBrowserFiltersDlg& CAssetBrowserDialog::GetAssetFiltersDlg()
{
    return *m_assetFiltersDlg;
}

CAssetBrowserReportControl& CAssetBrowserDialog::GetAssetListView()
{
    return *m_assetListView;
}

void CAssetBrowserDialog::closeEvent(QCloseEvent* event)
{
    m_ui->m_assetViewer->DeselectAll();
    QMainWindow::closeEvent(event);
}

void CAssetBrowserDialog::OnChangeStatusBarInfo(UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems)
{
    QString str;

    if (!nSelectedItems)
    {
        str = tr("No selection");
    }
    else
    {
        float fTotalSize = 0;
        TAssetItems items;

        m_ui->m_assetViewer->GetSelectedItems(items);

        for (size_t i = 0, iCount = items.size(); i < iCount; ++i)
        {
            fTotalSize += items[i]->GetFileSize();
        }

        QString strTotal;

        // if under a megabyte
        if (fTotalSize < AssetBrowser::kOneMegaByte)
        {
            strTotal = tr("%1 KB").arg(fTotalSize / AssetBrowser::kOneKiloByte, 1, 'f');
        }
        else
        {
            strTotal = tr("%1 MB").arg(ASSET_BROWSER_BYTES_TO_MBYTES(fTotalSize), 2, 'f');
        }

        str = tr("%1 selected assets, %2").arg(nSelectedItems).arg(strTotal);
    }

    m_ui->m_statusBar0->setText(str);

    str = tr("%1 listed assets, %2 total assets").arg(nVisibleItems).arg(nTotalItems);
    m_ui->m_statusBar1->setText(str);
}

void CAssetBrowserDialog::OnSelectionChanged()
{
    TAssetItems items;

    m_ui->m_assetViewer->GetSelectedItems(items);

    int index = m_ui->m_assetViewer->GetFirstSelectedItemIndex();

    if (!m_bSelectAssetsFromListView && index >= 0 && index < m_assetListView->model()->rowCount())
    {
        m_assetListView->scrollTo(m_assetListModel->index(index, 0));
    }
    else
    {
        // ok, got it, set to false, we did not sent any select operation to the list
        m_bSelectAssetsFromListView = false;
    }

    CAssetBrowserManager::StrVector allTags;

    for (TAssetItems::iterator item = items.begin(), end = items.end(); item != end; ++item)
    {
        const QString filename = (*item)->GetRelativePath() + (*item)->GetFilename();

        QString description;
        CAssetBrowserManager::Instance()->GetAssetDescription(filename, description);
        allTags.push_back(description);

        CAssetBrowserManager::StrVector tags;
        int tagCount = CAssetBrowserManager::Instance()->GetTagsForAsset(tags, filename);

        if (tagCount > 0)
        {
            allTags.append(tags);
        }
    }

    QString tagString;

    for (auto item = allTags.begin(), end = allTags.end(); item != end; ++item)
    {
        if (!tagString.isEmpty())
        {
            tagString.append(QStringLiteral(" "));
        }

        tagString.append((*item));
    }
}

void CAssetBrowserDialog::OnChangedPreviewedAsset(IAssetItem* pAsset)
{
    if (!m_pDockPanePreview)
    {
        return;
    }

    // end the previous asset previewing, if any
    m_assetPreviewDlg->EndPreviewAsset();

    QString str = tr("Asset Preview");
    // begin preview the current one
    if (pAsset)
    {
        str.append(tr(" - [%1]").arg(pAsset->GetFilename()));
    }

    m_pDockPanePreview->setWindowTitle(str);
    m_assetPreviewDlg->StartPreviewAsset(pAsset);
}

void CAssetBrowserDialog::OnCancelAssetCaching()
{
    m_bCancelBuildCache = true;
}

QProgressDialog* s_pCacheDlgProgress = 0;

static bool OnCacheProgress(int progress, const char* pMsg)
{
    if (s_pCacheDlgProgress)
    {
        s_pCacheDlgProgress->setValue(progress);
        s_pCacheDlgProgress->setLabelText(pMsg);
        QApplication::processEvents();
    }

    return CAssetBrowserDialog::Instance()->m_bCancelBuildCache;
}

void CAssetBrowserDialog::OnAssetBrowserBuildCache()
{
    CAssetBrowserCachingOptionsDlg dlgOptions;

    if (dlgOptions.exec() != QDialog::Accepted)
    {
        return;
    }

    m_assetPreviewDlg->EndPreviewAsset();

    QProgressDialog dlg(this);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setLabelText(tr("Caching the game assets thumbnails and information. This might take some time, please wait..."));
    dlg.setWindowTitle(tr("Asset Browser Caching"));
    dlg.setFixedSize(dlg.sizeHint() + QSize(40, 50));
    dlg.setMinimumDuration(0);
    dlg.setRange(0, 100);

    m_bCancelBuildCache = false;
    s_pCacheDlgProgress = &dlg;
    connect(&dlg, &QProgressDialog::canceled, this, &CAssetBrowserDialog::OnCancelAssetCaching);

    int oldThumbSize = m_ui->m_assetViewer->GetAssetThumbSize();
    auto dbs = m_ui->m_assetViewer->GetDatabases();

    m_ui->m_assetViewer->ClearDatabases();

    m_ui->m_assetViewer->SetAssetThumbSize(dlgOptions.GetThumbSize());
    CAssetBrowserManager::Instance()->CacheAssets(
        dlgOptions.GetSelectedDatabases(),
        dlgOptions.IsForceCache(),
        &OnCacheProgress);
    m_ui->m_assetViewer->SetDatabases(dbs);
    ApplyAllFiltering();

    m_ui->m_assetViewer->SetAssetThumbSize(oldThumbSize);
    s_pCacheDlgProgress = nullptr;
}

void CAssetBrowserDialog::OnAssetBrowserChangeThumbSize()
{
    int sel = m_lstThumbSize->currentIndex();

    if (sel != -1)
    {
        UINT size = (1 << (6 + sel));
        m_ui->m_assetViewer->SetAssetThumbSize(size);
    }
}

void CAssetBrowserDialog::OnAssetBrowserShowList()
{
    m_pDockPaneListView->setVisible(m_ui->actionShowList->isChecked());
    OnUpdateAssetBrowserShowList();
}

void CAssetBrowserDialog::OnAssetBrowserShowPreview()
{
    m_pDockPanePreview->setVisible(m_ui->actionShowPreview->isChecked());
    OnUpdateAssetBrowserShowPreview();
}

void CAssetBrowserDialog::OnAssetBrowserShowFilters()
{
    m_pDockPaneFilters->setVisible(m_ui->actionShowFilters->isChecked());
    OnUpdateAssetBrowserShowFilters();
}

void CAssetBrowserDialog::OnUpdateAssetBrowserShowList()
{
    m_ui->actionShowList->setChecked(m_pDockPaneListView->isVisible());
}

void CAssetBrowserDialog::OnUpdateAssetBrowserShowPreview()
{
    m_ui->actionShowPreview->setChecked(m_pDockPanePreview->isVisible());
}

void CAssetBrowserDialog::OnUpdateAssetBrowserShowFilters()
{
    m_ui->actionShowFilters->setChecked(m_pDockPaneFilters->isVisible());
}

void CAssetBrowserDialog::OnUpdateAssetBrowserEditTags()
{
    TAssetItems items;
    m_ui->m_assetViewer->GetSelectedItems(items);

    if (items.size() != 1)
    {
        return;
    }

    CAssetBrowserMetaTaggingDlg taggingDialog(&items);
    taggingDialog.exec();
    OnSelectionChanged();
}

void CAssetBrowserDialog::ApplyAllFiltering()
{
    m_ui->m_searchDlg->Search();
}

void CAssetBrowserDialog::OnAssetBrowserFromViewportSelection()
{
    // lets gather info about selection from viewport
    CSelectionGroup* pSelGrp = GetIEditor()->GetSelection();

    if (!pSelGrp)
    {
        QMessageBox::warning(this, QString(), tr("No selection. Please select entities/objects in the viewport."));
        return;
    }

    m_ui->m_searchDlg->m_bAllowOnlyFilenames = true;
    m_ui->m_searchDlg->m_allowedFilenames = "";

    QStringList foundResources;

    for (size_t i = 0, iCount = pSelGrp->GetCount(); i < iCount; ++i)
    {
        CBaseObject* pObj = pSelGrp->GetObject(i);
        CUsedResources resUsed;

        pObj->GatherUsedResources(resUsed);

        for (CUsedResources::TResourceFiles::iterator iter = resUsed.files.begin(); iter != resUsed.files.end(); ++iter)
        {
            foundResources.push_back(*iter);
        }

        switch (pObj->GetType())
        {
        case OBJTYPE_BRUSH:
        {
            CBrushObject* pBrush = (CBrushObject*)pObj;
            break;
        }

        case OBJTYPE_ENTITY:
        {
            CEntityObject* pEntity = (CEntityObject*)pObj;
            IEntity* pIEnt = pEntity->GetIEntity();

            for (size_t j = 0, jCount = pIEnt->GetSlotCount(); j < jCount; ++j)
            {
                //CEntityObject* pSlot = pEntity->GetSlot( j );

                //if( pSlot && pSlot->pStatObj )
                //{
                //  foundResources.insert( pSlot->pStatObj->GetFilePath() );
                //}
            }

            break;
        }
        }
    }

    m_ui->m_searchDlg->m_allowedFilenames = foundResources.join(QLatin1Char(' '));
    ApplyAllFiltering();
}

void CAssetBrowserDialog::OnAssetBrowserSelectInViewport()
{
    TAssetItems items;

    m_ui->m_assetViewer->GetSelectedItems(items);
    GetIEditor()->ClearSelection();

    CBaseObjectsArray objArr;

    GetIEditor()->GetObjectManager()->GetObjects(objArr);
    QString strFile;

    for (size_t i = 0, iCount = items.size(); i < iCount; ++i)
    {
        IAssetItem* pAsset = items[i];
        QString strFullPath = PathUtil::AddSlash(pAsset->GetRelativePath().toUtf8().data()) + pAsset->GetFilename();

        for (size_t j = 0, jCount = objArr.size(); j < jCount; ++j)
        {
            CBaseObject* pObj = objArr[j];
            CUsedResources resUsed;

            pObj->GatherUsedResources(resUsed);

            for (CUsedResources::TResourceFiles::iterator iter = resUsed.files.begin(); iter != resUsed.files.end(); ++iter)
            {
                strFile = iter->toLower();

                if (strFullPath == strFile)
                {
                    GetIEditor()->SelectObject(pObj);
                }
            }

            switch (pObj->GetType())
            {
            case OBJTYPE_BRUSH:
            {
                CBrushObject* pBrush = (CBrushObject*)pObj;
                break;
            }
            }
        }
    }

    if (GetIEditor()->GetSelection() && !GetIEditor()->GetSelection()->GetCount())
    {
        QMessageBox::critical(this, QString(), tr("No objects were selected.\nNone of the selected assets is used by any of the level's objects."));
    }
}

void CAssetBrowserDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_F5:
    {
        OnAssetBrowserBuildCache();
        break;
    }
    }

    QMainWindow::keyPressEvent(event);
}

void CAssetBrowserReportControl::OnContextMenu(const QPoint& point)
{
    QMenu menu;

    enum
    {
        eCmd_HideAll = 1,
        eCmd_ShowAll,

        eCmd_Last
    };

    menu.addAction(tr("Hide all columns"))->setData(eCmd_HideAll);
    menu.addAction(tr("Show all columns"))->setData(eCmd_ShowAll);
    menu.addSeparator();

    for (int i = 0; i < model()->columnCount(); ++i)
    {
        const int logicalIndex = header()->logicalIndex(i);
        const bool bVisible = !header()->isSectionHidden(logicalIndex);
        QAction* action = menu.addAction(model()->headerData(logicalIndex, Qt::Horizontal).toString());
        action->setCheckable(true);
        action->setChecked(bVisible);
        action->setData(i + eCmd_Last);
    }

    QAction* action = menu.exec((sender() == header() ? header() : viewport())->mapToGlobal(point));
    int nResult = action ? action->data().toInt() : 0;

    if (nResult < 1)
    {
        return;
    }

    if (nResult == eCmd_HideAll)
    {
        for (int i = 0; i < model()->columnCount(); ++i)
        {
            header()->setSectionHidden(i, true);
        }

        return;
    }
    else if (nResult == eCmd_ShowAll)
    {
        for (int i = 0; i < model()->columnCount(); ++i)
        {
            header()->setSectionHidden(i, false);
        }

        return;
    }

    int colIndex = nResult - eCmd_Last;
    bool bVisible = action->isChecked();

    header()->setSectionHidden(header()->logicalIndex(colIndex), !bVisible);
    SaveColumnsToSettings();
}

#include <AssetBrowser/AssetBrowserDialog.moc>

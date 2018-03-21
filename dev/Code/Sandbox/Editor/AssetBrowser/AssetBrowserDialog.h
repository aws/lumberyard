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

// Description : MFC class implementing the Asset browser dialog


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERDIALOG_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERDIALOG_H
#pragma once
#include <map>
#include <vector>
#include "IEditor.h"
#include "../Controls/AssetViewer.h"
#include "AssetBrowser/AssetBrowserSearchDlg.h"
#include "AssetBrowser/AssetBrowserPreviewDlg.h"
#include "AssetBrowser/AssetBrowserFiltersDlg.h"

#include <QAbstractTableModel>
#include <QBoxLayout>
#include <QMainWindow>
#include <QTreeView>

#define ASSET_BROWSER_VER "1.00"

class QComboBox;

namespace Ui
{
    class AssetBrowserDialog;
}

namespace AssetBrowser
{
    const UINT kMaxDatabaseCount = 100;
};

template<typename T>
class CWidgetWrapper;

//! This class is the actual asset browser list view
class CAssetBrowserModel
    : public QAbstractTableModel
    , public IAssetViewerObserver
{
    Q_OBJECT
public:
    CAssetBrowserModel(CAssetViewer* assetViewer, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void OnAssetFilterChanged() override;

    CAssetBrowserDialog* m_pAssetBrowserDlg;

protected:
    CAssetViewer* m_assetViewer;
    TAssetItems m_visibleItems;
    IAssetItemDatabase::TAssetFields m_allFields;
    IAssetItemDatabase::TAssetFields m_commonFields;
};

class CAssetBrowserReportControl
    : public QTreeView
{
    Q_OBJECT
public:
    CAssetBrowserReportControl(QWidget* parent = nullptr);
    void OnSelectionChanged();
    void OnContextMenu(const QPoint& point);

    void setModel(QAbstractItemModel* model) override;

    void SaveColumnsToSettings();

    CAssetBrowserDialog* m_pAssetBrowserDlg;
};

class CAssetBrowserDialog
    : public QMainWindow
    , public IEditorNotifyListener
    , public IAssetViewerObserver
{
    Q_OBJECT
public:
    friend class CAssetBrowserSearchDlg;

    CAssetBrowserDialog(QWidget* pParetn = nullptr);     // standard constructor
    virtual ~CAssetBrowserDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener
    //////////////////////////////////////////////////////////////////////////
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    static void Open(
        const char* pFilename = "",
        const char* pAssetDbName = "");
    QString GetFirstSelectedFilename();
    static CAssetBrowserDialog* Instance();
    void GetSelectedItems(TAssetItems& rcpoSelectedItemArray);
    CAssetViewer& GetAssetViewer();
    CAssetBrowserFiltersDlg& GetAssetFiltersDlg();
    CAssetBrowserReportControl& GetAssetListView();

protected:
    //////////////////////////////////////////////////////////////////////////
    // IAssetViewerObserver
    //////////////////////////////////////////////////////////////////////////
    void OnChangeStatusBarInfo(UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems);
    void OnSelectionChanged();
    void OnChangedPreviewedAsset(IAssetItem* pAsset);

    void InitToolbar();

public:
    void CreateAssetViewer();
    IAssetItemDatabase* GetDatabaseByName(const char* pName);
    void SortAssets(const char* pFieldname, bool bDescending = false);
    void ResetAssetListView();
    void ApplyAllFiltering();
    void OnCancelAssetCaching();

    //
    // Commands
    //
    static void Command_ShowViewportSelection();

    //
    // Message handlers
    //
    void OnInitDialog();
    void PostNcDestroy();
    void OnDestroy();
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void OnAssetBrowserBuildCache();
    void OnAssetBrowserChangeThumbSize();
    void OnAssetBrowserShowList();
    void OnAssetBrowserShowPreview();
    void OnAssetBrowserShowFilters();
    void OnUpdateAssetBrowserShowList();
    void OnUpdateAssetBrowserShowPreview();
    void OnUpdateAssetBrowserShowFilters();
    void OnUpdateAssetBrowserEditTags();
    void OnAssetBrowserFromViewportSelection();
    void OnAssetBrowserSelectInViewport();

    void addDockWidget(Qt::DockWidgetArea, QWidget* widget, const QString& title, bool closable = true);
    QDockWidget* findDockWidget(QWidget* widget);

    static CAssetBrowserDialog* s_pCurrentInstance;
    static bool s_bInitialized;

    CAssetBrowserReportControl* m_assetListView;
    CAssetBrowserModel* m_assetListModel;
    CAssetBrowserPreviewDlg* m_assetPreviewDlg;
    CAssetBrowserFiltersDlg* m_assetFiltersDlg;
    QDockWidget* m_pDockPaneListView;
    QDockWidget* m_pDockPanePreview;
    QDockWidget* m_pDockPaneFilters;
    QComboBox* m_lstThumbSize;
    IAssetItemDatabase::TAssetFields m_allFields;
    IAssetItemDatabase::TAssetFields m_commonFields;
    class CAssetListReportHeader* m_pReportHeader;
    class CAssetListVirtualRecord* m_pAssetVirtualRecord;
    bool m_bCancelBuildCache;
    //! guardian bool, to avoid ping-pong selection changed events between listview and thumbsview (this control) when selecting assets
    bool m_bSelectAssetsFromListView;

    QScopedPointer<Ui::AssetBrowserDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERDIALOG_H

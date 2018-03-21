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
#include "AssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetItem.h"
#include "Include/IAssetItemDatabase.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "AssetBrowser/AssetBrowserPreviewDlg.h"
#include "AssetBrowser/AssetBrowserManager.h"
#include "AssetBrowser/AssetBrowserDialog.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QInputDialog>

#include "QtUtil.h"

//--- Constants / defines

//! timer id for smooth panning with mouse right-click-release dragging
#define ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING 2
//! timer id for cache assets which cannot be cached inside a separate thread
#define ID_ASSET_VIEWER_TIMER_CACHE_ASSET 3
//! timer for warning blinking on asset thumbs, it will toggle a boolean, m_bWarningBlinkToggler, and redraw asset control
#define ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER 4
//! timer for when user selects an asset in the list view, and then the viewer must animate, attract user attention to the selected thumbnail
#define ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION 5
//! window control id to render assets in
#define ID_ASSET_VIEWER_THUMB_RENDER_WINDOW 1

namespace AssetViewer
{
    // this is the delay for the timer which caches assets that cannot be cached on a separate thread (ex.: models, characters)
    const int kNonThreadCacheTimerDelay = 80;
    // this is the loop sleep time (msec) for the caching thread when waiting for the metadata db to be loaded
    const int kThreadSleepTimeWaitLoadAndUpdateDone = 100;
    // this is the delay for the timer which pans/scrolls the canvas smoothly
    const UINT kSmoothPanningTimerDelay = USER_TIMER_MINIMUM;
    // this is the delay for the timer which toggles the warning blinker
    const UINT kWarningBlinkerTimerDelay = 500;
    // timer delay used to animate a rectangle to focus a thumb when selected in list view
    const UINT kThumbFocusAnimationTimerDelay = USER_TIMER_MINIMUM;
    // the maximum number of thumbnails that can be stored at once in the asset thumb queue
    const UINT kMaxThumbsCacheAssetCount = 500;
    // boosts the thumb smooth panning delta (multiplier), so it can smooth scroll/pan more/less
    const float kSmoothPanningDeltaBoost = 2;
    // interactive render timer delay
    const UINT kInteractiveRenderTimerDelay = USER_TIMER_MINIMUM;
    // this is the minimum offset on X and Y the mouse can be dragged over an asset selection, before it is considered a rectangle selection operation
    const UINT kMinimumSelectionDraggingOffset = 15;
    // this is the minimum offset on X and Y the mouse can be dragged over on right click and drag,
    // before it starts to pan the canvas, else it will show the asset context menu
    const UINT kMinimumStartPanningOffset = 1;
    // this is the number multiplied by the current smooth panning delta, to decrease it until in reaches near 0
    const float kSmoothPanning_SlowdownMultiplier = 0.9f;
    const float kSmoothPanning_Speed = 0.01f;

    // the key to be pressed to show
    const Qt::Key   kPreviewShortcutKey = Qt::Key_Return;

    const int       kThumb_VerticalSpacing = 35;
    const int       kThumb_HorizontalSpacing = 20;
    const UINT  kSmallThumbSize = 64;
    const UINT  kBigThumbSize = 512;

    const int kTooltip_Margin = 20;
    const int kTooltip_MinWidth = 200;
    const int kTooltip_MinHeight = 80;
    const int kTooltip_TitleContentSpacing = 5;
    const int kTooltip_BorderWidth = 2;
    const int kTooltip_BorderCornerSize = 0;
    const QColor kTooltip_BorderColor(10, 10, 10);
    const QColor kTooltip_FilenameShadowColor(0, 0, 0);
    const QColor kTooltip_FilenameColor(255, 255, 0);
    const QColor kSelectionDragLineColor(255, 255, 100);
    const QColor kLabelColor(255, 255, 100);
    const QColor kLabelBoxBackColor(100, 100, 100);
    const QColor kLabelBoxLineColor(255, 255, 0);
    const QColor kLabelBoxSelectedBackColor(20, 116, 173);
    const QColor kLabelBoxNonSelectedBackColor(120, 120, 120);
    const QColor kLabelBoxSelectedLineColor(110, 110, 110);
    const QColor kLabelBoxNonSelectedLineColor(40, 40, 40);
    const QColor kLabelTextColor(255, 255, 255);
    const QColor kOneLineInfoTextColor(255, 155, 55);
    const QColor kThumbContainerHoverLineColor(255, 255, 255);
    const QColor kThumbContainerLineColor(60, 60, 60);
    const QColor kInfoTextColor(0, 255, 255);
    const QColor kThumbContainerWarningBackColor(255, 50, 0);
    const QColor kThumbContainerBackColor(50, 50, 50);

    const int kLabelFontSize_Small = 60;
    const int kLabelFontSize_Normal = 80;
    const int kLabelFontSize_Large = 120;

    const int kOneLinerInfoFontSize_Small = 55;
    const int kOneLinerInfoFontSize_Normal = 70;
    const int kOneLinerInfoFontSize_Large = 110;

    const int kInfoTitle_FontSize = 85;
    const int kInfoText_FontSize = 75;

    const int       kMiniThumbButton_VerticalSpacing = 4;
    const int       kMiniThumbButton_Width = 16;
    const int       kMiniThumbButton_Height = 16;
    const BYTE  kMiniThumbButton_ActiveAlpha = 200;
    const BYTE  kMiniThumbButton_HoveredAlpha = 255;
    const BYTE  kMiniThumbButton_InactiveAlpha = 40;
    const QColor kMiniThumbButton_TextColor(0, 0, 0);
    const QColor kMiniThumbButton_BorderColor(22, 22, 22);
    const QColor kMiniThumbButton_BackColor(255, 255, 100);

    const UINT  kCachingInfoProgressBar_Height = 30;

    const char* kTooltipShadowBmpFilename = "Editor/UI/Icons/AssetBrowserTooltipShadow.png";
    const char* kThumbLoadingBmpFilename = "Editor/UI/Icons/AssetBrowserThumbLoading.png";
    const char* kThumbInvalidAssetBmpFilename = "Editor/UI/Icons/AssetBrowserThumbInvalid.png";
    const char* kBackgroundBmpFilename = "Editor/UI/Icons/AssetBrowserBack.png";
    const char* kEditTagsBmpFilename = "Editor/UI/Icons/AssetTag.png";
    const char* kToggleFavBmpFilename = "Editor/UI/Icons/AssetFav.png";
};

//---

SAssetField* CAssetViewer::s_pSortField = NULL;
bool CAssetViewer::s_bSortDescending = false;

CAssetViewer::CAssetViewer(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    m_itemHorizontalSpacing = AssetViewer::kThumb_HorizontalSpacing;
    m_itemVerticalSpacing = AssetViewer::kThumb_VerticalSpacing;
    m_assetThumbSize = gSettings.sAssetBrowserSettings.nThumbSize;
    m_bMouseLeftButtonDown = false;
    m_bMouseRightButtonDown = false;
    m_bDragging = false;
    m_bDraggingAssetFromBrowser = false;
    m_bDraggingCreatedAssetInWorld = false;
    m_pDraggedAsset = NULL;
    m_pDropInViewport = NULL;
    m_selectionRect = QRect(0, 0, 0, 0);
    m_clientRect = QRect(0, 0, 0, 0);
    m_yOffset = 0;
    m_idealClientWidth = 0;
    m_idealClientHeight = 0;
    m_pEnsureVisible = NULL;
    m_pEnsureVisibleOnBrowserOpen = NULL;
    m_pRenderer = gEnv->pRenderer;
    m_bMustRedraw = false;

    m_thumbLoadingBmp.load(AssetViewer::kThumbLoadingBmpFilename);
    m_thumbInvalidAssetBmp.load(AssetViewer::kThumbInvalidAssetBmpFilename);
    m_tooltipBackBmp.load(AssetViewer::kTooltipShadowBmpFilename);

    m_fontInfoTitle = QFont(QStringLiteral("Arial"), AssetViewer::kInfoTitle_FontSize / 10.0, QFont::Bold);
    m_fontInfoText = QFont(QStringLiteral("Arial"), AssetViewer::kInfoText_FontSize / 10.0);

    m_pClickedAsset = NULL;
    m_pHoveredAsset = NULL;
    m_smoothPanLastDelta = 0;
    m_smoothPanLeftAmount = 0;
    m_assetTotalCount = 0;
    m_thumbFocusAnimationTime = 1;
    m_pHoveredThumbButtonAsset = NULL;
    m_hoveredThumbButtonIndex = 0;

    m_wndAssetThumbRender.setUpdatesEnabled(false);
    m_wndAssetThumbRender.resize(m_assetThumbSize, m_assetThumbSize);

    if (m_pRenderer->CreateContext(reinterpret_cast<HWND>(m_wndAssetThumbRender.winId())))
    {
        m_pRenderer->MakeMainContextActive();
    }
    else
    {
        return;
    }

    GetIEditor()->RegisterNotifyListener(this);
    m_timers[ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER] = startTimer(AssetViewer::kWarningBlinkerTimerDelay);
    // setup thumb size, we do this here because we want the various fonts used to be created based on thumb size
    SetAssetThumbSize(gSettings.sAssetBrowserSettings.nThumbSize);
}

void CAssetViewer::OnDestroy()
{
    killTimer(m_timers[ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING]);
}

CAssetViewer::~CAssetViewer()
{
    OnDestroy();

    for (size_t i = 0, iCount = m_thumbMiniButtons.size(); i < iCount; ++i)
    {
        delete m_thumbMiniButtons[i];
    }

    m_thumbMiniButtons.clear();
    GetIEditor()->UnregisterNotifyListener(this);

    if (m_pRenderer)
    {
        m_pRenderer->DeleteContext(reinterpret_cast<HWND>(m_wndAssetThumbRender.winId()));
    }
}

void CAssetViewer::FreeData()
{
    m_assetTotalCount = 0;
    m_assetDatabases.clear();
    m_assetItems.clear();
    m_assetDrawingCache.clear();
    m_thumbsCache.clear();
}

void CAssetViewer::SetAssetThumbSize(const UINT nThumbSize)
{
    m_assetThumbSize = nThumbSize;
    gSettings.sAssetBrowserSettings.nThumbSize = m_assetThumbSize;
    m_wndAssetThumbRender.resize(m_assetThumbSize, m_assetThumbSize);

    // uncache all in-view asset thumbs, we need new thumbs
    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        for (auto iter = m_assetDatabases[i]->GetAssets().begin(),
             iterEnd = m_assetDatabases[i]->GetAssets().end();
             iter != iterEnd; ++iter)
        {
            iter->second->UnloadThumbnail();
        }
    }

    RecalculateLayout();
    UpdateScrollBar();
    UpdateVisibility();

    //
    // adjust the font size based on thumb size
    //
    int labelFontSize = AssetViewer::kLabelFontSize_Normal;
    int oneLinerInfoFontSize = AssetViewer::kOneLinerInfoFontSize_Normal;

    if (m_assetThumbSize <= AssetViewer::kSmallThumbSize)
    {
        labelFontSize = AssetViewer::kLabelFontSize_Small;
    }

    if (m_assetThumbSize >= AssetViewer::kBigThumbSize)
    {
        labelFontSize = AssetViewer::kLabelFontSize_Large;
    }

    if (m_assetThumbSize <= AssetViewer::kSmallThumbSize)
    {
        oneLinerInfoFontSize = AssetViewer::kOneLinerInfoFontSize_Small;
    }

    if (m_assetThumbSize >= AssetViewer::kBigThumbSize)
    {
        oneLinerInfoFontSize = AssetViewer::kOneLinerInfoFontSize_Large;
    }

    m_fontLabel = QFont(QStringLiteral("Arial"), labelFontSize / 10.0);
    m_fontOneLinerInfo = QFont(QStringLiteral("Arial"), oneLinerInfoFontSize / 10.0);
    viewport()->update();
}

UINT CAssetViewer::GetAssetThumbSize()
{
    return m_assetThumbSize;
}

void CAssetViewer::SetDatabases(TAssetDatabases& rcDatabases)
{
    DeselectAll();
    m_assetDatabases = rcDatabases;
    m_pClickedAsset = NULL;
    m_assetDrawingCache.clear();
    m_selectedAssets.clear();
    m_thumbsCache.clear();
    m_pEnsureVisible = NULL;
    m_pClickedAsset = NULL;
    m_pHoveredAsset = NULL;
    m_pDraggedAsset = NULL;
    m_pDropInViewport = NULL;
    m_pDraggedAssetInstance = NULL;
    CALL_OBSERVERS_METHOD(OnChangedPreviewedAsset(NULL));
    RecalculateTotalAssetCount();
}

void CAssetViewer::AddDatabase(IAssetItemDatabase* pDB)
{
    auto iter = std::find(m_assetDatabases.begin(), m_assetDatabases.end(), pDB);

    if (iter != m_assetDatabases.end())
    {
        return;
    }

    m_assetDatabases.push_back(pDB);
    SetDatabases(m_assetDatabases);
}

void CAssetViewer::RemoveDatabase(IAssetItemDatabase* pDB)
{
    auto iter = std::find(m_assetDatabases.begin(), m_assetDatabases.end(), pDB);

    if (iter == m_assetDatabases.end())
    {
        return;
    }

    m_assetDatabases.erase(iter);
    SetDatabases(m_assetDatabases);
}

TAssetDatabases& CAssetViewer::GetDatabases()
{
    return m_assetDatabases;
}

void CAssetViewer::ClearDatabases()
{
    m_assetTotalCount = 0;
    m_assetDatabases.clear();
    m_pClickedAsset = NULL;
    m_assetDrawingCache.clear();
    m_selectedAssets.clear();
    m_thumbsCache.clear();
    m_pEnsureVisible = NULL;
    m_pClickedAsset = NULL;
    m_pHoveredAsset = NULL;
    m_pDraggedAsset = NULL;
    m_pDropInViewport = NULL;
    m_pDraggedAssetInstance = NULL;
    m_thumbMiniButtonInstances.clear();
    m_assetItems.clear();
}

void CAssetViewer::GetSelectedItems(TAssetItems& rcpoSelectedItemArray)
{
    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
    {
        IAssetItem* const cpoCurrentItem = m_assetItems[nCurrentAsset];

        if (cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Selected))
        {
            rcpoSelectedItemArray.push_back(cpoCurrentItem);
        }
    }
}

int CAssetViewer::GetFirstSelectedItemIndex()
{
    int index = -1;

    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
    {
        IAssetItem* const cpoCurrentItem = m_assetItems[nCurrentAsset];

        if (cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Selected))
        {
            index = nCurrentAsset;
            break;
        }
    }

    return index;
}

IAssetItem* CAssetViewer::FindAssetByFullPath(const char* pFilename)
{
    for (size_t i = 0; i < m_assetDatabases.size(); ++i)
    {
        IAssetItem* pAsset = m_assetDatabases[i]->GetAsset(pFilename);

        if (pAsset)
        {
            return pAsset;
        }
    }

    return NULL;
}

void CAssetViewer::SelectAsset(IAssetItem* pAsset)
{
    DeselectAll();

    // if this asset is not in the current asset list, skip it
    if (m_assetItems.end() == std::find(m_assetItems.begin(), m_assetItems.end(), pAsset))
    {
        return;
    }

    if (pAsset)
    {
        pAsset->SetFlag(IAssetItem::eFlag_Selected, true);
        m_selectedAssets.push_back(pAsset);
    }

    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
}

void CAssetViewer::SelectAssets(const TAssetItems& rSelectedItems)
{
    DeselectAll(false);
    TAssetItems items;

    for (size_t i = 0, iCount = rSelectedItems.size(); i < iCount; ++i)
    {
        IAssetItem* const cpoCurrentItem = rSelectedItems[i];

        // if this asset is not in the current asset list, skip it
        if (m_assetItems.end() == std::find(m_assetItems.begin(), m_assetItems.end(), cpoCurrentItem))
        {
            continue;
        }

        cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, true);
        items.push_back(cpoCurrentItem);
    }

    m_selectedAssets = items;
    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
}

void CAssetViewer::DeselectAll(bool bCallObservers)
{
    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
    {
        IAssetItem* const cpoCurrentItem = m_assetItems[nCurrentAsset];

        cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, false);
    }

    m_selectedAssets.clear();

    if (bCallObservers)
    {
        CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
        CALL_OBSERVERS_METHOD(OnSelectionChanged());
    }
}

void CAssetViewer::OnEditorNotifyEvent(EEditorNotifyEvent aEvent)
{
    switch (aEvent)
    {
    case eNotify_OnIdleUpdate:
    {
        if (m_bMustRedraw)
        {
            m_bMustRedraw = false;

            viewport()->update();
        }

        break;
    }
    }
}

void CAssetViewer::resizeEvent(QResizeEvent* event)
{
    m_clientRect = rect();

    RecalculateLayout();

    int nNewOffset = m_yOffset;
    int nMaxScroll = m_idealClientHeight - m_clientRect.height();

    if (nMaxScroll < 0)
    {
        nNewOffset = 0;
    }

    if ((int)m_idealClientHeight - nNewOffset < m_clientRect.height())
    {
        nNewOffset = nMaxScroll;
    }

    if (nNewOffset < 0)
    {
        nNewOffset = 0;
    }

    viewport()->resize(m_idealClientWidth, std::max(rect().height(), (int)m_idealClientHeight));
    m_yOffset = nNewOffset;
    UpdateVisibility();

    if (m_pEnsureVisibleOnBrowserOpen && !m_clientRect.isNull())
    {
        EnsureAssetVisible(m_pEnsureVisibleOnBrowserOpen, true);
        m_pEnsureVisibleOnBrowserOpen = NULL;
    }

    if (m_pEnsureVisible && !m_clientRect.isNull())
    {
        EnsureAssetVisible(m_pEnsureVisible, false);
    }
    UpdateScrollBar();
}

void CAssetViewer::paintEvent(QPaintEvent* event)
{
    Draw();
}

void CAssetViewer::CheckClickedThumb(const QPoint& point, bool bShowInteractiveRenderWnd)
{
    QPoint stHitTestPoint(point);

    stHitTestPoint.ry() += m_yOffset;
    m_pClickedAsset = NULL;

    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetDrawingCache.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
    {
        IAssetItem* const cpoCurrentItem = m_assetDrawingCache[nCurrentAsset];

        if (!cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            continue;
        }
        else if (cpoCurrentItem->HitTest(stHitTestPoint.x(), stHitTestPoint.y()))
        {
            m_pClickedAsset = cpoCurrentItem;
            break;
        }
        else
        {
            QRect drawingRect;

            cpoCurrentItem->GetDrawingRectangle(drawingRect);

            // expand bottom with vertical margin
            drawingRect = drawingRect.marginsAdded(QMargins(0, 0, 0, m_itemVerticalSpacing));

            if (drawingRect.contains(stHitTestPoint))
            {
                m_pClickedAsset = cpoCurrentItem;
                break;
            }
        }
    }
}

void CAssetViewer::CheckHoveredThumb()
{
    m_pHoveredAsset = NULL;
    const QPoint point = viewport()->mapFromGlobal(QCursor::pos());

    for (size_t i = 0, iCount = m_assetDrawingCache.size(); i < iCount; ++i)
    {
        if (!m_assetDrawingCache[i]->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            continue;
        }

        if (m_assetDrawingCache[i]->HitTest(point.x(), point.y() + m_yOffset))
        {
            m_pHoveredAsset = m_assetDrawingCache[i];
            break;
        }
    }
}

void CAssetViewer::OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_selectionRect.setTopLeft(point);
    m_selectionRect.setBottomRight(point);
    m_bMouseLeftButtonDown = true;
    m_bDragging = true;
    m_lastPanDragPt = point;
    m_smoothPanLastDelta = 0;
    m_bDraggingAssetFromBrowser = false;
    m_pDraggedAssetInstance = NULL;
    m_pDraggedAsset = NULL;
    m_bDraggingCreatedAssetInWorld = false;
    m_bDraggingAssetFromBrowser = false;
    m_pDropInViewport = NULL;

    killTimer(m_timers[ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING]);
    CheckClickedThumb(point, true);

    //
    // if we have a clicked asset thumb, do notify it
    //
    if (m_pClickedAsset)
    {
        m_pClickedAsset->OnThumbClick(point, Qt::LeftButton, modifiers);
    }

    //
    // it seems we want to drag the asset in a viewport
    //
    if (m_bMouseLeftButtonDown)
    {
        if (m_pClickedAsset)
        {
            if (m_pClickedAsset->IsFlagSet(IAssetItem::eFlag_CanBeDraggedInViewports))
            {
                m_bDraggingAssetFromBrowser = true;
                DeselectAll();
                m_pClickedAsset->SetFlag(IAssetItem::eFlag_Selected, true);
                m_selectedAssets.push_back(m_pClickedAsset);
                setCursor(CMFCUtils::LoadCursor(IDC_POINTER_DRAG_ITEM));
            }
        }
    }

    //
    // we've decided where to place the asset, so the second click will position the asset instance in the viewport and end the creation process
    //
    if (m_bDraggingCreatedAssetInWorld)
    {
        m_bDraggingCreatedAssetInWorld = false;
    }

    viewport()->update();
}

void CAssetViewer::mouseMoveEvent(QMouseEvent* event)
{
    //
    // check if mini thumb buttons are hovered
    //
    bool bBreak = false;
    m_pHoveredThumbButtonAsset = NULL;
    m_hoveredThumbButtonIndex = 0;

    if (!m_bDraggingAssetFromBrowser)
    {
        for (auto iter = m_thumbMiniButtonInstances.begin(),
             iterEnd = m_thumbMiniButtonInstances.end();
             iter != iterEnd && !bBreak; ++iter)
        {
            std::vector<CThumbMiniButtonInstance>& instances = iter->second;

            for (size_t i = 0, iCount = instances.size(); i < iCount; ++i)
            {
                if (instances[i].m_rect.contains(event->pos()))
                {
                    m_pHoveredThumbButtonAsset = iter->first;
                    m_hoveredThumbButtonIndex = i;
                    bBreak = true;
                    break;
                }
            }
        }
    }

    //
    // if we dropped an asset in the viewport, now we are moving it in the world to place it
    //
    if (m_bDraggingCreatedAssetInWorld
        && m_pDropInViewport
        && m_pDraggedAsset
        && m_pDraggedAssetInstance)
    {
        Vec3 pos;

        QPoint vp(event->globalX(), event->globalY());
        m_pDropInViewport->ScreenToClient(vp);

        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = m_pDropInViewport->MapViewToCP(vp);
        }
        else
        {
            bool bHitTerrain = false;

            pos = m_pDropInViewport->ViewToWorld(vp, &bHitTerrain);

            if (bHitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
            }
        }

        pos = m_pDropInViewport->SnapToGrid(pos);
        m_pDraggedAsset->MoveInstanceInViewport(m_pDraggedAssetInstance, pos.x, pos.y, pos.z);
        GetIEditor()->Get3DEngine()->Update();
        m_pDropInViewport->Update();
        GetIEditor()->GetViewManager()->UpdateViews();
    }

    //
    // update the dragging selection rectangle, but only if we're not dragging some asset out of the browser
    //
    if (m_bMouseLeftButtonDown && !m_bDraggingAssetFromBrowser)
    {
        m_selectionRect.setBottomRight(event->pos());
    }

    //
    // smooth panning the asset browser thumbs canvas
    //
    if (m_bMouseRightButtonDown)
    {
        m_smoothPanLastDelta = m_lastPanDragPt.y() - event->y();
        m_yOffset += m_smoothPanLastDelta;
        m_smoothPanLastDelta *= AssetViewer::kSmoothPanningDeltaBoost;
        m_lastPanDragPt = event->pos();
        m_pEnsureVisible = NULL;
        UpdateScrollBar();
        UpdateVisibility();
    }

    if (m_smoothPanLastDelta)
    {
        // we must call the smooth panning here too, because when we move the mouse,
        // WM_TIMER messages do not get through, having lower priority
        // and the panning is not smooth, so we call this function in timer and also here, on mouse move
        // the function has builtin timing, does not rely on WM_TIMER tick
        UpdateSmoothPanning();
    }

    viewport()->update();
}

void CAssetViewer::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->pos(), event->modifiers());
        break;
    }
}

void CAssetViewer::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->pos(), event->modifiers());
        break;
    }
}

void CAssetViewer::mouseDoubleClickEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDblClk(event->pos(), event->modifiers());
        break;
    }
}

void CAssetViewer::UpdateScrollBar()
{
    int nNewOffset = m_yOffset;
    int nMaxScroll = m_idealClientHeight - m_clientRect.height();

    if (nMaxScroll < 0)
    {
        nNewOffset = 0;
    }

    if ((int)m_idealClientHeight - nNewOffset < m_clientRect.height())
    {
        nNewOffset = nMaxScroll;
    }

    if (nNewOffset < 0)
    {
        nNewOffset = 0;
    }

    m_yOffset = nNewOffset;
    verticalScrollBar()->setRange(0, m_idealClientHeight - m_clientRect.height());
    verticalScrollBar()->setValue(m_yOffset);
}

bool CAssetViewer::SortAssetsByIndex(IAssetItem* pA, IAssetItem* pB)
{
    assert(pA && pB);
    return pA->GetIndex() > pB->GetIndex();
}

void CAssetViewer::OnThumbButtonClickTags(IAssetItem* pAsset)
{
    QString tags = pAsset->GetAssetFieldValue("tags").toString();

    bool ok;
    tags = QInputDialog::getText(nullptr, tr("Asset Tags"), QString(), QLineEdit::Normal, tags, &ok);
    if (ok)
    {
        pAsset->SetAssetFieldValue("tags", &tags);
        pAsset->GetOwnerDatabase()->OnMetaDataChange(pAsset);
    }
}

HWND CAssetViewer::GetRenderWindow()
{
    return reinterpret_cast<HWND>(m_wndAssetThumbRender.winId());
}

void CAssetViewer::OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_bDragging = false;
    m_lastPanDragPt = point;

    if (m_bDraggingAssetFromBrowser && m_pClickedAsset)
    {
        m_bDraggingAssetFromBrowser = false;
        unsetCursor();
        QPoint vp = viewport()->mapToGlobal(point);
        m_pDropInViewport = GetIEditor()->GetViewManager()->GetViewportAtPoint(vp);

        // check if level is loaded
        if (m_pDropInViewport && GetIEditor()->GetGameEngine()->IsLevelLoaded())
        {
            Vec3 pos;

            m_pDropInViewport->ScreenToClient(vp);

            if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
            {
                pos = m_pDropInViewport->MapViewToCP(vp);
            }
            else
            {
                bool bHitTerrain = false;

                pos = m_pDropInViewport->ViewToWorld(vp, &bHitTerrain);

                if (bHitTerrain)
                {
                    pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);  //TODO: why 1m from ground? + 1.0f;
                }
            }

            pos = m_pDropInViewport->SnapToGrid(pos);

            m_pDraggedAssetInstance = m_pClickedAsset->CreateInstanceInViewport(pos.x, pos.y, pos.z);
            m_pDraggedAsset = m_pClickedAsset;
            m_bDraggingCreatedAssetInWorld = true;

            if (m_pDraggedAsset->IsFlagSet(IAssetItem::eFlag_CanBeMovedAfterDroppedIntoViewport))
            {
                // we set capture yet again, beginning to drag/move in the world the newly dropped asset instance
                setMouseTracking(true);
            }
            else
            {
                // end of dragging, clear up stuff
                m_pDraggedAssetInstance = NULL;
                m_pDraggedAsset = NULL;
                m_bDraggingCreatedAssetInWorld = false;
                m_bDraggingAssetFromBrowser = false;
                m_pDropInViewport = NULL;
            }
        }
    }
    else
    {
        if (m_pHoveredThumbButtonAsset
            && m_thumbMiniButtonInstances[m_pHoveredThumbButtonAsset][m_hoveredThumbButtonIndex].m_pMiniButton->m_pfnOnClick)
        {
            m_thumbMiniButtonInstances[m_pHoveredThumbButtonAsset][m_hoveredThumbButtonIndex].m_pMiniButton->m_pfnOnClick(
                m_pHoveredThumbButtonAsset);
        }

        if (m_bMouseLeftButtonDown)
        {
            m_selectionRect.setBottomRight(point);

            m_selectionRect = m_selectionRect.normalized();

            // For adjust the scrolling position to the hit test.
            // For drawing things to the client area we should
            // subtract the scroll position instead.
            QRect stCurrentRect = m_selectionRect.adjusted(0, m_yOffset, 0, m_yOffset);

            m_bDragging = false;
            m_bMouseLeftButtonDown = false;

            // For adjust the scrolling position to the hit test.
            // For drawing things to the client area we should
            // subtract the scroll position instead.
            const QPoint stHitTestPoint = point + QPoint(0, m_yOffset);

            QString     strSelectedFilename;
            QString     strDirectory;
            bool            bSelect = false;
            qint64     nTotalSelectionSize = 0;
            bool            bAddSelection = modifiers & Qt::ControlModifier;
            bool            bUnselect = modifiers & Qt::AltModifier;
            bool            bSelectedRange = modifiers & Qt::ShiftModifier;
            bool            bStartSelection = (!bAddSelection && !bUnselect);
            bool            bHitItem = false;
            bool            bClickSelection =
                (stCurrentRect.width() <= AssetViewer::kMinimumSelectionDraggingOffset
                 && stCurrentRect.height() <= AssetViewer::kMinimumSelectionDraggingOffset);

            if (m_pClickedAsset && !bClickSelection)
            {
                if (!bAddSelection && !bSelectedRange)
                {
                    DeselectAll();
                    m_pClickedAsset->SetFlag(IAssetItem::eFlag_Selected, true);
                    m_selectedAssets.push_back(m_pClickedAsset);
                }
            }
            else if (m_pClickedAsset && bSelectedRange)
            {
                if (bClickSelection && !m_selectedAssets.empty())
                {
                    IAssetItem* pLastSelectedAsset = m_selectedAssets.back();

                    int direction = pLastSelectedAsset->GetIndex() <= m_pClickedAsset->GetIndex() ? 1 : -1;

                    m_selectedAssets.resize(0);

                    // unselect all, SHIFT selection is exclusive
                    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
                    {
                        IAssetItem* const cpoCurrentItem = m_assetItems[nCurrentAsset];

                        cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, false);
                    }

                    // get last asset in selection
                    for (int iFrom = pLastSelectedAsset->GetIndex() + direction,
                         iTo = m_pClickedAsset->GetIndex();
                         (direction > 0 ? iFrom <= iTo : iFrom >= iTo);
                         iFrom += direction)
                    {
                        IAssetItem* const cpoCurrentItem = m_assetItems[iFrom];

                        if (!cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Visible))
                        {
                            continue;
                        }

                        cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, true);
                        m_selectedAssets.push_back(cpoCurrentItem);
                    }

                    pLastSelectedAsset->SetFlag(IAssetItem::eFlag_Selected, true);
                    m_selectedAssets.push_back(pLastSelectedAsset);
                }
            }
            else
            {
                m_selectedAssets.resize(0);

                for (size_t nCurrentAsset = 0, nTotalAssets = m_assetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
                {
                    IAssetItem* const cpoCurrentItem = m_assetItems[nCurrentAsset];

                    if (!cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Visible))
                    {
                        continue;
                    }

                    if (bClickSelection)
                    {
                        bHitItem = cpoCurrentItem->HitTest(stHitTestPoint.x(), stHitTestPoint.y());
                    }
                    else
                    {
                        bHitItem = cpoCurrentItem->HitTest(stCurrentRect);
                    }

                    if (bHitItem)
                    {
                        // When you're clicking over an unselected item, even in unselect mode you will
                        // want to add it to selection.
                        if (bClickSelection)
                        {
                            // When you're clicking over an unselected item, even in unselect mode you will
                            // want to add it to selection. (actually this inverses selection behavior)
                            bSelect = !cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Selected);
                            cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, bSelect);

                            if (bSelect)
                            {
                                m_selectedAssets.push_back(cpoCurrentItem);
                            }
                        }
                        else
                        {
                            // We have to add those which hit the selection unless we are unselecting items
                            // so if unselect is false, it will select items, otherwise it will unselect them
                            bSelect = !bUnselect;
                            cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, bSelect);

                            if (bSelect)
                            {
                                m_selectedAssets.push_back(cpoCurrentItem);
                            }
                        }
                    }
                    else
                    {
                        // If we are starting a new selection, the item wasn't hit in the selection,
                        // we must unselect it.
                        if (bStartSelection)
                        {
                            cpoCurrentItem->SetFlag(IAssetItem::eFlag_Selected, false);
                        }
                        else
                        {
                            if (cpoCurrentItem->IsFlagSet(IAssetItem::eFlag_Selected))
                            {
                                m_selectedAssets.push_back(cpoCurrentItem);
                            }
                        }
                    }
                }
            }

            std::sort(m_selectedAssets.begin(), m_selectedAssets.end(), SortAssetsByIndex);
        }
    }

    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
    CALL_OBSERVERS_METHOD(OnChangedPreviewedAsset(m_pClickedAsset));
    viewport()->update();
}

void CAssetViewer::OnLButtonDblClk(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());

    //
    // if we have a double clicked asset thumb, do notify it
    //
    if (!m_selectedAssets.empty())
    {
        m_selectedAssets[0]->OnThumbDblClick(point, Qt::LeftButton, modifiers);
        CALL_OBSERVERS_METHOD(OnAssetDblClick(m_selectedAssets[0]));
    }
}

void CAssetViewer::OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_bMouseRightButtonDown = true;
    m_lastPanDragPt = m_lastRightClickPt = point;
    m_smoothPanLeftAmount = 0;
    m_smoothPanLastDelta = 0;
    CheckClickedThumb(point);
    setCursor(CMFCUtils::LoadCursor(IDC_CURSOR_HAND_DRAG));
}

void CAssetViewer::OnRButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    unsetCursor();
    m_bMouseRightButtonDown = false;
    m_smoothPanLeftAmount = m_smoothPanLastDelta;

    // if we clicked an asset and we didnt moved the mouse (too much), then show the context menu
    if (m_pClickedAsset
        && (m_lastRightClickPt.x() - point.x()) <= AssetViewer::kMinimumStartPanningOffset
        && (m_lastRightClickPt.y() - point.y()) <= AssetViewer::kMinimumStartPanningOffset)
    {
        QStringList extraMenuItems;
        extraMenuItems.push_back(tr("Recache thumbnail(s)"));

        if (m_selectedAssets.size() == 1)
        {
            extraMenuItems.push_back(tr("Edit tags..."));

            const QString clickedAssetName = m_pClickedAsset->GetFilename();
            const bool isGeometry = QFileInfo(clickedAssetName).completeSuffix().compare(CRY_GEOMETRY_FILE_EXT, Qt::CaseInsensitive);
            if (!clickedAssetName.contains(QStringLiteral("_lod")) && isGeometry)
            {
                extraMenuItems.push_back(tr("Send to LOD Generator"));
            }
        }

        const QString selectedTextIfAny = CFileUtil::PopupQMenu(m_pClickedAsset->GetFilename(), m_pClickedAsset->GetRelativePath(), this,
                0, extraMenuItems);

        if (selectedTextIfAny == extraMenuItems[0])
        {
            CAssetBrowserManager::Instance()->CreateThumbsFolderPath();
            QWaitCursor wait;

            for (size_t i = 0; i < m_selectedAssets.size(); ++i)
            {
                m_selectedAssets[i]->ForceCache();
                m_selectedAssets[i]->UnloadThumbnail();
                m_selectedAssets[i]->LoadThumbnail();
            }
        }
        else if (extraMenuItems.count() >= 2 && selectedTextIfAny == extraMenuItems[1])
        {
            CAssetBrowserDialog::Instance()->OnUpdateAssetBrowserEditTags();
        }
        else if (extraMenuItems.count() >= 3 && selectedTextIfAny == extraMenuItems[2])
        {
            GetIEditor()->ExecuteCommand(QStringLiteral("lodtools.loadcgfintool '%1%2'").arg(m_pClickedAsset->GetRelativePath(), m_pClickedAsset->GetFilename()));
        }
    }

    // if we have smooth panning movement, start the smooth panning timer operation
    if (fabs(m_smoothPanLeftAmount))
    {
        m_smoothPanningLastTickCount = GetTickCount();
        m_timers[ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING] = startTimer(AssetViewer::kSmoothPanningTimerDelay);
    }
}

void CAssetViewer::OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_bMouseMiddleButtonDown = true;
    m_lastPanDragPt = point;
    m_smoothPanLeftAmount = 0;
    m_smoothPanLastDelta = 0;
    CheckClickedThumb(point);
}

void CAssetViewer::OnMButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_bMouseMiddleButtonDown = false;
    m_pClickedAsset = NULL;
}

void CAssetViewer::scrollContentsBy(int dx, int dy)
{
    m_pEnsureVisible = NULL;
    m_yOffset = verticalScrollBar()->value();
    m_smoothPanLeftAmount = 0;
    m_smoothPanLastDelta = 0;
    killTimer(m_timers[ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING]);
    UpdateScrollBar();
    UpdateVisibility();
    viewport()->update();
}

void CAssetViewer::wheelEvent(QWheelEvent* event)
{
    m_yOffset -= event->delta();
    m_pEnsureVisible = NULL;
    UpdateScrollBar();
    UpdateVisibility();
    viewport()->update();
}

void CAssetViewer::timerEvent(QTimerEvent* event)
{
    const int nIDEvent = m_timers.key(event->timerId());
    if (nIDEvent == ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING)
    {
        UpdateSmoothPanning();
        viewport()->update();
    }
    else if (nIDEvent == ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER)
    {
        m_bWarningBlinkToggler = !m_bWarningBlinkToggler;
        viewport()->update();
    }
    else if (nIDEvent == ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION)
    {
        if (!m_pEnsureVisible)
        {
            m_thumbFocusAnimationTime = 1;
            killTimer(m_timers[ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION]);
            viewport()->update();
            return;
        }

        QRect rc;

        m_pEnsureVisible->GetDrawingRectangle(rc);
        rc.translate(0, -m_yOffset);

        m_thumbFocusAnimationRect.setLeft(m_clientRect.left() + m_thumbFocusAnimationTime * (rc.left() - m_clientRect.left()));
        m_thumbFocusAnimationRect.setRight(m_clientRect.right() + m_thumbFocusAnimationTime * (rc.right() - m_clientRect.right()));
        m_thumbFocusAnimationRect.setTop(m_clientRect.top() + m_thumbFocusAnimationTime * (rc.top() - m_clientRect.top()));
        m_thumbFocusAnimationRect.setBottom(m_clientRect.bottom() + m_thumbFocusAnimationTime * (rc.bottom() - m_clientRect.bottom()));
        m_thumbFocusAnimationTime += 0.1f;

        viewport()->update();

        if (m_thumbFocusAnimationTime >= 1)
        {
            m_thumbFocusAnimationTime = 1;
            killTimer(m_timers[ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION]);
        }
    }
    else
    {
        QAbstractScrollArea::timerEvent(event);
    }
}

void CAssetViewer::UpdateSmoothPanning()
{
    if (0 == m_smoothPanLeftAmount)
    {
        return;
    }

    UINT crtTick = GetTickCount();
    UINT delta = crtTick - m_smoothPanningLastTickCount;
    m_smoothPanningLastTickCount = crtTick;
    float fDeltaTime = (float)delta / 1000.0f;
    static float sfTimerAmount = 0;
    sfTimerAmount += fDeltaTime;

    if (sfTimerAmount < AssetViewer::kSmoothPanning_Speed)
    {
        return;
    }

    sfTimerAmount = 0;
    m_smoothPanLeftAmount *= AssetViewer::kSmoothPanning_SlowdownMultiplier;
    m_yOffset += m_smoothPanLeftAmount;

    // if smaller than 1 pixel, dont bother, end smooth panning operation
    if (fabs(m_smoothPanLeftAmount) < 1.0f)
    {
        m_smoothPanLeftAmount = 0;
        m_smoothPanLastDelta = 0;
        killTimer(m_timers[ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING]);
    }

    m_pEnsureVisible = NULL;
    UpdateScrollBar();
    UpdateVisibility();
}

void CAssetViewer::RecalculateLayout()
{
    int     nCurrentTop = 0, nCurrentLeft = 0;
    UINT    thumbsPerRow = 0;
    float   horizMargin = 0;
    UINT    rowItemCount = 0;
    QRect   rstElementRect;

    // magic! compute number of thumbs per one row of asset thumbs, and the needed margin between them
    ComputeThumbsLayoutInfo(m_clientRect.width(), m_assetThumbSize, m_itemHorizontalSpacing, m_assetItems.size(), thumbsPerRow, horizMargin);

    m_thumbsPerRow = thumbsPerRow;
    nCurrentTop = m_itemVerticalSpacing;
    nCurrentLeft = horizMargin;

    for (size_t nCurrentItem = 0, nTotalItems = m_assetItems.size(); nCurrentItem < nTotalItems; ++nCurrentItem)
    {
        IAssetItem* const cpoCurrentAssetItem = m_assetItems[nCurrentItem];

        // Invisible items, due to filtering or any other reason
        // are not considered in the layout calculation...
        if (!cpoCurrentAssetItem->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            continue;
        }

        rstElementRect.setLeft(nCurrentLeft);
        rstElementRect.setRight(rstElementRect.left() + m_assetThumbSize);
        rstElementRect.setTop(nCurrentTop);
        rstElementRect.setBottom(rstElementRect.top() + m_assetThumbSize);

        cpoCurrentAssetItem->SetDrawingRectangle(rstElementRect);

        nCurrentLeft += m_assetThumbSize + horizMargin * 2;
        ++rowItemCount;

        // begin a new row of thumbs
        if (rowItemCount >= thumbsPerRow && nCurrentItem < (nTotalItems - 1))
        {
            rowItemCount = 0;
            nCurrentLeft = horizMargin;
            nCurrentTop += m_itemVerticalSpacing * 2 + m_assetThumbSize;
        }
    }

    m_idealClientHeight = nCurrentTop + m_assetThumbSize + 2 * m_itemVerticalSpacing;

    if (m_idealClientHeight > m_clientRect.height())
    {
        m_idealClientWidth = m_clientRect.width() - verticalScrollBar()->width();
    }
    else
    {
        m_idealClientWidth = m_clientRect.width();
    }
}

void CAssetViewer::UpdateVisibility()
{
    QRect oCurrentRect;

    // First we check all bitmaps which were previously visible and which are not anymore
    // and then we uncache them. The ones still visible will keep cached.
    for (TAssetItems::iterator iter = m_assetItems.begin(), iterEnd = m_assetItems.end(); iter != iterEnd; ++iter)
    {
        IAssetItem* const cpoDatabaseItem = *iter;

        // No need to cache bitmaps that should not be displayed for one reason or another...
        // specially because we have no guarantee that their positions will be valid...
        // so it is a cached item, we must remove it from the cache.
        if (!cpoDatabaseItem->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            continue;
        }

        cpoDatabaseItem->GetDrawingRectangle(oCurrentRect);
        oCurrentRect.adjust(0, -m_yOffset, 0, -m_yOffset);

        // We don't need to uncache Assets which are still visible...
        if (oCurrentRect.intersects(m_clientRect))
        {
            continue;
        }
    }

    m_assetDrawingCache.resize(0);

    for (size_t nCurrentItem = 0, nTotalItems = m_assetItems.size(); nCurrentItem < nTotalItems; ++nCurrentItem)
    {
        IAssetItem* const cpoDatabaseItem = m_assetItems[nCurrentItem];

        // No need to cache bitmaps that should not be displayed for one reason or another...
        // specially because we have no guarantee that their positions will be valid...
        if (!cpoDatabaseItem->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            continue;
        }

        cpoDatabaseItem->GetDrawingRectangle(oCurrentRect);

        // We may need to have 2 copies of the rectangles, one the drawing and one the
        // layout rectangle...
        oCurrentRect.adjust(0, -m_yOffset, 0, -m_yOffset);

        // If the item is entirely outside of the client rect, no need to cache a bitmap for it.
        if (oCurrentRect.bottom() < 0)
        {
            continue;
        }

        // If the item is entirely below the client rect, as Y coordinates grow monotonically,
        // no need to cache anything else.
        if (oCurrentRect.top() > m_clientRect.bottom())
        {
            // everything below this item is not visible anyway, exit the loop
            break;
        }

        // Added to use the same criteria as in the remove list.
        if (!m_clientRect.intersects(oCurrentRect))
        {
            continue;
        }

        m_assetDrawingCache.push_back(cpoDatabaseItem);
        m_thumbsCache.push_back(cpoDatabaseItem);
    }

    // unload older thumb images from the cache
    while (m_thumbsCache.size() >= AssetViewer::kMaxThumbsCacheAssetCount)
    {
        m_thumbsCache.front()->UnloadThumbnail();
        m_thumbsCache.pop_front();
    }

    if (m_thumbsPerRow)
    {
        m_visibleThumbRows = std::ceil(m_assetDrawingCache.size() / (double)m_thumbsPerRow);
    }
    else
    {
        m_visibleThumbRows = 0;
    }
}

void CAssetViewer::Draw()
{
    QPainter painter(viewport());

    QString     strFilename;
    QRect           stElementRect;

    m_thumbMiniButtonInstances.clear();

    for (size_t nCurrentAsset = 0, nTotalAssets = m_assetDrawingCache.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset)
    {
        IAssetItem* const cpoAssetDatabaseItem = m_assetDrawingCache[nCurrentAsset];

        cpoAssetDatabaseItem->GetDrawingRectangle(stElementRect);
        stElementRect.adjust(0, -m_yOffset, 0, -m_yOffset);

        // If the rectangle is outside, skip it
        if (!stElementRect.intersects(m_clientRect))
        {
            continue;
        }

        painter.fillRect(stElementRect, Qt::blue);
        DrawAsset(&painter, cpoAssetDatabaseItem, stElementRect);
    }

    if (m_bDragging && !m_pClickedAsset)
    {
        painter.setPen(QPen(AssetViewer::kSelectionDragLineColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_selectionRect);
    }

    CheckHoveredThumb();
    DrawThumbTooltip(&painter);
    DrawFocusThumbAnimationRect(&painter);
    DrawThumbMiniButtonTooltip(&painter);
}

void CAssetViewer::DrawAsset(QPainter* painter, IAssetItem* poItem, QRect& roUpdateRect)
{
    QString     strOutputText;
    bool            bSelected = poItem->IsFlagSet(IAssetItem::eFlag_Selected), bHovered = (poItem == m_pHoveredAsset);
    QString     strTempText, strAssetThumbOneLineInfo;
    QString     strLabel, strInfo, strAssetTags;
    QString     strTmp;
    bool            bTagged = false, bFav = false;

    QRect rcShadow = roUpdateRect;
    QRect rcBorder = roUpdateRect;
    QRect rcLabel = roUpdateRect;

    rcBorder.adjust(-5, -5, 5, 5);
    rcShadow.adjust(-15, -15, 15, 15);

    strLabel = poItem->GetAssetFieldValue("filename").toString();

    // kill extension, irrelevant
    strLabel.remove(QStringLiteral(".") + poItem->GetFileExtension());
    strAssetThumbOneLineInfo = poItem->GetAssetFieldValue("thumbOneLineText").toString();
    //strAssetTags = poItem->GetAssetFieldValue( "tags" ).toString();
    bFav = poItem->GetAssetFieldValue("favorite").toBool();

    bTagged = !strAssetTags.isEmpty();
    strOutputText = strLabel;
    painter->setFont(m_fontLabel);

    rcLabel.setTop(rcLabel.bottom() + 10);
    rcLabel.setBottom(rcLabel.top() + m_itemVerticalSpacing);
    // no text draw, just compute rect
    rcLabel = painter->fontMetrics().boundingRect(rcLabel, Qt::AlignCenter, strLabel);

    const int kBorderRadius = 0;

    //
    // draw the label
    //
    rcLabel.setLeft(roUpdateRect.left());
    rcLabel.setRight(roUpdateRect.right());
    painter->setPen(AssetViewer::kLabelTextColor);
    painter->drawText(rcLabel, Qt::AlignCenter, strLabel);

    //
    // draw one line info text for thumb
    //
    rcLabel.setTop(rcLabel.bottom() + 10);
    rcLabel.setBottom(rcLabel.top() + 15);
    painter->setPen(AssetViewer::kOneLineInfoTextColor);
    painter->setFont(m_fontOneLinerInfo);
    painter->drawText(rcLabel, Qt::AlignCenter, strAssetThumbOneLineInfo);

    //
    // draw the thumb container
    //
    painter->setPen(bHovered ? AssetViewer::kThumbContainerHoverLineColor : AssetViewer::kThumbContainerLineColor);
    painter->setBrush(AssetViewer::kThumbContainerBackColor);

    if (bSelected)
    {
        rcBorder.adjust(-4, -4, 4, 4);
    }

    painter->drawRoundRect(rcBorder, kBorderRadius, kBorderRadius);

    //
    // draw selection rect
    //
    if (poItem->IsFlagSet(IAssetItem::eFlag_Selected))
    {
        QRect stSelectionRect(roUpdateRect);

        stSelectionRect.adjust(-4, -4, 5, 5);

        painter->setPen(QPen(QColor(255, 255, 0), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundRect(stSelectionRect, kBorderRadius, kBorderRadius);
    }

    DrawThumbnail(painter, poItem, roUpdateRect);
    DrawThumbMiniButtons(poItem, rcBorder);
}

void CAssetViewer::DrawFocusThumbAnimationRect(QPainter* painter)
{
    //
    // draw thumb focus animation rect
    //
    if (m_thumbFocusAnimationTime != 1)
    {
        painter->setPen(QPen(Qt::white, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(m_thumbFocusAnimationRect);
    }
}

void CAssetViewer::DrawThumbTooltip(QPainter* painter)
{
    if (m_pHoveredAsset)
    {
        QRect       rcTitle, rcThumb;
        const QPoint point = viewport()->mapFromGlobal(QCursor::pos());

        const QString strFilename = m_pHoveredAsset->GetAssetFieldValue("filename").toString();
        const QString strAssetThumbInfo = m_pHoveredAsset->GetAssetFieldValue("relativepath").toString();
        m_pHoveredAsset->GetDrawingRectangle(rcThumb);

        QRect rcTip(QPoint(point.x(), point.y() + AssetViewer::kTooltip_Margin), QSize(AssetViewer::kTooltip_MinWidth, AssetViewer::kTooltip_MinHeight));

        painter->setFont(m_fontInfoTitle);
        const QSize titleSize = painter->fontMetrics().boundingRect(strFilename).size();

        // choose the larger one
        rcTip.setWidth(MAX(AssetViewer::kTooltip_MinWidth, titleSize.width()) + AssetViewer::kTooltip_Margin);
        QRect rcInfo = rcTip;

        // compute text bounds, no draw
        painter->setFont(m_fontInfoText);
        const QString strAssetThumbInfoFinal = strAssetThumbInfo;
        rcInfo = painter->fontMetrics().boundingRect(rcInfo, Qt::TextWordWrap, strAssetThumbInfoFinal);

        rcTip.setHeight(titleSize.height() + AssetViewer::kTooltip_Margin + rcInfo.height() + AssetViewer::kTooltip_TitleContentSpacing);
        rcTip.setWidth(max(rcInfo.width() + AssetViewer::kTooltip_Margin, rcTip.width()));

        //
        // relocate tip box if is overlapping the thumb
        //

        rcThumb.translate(0, -m_yOffset);

        int deltaX = rcTip.left() - rcThumb.right();
        int deltaY = rcTip.top() - rcThumb.bottom();

        if (deltaX < 0 && deltaY < 0)
        {
            if (abs(deltaX) >= abs(deltaY))
            {
                rcTip.translate(0, -deltaY);
            }
            else
            {
                rcTip.translate(-deltaX, 0);
            }
        }

        //
        // relocate tip box, if out of bounds
        //
        if (rcTip.right() > m_clientRect.width())
        {
            int delta = rcTip.right() - m_clientRect.width();
            rcTip.moveLeft(rcTip.left() - delta);
        }

        if (rcTip.bottom() > m_clientRect.height())
        {
            int delta = rcTip.bottom() - m_clientRect.height();
            rcTip.moveTop(rcTip.top() - delta);
        }

        // draw shadow
        painter->drawPixmap(rcTip, m_tooltipBackBmp);

        //
        // draw tip box round rectangle
        //
        painter->setPen(QPen(AssetViewer::kTooltip_BorderColor, AssetViewer::kTooltip_BorderWidth));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rcTip, AssetViewer::kTooltip_BorderCornerSize, AssetViewer::kTooltip_BorderCornerSize);

        //
        // draw small info icon
        //
        const QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
        painter->drawPixmap(rcTip.bottomRight() - QPoint(32, 32), icon.pixmap(32, 32));

        //
        // draw filename
        //
        painter->setFont(m_fontInfoTitle);
        painter->setPen(AssetViewer::kTooltip_FilenameShadowColor);
        painter->drawText(rcTip.topLeft() + QPoint(12, 12), strFilename);
        painter->setPen(AssetViewer::kTooltip_FilenameColor);
        painter->drawText(rcTip.topLeft() + QPoint(10, 10), strFilename);

        //
        // draw info text lines
        //
        QRect rcTextInfo = rcTip;
        rcTextInfo.setLeft(rcTextInfo.left() + 10);
        rcTextInfo.setRight(rcTextInfo.right() - 10);
        rcTextInfo.setTop(rcTextInfo.top() + titleSize.height() + 15);
        rcTextInfo.setBottom(rcTextInfo.bottom() - 10);
        painter->setPen(AssetViewer::kInfoTextColor);
        painter->setFont(m_fontInfoText);
        painter->drawText(rcTextInfo, Qt::TextWordWrap, strAssetThumbInfoFinal);
    }
}

void CAssetViewer::DrawThumbMiniButtonTooltip(QPainter* painter)
{
    //
    // draw the tooltip for the mini thumb button currently hovered, if any
    //
    if (!m_thumbMiniButtonInstances.empty() && m_pHoveredThumbButtonAsset != NULL)
    {
        if (!m_thumbMiniButtonInstances[m_pHoveredThumbButtonAsset].empty()
            && m_hoveredThumbButtonIndex >= 0
            && m_hoveredThumbButtonIndex < m_thumbMiniButtonInstances[m_pHoveredThumbButtonAsset].size())
        {
            CThumbMiniButtonInstance& btnInst = m_thumbMiniButtonInstances[m_pHoveredThumbButtonAsset][m_hoveredThumbButtonIndex];
            QRect rcTip;
            QRect rcBtn = btnInst.m_rect;
            QFont fntTip(QStringLiteral("Arial"), 6);
            QPen pen;
            QSize textSize;

            painter->setFont(fntTip);
            textSize = QFontMetrics(fntTip).boundingRect(btnInst.m_pMiniButton->m_toolTip).size();
            rcTip.setLeft(rcBtn.right() + 1);
            rcTip.setRight(rcTip.left() + textSize.width() + 10);
            rcTip.setTop(rcBtn.top());
            rcTip.setBottom(rcBtn.bottom());
            painter->fillRect(rcTip, AssetViewer::kMiniThumbButton_BackColor);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(AssetViewer::kMiniThumbButton_BorderColor);
            painter->drawRect(rcTip);
            painter->setPen(AssetViewer::kMiniThumbButton_TextColor);
            painter->drawText(rcTip, Qt::TextSingleLine | Qt::AlignCenter, btnInst.m_pMiniButton->m_toolTip);
        }
    }
}

void CAssetViewer::DrawThumbnail(QPainter* painter, IAssetItem* const poItem, const QRect& roUpdateRect)
{
    // cache items that were not cached in the caching thread, due to no eFlag_ThreadCachingSupported flag
    if (!poItem->IsFlagSet(IAssetItem::eFlag_ThumbnailLoaded))
    {
        CAssetBrowserManager::Instance()->EnqueueAssetForThumbLoad(poItem);
        poItem->LoadThumbnail();
    }

    if (poItem->IsFlagSet(IAssetItem::eFlag_Invalid))
    {
        // draw a INVALID ASSET placeholder
        painter->drawPixmap(roUpdateRect, m_thumbInvalidAssetBmp);
    }
    else if (!poItem->DrawThumbImage(painter, roUpdateRect))
    {
        painter->drawPixmap(roUpdateRect, m_thumbLoadingBmp);
    }
}

void CAssetViewer::DrawThumbMiniButtons(IAssetItem* const poItem, const QRect& rcThumbBorder)
{
}

void CAssetViewer::keyPressEvent(QKeyEvent* event)
{
    if (m_bMouseLeftButtonDown)
    {
        if (m_pClickedAsset)
        {
            const bool bAltClick = (event->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (event->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (event->modifiers() & Qt::ShiftModifier);
            const int nFlags = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                (bAltClick ? MK_ALT : 0);
            m_pClickedAsset->OnPreviewRenderKeyEvent(true, event->nativeVirtualKey(), nFlags);
        }
    }

    if (event->key() == AssetViewer::kPreviewShortcutKey && !m_selectedAssets.empty())
    {
        // choose what item to preview: if we hover an asset, show that, if we don't hover any and have a selected asset, show that one instead
        IAssetItem* pAsset = m_selectedAssets[0];

        CALL_OBSERVERS_METHOD(OnSelectionChanged());
        CALL_OBSERVERS_METHOD(OnChangedPreviewedAsset(pAsset));
    }

    if (m_bDraggingAssetFromBrowser || m_bDraggingCreatedAssetInWorld)
    {
        if (m_pDraggedAsset && m_pDraggedAssetInstance)
        {
            m_pDraggedAsset->AbortCreateInstanceInViewport(m_pDraggedAssetInstance);
        }

        // end of dragging, clear up stuff
        m_pDraggedAssetInstance = NULL;
        m_pDraggedAsset = NULL;
        m_bDraggingCreatedAssetInWorld = false;
        m_bDraggingAssetFromBrowser = false;
        m_pDropInViewport = NULL;
        setCursor(Qt::ArrowCursor);
    }

    //
    // navigate using the ARROW KEYS and PGUP PGDOWN HOME END
    //
    if (m_assetItems.size())
    {
        IAssetItem* pAsset = NULL;
        bool bHasSelection = !m_selectedAssets.empty(), bEpicFail = false;

        if (bHasSelection)
        {
            pAsset = m_selectedAssets[0];
        }
        else
        {
            pAsset = m_assetItems[0];
        }

        int index = 0;

        // search asset's index
        if (pAsset)
        {
            index = pAsset->GetIndex();
        }

        switch (event->key())
        {
        case Qt::Key_Down:
        {
            index += m_thumbsPerRow;

            if (index < m_assetItems.size())
            {
                SelectAsset(m_assetItems[index]);
                EnsureAssetVisible(index);
            }
            else
            {
                bEpicFail = true;
            }

            break;
        }

        case Qt::Key_Up:
        {
            index -= m_thumbsPerRow;

            if (index >= 0)
            {
                SelectAsset(m_assetItems[index]);
                EnsureAssetVisible(index);
            }
            else
            {
                bEpicFail = true;
            }

            break;
        }

        case Qt::Key_Left:
        {
            --index;

            if (index < 0)
            {
                index = 0;
            }

            SelectAsset(m_assetItems[index]);
            EnsureAssetVisible(index);
            break;
        }

        case Qt::Key_Right:
        {
            ++index;

            if (index >= m_assetItems.size())
            {
                index = m_assetItems.size() - 1;
            }

            SelectAsset(m_assetItems[index]);
            EnsureAssetVisible(index);
            break;
        }

        case Qt::Key_PageDown:
        {
            index += m_thumbsPerRow * m_visibleThumbRows;

            if (index < m_assetItems.size())
            {
                SelectAsset(m_assetItems[index]);
                EnsureAssetVisible(index);
            }
            else
            {
                SelectAsset(m_assetItems[m_assetItems.size() - 1]);
                EnsureAssetVisible(m_assetItems.size() - 1);
            }

            break;
        }

        case Qt::Key_PageUp:
        {
            index -= m_thumbsPerRow * m_visibleThumbRows;

            if (index >= 0)
            {
                SelectAsset(m_assetItems[index]);
                EnsureAssetVisible(index);
            }
            else
            {
                SelectAsset(m_assetItems[0]);
                EnsureAssetVisible(m_assetItems[0]);
            }

            break;
        }

        case Qt::Key_Home:
        {
            SelectAsset(m_assetItems[0]);
            EnsureAssetVisible(m_assetItems[0]);
            break;
        }

        case Qt::Key_End:
        {
            SelectAsset(m_assetItems[m_assetItems.size() - 1]);
            EnsureAssetVisible(m_assetItems.size() - 1);
            break;
        }
        }

        // if we couldn't advance, then "failback" to previous selection
        if (bHasSelection && bEpicFail && pAsset)
        {
            // just set the old selected asset back
            SelectAsset(pAsset);
            EnsureAssetVisible(pAsset);
        }
    }
}

void CAssetViewer::keyReleaseEvent(QKeyEvent* event)
{
    if (m_bMouseLeftButtonDown)
    {
        if (m_pClickedAsset)
        {
            const bool bAltClick = (event->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (event->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (event->modifiers() & Qt::ShiftModifier);
            const int nFlags = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                (bAltClick ? MK_ALT : 0);
            m_pClickedAsset->OnPreviewRenderKeyEvent(false, event->nativeVirtualKey(), nFlags);
        }
    }
}

void CAssetViewer::RecalculateTotalAssetCount()
{
    m_assetTotalCount = 0;

    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        IAssetItemDatabase::TFilenameAssetMap& rAssets = m_assetDatabases[i]->GetAssets();
        m_assetTotalCount += rAssets.size();
    }
}

TAssetItems& CAssetViewer::GetAssetItems()
{
    return m_assetItems;
}

bool CAssetViewer::IsAssetVisibleInView(IAssetItem* pAsset)
{
    assert(pAsset);

    if (!pAsset)
    {
        return false;
    }

    if (!pAsset->IsFlagSet(IAssetItem::eFlag_Visible))
    {
        return false;
    }

    QRect oCurrentRect;

    pAsset->GetDrawingRectangle(oCurrentRect);
    oCurrentRect.adjust(0, -m_yOffset, 0, -m_yOffset);

    // If the item is entirely outside of the client rect, no need to cache a bitmap for it.
    if (oCurrentRect.bottom() < 0)
    {
        return false;
    }

    // If the item is entirely below the client rect, as Y coordinates grow monotonically,
    // no need to cache anything else.
    if (oCurrentRect.top() > m_clientRect.bottom())
    {
        return false;
    }

    // Added to use the same criteria as in the remove list.
    if (!m_clientRect.intersects(oCurrentRect))
    {
        return false;
    }

    return true;
}

QString CAssetViewer::GetAssetFieldDisplayValue(IAssetItem* pAsset, const char* pFieldname)
{
    assert(pAsset);
    assert(pAsset->GetOwnerDatabase());
    SAssetField* pField = pAsset->GetOwnerDatabase()->GetAssetFieldByName(pFieldname);

    if (!pField)
    {
        return "-";
    }

    switch (pField->m_fieldType)
    {
    case SAssetField::eType_Bool:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        bool value = v.toBool();

        if (!v.isNull())
        {
            return value ? "Yes" : "No";
        }

        break;
    }

    case SAssetField::eType_Int8:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        char value = v.toInt();

        if (!v.isNull())
        {
            return QString::number(value);
        }

        break;
    }

    case SAssetField::eType_Int16:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        short int value = v.toInt();

        if (!v.isNull())
        {
            return QString::number(value);
        }

        break;
    }

    case SAssetField::eType_Int32:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        int value = v.toInt();

        if (!v.isNull())
        {
            return QString::number(value);
        }

        break;
    }

    case SAssetField::eType_Int64:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        qint64 value = v.toLongLong();

        if (!v.isNull())
        {
            return QString::number(value);
        }

        break;
    }

    case SAssetField::eType_Float:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        float value = v.toDouble();

        if (!v.isNull())
        {
            return QString::number(value, 'f', 4);
        }

        break;
    }

    case SAssetField::eType_Double:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        double value = v.toDouble();

        if (!v.isNull())
        {
            return QString::number(value);
        }

        break;
    }

    case SAssetField::eType_String:
    {
        QVariant v = pAsset->GetAssetFieldValue(pFieldname);
        QString value = v.toString();

        if (!v.isNull())
        {
            return value;
        }
    }
    }

    return QStringLiteral("-");
}

//------------------------------------------------------------------------------
// SORTING ASSETS
//------------------------------------------------------------------------------
bool CAssetViewer::SortAssetItems(IAssetItem* pA, IAssetItem* pB)
{
    assert(pA);
    assert(pB);

    if (!pA || !pB)
    {
        return false;
    }

    switch (CAssetViewer::s_pSortField->m_fieldType)
    {
    case SAssetField::eType_Bool:
    {
        return CompareAssetFieldsValues<bool>(pA, pB);
    }

    case SAssetField::eType_Int8:
    {
        return CompareAssetFieldsValues<char>(pA, pB);
    }

    case SAssetField::eType_Int16:
    {
        return CompareAssetFieldsValues<short int>(pA, pB);
    }

    case SAssetField::eType_Int32:
    {
        return CompareAssetFieldsValues<int>(pA, pB);
    }

    case SAssetField::eType_Int64:
    {
        return CompareAssetFieldsValues<qint64>(pA, pB);
    }

    case SAssetField::eType_Float:
    {
        return CompareAssetFieldsValues<float>(pA, pB);
    }

    case SAssetField::eType_Double:
    {
        return CompareAssetFieldsValues<double>(pA, pB);
    }

    case SAssetField::eType_String:
    {
        return CompareAssetFieldsValues<QString>(pA, pB);
    }
    }

    return false;
}

void CAssetViewer::SortAssets(const char* pFieldname, bool bDescending)
{
    if (m_assetItems.empty())
    {
        return;
    }

    CAssetViewer::s_bSortDescending = bDescending;
    CAssetViewer::s_pSortField = m_assetItems[0]->GetOwnerDatabase()->GetAssetFieldByName(pFieldname);

    if (CAssetViewer::s_pSortField)
    {
        std::sort(m_assetItems.begin(), m_assetItems.end(), SortAssetItems);
    }

    RecalculateLayout();
    UpdateScrollBar();
    UpdateVisibility();
    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
}

void CAssetViewer::SetFilters(const IAssetItemDatabase::TAssetFieldFiltersMap& rFieldFilters)
{
    m_currentFilters = rFieldFilters;
}

void CAssetViewer::ApplyFilters(const IAssetItemDatabase::TAssetFieldFiltersMap& rFieldFilters)
{
    m_currentFilters = rFieldFilters;
#ifdef DEBUG
    Log("------------- Debug field filters");
    for (auto iter = rFieldFilters.begin(); iter != rFieldFilters.end(); ++iter)
    {
        Log("FilterField: name:'%s', val:'%s', maxval:'%s', type:%d, cond:%d",
            iter->second.m_fieldName.toUtf8().data(),
            iter->second.m_filterValue.toUtf8().data(),
            iter->second.m_maxFilterValue.toUtf8().data(),
            iter->second.m_fieldType,
            iter->second.m_filterCondition
            );
    }
#endif

    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        m_assetDatabases[i]->ApplyFilters(rFieldFilters);
    }

    m_assetItems.clear();
    m_thumbsCache.clear();
    m_assetDrawingCache.clear();

    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        IAssetItemDatabase::TFilenameAssetMap& rAssets = m_assetDatabases[i]->GetAssets();

        // add items in the list if not already added
        for (IAssetItemDatabase::TFilenameAssetMap::iterator iter = rAssets.begin(), iterEnd = rAssets.end(); iter != iterEnd; ++iter)
        {
            // skip this one, is filtered out
            if (!iter->second->IsFlagSet(IAssetItem::eFlag_Visible))
            {
                continue;
            }

            iter->second->SetIndex(m_assetItems.size());
            m_assetItems.push_back(iter->second);
        }
    }

    m_yOffset = 0;
    RecalculateLayout();
    UpdateScrollBar();
    UpdateVisibility();
    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
    CALL_OBSERVERS_METHOD(OnAssetFilterChanged());
    viewport()->update();
}

const IAssetItemDatabase::TAssetFieldFiltersMap& CAssetViewer::GetCurrentFilters()
{
    return m_currentFilters;
}

void CAssetViewer::ClearFilters()
{
    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        m_assetDatabases[i]->ClearFilters();
    }

    m_assetItems.clear();
    m_thumbsCache.clear();
    m_assetDrawingCache.clear();
    m_currentFilters.clear();

    for (size_t i = 0, iCount = m_assetDatabases.size(); i < iCount; ++i)
    {
        IAssetItemDatabase::TFilenameAssetMap& rAssets = m_assetDatabases[i]->GetAssets();

        // add items in the list if not already added
        for (IAssetItemDatabase::TFilenameAssetMap::iterator iter = rAssets.begin(), iterEnd = rAssets.end(); iter != iterEnd; ++iter)
        {
            // skip this one, is filtered out
            if (!iter->second->IsFlagSet(IAssetItem::eFlag_Visible))
            {
                continue;
            }

            iter->second->SetIndex(m_assetItems.size());
            m_assetItems.push_back(iter->second);
        }
    }

    m_yOffset = 0;
    RecalculateLayout();
    UpdateScrollBar();
    UpdateVisibility();
    CALL_OBSERVERS_METHOD(OnChangeStatusBarInfo(m_selectedAssets.size(), m_assetItems.size(), m_assetTotalCount));
    CALL_OBSERVERS_METHOD(OnSelectionChanged());
}

void CAssetViewer::EnsureAssetVisible(UINT aIndex, bool bPlayFocusAnimation)
{
    if (aIndex >= m_assetItems.size())
    {
        m_pEnsureVisible = NULL;
        return;
    }

    m_pEnsureVisible = m_assetItems[aIndex];
    EnsureAssetVisible(m_pEnsureVisible, bPlayFocusAnimation);
}

void CAssetViewer::EnsureAssetVisible(IAssetItem* pAsset, bool bPlayFocusAnimation)
{
    m_pEnsureVisible = pAsset;
    m_smoothPanLastDelta = 0;
    m_smoothPanLeftAmount = 0;

    if (m_pEnsureVisible)
    {
        m_pEnsureVisible->SetFlag(IAssetItem::eFlag_Selected, true);

        if (m_pEnsureVisible->IsFlagSet(IAssetItem::eFlag_Visible))
        {
            QRect oElementRect, oDrawRect;

            m_pEnsureVisible->GetDrawingRectangle(oElementRect);
            oDrawRect = oElementRect;
            oDrawRect.translate(0, -m_yOffset);

            if (oDrawRect.top() < 0 || oDrawRect.bottom() > m_clientRect.height())
            {
                m_yOffset = oElementRect.top() - m_itemVerticalSpacing;
            }
        }
    }

    UpdateScrollBar();
    UpdateVisibility();

    if (bPlayFocusAnimation)
    {
        m_thumbFocusAnimationTime = 0;
        killTimer(m_timers[ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION]);
        m_timers[ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION] = startTimer(AssetViewer::kThumbFocusAnimationTimerDelay);
    }

    viewport()->update();
}

void CAssetViewer::EnsureAssetVisibleAfterBrowserOpened(IAssetItem* pAsset)
{
    m_pEnsureVisibleOnBrowserOpen = pAsset;
}

#include <Controls/AssetViewer.moc>
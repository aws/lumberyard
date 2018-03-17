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

// Description : This file declares a control which objective is to display
//               multiple Assets allowing selection and preview of such things.
//               It also must handle scrolling and changes in the Asset
//               cell display size.

#ifndef CRYINCLUDE_EDITOR_CONTROLS_ASSETVIEWER_H
#define CRYINCLUDE_EDITOR_CONTROLS_ASSETVIEWER_H
#pragma once
#include "MultiThread_Containers.h"
#include "Include/IAssetViewer.h"
#include "Include/IAssetItemDatabase.h"
#include "Util/GdiUtil.h"
#include "Util/Observable.h"
#include "AssetBrowser/AssetBrowserCommon.h"

#include <QAbstractScrollArea>
#include <QMap>

// Description:
//      This class is the main control to display and browser assets by their thumbnail images, also it has tag editing and other functionalities
class CAssetViewer
    : public QAbstractScrollArea
    , public IAssetViewer
    , public CObservable<IAssetViewerObserver>
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    // needed only because we are inheriting from a pure interface
    IMPLEMENT_OBSERVABLE_METHODS(IAssetViewerObserver);

    // Description:
    //      This struct is used to hold info about mini buttons found on the thumbnail side
    struct CThumbMiniButton
    {
        // callback type used when a thumbnail mini button is clicked
        typedef void (* TClickCallback)(IAssetItem* pAsset);

        CThumbMiniButton()
        {
            m_pfnOnClick = NULL;
        }

        TClickCallback m_pfnOnClick;
        QString m_toolTip;
    };

    // Description:
    //      This struct is used to hold info about mini buttons found on the thumbnail side
    struct CThumbMiniButtonInstance
    {
        CThumbMiniButtonInstance()
        {
            m_pMiniButton = NULL;
            m_pAsset = NULL;
            m_bOn = false;
        }

        CThumbMiniButtonInstance(CThumbMiniButton* pMiniButton, const QRect& rRect, IAssetItem* pAsset, bool bOn)
        {
            m_pMiniButton = pMiniButton;
            m_rect = rRect;
            m_pAsset = pAsset;
            m_bOn = bOn;
        }

        CThumbMiniButton* m_pMiniButton;
        QRect m_rect;
        IAssetItem* m_pAsset;
        bool m_bOn;
    };

    // For each visible asset, we will have a vector of thumb button instances
    typedef std::map<IAssetItem*, std::vector<CThumbMiniButtonInstance> > TThumbMiniButtonInstances;

    CAssetViewer(QWidget* parent = nullptr);
    ~CAssetViewer();
    void FreeData();

    void SetDatabases(TAssetDatabases& rcDatabases);
    void AddDatabase(IAssetItemDatabase* pDB);
    void RemoveDatabase(IAssetItemDatabase* pDB);
    TAssetDatabases& GetDatabases();
    void ClearDatabases();
    void SortAssets(const char* pFieldname, bool bDescending = false);
    void SetFilters(const IAssetItemDatabase::TAssetFieldFiltersMap& rFieldFilters);
    void ApplyFilters(const IAssetItemDatabase::TAssetFieldFiltersMap& rFieldFilters);
    const IAssetItemDatabase::TAssetFieldFiltersMap& GetCurrentFilters();
    void ClearFilters();
    void EnsureAssetVisible(UINT aIndex, bool bPlayFocusAnimation = false);
    void EnsureAssetVisible(IAssetItem* pAsset, bool bPlayFocusAnimation = false);
    void EnsureAssetVisibleAfterBrowserOpened(IAssetItem* pAsset);
    void SetAssetThumbSize(const unsigned int nThumbSize);
    UINT GetAssetThumbSize();
    void GetSelectedItems(TAssetItems& rSelectedItems);
    int GetFirstSelectedItemIndex();
    void SelectAsset(IAssetItem* pAsset);
    void SelectAssets(const TAssetItems& rSelectedItems);
    IAssetItem* FindAssetByFullPath(const char* pFilename);
    void DeselectAll(bool bCallObservers = true);
    HWND GetRenderWindow() override;
    TAssetItems& GetAssetItems();
    bool IsAssetVisibleInView(IAssetItem* pAsset);
    void UpdateSmoothPanning();
    static QString GetAssetFieldDisplayValue(IAssetItem* pAsset, const char* pFieldname);
    void RecalculateTotalAssetCount();
    // from IEditorNotifyListener
    // Here we receive the idle updates, from which we call the rendering method
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnLButtonDblClk(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void scrollContentsBy(int dx, int dy) override;
    void wheelEvent(QWheelEvent* event) override;
    void OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void timerEvent(QTimerEvent* event) override;
    void OnDestroy();
    void keyReleaseEvent(QKeyEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // Description:
    //      Calculates the layout of all visible assets
    void RecalculateLayout();
    // Description:
    //      Calculates the currently visible elements.
    void UpdateVisibility();
    // Description:
    //      Update scroll bar range and vertical offset
    void UpdateScrollBar();
    // Description:
    //      Draw all ( asset thumbs, tooltip, etc. )
    void Draw();
    // Description:
    //      Draw a single asset
    // Arguments:
    //      poItem - the item do be drawn
    //      roDrawRect - where to draw the item
    void DrawAsset(QPainter* painter, IAssetItem* poItem, QRect& roDrawRect);
    void DrawThumbTooltip(QPainter* dc);
    void DrawThumbMiniButtonTooltip(QPainter* dc);
    void DrawFocusThumbAnimationRect(QPainter* painter);
    void DrawThumbnail(QPainter* painter, IAssetItem* const poItem, const QRect& roUpdateRect);
    void DrawThumbMiniButtons(IAssetItem* const poItem, const QRect& rcThumbBorder);
    // Description:
    //      Check for the currently clicked thumbnail, will set the m_poClickedAsset member
    // Arguments:
    //      nFlags - keyboard/mouse flags, see WM_LBUTTONDOWN for example
    //      point - the current mouse position, relative to the control
    //      bShowInteractiveRenderWnd - if true, the small thumb window will be placed and shown on the thumb spot if some asset is clicked
    void CheckClickedThumb(const QPoint& point, bool bShowInteractiveRenderWnd = false);
    void CheckHoveredThumb();
    // Description:
    //      Function used by the sorting mechanism
    // Arguments:
    //      pA - first item to compare
    //      pB - second item to compare
    // Return Value:
    //      true if pA must be before pB
    static bool SortAssetItems(IAssetItem* pA, IAssetItem* pB);
    static bool SortAssetsByIndex(IAssetItem* pA, IAssetItem* pB);
    static void OnThumbButtonClickTags(IAssetItem* pAsset);
    static void OnThumbButtonClickFavorite(IAssetItem* pAsset);

    // Description:
    //      Used for shortening the code in the switch-case construct, in CAssetViewer::SortAssetItems
    template<class T>
    static inline bool CompareAssetFieldsValues(IAssetItem* pA, IAssetItem* pB)
    {
        QVariant vA = pA->GetAssetFieldValue(s_pSortField->m_fieldName.toUtf8().data());
        QVariant vB = pB->GetAssetFieldValue(s_pSortField->m_fieldName.toUtf8().data());
        T valueA = vA.value<T>();
        T valueB = vB.value<T>();

        if (vA.isNull() || vB.isNull())
        {
            return false;
        }

        return s_bSortDescending ? valueA > valueB : valueA < valueB;
    }

protected:
    // current asset field used in sorting of the assets
    static SAssetField* s_pSortField;
    // used for the sorting function
    static bool s_bSortDescending;
    // reference count of the class
    ULONG m_ref;
    // the current asset items rendered in this viewer
    TAssetItems m_assetItems;
    // the current visible/drawing asset thumbs in the control
    TAssetItems m_assetDrawingCache;
    // the list of currently selected assets
    TAssetItems m_selectedAssets;
    // the databases currently visible and used in the control
    TAssetDatabases m_assetDatabases;
    // the total number of assets from all databases and all non filtered items
    UINT m_assetTotalCount;
    // the thumbs cache used to hold track of the generated thumbs, it can have a certain size, afterwards it starts discarding older items' thumbnail image
    std::deque<IAssetItem*> m_thumbsCache;
    // point used for dragging a rectangle to select items
    QPoint m_startDraggingPoint;
    // true if mouse left button is down
    bool m_bMouseLeftButtonDown;
    // true if mouse right button is down
    bool m_bMouseRightButtonDown;
    // true if mouse middle/wheel button is down
    bool m_bMouseMiddleButtonDown;
    // true if dragging of a selection rectangle is in process
    bool m_bDragging;
    // the current selection rectangle
    QRect m_selectionRect;
    // horizontal spacing between thumbs
    UINT m_itemHorizontalSpacing;
    // vertical spacing between thumbs
    UINT m_itemVerticalSpacing;
    // the asset thumb size (width and height are always equal)
    UINT m_assetThumbSize;
    // the current offset of the thumbs canvas, depends on the scrolling position and the vertical scrollbar
    int m_yOffset;
    // the amount used to pan vertically the thumbs canvas; the remaining amount which is decreased by a timer to have a smooth animated pan effect
    float m_smoothPanLeftAmount;
    // the last delta value between two dragging mouse actions, used in smooth panning
    int m_smoothPanLastDelta;
    // the client rectangle changes only on control resize
    QRect m_clientRect;
    // the width of the virtual canvas on which thumbs are drawn, used for computing scrollbar ranges
    UINT m_idealClientWidth;
    // the height of the virtual canvas on which thumbs are drawn, used for computing scrollbar ranges
    UINT m_idealClientHeight;
    // if not NULL, this asset item will be always kept into the view
    IAssetItem* m_pEnsureVisible;
    IAssetItem* m_pEnsureVisibleOnBrowserOpen;
    // the currently clicked asset item
    IAssetItem* m_pClickedAsset;
    // the currently hovered asset item
    IAssetItem* m_pHoveredAsset;
    // the pointer to the current engine renderer
    IRenderer* m_pRenderer;
    // the (child) window used to render thumbnails, and sometimes if the asset type supports it, the user can interact with the asset in this window
    // placed at the selected thumb position
    QWidget m_wndAssetThumbRender;
    QPixmap // bitmap used for the asset thumb loading state
        m_thumbLoadingBmp,
        m_tooltipBackBmp,
    // bitmap used for the invalid asset thumb state
        m_thumbInvalidAssetBmp;
    // font for the thumb labels
    QFont m_fontLabel,
    // font for the info text found in the tooltip over asset thumbs
          m_fontInfoText,
    // font for the tooltip info title (asset filename)
          m_fontInfoTitle,
    // font for the text below the thumb label, used for additional asset-type-specific info, on one single line
          m_fontOneLinerInfo;
    // used for drag panning of the thumbs canvas, relative to control
    QPoint m_lastPanDragPt,
    // used to remember the last right mouse click location, relative to control
           m_lastRightClickPt;
    // will be set to true by the caching thread when the control must be redrawn
    volatile bool m_bMustRedraw;
    // true when the user is currently dragging asset from browser to viewports
    bool m_bDraggingAssetFromBrowser,
    // true when the asset was dragged and mouse button released, the user is now moving the created asset in the world
    // a second click will set the translation and end creation process, and ESCAPE will abort creation
         m_bDraggingCreatedAssetInWorld;
    // the dragged and created asset instance, depends on the asset, usually a CBaseObject
    void* m_pDraggedAssetInstance;
    // the viewport on which the user has dropped the dragged asset
    CViewport* m_pDropInViewport;
    // the actual viewport dragged asset
    IAssetItem* m_pDraggedAsset;
    // used for smooth panning with right mouse button drag, timing, last tick count (GetTickCount)
    UINT m_smoothPanningLastTickCount;
    // true when a red rectangle is preset and must be drawn on assets which have the eAssetFlag_HasErrors flag on
    bool m_bWarningBlinkToggler;
    // when the user select an asset in the list view, the browser will animate a focus rect towards the selected asset thumb
    // this is the animated rectangle
    QRect m_thumbFocusAnimationRect;
    // used in animating the focusing rect for the selected asset thumb, between 0..1, 1 is completed animation
    float m_thumbFocusAnimationTime;
    // asset thumbs per one row
    UINT m_thumbsPerRow,
    // the current visible asset thumb row count
         m_visibleThumbRows;
    // thumb mini buttons templates, found on the side of each asset thumb, for example: Edit tags, Toggle favorite
    std::vector<CThumbMiniButton*> m_thumbMiniButtons;
    // the instances with position and state, of the thumb mini buttons for all the visible in-view asset thumbs
    TThumbMiniButtonInstances m_thumbMiniButtonInstances;
    // the currently asset which has a mini thumb button hovered
    IAssetItem* m_pHoveredThumbButtonAsset;
    // the currently hovered mini thumb button index (in the m_thumbMiniButtons vector)
    UINT m_hoveredThumbButtonIndex;
    // the current used assets in the loaded level
    std::set<QString> m_usedAssetsInLevel;
    IAssetItemDatabase::TAssetFieldFiltersMap m_currentFilters;

    QMap<int, int> m_timers;
};
#endif // CRYINCLUDE_EDITOR_CONTROLS_ASSETVIEWER_H

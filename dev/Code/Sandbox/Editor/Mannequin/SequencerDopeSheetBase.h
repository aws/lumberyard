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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEETBASE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEETBASE_H
#pragma once


#include "ISequencerSystem.h"
#include <Mannequin/Controls/FragmentBrowser.h>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include "SequencerNode.h"

#include <QWidget>
#include <QMimeData>

class QRubberBand;
class QScrollBar;

class CSequencerTrackPropsDialog;
class CSequencerKeyPropertiesDlg;
class CSequencerNode;

// CSequencerKeys

enum ESequencerActionMode
{
    SEQMODE_MOVEKEY = 1,
    SEQMODE_ADDKEY,
    SEQMODE_SLIDEKEY,
    SEQMODE_SCALEKEY,
};

enum ESequencerSnappingMode
{
    SEQKEY_SNAP_NONE = 0,
    SEQKEY_SNAP_TICK,
    SEQKEY_SNAP_MAGNET,
    SEQKEY_SNAP_FRAME,
};

enum ESequencerTickMode
{
    SEQTICK_INSECONDS = 0,
    SEQTICK_INFRAMES,
};

enum ESequencerKeyBitmaps
{
    SEQBMP_ERROR = 0,
    SEQBMP_TAGS,
    SEQBMP_FRAGMENTID,
    SEQBMP_ANIMLAYER,
    SEQBMP_PROCLAYER,
    SEQBMP_PARAMS,
    SEQBMP_TRANSITION
};

/** Base class for Sequencer key editing dialogs.
*/
class CSequencerDopeSheetBase
    : public QWidget
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    enum EDSRenderFlags
    {
        DRAW_BACKGROUND = 1 << 0,
        DRAW_HANDLES    = 1 << 1,
        DRAW_NAMES      = 1 << 2,
    };

    enum EDSSourceControlResponse
    {
        SOURCE_CONTROL_UNAVAILABLE = 0,
        USER_ABORTED_OPERATION,
        FAILED_OPERATION,
        SUCCEEDED_OPERATION,
    };

    struct Item
    {
        int nHeight;
        _smart_ptr<CSequencerTrack> track;
        _smart_ptr<CSequencerNode> node;
        int paramId;
        bool bSelected;
        bool bReadOnly;
        //////////////////////////////////////////////////////////////////////////
        Item() { nHeight = 16; track = 0; node = 0; paramId = 0; bSelected = false; bReadOnly = false; }
        Item(int height) { nHeight = height; track = 0; node = 0; paramId = 0; bSelected = false; bReadOnly = false; }
        Item(CSequencerNode* pNode, int nParamId, CSequencerTrack* pTrack)
        {
            nHeight = 16;
            track = pTrack;
            paramId = nParamId;
            node = pNode;
            bSelected = false;
            bReadOnly = false;
        }
        Item(int height, CSequencerNode* pNode, int nParamId, CSequencerTrack* pTrack)
        {
            nHeight = height;
            track = pTrack;
            paramId = nParamId;
            node = pNode;
            bSelected = false;
            bReadOnly = false;
        }
        Item(int height, CSequencerNode* pNode)
        {
            nHeight = height;
            track = 0;
            paramId = 0;
            node = pNode;
            bSelected = false;
            bReadOnly = false;
        }
    };

    CSequencerDopeSheetBase(QWidget* parent = nullptr);
    virtual ~CSequencerDopeSheetBase();

    bool IsDragging() const;

    void SetSequence(CSequencerSequence* pSequence) { m_pSequence = pSequence; };
    void SetTimeScale(float timeScale, float fAnchorTime);
    float GetTimeScale() { return m_timeScale; }

    void SetScrollOffset(int hpos);

    void SetTimeRange(float start, float end);
    virtual void SetCurrTime(float time, bool bForce = false);
    float GetCurrTime() const;
    void SetStartMarker(float fTime);
    void SetEndMarker(float fTime);

    void DelSelectedKeys(bool bPrompt, bool bAllowUndo = true, bool bIgnorePermission = false);
    void SetMouseActionMode(ESequencerActionMode mode);

    bool CanCopyPasteKeys();
    bool CopyPasteKeys();

    bool CopyKeys(bool bPromptAllowed = true, bool bUseClipboard = true, bool bCopyTrack = false);
    bool PasteKeys(CSequencerNode* pAnimNode, CSequencerTrack* pAnimTrack, float fTimeOffset);
    void StartDraggingKeys(const QPoint& point);
    void StartPasteKeys();
    void FinalizePasteKeys();

    void CopyTrack();

    void SerializeTracks(XmlNodeRef& destination);
    void DeserializeTracks(const XmlNodeRef& source);

    void ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack);
    void HideKeyPropertyCtrlOnSpot();

    bool SelectFirstKey();
    bool SelectFirstKey(const ESequencerParamType type);

    void SetKeyPropertiesDlg(CSequencerKeyPropertiesDlg* dlg)
    { m_keyPropertiesDlg = dlg; }

    //////////////////////////////////////////////////////////////////////////
    // Tracks access.
    //////////////////////////////////////////////////////////////////////////
    int GetCount() const { return m_tracks.size(); }
    void AddItem(const Item& item);
    const Item& GetItem(int item) const;
    CSequencerTrack* GetTrack(int item) const;
    CSequencerNode*  GetNode(int item) const;
    int GetHorizontalExtent() const { return m_itemWidth; };

    bool GetSelectedTracks(std::vector< CSequencerTrack* >& tracks) const;
    bool GetSelectedNodes(std::vector< CSequencerNode* >& nodes) const;

    virtual void    SetHorizontalExtent(int min, int max);
    virtual int     ItemFromPoint(const QPoint& pnt) const;
    virtual int     GetItemRect(int item, QRect& rect) const;
    virtual void  InvalidateItem(int item);
    virtual void    ResetContent() { m_tracks.clear(); }

    void ClearSelection();
    void SelectItem(int item);
    bool IsSelectedItem(int item);

    void SetSnappingMode(ESequencerSnappingMode mode)
    { m_snappingMode = mode; }
    ESequencerSnappingMode GetSnappingMode() const
    { return m_snappingMode; }
    void SetSnapFPS(UINT fps)
    { m_snapFrameTime = fps == 0 ? 0.033333f : 1.0f / float(fps); }

    float GetSnapFps() const
    {
        // To address issues of m_snapFrameTime being <FLT_EPSILON or >FLT_MAX
        return 1.0f / CLAMP(m_snapFrameTime, FLT_EPSILON, FLT_MAX);
    }

    ESequencerTickMode GetTickDisplayMode() const
    { return m_tickDisplayMode; }
    void SetTickDisplayMode(ESequencerTickMode mode)
    {
        m_tickDisplayMode = mode;
        SetTimeScale(GetTimeScale(), 0);    // for refresh
    }

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    uint32 GetChangeCount() const
    {
        return m_changeCount;
    }

    const Range& GetMarkedTime() const
    {
        return m_timeMarked;
    }


    //////////////////////////////////////////////////////////////////////////
    // Drag / drop helper
    struct SDropFragmentData
    {
        FragmentID fragID;
        SFragTagState tagState;
        uint32 option;
    };

    bool IsPointValidForFragmentInPreviewDrop(const QPoint& point, STreeFragmentDataPtr pFragData) const;
    bool CreatePointForFragmentInPreviewDrop(const QPoint& point, STreeFragmentDataPtr pFragData);

    bool IsPointValidForAnimationInLayerDrop(const QPoint& point, QString animationName) const;
    bool CreatePointForAnimationInLayerDrop(const QPoint& point, QString animationName);

    CSequencerSequence* GetSequence() { return m_pSequence; }

protected:

    void OnCreate();
    void OnDestroy();
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void OnHScroll();
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnLButtonDblClk(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void OnRButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void OnArrowLeft() { OnArrow(true); }
    void OnArrowRight() { OnArrow(false); }

    void OnArrow(bool bLeft);
    void OnDelete();
    void OnCopy();
    void OnCut();
    void OnPaste();
    void OnUndo();
    void OnRedo();

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    //////////////////////////////////////////////////////////////////////////
    // Drawing methods.
    //////////////////////////////////////////////////////////////////////////
    virtual void DrawControl(QPainter* painter, const QRect& rcUpdate);
    virtual void DrawTrack(int item, QPainter* dc, const QRect& rcItem);
    virtual void DrawTicks(QPainter* dc, const QRect& rc, const Range& timeRange);

    // Helper functions
    void ComputeFrameSteps(const Range& VisRange);
    void DrawTimeLineInFrames(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);
    void DrawTimeLineInSeconds(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);

    virtual void DrawTimeline(QPainter* dc, const QRect& rcUpdate);
    virtual void DrawSummary(QPainter* dc, const QRect& rcUpdate);
    virtual void DrawSelectedKeyIndicators(QPainter* dc);
    virtual void DrawKeys(CSequencerTrack* track, QPainter* painter, const QRect& rc, const Range& timeRange, EDSRenderFlags renderFlags);
    virtual void RedrawItem(int item);

    //////////////////////////////////////////////////////////////////////////
    // Must be overriden.
    //////////////////////////////////////////////////////////////////////////
    //! Find a key near this point.
    virtual int FirstKeyFromPoint(const QPoint& point, bool exact = false) const;
    virtual int LastKeyFromPoint(const QPoint& point, bool exact = false) const;
    //! Select keys inside this client rectangle.
    virtual void SelectKeys(const QRect& rc);
    //! Select all keys within time frame defined by this client rectangle.
    virtual void SelectAllKeysWithinTimeFrame(const QRect& rc);

    //////////////////////////////////////////////////////////////////////////
    //! Return time snapped to timestep,
    double GetTickTime() const;
    float TickSnap(float time) const;
    float MagnetSnap(const float time, const CSequencerNode* node) const;
    float FrameSnap(float time) const;

    //! Returns visible time range.
    Range GetVisibleRange();
    Range GetTimeRange(const QRect& rc);

    //! Return client position for given time.
    int TimeToClient(float time) const;

    float TimeFromPoint(const QPoint& point) const;
    float TimeFromPointUnsnapped(const QPoint& point) const;

    //! Unselect all selected keys.
    void UnselectAllKeys(bool bNotify);
    //! Offset all selected keys by this offset.
    void OffsetSelectedKeys(const float timeOffset, const bool bSnapKeys);
    //! Scale all selected keys by this offset.
    void ScaleSelectedKeys(float timeOffset, bool bSnapKeys);
    void CloneSelectedKeys();

    bool FindSingleSelectedKey(CSequencerTrack*& track, int& key);

    //////////////////////////////////////////////////////////////////////////
    void SetKeyInfo(CSequencerTrack* track, int key, bool openWindow = false);

    void UpdateAnimation(int item);
    void UpdateAnimation(CSequencerTrack* animTrack);
    void SetLeftOffset(int ofs) { m_leftOffset = ofs; };

    void RecordTrackUndo(const Item& item);
    void ShowKeyTooltip(CSequencerTrack* pTrack, int nKey, int secondarySelection, const QPoint& point);

protected:
    //////////////////////////////////////////////////////////////////////////
    // FIELDS.
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    QBrush m_bkgrBrush;
    QBrush m_bkgrBrushEmpty;
    QBrush m_selectedBrush;
    QBrush m_timeBkgBrush;
    QBrush m_timeHighlightBrush;
    QBrush m_visibilityBrush;
    QBrush m_selectTrackBrush;

    QCursor m_crsLeftRight;
    QCursor m_crsAddKey;
    QCursor m_crsCross;
    QCursor m_crsMoveBlend;

    QRect m_rcClient;
    QPoint m_scrollOffset;
    QRubberBand* m_rcSelect;
    QRect m_rcTimeline;
    QRect m_rcSummary;

    QScrollBar* m_scrollBar;

    QPoint m_lastTooltipPos;
    QPoint m_mouseDownPos;
    QPoint m_mouseOverPos;
    float m_mouseMoveStartTimeOffset;
    int m_mouseMoveStartTrackOffset;
    static const int SNumOfBmps = 7;
    QPixmap m_imageSeqKeyBody[SNumOfBmps];
    QVector<QPixmap> m_imageList;

    bool m_bZoomDrag;
    bool m_bMoveDrag;

    bool m_bMouseOverKey;

    //////////////////////////////////////////////////////////////////////////
    // Time.
    float m_timeScale;
    float m_currentTime;
    float m_storedTime;
    Range m_timeRange;
    Range m_realTimeRange;
    Range m_timeMarked;

    //! This is how often to place ticks.
    //! value of 10 means place ticks every 10 second.
    double m_ticksStep;

    //////////////////////////////////////////////////////////////////////////
    int m_mouseMode;
    int m_mouseActionMode;
    int m_actionMode;
    bool m_bAnySelected;
    float m_keyTimeOffset;
    float m_grabOffset;
    int m_secondarySelection;

    // Drag / Drop
    bool m_bUseClipboard;
    XmlNodeRef m_dragClipboard;
    XmlNodeRef m_prePasteSheetData;

    CSequencerKeyPropertiesDlg* m_keyPropertiesDlg;
    ReflectedPropertyControl* m_wndPropsOnSpot;
    CSequencerTrack* m_pLastTrackSelectedOnSpot;

    QFont m_descriptionFont;

    //////////////////////////////////////////////////////////////////////////
    // Track list related.
    //////////////////////////////////////////////////////////////////////////
    std::vector<Item> m_tracks;

    int m_itemWidth;

    int m_leftOffset;
    int m_scrollMin, m_scrollMax;

    CSequencerSequence* m_pSequence;

    bool m_bCursorWasInKey;
    bool m_bValidLastPaste;

    float m_fJustSelected;

    bool m_bMouseMovedAfterRButtonDown;

    uint32 m_changeCount;

    ESequencerSnappingMode m_snappingMode;
    double m_snapFrameTime;

    ESequencerTickMode m_tickDisplayMode;
    double m_fFrameTickStep;
    double m_fFrameLabelStep;

    bool m_bEditLock;

    void OffsetKey(CSequencerTrack* pTrack, const int keyIndex, const float timeOffset) const;
    void NotifyKeySelectionUpdate();
    bool IsOkToAddKeyHere(const CSequencerTrack* pTrack, float time) const;

    void MouseMoveSelect(const QPoint& point);
    void MouseMoveMove(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveOver(const QPoint& point);
    void MouseMovePaste(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);

    int GetAboveKey(CSequencerTrack*& track);
    int GetBelowKey(CSequencerTrack*& track);
    int GetRightKey(CSequencerTrack*& track);
    int GetLeftKey(CSequencerTrack*& track);

    bool AddOrCheckoutFile(const string& filename);
    void TryOpenFile(const QString& relativePath, const QString& fileName, const QString& extension) const;
    EDSSourceControlResponse TryGetLatestOnFiles(const QStringList& paths, bool bPromptUser = true);
    EDSSourceControlResponse TryCheckoutFiles(const QStringList& paths, bool bPromptUser = true);
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERDOPESHEETBASE_H

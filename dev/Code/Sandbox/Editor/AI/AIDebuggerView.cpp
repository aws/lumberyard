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

// Description : View that draws main debugging output


#include "StdAfx.h"
#include "AIDebuggerView.h"

#include <cmath>
#include <cstdlib>
#include <IAIAction.h>
#include <IAISystem.h>
#include <IAgent.h>

#include "IViewPane.h"
#include "Objects/EntityObject.h"
#include "Objects/SelectionGroup.h"
#include "Clipboard.h"
#include "ViewManager.h"

#include "ItemDescriptionDlg.h"

#include "AIManager.h"
#include "IAIObjectManager.h"

#include <QColorDialog>
#include <QPainter>
#include <QStyle>
#include <QWheelEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

#include <AI/ui_AIDebuggerViewFindDialog.h>

#ifdef LoadCursor
#undef LoadCursor
#endif

#define ID_AIDEBUGGER_COPY_LABEL                    100
#define ID_AIDEBUGGER_FIND                          101
#define ID_AIDEBUGGER_GOTO_START                    102
#define ID_AIDEBUGGER_GOTO_END                      103
#define ID_AIDEBUGGER_COPY_POS                      104
#define ID_AIDEBUGGER_GOTO_POS                      105

#define ID_AIDEBUGGER_LISTOPTION_DEBUGENABLED       51
#define ID_AIDEBUGGER_LISTOPTION_COLOR              52
#define ID_AIDEBUGGER_LISTOPTION_SETVIEW            53

AIDebuggerViewFindDialog::AIDebuggerViewFindDialog(QWidget* parent /* = nullptr */)
    : QDialog(parent)
    , m_ui(new Ui::AIDebuggerViewFindDialog)
{
    m_ui->setupUi(this);

    QPushButton* findNext = new QPushButton(tr("&Find Next"));
    findNext->setDefault(true);
    m_ui->buttonBox->addButton(findNext, QDialogButtonBox::ActionRole);
    QPushButton* cancel = m_ui->buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(findNext, &QPushButton::clicked, [this]() { emit this->findNext(m_ui->findWhatEdit->text()); });
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, [&]() { reject(); done(); });

    setFixedSize(size());
}

AIDebuggerViewFindDialog::~AIDebuggerViewFindDialog()
{
}

void AIDebuggerViewFindDialog::done()
{
    QSettings settings;
    settings.beginGroup("AIDebugger");
    settings.setValue("findDialogPosition", pos());
    settings.endGroup();
    settings.sync();
}

//////////////////////////////////////////////////////////////////////////
CAIDebuggerView::CAIDebuggerView(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_dragType(DRAG_NONE)
    , m_timeRulerStart(0)
    , m_timeRulerEnd(0)
    , m_timeCursor(0)
    , m_findDialog(new AIDebuggerViewFindDialog)
    , m_itemsOffset(0)
    , m_listWidth(LIST_WIDTH)
    , m_listFullHeight(0)
    , m_detailsWidth(DETAILS_WIDTH)
{
    setMouseTracking(true);
    Create();
}

//////////////////////////////////////////////////////////////////////////
CAIDebuggerView::~CAIDebuggerView()
{
    QSettings settings;
    settings.beginGroup("AIDebugger");

    settings.setValue("AIDebugger_ListWidth", m_listWidth);
    settings.setValue("AIDebugger_DetailsWidth", m_detailsWidth);

    settings.setValue("AIDebugger_StreamSignalReceived", m_streamSignalReceived->isChecked());
    settings.setValue("AIDebugger_StreamSignalReceivedAux", m_streamSignalReceivedAux->isChecked());
    settings.setValue("AIDebugger_StreamBehaviorSelected", m_streamBehaviorSelected->isChecked());
    settings.setValue("AIDebugger_StreamAttentionTarget", m_streamAttenionTarget->isChecked());
    settings.setValue("AIDebugger_StreamRegisterStimulus", m_streamRegisterStimulus->isChecked());
    settings.setValue("AIDebugger_StreamGoalPipeSelected", m_streamGoalPipeSelected->isChecked());
    settings.setValue("AIDebugger_StreamGoalPipeInserted", m_streamGoalPipeInserted->isChecked());
    settings.setValue("AIDebugger_StreamLuaComment", m_streamLuaComment->isChecked());
    settings.setValue("AIDebugger_StreamPersonalLog", m_streamPersonalLog->isChecked());
    settings.setValue("AIDebugger_StreamHealth", m_streamHealth->isChecked());
    settings.setValue("AIDebugger_StreamHitDamage", m_streamHitDamage->isChecked());
    settings.setValue("AIDebugger_StreamDeath", m_streamDeath->isChecked());
    settings.setValue("AIDebugger_StreamPressureGraph", m_streamPressureGraph->isChecked());
    settings.setValue("AIDebugger_StreamBookmarks", m_streamBookmarks->isChecked());

    settings.endGroup();
    settings.sync();

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());
    if (pAISystem)
    {
        SAIRecorderDebugContext* pContext;
        pAISystem->GetRecorderDebugContext(pContext);

        pContext->fStartPos = 0.0f;
        pContext->fEndPos = 0.0f;
        pContext->fCursorPos = 0.0f;
        pContext->DebugObjects.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::Create()
{
    QSettings settings;
    settings.beginGroup("AIDebugger");

    connect(m_findDialog.data(), &AIDebuggerViewFindDialog::findNext, this, &CAIDebuggerView::OnFindReplace);

    // Create a custom font
    m_smallFont = QFont();
    m_smallFont.setPointSizeF(m_smallFont.pointSizeF() * 0.85f);

    // Data initialisation
    m_timeStart = 0.0f;
    m_timeScale = 5.0f / 100.0f;

    m_listWidth = settings.value("AIDebugger_ListWidth", LIST_WIDTH).toInt();
    m_detailsWidth = settings.value("AIDebugger_DetailsWidth", DETAILS_WIDTH).toInt();

    // Create the timeline context menu
    m_timelineContext = new QMenu(this);
    m_copyLabel = m_timelineContext->addAction(tr("Copy Label"));
    m_copyLabel->setData(ID_AIDEBUGGER_COPY_LABEL);
    m_timelineContext->addAction(tr("Find..."))
        ->setData(ID_AIDEBUGGER_FIND);

    m_timelineContext->addSeparator();

    m_timelineContext->addAction(tr("Goto Start"))
        ->setData(ID_AIDEBUGGER_GOTO_START);
    m_timelineContext->addAction(tr("Goto End"))
        ->setData(ID_AIDEBUGGER_GOTO_END);

    m_timelineContext->addSeparator();

    m_gotoPos = m_timelineContext->addAction(tr("Goto Pos"));
    m_gotoPos->setData(ID_AIDEBUGGER_GOTO_POS);
    m_copyPos = m_timelineContext->addAction(tr("Copy Pos"));
    m_copyPos->setData(ID_AIDEBUGGER_COPY_POS);

    m_timelineContext->addSeparator();

    m_streamSignalReceived = m_timelineContext->addAction(tr("Signal Received"));
    m_streamSignalReceived->setCheckable(true);
    m_streamSignalReceivedAux = m_timelineContext->addAction(tr("Signal Received Aux"));
    m_streamSignalReceivedAux->setCheckable(true);
    m_streamBehaviorSelected = m_timelineContext->addAction(tr("Behaviour Selected"));
    m_streamBehaviorSelected->setCheckable(true);
    m_streamAttenionTarget = m_timelineContext->addAction(tr("Attention Target"));
    m_streamAttenionTarget->setCheckable(true);
    m_streamRegisterStimulus = m_timelineContext->addAction(tr("Registered Stimulus"));
    m_streamRegisterStimulus->setCheckable(true);
    m_streamGoalPipeSelected = m_timelineContext->addAction(tr("Selected Goal Pipe"));
    m_streamGoalPipeSelected->setCheckable(true);
    m_streamGoalPipeInserted = m_timelineContext->addAction(tr("Inserted Goal Pipe"));
    m_streamGoalPipeInserted->setCheckable(true);
    m_streamLuaComment = m_timelineContext->addAction(tr("Lua Comment"));
    m_streamLuaComment->setCheckable(true);
    m_streamPersonalLog = m_timelineContext->addAction(tr("Personal Log"));
    m_streamPersonalLog->setCheckable(true);

    m_timelineContext->addSeparator();

    m_streamHealth = m_timelineContext->addAction(tr("Health"));
    m_streamHealth->setCheckable(true);
    m_streamHitDamage = m_timelineContext->addAction(tr("HitDamage"));
    m_streamHitDamage->setCheckable(true);
    m_streamDeath = m_timelineContext->addAction(tr("Death"));
    m_streamDeath->setCheckable(true);
    m_streamPressureGraph = m_timelineContext->addAction(tr("Pressure Graph"));
    m_streamPressureGraph->setCheckable(true);

    m_timelineContext->addSeparator();

    m_streamBookmarks = m_timelineContext->addAction(tr("Bookmarks"));

    // restore initial checked states

    m_streamSignalReceived->setChecked(settings.value("AIDebugger_StreamSignalReceived", true).toBool());
    m_streamSignalReceivedAux->setChecked(settings.value("AIDebugger_StreamSignalReceivedAux", false).toBool());
    m_streamBehaviorSelected->setChecked(settings.value("AIDebugger_StreamBehaviorSelected", true).toBool());
    m_streamAttenionTarget->setChecked(settings.value("AIDebugger_StreamAttentionTarget", false).toBool());
    m_streamRegisterStimulus->setChecked(settings.value("AIDebugger_StreamRegisterStimulus", false).toBool());
    m_streamGoalPipeSelected->setChecked(settings.value("AIDebugger_StreamGoalPipeSelected", false).toBool());
    m_streamGoalPipeInserted->setChecked(settings.value("AIDebugger_StreamGoalPipeInserted", false).toBool());
    m_streamLuaComment->setChecked(settings.value("AIDebugger_StreamLuaComment", false).toBool());
    m_streamPersonalLog->setChecked(settings.value("AIDebugger_StreamPersonalLog", false).toBool());
    m_streamHealth->setChecked(settings.value("AIDebugger_StreamHealth", false).toBool());
    m_streamHitDamage->setChecked(settings.value("AIDebugger_StreamHitDamage", false).toBool());
    m_streamDeath->setChecked(settings.value("AIDebugger_StreamDeath", false).toBool());
    m_streamPressureGraph->setChecked(settings.value("AIDebugger_StreamPressureGraph", false).toBool());
    m_streamBookmarks->setChecked(settings.value("AIDebugger_StreamBookmarks", false).toBool());

    // Create the left-hand list context menu
    m_listContext = new QMenu(this);
    m_listOptionDebug = m_listContext->addAction(tr("Enable Debug Drawing"));
    m_listOptionDebug->setData(ID_AIDEBUGGER_LISTOPTION_DEBUGENABLED);
    m_listOptionDebug->setCheckable(true);
    m_listOptionSetView = m_listContext->addAction(tr("Set Editor View"));
    m_listOptionSetView->setData(ID_AIDEBUGGER_LISTOPTION_SETVIEW);
    m_listOptionSetView->setCheckable(true);
    m_listContext->addAction(tr("Set Color"))
        ->setData(ID_AIDEBUGGER_LISTOPTION_COLOR);

    UpdateStreamDesc();

    m_timeStart = GetRecordStartTime();

    // Update views
    UpdateViewRects();
    UpdateView();
}

//////////////////////////////////////////////////////////////////////////
QSize CAIDebuggerView::minimumSizeHint() const
{
    return {
               -1, RULER_HEIGHT + m_listFullHeight
    };
}

//////////////////////////////////////////////////////////////////////////
QSize CAIDebuggerView::sizeHint() const
{
    return QWidget::sizeHint();// return minimumSizeHint();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::wheelEvent(QWheelEvent* event)
{
    if (m_timelineRect.contains(event->pos()) || m_rulerRect.contains(event->pos()))
    {
        float   val = (event->angleDelta().y() / (float)WHEEL_DELTA) * 0.75f;
        int x = event->x() - m_timelineRect.left();
        float   mouseTime = ClientToTime(x);

        if (val > 0)
        {
            m_timeScale *= val;
        }
        else if (val < 0)
        {
            m_timeScale /= -val;
        }

        // Make sure the numbers stay sensible
        m_timeScale = qBound(0.000001f, m_timeScale, 10000.0f);

        m_timeStart = mouseTime - x * m_timeScale;

        //      UpdateView();
        UpdateAIDebugger();
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragType != DRAG_NONE)
    {
        int dx = event->x() - m_dragStartPt.x();
        int dy = event->y() - m_dragStartPt.y();
        float   dt = dx * m_timeScale;
        if (m_dragType == DRAG_PAN)
        {
            m_timeStart = m_dragStartVal - dt;

            if (!m_items.empty())
            {
                m_itemsOffset = (int)m_dragStartVal2 + dy;
                int maxItemsOffset = std::max(0, m_listFullHeight - m_listRect.height());
                if (m_itemsOffset > 0)
                {
                    m_itemsOffset = 0;
                }
                if (m_itemsOffset < -maxItemsOffset)
                {
                    m_itemsOffset = -maxItemsOffset;
                }
            }
            emit scrollRequest({ 0, -m_itemsOffset });
            UpdateViewRects();
        }
        else if (m_dragType == DRAG_RULER_START)
        {
            m_timeRulerStart = m_dragStartVal + dt;
        }
        else if (m_dragType == DRAG_RULER_END)
        {
            m_timeRulerEnd = m_dragStartVal + dt;
        }
        else if (m_dragType == DRAG_RULER_SEGMENT)
        {
            m_timeRulerStart = m_dragStartVal + dt;
            m_timeRulerEnd = m_dragStartVal2 + dt;
        }
        else if (m_dragType == DRAG_TIME_CURSOR)
        {
            m_timeCursor = m_dragStartVal + dt;
        }
        else if (m_dragType == DRAG_LIST_WIDTH)
        {
            m_listWidth = qBound(10, (int)m_dragStartVal + dx, width() - m_detailsWidth - 100);
            UpdateViewRects();
        }
        else if (m_dragType == DRAG_DETAILS_WIDTH)
        {
            m_detailsWidth = qBound(10, (int)m_dragStartVal - dx, width() - m_listWidth - 100);
            UpdateViewRects();
        }
        else if (m_dragType == DRAG_ITEMS)
        {
            if (!m_items.empty())
            {
                m_itemsOffset = (int)m_dragStartVal + dy;
                int maxItemsOffset = std::max(0, m_listFullHeight - m_listRect.height());
                if (m_itemsOffset > 0)
                {
                    m_itemsOffset = 0;
                }
                if (m_itemsOffset < -maxItemsOffset)
                {
                    m_itemsOffset = -maxItemsOffset;
                }
            }
            emit scrollRequest({ 0, -m_itemsOffset });
            UpdateViewRects();
        }

        SItem* pItem = ClientToItem(event->y());
        if (pItem)
        {
            m_sFocusedItemName = pItem->name;
        }
        else
        {
            m_sFocusedItemName.clear();
        }

        //      UpdateView();
        UpdateAIDebugger();
        update();
    }

    OnSetCursor(event->pos());
}

void CAIDebuggerView::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event);
        return;
    case Qt::MiddleButton:
        OnMButtonDown(event);
        return;
    }
}

void CAIDebuggerView::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event);
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event);
        break;
    case Qt::RightButton:
        OnRButtonUp(event);
        break;
    }

    OnSetCursor(event->pos());
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnLButtonUp(QMouseEvent* event)
{
    if (m_timeRulerStart > m_timeRulerEnd)
    {
        std::swap(m_timeRulerStart, m_timeRulerEnd);
    }

    if (m_dragType != DRAG_NONE)
    {
        m_dragType = DRAG_NONE;
        //      UpdateView();
        UpdateAIDebugger();
        update();

        releaseMouse();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnMButtonDown(QMouseEvent* event)
{
    if (m_dragType == DRAG_NONE)
    {
        if (m_timelineRect.contains(event->pos()) || m_rulerRect.contains(event->pos()))
        {
            m_dragStartVal = m_timeStart;
            m_dragStartVal2 = m_itemsOffset;
            m_dragStartPt = event->pos();
            m_dragType = DRAG_PAN;
            //          UpdateView();
            UpdateAIDebugger();
            update();
            grabMouse();
        }
        else if (m_listRect.contains(event->pos()) || m_listRect.contains(event->pos()))
        {
            m_dragStartVal = m_itemsOffset;
            m_dragStartPt = event->pos();
            m_dragType = DRAG_ITEMS;
            //          UpdateView();
            UpdateAIDebugger();
            update();
            grabMouse();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnMButtonUp(QMouseEvent* event)
{
    if (m_dragType != DRAG_NONE)
    {
        m_dragType = DRAG_NONE;
        //      UpdateView();
        UpdateAIDebugger();
        update();

        releaseMouse();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnLButtonDown(QMouseEvent* event)
{
    int rulerHit = HitTestRuler(event->pos());
    int cursorHit = HitTestTimeCursor(event->pos());
    int separatorHit = HitTestSeparators(event->pos());

    if (rulerHit != RULERHIT_NONE)
    {
        if (rulerHit == RULERHIT_EMPTY)
        {
            // Create ruler
            m_timeRulerStart = ClientToTime(event->x() - m_rulerRect.left());
            m_timeRulerEnd = m_timeRulerStart;
            m_dragStartVal = m_timeRulerStart;
            m_dragType = DRAG_RULER_END;
        }
        else if (rulerHit == RULERHIT_START)
        {
            m_dragStartVal = m_timeRulerStart;
            m_dragType = DRAG_RULER_START;
        }
        else if (rulerHit == RULERHIT_END)
        {
            m_dragStartVal = m_timeRulerEnd;
            m_dragType = DRAG_RULER_END;
        }
        else if (rulerHit == RULERHIT_SEGMENT)
        {
            m_dragStartVal = m_timeRulerStart;
            m_dragStartVal2 = m_timeRulerEnd;
            m_dragType = DRAG_RULER_SEGMENT;
        }
        m_dragStartPt = event->pos();
        //      UpdateView();
        UpdateAIDebugger();
        update();
        grabMouse();
    }
    else if (separatorHit != SEPARATORHIT_NONE)
    {
        if (separatorHit == SEPARATORHIT_LIST)
        {
            m_dragStartVal = m_listWidth;
            m_dragType = DRAG_LIST_WIDTH;
        }
        else
        {
            m_dragStartVal = m_detailsWidth;
            m_dragType = DRAG_DETAILS_WIDTH;
        }
        m_dragStartPt = event->pos();
        //      UpdateView();
        UpdateAIDebugger();
        update();
        grabMouse();
    }
    else if (cursorHit != CURSORHIT_NONE)
    {
        if (cursorHit == CURSORHIT_EMPTY)
        {
            m_timeCursor = ClientToTime(event->pos().x() - m_timelineRect.left());
        }
        m_dragStartVal = m_timeCursor;
        m_dragType = DRAG_TIME_CURSOR;
        m_dragStartPt = event->pos();
        //      UpdateView();
        UpdateAIDebugger();
        update();
        grabMouse();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnRButtonUp(QMouseEvent* event)
{
    if (m_timelineRect.contains(event->pos()))
    {
        OnRButtonUp_Timeline(event);
    }
    else if (m_listRect.contains(event->pos()))
    {
        OnRButtonUp_List(event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnRButtonUp_Timeline(QMouseEvent* event)
{
    QString curLabel = GetLabelAtPoint(event->pos());
    if (!curLabel.isEmpty())
    {
        m_copyLabel->setText(tr("Copy Label %1").arg(curLabel));
        m_copyLabel->setEnabled(true);
    }
    else
    {
        m_copyLabel->setEnabled(false);
    }

    Vec3    pos = GetPosAtPoint(event->pos());
    QString position;
    if (!pos.IsZero(0.0001f))
    {
        position = QString("%1, %2, %3").arg(pos.x, 2).arg(pos.y, 2).arg(pos.z, 2);
        m_gotoPos->setText(tr("Goto agent location %1").arg(position));
        m_copyPos->setText(tr("Copy agent location %1").arg(position));
        m_gotoPos->setEnabled(true);
        m_copyPos->setEnabled(true);
    }
    else
    {
        m_gotoPos->setEnabled(false);
        m_copyPos->setEnabled(false);
    }

    QAction* selected = m_timelineContext->exec(mapToGlobal(event->pos()));
    if (!selected)
    {
        return;
    }

    if (selected->data() == ID_AIDEBUGGER_COPY_LABEL)
    {
        if (!curLabel.isEmpty())
        {
            CClipboard clipboard(this);
            clipboard.PutString(curLabel);
        }
    }
    else if (selected->data() == ID_AIDEBUGGER_FIND)
    {
        QSettings settings;
        settings.beginGroup("AIDebugger");
        if (settings.contains("findDialogPosition"))
        {
            QPoint position = settings.value("findDialogPosition").toPoint();
            if (!position.isNull())
            {
                m_findDialog->move(position);
            }
        }

        m_findDialog->show();
        m_findDialog->setFocus();
    }
    else if (selected->data() == ID_AIDEBUGGER_GOTO_START)
    {
        m_timeCursor = GetRecordStartTime();
        AdjustViewToTimeCursor();
    }
    else if (selected->data() == ID_AIDEBUGGER_GOTO_END)
    {
        m_timeCursor = GetRecordEndTime();
        AdjustViewToTimeCursor();
    }
    else if (selected->data() == ID_AIDEBUGGER_COPY_POS)
    {
        CClipboard clipboard(this);
        clipboard.PutString(position);
    }
    else if (selected->data() == ID_AIDEBUGGER_GOTO_POS)
    {
        CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
        if (pRenderViewport)
        {
            Matrix34 tm = pRenderViewport->GetViewTM();
            tm.SetTranslation(pos);
            pRenderViewport->SetViewTM(tm);
        }
    }


    UpdateStreamDesc();

    UpdateView();
    UpdateAIDebugger();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnRButtonUp_List(QMouseEvent* event)
{
    SItem* pItem = ClientToItem(event->y());

    if (pItem)
    {
        m_sFocusedItemName = pItem->name;

        m_listOptionDebug->setChecked(pItem->bDebugEnabled);
        m_listOptionSetView->setChecked(pItem->bSetView);
        QAction* selected = m_listContext->exec(mapToGlobal(event->pos()));

        if (selected)
        {
            if (selected->data() == ID_AIDEBUGGER_LISTOPTION_DEBUGENABLED)
            {
                pItem->bDebugEnabled = !pItem->bDebugEnabled;
            }
            else if (selected->data() == ID_AIDEBUGGER_LISTOPTION_COLOR)
            {
                QColor color = QColorDialog::getColor(pItem->debugColor, this, tr("Choose Debug Color"));
                if (color.isValid())
                {
                    pItem->debugColor = color;
                }
            }
            else if (selected->data() == ID_AIDEBUGGER_LISTOPTION_SETVIEW)
            {
                if (pItem->bSetView)
                {
                    pItem->bSetView = false;
                }
                else
                {
                    ClearSetViewProperty();
                    pItem->bSetView = true;
                }
            }
        }

        // Stop iterating
        UpdateView();
        UpdateAIDebugger();
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::ClearSetViewProperty()
{
    TItemList::iterator itItem = m_items.begin();
    TItemList::iterator itItemEnd = m_items.end();
    for (; itItem != itItemEnd; ++itItem)
    {
        SItem& currItem(*itItem);
        currItem.bSetView = false;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAIDebuggerView::GetSetViewData(const SItem& sItem, Vec3& vPos, Vec3& vDir) const
{
    bool bResult = false;
    vPos.zero();
    vDir = Vec3_OneY;

    IAIObjectManager* pAIObjMgr = gEnv->pAISystem->GetAIObjectManager();
    IAIObject* pAI = pAIObjMgr->GetAIObjectByName(sItem.type, sItem.name.toUtf8().data());
    IAIDebugRecord* pRecord = pAI ? pAI->GetAIDebugRecord() : NULL;
    if (pRecord)
    {
        IAIDebugStream* pPosStream = pRecord->GetStream(IAIRecordable::E_AGENTPOS);
        IAIDebugStream* pDirStream = pRecord->GetStream(IAIRecordable::E_AGENTDIR);
        if (pPosStream && pDirStream)
        {
            pPosStream->Seek(m_timeCursor);
            pDirStream->Seek(m_timeCursor);

            float fCurrTime;
            Vec3* pPos = (Vec3*)pPosStream->GetCurrent(fCurrTime);
            Vec3* pDir = (Vec3*)pDirStream->GetCurrent(fCurrTime);

            if (pPos)
            {
                vPos = *pPos;
            }
            if (pDir)
            {
                vDir = *pDir;
            }

            bResult = (pPos || pDir);
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Left:
    case Qt::Key_A:
    {
        SetCursorPrevEvent(event->modifiers() & Qt::ShiftModifier);
    }
    break;

    case Qt::Key_Right:
    case Qt::Key_D:
    {
        SetCursorNextEvent(event->modifiers() & Qt::ShiftModifier);
    }
    break;
    }
}


///////////////////////////////
// Internal functions
///////////////////////////////

//////////////////////////////////////////////////////////////////////////
int CAIDebuggerView::HitTestRuler(const QPoint& pt)
{
    if (!m_rulerRect.contains(pt))
    {
        return RULERHIT_NONE;
    }

    bool    hasRuler(false);
    int     rulerStartX, rulerEndX;

    if (fabsf(m_timeRulerEnd - m_timeRulerStart) > 0.001f)
    {
        rulerStartX = m_rulerRect.left() + TimeToClient(m_timeRulerStart);
        rulerEndX = m_rulerRect.left() + TimeToClient(m_timeRulerEnd);
        if (rulerStartX > rulerEndX)
        {
            std::swap(rulerStartX, rulerEndX);
        }
        hasRuler = true;
    }

    if (hasRuler)
    {
        if (abs(pt.x() - rulerStartX) < 2)
        {
            return RULERHIT_START;
        }
        else if (std::abs(pt.x() - rulerEndX) < 2)
        {
            return RULERHIT_END;
        }
        else if (pt.x() > rulerStartX && pt.x() < rulerEndX)
        {
            return RULERHIT_SEGMENT;
        }
    }

    return RULERHIT_EMPTY;
}

//////////////////////////////////////////////////////////////////////////
int CAIDebuggerView::HitTestTimeCursor(const QPoint& pt)
{
    if (!m_timelineRect.contains(pt))
    {
        return CURSORHIT_NONE;
    }

    int cursorX = m_timelineRect.left() + TimeToClient(m_timeCursor);

    if (std::abs(pt.x() - cursorX) < 3)
    {
        return CURSORHIT_CURSOR;
    }

    return CURSORHIT_EMPTY;
}

//////////////////////////////////////////////////////////////////////////
int CAIDebuggerView::HitTestSeparators(const QPoint& pt)
{
    if (std::abs(pt.x() - m_listRect.right()) < 3)
    {
        return SEPARATORHIT_LIST;
    }
    else if (std::abs(pt.x() - m_detailsRect.left()) < 3)
    {
        return SEPARATORHIT_DETAILS;
    }

    return SEPARATORHIT_NONE;
}

//////////////////////////////////////////////////////////////////////////
QString CAIDebuggerView::GetLabelAtPoint(const QPoint& point)
{
    QPoint adjusted = point - m_timelineRect.topLeft();

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    float   t = ClientToTime(adjusted.x());

    for (TItemList::iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        const SItem&    item (*it);
        int h = adjusted.y() - item.y;
        if (h < 0 && h >= item.h)
        {
            continue;
        }

        //      int n = h / ITEM_HEIGHT;
        int n = 0;
        int y = 0;
        for (; n < (int)m_streams.size(); ++n)
        {
            y += m_streams[n].height;
            if (h <= y)
            {
                break;
            }
        }

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        if (n >= 0 && n < (int)m_streams.size() && m_streams[n].track == TRACK_TEXT)
        {
            IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)m_streams[n].type);
            if (!pStream || pStream->IsEmpty())
            {
                return 0;
            }
            float   curTime;
            pStream->Seek(t);
            return (char*)(pStream->GetCurrent(curTime));
        }
    }

    return {};
}

//////////////////////////////////////////////////////////////////////////
Vec3 CAIDebuggerView::GetPosAtPoint(const QPoint& point)
{
    QPoint adjusted = point - m_timelineRect.topLeft();

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    float   t = ClientToTime(adjusted.x());

    for (TItemList::iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        const SItem&    item (*it);
        int h = adjusted.y() - item.y;
        if (h < 0 && h >= item.h)
        {
            continue;
        }

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        IAIDebugStream* pStream = pRecord->GetStream(IAIRecordable::E_AGENTPOS);
        if (!pStream || pStream->IsEmpty())
        {
            return Vec3(ZERO);
        }
        float   curTime;
        pStream->Seek(t);
        Vec3*   pos = (Vec3*)pStream->GetCurrent(curTime);
        if (pos)
        {
            return *pos;
        }
        return Vec3(ZERO);
    }

    return Vec3(ZERO);
}

//////////////////////////////////////////////////////////////////////////
bool CAIDebuggerView::FindNext(const QString& what, float start, float& t)
{
    float   bestTime = GetRecordEndTime();
    bool    found = false;

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    for (TItemList::iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        const SItem&    item (*it);

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        for (size_t i = 0; i < m_streams.size(); i++)
        {
            IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)m_streams[i].type);
            if (pStream && !pStream->IsEmpty())
            {
                float   t;
                if (FindInStream(what, start, pStream, t))
                {
                    if (t > start && t < bestTime)
                    {
                        bestTime = t;
                    }
                    found = true;
                }
            }
        }
    }

    t = bestTime;

    return found;
}

//////////////////////////////////////////////////////////////////////////
bool CAIDebuggerView::FindInStream(const QString& what, float start, IAIDebugStream* pStream, float& t)
{
    float   curTime;
    pStream->Seek(start);
    QString pString = (char*)(pStream->GetCurrent(curTime));

    while (pStream->GetCurrentIdx() < pStream->GetSize())
    {
        float nextTime;
        char* pNextString = (char*)(pStream->GetNext(nextTime));

        if (!pString.isEmpty() && curTime > start && pString.contains(what, Qt::CaseInsensitive))
        {
            t = curTime;
            return true;
        }

        curTime = nextTime;
        pString = pNextString;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::AdjustViewToTimeCursor()
{
    float   timeStart = m_timeStart;
    float   timeEnd = m_timeStart + m_timelineRect.width() * m_timeScale;
    float   timeGap = 10 * m_timeScale;

    timeStart += timeGap;
    timeEnd -= timeGap;

    if (timeEnd < timeStart)
    {
        timeEnd = timeStart;
    }

    if (m_timeCursor < timeStart)
    {
        m_timeStart = m_timeCursor - timeGap;
    }
    else if (m_timeCursor > timeEnd)
    {
        float   dt = timeEnd - timeStart;
        m_timeStart = m_timeCursor - dt;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnFindReplace(const QString& target)
{
    float   t;
    bool    found = FindNext(target, m_timeCursor, t);
    if (!found && fabsf(t - GetRecordEndTime()) < 0.001f)
    {
        float   start = GetRecordStartTime();
        found = FindNext(target, start, t);
        if (!found)
        {
            QMessageBox::warning(this, tr("Not Found"), tr("The following specified text was not found:\n%1").arg(target));
        }
    }
    if (found)
    {
        m_timeCursor = t;
        AdjustViewToTimeCursor();
    }

    //      UpdateView();
    UpdateAIDebugger();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::UpdateViewRects()
{
    const QRect bounds = rect();

    m_listRect = {
        0,
        RULER_HEIGHT,
        m_listWidth,
        bounds.height() - RULER_HEIGHT
    };

    m_rulerRect = {
        m_listWidth,
        0,
        bounds.width() - m_listWidth - m_detailsWidth,
        RULER_HEIGHT,
    };

    m_timelineRect = {
        m_listWidth,
        RULER_HEIGHT,
        bounds.width() - m_listWidth - m_detailsWidth,
        bounds.height() - RULER_HEIGHT
    };

    m_detailsRect = {
        m_listWidth + m_timelineRect.width(),
        RULER_HEIGHT,
        m_detailsWidth,
        bounds.height() - RULER_HEIGHT
    };
}

//////////////////////////////////////////////////////////////////////////
bool CAIDebuggerView::IsStreamEmpty(const SItem& sItem, int streamType) const
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());
    assert(pAISystem);

    IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(sItem.type, sItem.name.toUtf8().data());
    IAIDebugRecord* pRecord = (pAI ? pAI->GetAIDebugRecord() : NULL);
    IAIDebugStream* pStream = (pRecord ? pRecord->GetStream((IAIRecordable::e_AIDbgEvent)streamType) : NULL);

    return (!pStream || pStream->IsEmpty());
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::UpdateView()
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());
    if (!pAISystem)
    {
        return;
    }

    const size_t streamCount = m_streams.size();

    TItemList newList;

    /*AutoAIObjectIter  it = pAISystem->GetFirstAIObject(OBJFILTER_TYPE, AIOBJECT_ACTOR);
    for(; it->GetObject(); it->Next())
        newList.push_front(SItem(it->GetObject()->GetName(), AIOBJECT_ACTOR));
    AutoAIObjectIter    itv = pAISystem->GetFirstAIObject(OBJFILTER_TYPE, AIOBJECT_VEHICLE);
    for(; itv->GetObject(); itv->Next())
        newList.push_front(SItem(itv->GetObject()->GetName(), AIOBJECT_VEHICLE));
    AutoAIObjectIter    itd = pAISystem->GetFirstAIObject(OBJFILTER_TYPE, AIOBJECT_DUMMY);
    for(; itd->GetObject(); itd->Next())
        newList.push_front(SItem(itd->GetObject()->GetName(), AIOBJECT_DUMMY));

    AutoAIObjectIter    itp = pAISystem->GetFirstAIObject(OBJFILTER_TYPE, AIOBJECT_PLAYER);
    for(; itp->GetObject(); itp->Next())
        newList.push_front(SItem(itp->GetObject()->GetName(), AIOBJECT_PLAYER));*/

    // System objects
    {
        AutoAIObjectIter pIter(pAISystem->GetAIObjectManager()->GetFirstAIObject(OBJFILTER_TYPE, 0));
        while (IAIObject* pObject = pIter->GetObject())
        {
            if (pObject->GetAIDebugRecord() != NULL)
            {
                newList.push_front(SItem(pObject->GetName(), pObject->GetAIType()));
            }
            pIter->Next();
        }
    }

    // Dummy objects
    {
        AutoAIObjectIter pIter(pAISystem->GetAIObjectManager()->GetFirstAIObject(OBJFILTER_DUMMYOBJECTS, 0));
        while (IAIObject* pObject = pIter->GetObject())
        {
            if (pObject->GetAIDebugRecord() != NULL)
            {
                newList.push_front(SItem(pObject->GetName(), pObject->GetAIType()));
            }
            pIter->Next();
        }
    }

    for (TItemList::iterator it = newList.begin(); it != newList.end(); ++it)
    {
        // Copy over options from current list
        const SItem* pOldItemEntry = FindItemByName(it->name);
        if (pOldItemEntry)
        {
            it->bDebugEnabled = pOldItemEntry->bDebugEnabled;
            it->bSetView = pOldItemEntry->bSetView;
            it->debugColor = pOldItemEntry->debugColor;
        }
    }

    m_items.clear();
    m_items = newList;
    m_items.sort();

    int y = 0;
    TItemList::iterator itNewItem = m_items.begin();
    while (itNewItem != m_items.end())
    {
        SItem& item = *itNewItem;

        // Check if it has any streams that aren't empty; calculate height using non-empty streams only.
        int h = 0;
        for (size_t stream = 0; stream < streamCount; ++stream)
        {
            if (!IsStreamEmpty(item, m_streams[stream].type))
            {
                h += m_streams[stream].height;
            }
        }

        if (h > 0)
        {
            item.y = y;
            item.h = h;
            if (item.bDebugEnabled)
            {
                item.h = max(item.h, ITEM_HEIGHT * 3);
            }
            y += item.h + 2;    // small padding to separate sections.

            ++itNewItem;
        }
        else
        {
            // Remove it from the stream list
            itNewItem = m_items.erase(itNewItem);
        }
    }

    m_listFullHeight = y;

    UpdateAIDebugger();
    updateGeometry();
}


//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::UpdateAIDebugger() const
{
    IAISystem* pAISystem = GetIEditor()->GetAI()->GetAISystem();
    if (pAISystem)
    {
        SAIRecorderDebugContext* pContext;
        pAISystem->GetRecorderDebugContext(pContext);

        pContext->fStartPos = m_timeRulerStart;
        pContext->fEndPos = m_timeRulerEnd;
        pContext->fCursorPos = m_timeCursor;

        // Refresh list of who to debug
        pContext->DebugObjects.clear();
        TItemList::const_iterator itItem = m_items.begin();
        TItemList::const_iterator itItemEnd = m_items.end();
        for (; itItem != itItemEnd; ++itItem)
        {
            const SItem& currItem(*itItem);
            SAIRecorderObjectDebugContext objectContext;

            objectContext.sName = currItem.name.toUtf8().data();
            objectContext.bEnableDrawing = currItem.bDebugEnabled;
            objectContext.bSetView = currItem.bSetView;
            objectContext.color.r = currItem.debugColor.red();
            objectContext.color.g = currItem.debugColor.green();
            objectContext.color.b = currItem.debugColor.blue();
            objectContext.color.a = 255;

            pContext->DebugObjects.push_back(objectContext);

            if (objectContext.bSetView)
            {
                Vec3 vPos, vDir;
                GetSetViewData(currItem, vPos, vDir);

                // Update camera viewport if needed to current pos
                CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
                if (pRenderViewport)
                {
                    const Vec3& vForward = vDir.GetNormalizedSafe(Vec3_OneY);
                    const Vec3 vRight = (vForward.Cross(Vec3_OneZ)).GetNormalizedSafe(Vec3_OneX);
                    const Vec3 vUp = vRight.Cross(vForward);

                    Matrix34 tm = Matrix34::CreateFromVectors(vRight, vForward, vUp, vPos);
                    pRenderViewport->SetViewTM(tm);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::resizeEvent(QResizeEvent* event)
{
    UpdateViewRects();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    DrawList(painter);
    DrawTimeline(painter);
    DrawDetails(painter);
    DrawRuler(painter);
}

//////////////////////////////////////////////////////////////////////////
inline int Blend(int a, int b, int c)
{
    return a + ((b - a) * c) / 255;
}

//////////////////////////////////////////////////////////////////////////
inline QColor Blend(const QColor& a, const QColor& b, int c)
{
    return QColor(Blend(a.red(), b.red(), c),
        Blend(a.green(), b.green(), c),
        Blend(a.blue(), b.blue(), c));
}

// From:
// Nice Numbers for Graph Labels
// by Paul Heckbert
// from "Graphics Gems", Academic Press, 1990
static float nicenum(float x, bool round)
{
    int exp;
    float f;
    float nf;

    exp = std::floorf(std::log10f(x));
    f = x / std::powf(10.0f, exp);          /* between 1 and 10 */
    if (round)
    {
        if (f < 1.5f)
        {
            nf = 1.0f;
        }
        else if (f < 3.0f)
        {
            nf = 2.0f;
        }
        else if (f < 7.0f)
        {
            nf = 5.0f;
        }
        else
        {
            nf = 10.0f;
        }
    }
    else
    {
        if (f <= 1.0f)
        {
            nf = 1.0f;
        }
        else if (f <= 2.0f)
        {
            nf = 2.0f;
        }
        else if (f <= 5.0f)
        {
            nf = 5.0f;
        }
        else
        {
            nf = 10.0f;
        }
    }
    return nf * std::powf(10.0f, exp);
}

//////////////////////////////////////////////////////////////////////////
int CAIDebuggerView::TimeToClient(float t)
{
    return (int)((t - m_timeStart) / m_timeScale);
}

//////////////////////////////////////////////////////////////////////////
float   CAIDebuggerView::ClientToTime(int x)
{
    return (float)x * m_timeScale + m_timeStart;
}

//////////////////////////////////////////////////////////////////////////
// Returns the item at the given clientY or NULL
CAIDebuggerView::SItem* CAIDebuggerView::ClientToItem(int clientY)
{
    const int yListOffset = m_listRect.top() + m_itemsOffset;

    TItemList::iterator endIter = m_items.end();
    for (TItemList::iterator iter = m_items.begin(); iter != endIter; ++iter)
    {
        SItem& item = *iter;

        const int minY = yListOffset + item.y;
        const int maxY = minY + item.h;

        if (clientY >= minY && clientY <= maxY)
        {
            return &item;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawRuler(QPainter& dc)
{
    dc.save();

    dc.setClipRect(m_rulerRect);

    bool    hasRuler(false);
    int     rulerStartX, rulerEndX;
    float   timeRulerLen(std::fabsf(m_timeRulerEnd - m_timeRulerStart));

    if (timeRulerLen > 0.001f)
    {
        rulerStartX = qBound(m_rulerRect.left(), m_rulerRect.left() + TimeToClient(m_timeRulerStart), m_rulerRect.right());
        rulerEndX = qBound(m_rulerRect.left(), m_rulerRect.left() + TimeToClient(m_timeRulerEnd), m_rulerRect.right());
        if (rulerEndX < rulerStartX)
        {
            std::swap(rulerEndX, rulerStartX);
        }
        hasRuler = true;
    }

    dc.fillRect(m_rulerRect, palette().color(QPalette::Normal, QPalette::Button));

    if (hasRuler)
    {
        dc.fillRect(
            rulerStartX, m_rulerRect.top(), rulerEndX - rulerStartX, m_rulerRect.height(),
            Blend(palette().color(QPalette::Normal, QPalette::Button), palette().color(QPalette::Normal, QPalette::Light), 100));
    }

    int ntick = (int)std::ceilf(m_timelineRect.width() / (float)TIME_TICK_SPACING);
    int w = ntick * TIME_TICK_SPACING;

    float   timeMin = m_timeStart;
    float   timeMax = m_timeStart + m_timeScale * w;

    float   range = nicenum(timeMax - timeMin, 0.0f);
    float   d = nicenum(range / (ntick - 1), false);
    float   graphMin = std::floorf(timeMin / d) * d;
    float   graphMax = std::ceilf(timeMax / d) * d;
    int nfrac = max(-std::floorf(log10f(d)), 0.0f);
    int nsub = (int)(d / std::powf(10.0f, std::floorf(std::log10f(d))));
    if (nsub < 2)
    {
        nsub *= 10;
    }

    dc.setFont(m_smallFont);

    QFontMetrics smallMetrics(m_smallFont);
    int texth = smallMetrics.height();
    int y = m_rulerRect.bottom() - 2 - texth;

    QPen textPen(palette().color(QPalette::Normal, QPalette::ButtonText));
    QPen linePen(palette().color(QPalette::Normal, QPalette::Dark));

    // Make this loop rock solid
    int steps = (graphMax - graphMin) / d;

    float t = graphMin;
    for (int i = 0; i < steps; i++, t = graphMin + i * d)
    {
        dc.setPen(linePen);
        int x = m_timelineRect.left() + TimeToClient(t);

        if (x >= m_rulerRect.left() && x < m_rulerRect.right())
        {
            dc.drawLine(x, m_rulerRect.top() + RULER_HEIGHT / 2, x, m_rulerRect.top() + RULER_HEIGHT);
        }

        for (int i = 1; i < nsub; i++)
        {
            float   t2 = t + ((float)i / (float)nsub) * d;
            int x2 = m_timelineRect.left() + TimeToClient(t2);
            if (x2 >= m_rulerRect.left() && x2 < m_rulerRect.right())
            {
                dc.drawLine(x2, m_rulerRect.top() + RULER_HEIGHT * 3 / 4, x2, m_rulerRect.top() + RULER_HEIGHT);
            }
        }

        QString label = QString("%1").arg(t, nfrac);
        dc.setPen(textPen);
        dc.drawText(QRect(x + 3, y, m_rulerRect.right(), m_rulerRect.bottom()), label);
    }

    dc.setClipping(false);

    if (hasRuler)
    {
        dc.setPen(palette().color(QPalette::Normal, QPalette::Highlight));

        dc.drawLine(rulerStartX, m_rulerRect.top(), rulerStartX, m_timelineRect.bottom());
        dc.drawLine(rulerEndX, m_rulerRect.top(), rulerEndX, m_timelineRect.bottom());

        QString label = QString("%1").arg(timeRulerLen, 2, 'f', 2);
        QRect extend = smallMetrics.boundingRect(label);
        extend.adjust(0, 0, 4, 0);

        if (extend.width() < std::abs(rulerEndX - rulerStartX))
        {
            dc.drawLine(rulerStartX, m_rulerRect.top() + 2 + texth / 2, (rulerStartX + rulerEndX) / 2 - extend.width() / 2, m_rulerRect.top() + 2 + texth / 2);
            dc.drawLine(rulerEndX, m_rulerRect.top() + 2 + texth / 2, (rulerStartX + rulerEndX) / 2 + extend.width() / 2, m_rulerRect.top() + 2 + texth / 2);
        }

        dc.drawText(
            QRect(rulerStartX, m_rulerRect.top(), rulerEndX - rulerStartX, m_rulerRect.top() + 2 + texth),
            label, Qt::AlignHCenter | Qt::AlignVCenter);
    }

    dc.setPen(QPen(Qt::black, 1));
    dc.drawRect(m_rulerRect.adjusted(0, 0, -1, -1));

    {
        dc.setPen(QColor(240, 16, 6));
        int cursorX = m_rulerRect.left() + TimeToClient(m_timeCursor);
        if (cursorX >= m_rulerRect.left() && cursorX < m_rulerRect.right())
        {
            dc.drawLine(cursorX, m_rulerRect.top(), cursorX, m_timelineRect.bottom());
            QString label = QString("%1").arg(m_timeCursor, 0, 'f', 2);
            dc.drawText(QRect(QPoint(cursorX + 2, m_rulerRect.top() + 2), m_rulerRect.bottomRight()), label);
        }
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawList(QPainter& dc)
{
    dc.save();

    dc.setClipRect(m_listRect.adjusted(0, 0, 1, 1));

    const QColor& stdBG = palette().color(QPalette::Normal, QPalette::Window);
    QColor altBG = Blend(stdBG, Qt::black, 8);
    QColor focusBG = Blend(stdBG, Qt::blue, 32);

    dc.fillRect(m_listRect, stdBG);

    QFont normal;
    QFontMetrics normalMetrics(normal);
    QFontMetrics smallMetrics(m_smallFont);

    dc.setFont(m_smallFont);

    for (size_t i = 0; i < m_streams.size(); i++)
    {
        m_streams[i].nameWidth = smallMetrics.width(m_streams[i].name);
    }

    int texth = normalMetrics.height();
    if (m_items.empty())
    {
        dc.setPen(palette().color(QPalette::Disabled, QPalette::Text));
        dc.drawText(m_listRect.topLeft(), QStringLiteral("No AI Objects"));
    }
    else
    {
        const int margin = ITEM_HEIGHT / 3;
        int i = 0;
        for (const SItem& item : m_items)
        {
            ++i;
            int y = m_listRect.top() + item.y;

            QRect itemBounds = { 0, y, m_listRect.width(), item.h };

            // Draw BG
            {
                const bool hasFocus = (item.name == m_sFocusedItemName);
                const QColor& bgColor = hasFocus ? focusBG : (((i & 1) == 1) ? stdBG : altBG);

                dc.fillRect(itemBounds, bgColor);
            }

            const QColor& textColor = palette().color(QPalette::Normal, QPalette::ButtonText);
            dc.setPen(textColor);
            dc.setFont(normal);

            dc.drawText(itemBounds.adjusted(margin, 0, 0, 0), Qt::AlignVCenter, item.name);

            if (item.bDebugEnabled)
            {
                QString sDebugging = "Debugging";
                if (item.bSetView)
                {
                    sDebugging += " [SET VIEW]";
                }

                dc.drawText(itemBounds.bottomLeft(), sDebugging);

                dc.setPen(item.debugColor);
                dc.drawText(itemBounds.left(), itemBounds.bottom() + normalMetrics.lineSpacing(), sDebugging);

                dc.setPen(textColor);
            }

            dc.setPen(palette().color(QPalette::Normal, QPalette::Dark));
            dc.setFont(m_smallFont);
            int yy = 0;
            QRect streamBounds = itemBounds;
            streamBounds.adjust(0, 0, -margin, 0);
            for (const auto& stream : m_streams)
            {
                streamBounds.setHeight(stream.height);
                dc.drawText(streamBounds, Qt::AlignRight | Qt::AlignVCenter, stream.name);

                streamBounds.moveTop(streamBounds.top() + stream.height);
            }
        }
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawDetails(QPainter& dc)
{
    dc.save();

    dc.setClipRect(m_detailsRect.adjusted(0, 0, 1, 1));

    const QColor& background = palette().color(QPalette::Normal, QPalette::Button);

    if (m_items.empty())
    {
        dc.restore();
        return;
    }

    QColor altBG = Blend(background, Qt::black, 8);

    QFont normal;
    QFontMetrics normalMetrics(normal);
    dc.setFont(normal);

    int texth = normalMetrics.height();

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    dc.setPen(palette().color(QPalette::Normal, QPalette::ButtonText));

    int i = 0;
    for (const SItem& item : m_items)
    {
        ++i;
        int y = m_detailsRect.top() + item.y;
        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        QRect detailsRect = { m_detailsRect.left(), y, m_detailsRect.width(), item.h };

        // Color alternate lines
        if ((i & 1) == 1)
        {
            dc.fillRect(detailsRect, altBG);
        }

        detailsRect.moveLeft(detailsRect.left() + ITEM_HEIGHT / 3);
        for (const auto& stream : m_streams)
        {
            IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)stream.type);
            if (pStream && !pStream->IsEmpty())
            {
                detailsRect.setHeight(stream.height);
                if (stream.track == TRACK_TEXT)
                {
                    DrawStreamDetail(dc, pStream, detailsRect, stream.color);
                }
                else if (stream.track == TRACK_FLOAT)
                {
                    DrawStreamFloatDetail(dc, pStream, detailsRect, stream.color);
                }
            }
            detailsRect.moveTop(detailsRect.top() + stream.height);
        }
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawStreamDetail(QPainter& dc, IAIDebugStream* pStream, const QRect& bounds, const QColor& color)
{
    dc.save();

    dc.setClipRect(m_detailsRect.adjusted(0, 0, 1, 1));

    float   curTime;
    pStream->Seek(m_timeCursor);
    char*   pString = (char*)(pStream->GetCurrent(curTime));

    if (pString)
    {
        dc.drawText(bounds, Qt::AlignVCenter, pString);
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawStreamFloatDetail(QPainter& dc, IAIDebugStream* pStream, const QRect& bounds, const QColor& color)
{
    float   curTime;
    pStream->Seek(m_timeCursor);
    float*  pVal = (float*)(pStream->GetCurrent(curTime));

    if (pVal)
    {
        dc.drawText(bounds, Qt::AlignVCenter, QString("%1").arg(*pVal, 0, 'f', 3));
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawTimeline(QPainter& dc)
{
    dc.save();

    dc.setClipRect(m_timelineRect.adjusted(0, 0, 1, 1));

    const QColor& background = palette().color(QPalette::Normal, QPalette::Window);
    const QColor& dark = palette().color(QPalette::Normal, QPalette::Dark);
    dc.fillRect(m_timelineRect, background);

    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    QColor altBG = Blend(background, Qt::black, 8);

    dc.setPen(palette().color(QPalette::Normal, QPalette::ButtonText));

    QFontMetrics smallMetrics(m_smallFont);
    int texth = smallMetrics.height();

    int n = 0;
    for (const SItem& item : m_items)
    {
        ++n;

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        int y = m_timelineRect.top() + item.y;
        for (const auto& stream : m_streams)
        {
            IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)stream.type);
            if (pStream && !pStream->IsEmpty())
            {
                if (stream.track == TRACK_TEXT)
                {
                    DrawStream(dc, pStream, y, stream.height, stream.color, n);
                }
                else if (stream.track == TRACK_FLOAT)
                {
                    DrawStreamFloat(dc, pStream, y, stream.height, stream.color, n, stream.vmin, stream.vmax);
                }

                dc.setPen(Blend(background, dark, 128));
            }
            dc.drawLine(m_timelineRect.x(), y, m_timelineRect.right(), y);
            y += stream.height;
        }
        dc.setPen(dark);
        dc.drawLine(m_timelineRect.x(), y, m_timelineRect.right(), y);
    }

    dc.setPen(Qt::black);
    dc.drawLine(m_timelineRect.topLeft(), m_timelineRect.bottomLeft());
    dc.drawLine(m_timelineRect.topRight(), m_timelineRect.bottomRight());

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawStream(QPainter& dc, IAIDebugStream* pStream, int y, int ih, const QColor& color, int i)
{
    dc.save();

    const QColor bg = Blend(palette().color(QPalette::Normal, QPalette::Window), color, ((i & 1) == 0) ? 64 : 32);

    dc.setPen(QPen(color, 2));

    float   curTime;
    pStream->Seek(m_timeStart);
    char*   pString = (char*)(pStream->GetCurrent(curTime));

    int leftSide, rightSide;

    leftSide = TimeToClient(curTime);
    rightSide = 0;

    while (leftSide < m_timelineRect.width() && pStream->GetCurrentIdx() < pStream->GetSize())
    {
        float nextTime;
        char* pNextString = (char*)(pStream->GetNext(nextTime));
        rightSide = pNextString ? TimeToClient(nextTime) : pStream->GetEndTime();

        QRect bounds = { m_timelineRect.x() + std::max(leftSide, 0), y + 1, qMax(1, rightSide - leftSide), ih };

        dc.fillRect(bounds, bg);

        dc.drawLine(bounds.topLeft(), bounds.bottomLeft());

        if (bounds.width() > 4)
        {
            dc.drawText(bounds.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, pString);
        }

        curTime = nextTime;
        leftSide = rightSide;
        pString = pNextString;
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DrawStreamFloat(QPainter& dc, IAIDebugStream* pStream, int iy, int ih, const QColor& color, int i, float vmin, float vmax)
{
    dc.save();

    const QColor& background = palette().color(QPalette::Normal, QPalette::Window);

    dc.setPen(color);

    float   curTime;
    pStream->Seek(m_timeStart);
    float*  pVal = (float*)(pStream->GetCurrent(curTime));

    int y = iy;
    int leftSide, rightSide;

    leftSide = m_timelineRect.left() + qBound(0, TimeToClient(curTime), m_timelineRect.width());
    rightSide = 0;

    int streamStartX = qBound(0, TimeToClient(pStream->GetStartTime()), m_timelineRect.width());
    int streamEndX = qBound(0, TimeToClient(pStream->GetEndTime()), m_timelineRect.width());
    if (streamEndX > streamStartX)
    {
        QColor bg = Blend(background, color, ((i & 1) == 0) ? 32 : 16);
        dc.fillRect(m_timelineRect.left() + streamStartX, y + 1, qMax(1, streamEndX - streamStartX), ih, bg);
    }

    QPoint firstPoint;

    if (pVal)
    {
        int x = leftSide;
        float   v = qBound(0.0f, (*pVal - vmin) / (vmax - vmin), 1.0f);
        int y = iy + ih - 1 - (int)(v * (ih - 3));
        firstPoint = { x, y };
    }

    while (leftSide < m_timelineRect.right() && pStream->GetCurrentIdx() < pStream->GetSize())
    {
        float nextTime;
        float* pNextVal = (float*)(pStream->GetNext(nextTime));

        if (pNextVal)
        {
            int x = m_timelineRect.left() + qBound(0, TimeToClient(nextTime), m_timelineRect.width());
            float   v = qBound(0.0f, (*pNextVal - vmin) / (vmax - vmin), 1.0f);
            int y = iy + ih - 1 - (int)(v * (ih - 3));
            if (x > firstPoint.x() + 1)
            {
                dc.drawLine(firstPoint, { x, firstPoint.y() });
                dc.drawLine(QPoint { x, firstPoint.y() }, QPoint { x, y });
                firstPoint = { x, y };
            }

            dc.drawLine(firstPoint, { x, y });
            firstPoint = { x, y };
        }

        curTime = nextTime;
        pVal = pNextVal;
    }

    dc.restore();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::OnSetCursor(const QPoint& position)
{
    int rulerHit = HitTestRuler(position);
    int cursorHit = HitTestTimeCursor(position);
    int separatorHit = HitTestSeparators(position);

    if ((m_dragType == DRAG_NONE && (rulerHit == RULERHIT_START || rulerHit == RULERHIT_END)) ||
        m_dragType == DRAG_RULER_START || m_dragType == DRAG_RULER_END)
    {
        setCursor(Qt::SizeHorCursor);
    }
    else if ((m_dragType == DRAG_NONE && rulerHit == RULERHIT_SEGMENT) || m_dragType == DRAG_RULER_SEGMENT)
    {
        setCursor(CMFCUtils::LoadCursor(IDC_ARRWHITE));
    }
    else if ((m_dragType == DRAG_NONE && separatorHit != SEPARATORHIT_NONE) || m_dragType == DRAG_LIST_WIDTH || m_dragType == DRAG_DETAILS_WIDTH)
    {
        setCursor(Qt::SizeHorCursor);
    }
    else if ((m_dragType == DRAG_NONE && cursorHit == CURSORHIT_CURSOR) || m_dragType == DRAG_TIME_CURSOR)
    {
        setCursor(Qt::SizeHorCursor);
    }
    else if (rulerHit == RULERHIT_EMPTY)
    {
        setCursor(CMFCUtils::LoadCursor(IDC_HIT_CURSOR));
    }
    else if (m_dragType == DRAG_PAN)
    {
        setCursor(Qt::ClosedHandCursor);
    }
    else
    {
        unsetCursor();
    }
}
//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::SetCursorPrevEvent(bool bShiftDown)
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());
    float fSetTime = FLT_MIN;

    if (bShiftDown)
    {
        TItemList::const_iterator itItem = m_items.begin();
        TItemList::const_iterator itItemEnd = m_items.end();
        for (; itItem != itItemEnd; ++itItem)
        {
            const SItem& currItem(*itItem);

            IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(currItem.type, currItem.name.toUtf8().data());
            if (!pAI)
            {
                continue;
            }

            IAIDebugRecord* pRecord = pAI->GetAIDebugRecord();
            if (!pRecord)
            {
                continue;
            }

            TStreamVec::const_iterator itStream = m_streams.begin();
            TStreamVec::const_iterator itStreamEnd = m_streams.end();
            for (; itStream != itStreamEnd; ++itStream)
            {
                IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)itStream->type);
                if (!pStream || pStream->IsEmpty())
                {
                    continue;
                }

                // Seek to start, look for best one before we pass current time
                pStream->Seek(0);
                float fPrevTime = 0.0f;
                float fCurrTime = 0.0f;
                if (pStream->GetCurrent(fCurrTime))
                {
                    while (fCurrTime < m_timeCursor)
                    {
                        fPrevTime = fCurrTime;
                        if (!pStream->GetNext(fCurrTime))
                        {
                            break;
                        }
                    }

                    if (fPrevTime > fSetTime)
                    {
                        fSetTime = fPrevTime;
                    }
                }
            }
        }
    }
    else
    {
        fSetTime = m_timeCursor - 0.1f;
    }

    m_timeCursor = max(fSetTime, GetRecordStartTime());
    UpdateAIDebugger();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::SetCursorNextEvent(bool bShiftDown)
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());
    float fSetTime = FLT_MAX;

    if (bShiftDown)
    {
        TItemList::const_iterator itItem = m_items.begin();
        TItemList::const_iterator itItemEnd = m_items.end();
        for (; itItem != itItemEnd; ++itItem)
        {
            const SItem& currItem(*itItem);

            IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(currItem.type, currItem.name.toUtf8().data());
            if (!pAI)
            {
                continue;
            }

            IAIDebugRecord* pRecord = pAI->GetAIDebugRecord();
            if (!pRecord)
            {
                continue;
            }

            TStreamVec::const_iterator itStream = m_streams.begin();
            TStreamVec::const_iterator itStreamEnd = m_streams.end();
            for (; itStream != itStreamEnd; ++itStream)
            {
                IAIDebugStream* pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)itStream->type);
                if (!pStream || pStream->IsEmpty())
                {
                    continue;
                }

                // Seek to current time, get next
                pStream->Seek(m_timeCursor);
                float fCurrTime = 0.0f;
                if (pStream->GetCurrent(fCurrTime))
                {
                    while (fCurrTime <= m_timeCursor)
                    {
                        if (!pStream->GetNext(fCurrTime))
                        {
                            break;
                        }
                    }

                    if (fCurrTime > m_timeCursor && fCurrTime < fSetTime)
                    {
                        fSetTime = fCurrTime;
                    }
                }
            }
        }
    }
    else
    {
        fSetTime = m_timeCursor + 0.1f;
    }

    m_timeCursor = min(fSetTime, GetRecordEndTime());
    UpdateAIDebugger();
    update();
}

//////////////////////////////////////////////////////////////////////////
float   CAIDebuggerView::GetRecordStartTime()
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    float   minTime = 0.0f;
    int i = 0;
    for (TItemList::iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        const SItem&    item (*it);

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        IAIDebugStream* pStream(0);
        for (size_t i = 0; i < m_streams.size(); i++)
        {
            pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)m_streams[i].type);
            if (pStream && !pStream->IsEmpty())
            {
                const float t = pStream->GetStartTime();
                if (t > FLT_EPSILON)
                {
                    minTime = minTime > FLT_EPSILON ? min(minTime, t) : t;
                }
            }
        }
    }

    return minTime;
}

//////////////////////////////////////////////////////////////////////////
float   CAIDebuggerView::GetRecordEndTime()
{
    IAISystem* pAISystem(GetIEditor()->GetAI()->GetAISystem());

    float   maxTime = 0.0f;
    int i = 0;
    for (TItemList::iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        const SItem&    item (*it);

        IAIObject* pAI = pAISystem->GetAIObjectManager()->GetAIObjectByName(item.type, item.name.toUtf8().data());
        if (!pAI)
        {
            continue;
        }

        IAIDebugRecord* pRecord(pAI->GetAIDebugRecord());
        if (!pRecord)
        {
            continue;
        }

        IAIDebugStream* pStream(0);
        for (size_t i = 0; i < m_streams.size(); i++)
        {
            pStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)m_streams[i].type);
            if (pStream && !pStream->IsEmpty())
            {
                const float t = pStream->GetEndTime();
                maxTime = max(maxTime, t);
            }
        }
    }

    return maxTime;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::UpdateStreamDesc()
{
    m_streams.clear();

    if (m_streamSignalReceived->isChecked())
    {
        m_streams.push_back(SStreamDesc("Signal", IAIRecordable::E_SIGNALRECIEVED, TRACK_TEXT, QColor(13, 41, 122), ITEM_HEIGHT));
    }
    if (m_streamSignalReceivedAux->isChecked())
    {
        m_streams.push_back(SStreamDesc("AuxSignal", IAIRecordable::E_SIGNALRECIEVEDAUX, TRACK_TEXT, QColor(49, 122, 13), ITEM_HEIGHT));
    }
    if (m_streamBehaviorSelected->isChecked())
    {
        m_streams.push_back(SStreamDesc("Behavior", IAIRecordable::E_BEHAVIORSELECTED, TRACK_TEXT, QColor(218, 157, 10), ITEM_HEIGHT));
    }
    if (m_streamAttenionTarget->isChecked())
    {
        m_streams.push_back(SStreamDesc("AttTarget", IAIRecordable::E_ATTENTIONTARGET, TRACK_TEXT, QColor(167, 0, 0), ITEM_HEIGHT));
    }
    if (m_streamRegisterStimulus->isChecked())
    {
        m_streams.push_back(SStreamDesc("Stimulus", IAIRecordable::E_REGISTERSTIMULUS, TRACK_TEXT, QColor(100, 200, 100), ITEM_HEIGHT));
    }
    if (m_streamGoalPipeSelected->isChecked())
    {
        m_streams.push_back(SStreamDesc("GoalPipe", IAIRecordable::E_GOALPIPESELECTED, TRACK_TEXT, QColor(171, 60, 184), ITEM_HEIGHT));
    }
    if (m_streamGoalPipeInserted->isChecked())
    {
        m_streams.push_back(SStreamDesc("GoalPipe Ins.", IAIRecordable::E_GOALPIPEINSERTED, TRACK_TEXT, QColor(110, 60, 184), ITEM_HEIGHT));
    }
    if (m_streamLuaComment->isChecked())
    {
        m_streams.push_back(SStreamDesc("Lua Comment", IAIRecordable::E_LUACOMMENT, TRACK_TEXT, QColor(133, 126, 108), ITEM_HEIGHT));
    }
    if (m_streamPersonalLog->isChecked())
    {
        m_streams.push_back(SStreamDesc("Personal Log", IAIRecordable::E_PERSONALLOG, TRACK_TEXT, QColor(133, 126, 108), ITEM_HEIGHT));
    }

    if (m_streamHealth->isChecked())
    {
        m_streams.push_back(SStreamDesc("Health", IAIRecordable::E_HEALTH, TRACK_FLOAT, QColor(23, 154, 229), ITEM_HEIGHT * 2, 0.0f, 1000.0f));
    }
    if (m_streamHitDamage->isChecked())
    {
        m_streams.push_back(SStreamDesc("Hit Damage", IAIRecordable::E_HIT_DAMAGE, TRACK_TEXT, QColor(229, 96, 23), ITEM_HEIGHT));
    }
    if (m_streamDeath->isChecked())
    {
        m_streams.push_back(SStreamDesc("Death", IAIRecordable::E_DEATH, TRACK_TEXT, QColor(32, 19, 6), ITEM_HEIGHT));
    }

    if (m_streamPressureGraph->isChecked())
    {
        m_streams.push_back(SStreamDesc("Pressure Graph", IAIRecordable::E_PRESSUREGRAPH, TRACK_FLOAT, QColor(255, 0, 0), ITEM_HEIGHT, 0.0f, 1.0f));
    }

    if (m_streamBookmarks->isChecked())
    {
        m_streams.push_back(SStreamDesc("Bookmarks", IAIRecordable::E_BOOKMARK, TRACK_TEXT, QColor(0, 0, 255), ITEM_HEIGHT));
    }
}

//////////////////////////////////////////////////////////////////////////
const CAIDebuggerView::SItem* CAIDebuggerView::FindItemByName(const QString& sName) const
{
    for (const SItem& item : m_items)
    {
        if (item.name.compare(sName, Qt::CaseInsensitive) == 0)
        {
            return &item;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DebugEnableAll()
{
    TItemList::iterator itItem = m_items.begin();
    TItemList::iterator itItemEnd = m_items.end();
    for (; itItem != itItemEnd; ++itItem)
    {
        SItem& currItem(*itItem);
        currItem.bDebugEnabled = true;
    }

    UpdateView();
    UpdateAIDebugger();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CAIDebuggerView::DebugDisableAll()
{
    TItemList::iterator itItem = m_items.begin();
    TItemList::iterator itItemEnd = m_items.end();
    for (; itItem != itItemEnd; ++itItem)
    {
        SItem& currItem(*itItem);
        currItem.bDebugEnabled = false;
    }

    UpdateView();
    UpdateAIDebugger();
    update();
}

#include <AI/AIDebuggerView.moc>

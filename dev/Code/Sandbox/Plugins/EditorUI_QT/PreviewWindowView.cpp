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
#include "stdafx.h"

//Cry
#include <IParticles.h>
#include <ParticleParams.h>

//Editor
#include <Particles/ParticleItem.h>
#include <IEditorParticleUtils.h>
#include <Include/ILogFile.h>

//Local
#include <PreviewWindowView.h>
#include <Controls/QToolTipWidget.h>
#include "VariableWidgets/QAmazonDoubleSpinBox.h"
#include "VariableWidgets/QCustomColorDialog.h"
#include "UIFactory.h"
#include "Utils.h"
#include "ContextMenu.h"
#include "ParticlePreviewModelView.h"

//QT
#include "Particles/ParticleItem.h"
#include <QSettings>
#include <QPushButton>
#include <QFrame>
#include <QPainter>
#include <QDateTime>
#include <QWidgetAction>
#include <QShortcutEvent>
#include <QMessageBox>

CPreviewWindowView::CPreviewWindowView(QWidget* parent)
    : QWidget(parent)
    , m_pActiveItem(nullptr)
{
    BuildPreviewWindowUI();

    m_tooltip = new QToolTipWidget(this);
    HandleEmitterViewChange(ShowEmitterSetting::EmitterOnly);

    /////////////////////////////////////////////////////////////////////////////////////////////
    customEmitterMenu = new ContextMenu(this);
    loadEmitterMenu = new ContextMenu(this);
    m_PreviewMenu = new QMenu;
    connect(m_PreviewMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));
}

CPreviewWindowView::~CPreviewWindowView()
{
    delete m_tooltip;
    delete m_PreviewMenu;
    delete m_previewModelView;
}

void CPreviewWindowView::ExecContextMenu(QPoint position)
{
    CRY_ASSERT(m_previewModelView);

    ContextMenu* menu = new ContextMenu(this);

    menu->setVisible(false);
    menu->addAction("Wireframe view")->setData(static_cast<unsigned int>(MenuActions::PRESET_WIREFRAME_VIEW));
    if (m_numOfParticle->isHidden())
    {
        menu->addAction("Show particle count")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_PARTICLE_COUNT));
    }
    else
    {
        menu->addAction("Hide particle count")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_PARTICLE_COUNT));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX))
    {
        menu->addAction("Hide bounding box")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_BOUNDING_BOX));
    }
    else
    {
        menu->addAction("Show bounding box")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_BOUNDING_BOX));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO))
    {
        menu->addAction("Hide gizmo")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GIZMO));
    }
    else
    {
        menu->addAction("Show gizmo")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GIZMO));
    }

    if (m_botFrame->isHidden())
    {
        menu->addAction("Show playback controls")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS));
    }
    else
    {
        menu->addAction("Hide playback controls")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID))
    {
        menu->addAction("Hide grid")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GRID));
    }
    else
    {
        menu->addAction("Show grid")->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GRID));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE))
    {
        menu->addAction("Hide Emitter Shape")->setData(static_cast<unsigned int>(MenuActions::PRESET_TOGGLE_EMITTERSHAPE));
    }
    else
    {
        menu->addAction("Show Emitter Shape")->setData(static_cast<unsigned int>(MenuActions::PRESET_TOGGLE_EMITTERSHAPE));
    }

    menu->addSeparator();
    menu->addAction("Background color...")->setData(static_cast<unsigned int>(MenuActions::PRESET_BACKGROUND_COLOR));
    menu->addAction("Grid color...")->setData(static_cast<unsigned int>(MenuActions::PRESET_GRID_COLOR));
    menu->addAction("Reset colors")->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET_COLOR));

    //************************Spline Sub-menu********************************************************
    menu->addSeparator();
    QAction* newaction = nullptr;
    QMenu* splineMovementMenu = new ContextMenu(this);
    splineMovementMenu->setTitle("Move on spline");

    QMenu* splineModeMenu = new ContextMenu("Spline mode", this);
    splineMovementMenu->addMenu(splineModeMenu);
    newaction = splineModeMenu->addAction("Single path");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SINGLEPATH_EMITTER));
    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING) || m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG))
    {
        newaction->setCheckable(false);
    }
    else
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }

    newaction = splineModeMenu->addAction("Loop");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_LOOP_EMITTER));
    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING))
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splineModeMenu->addAction("Ping-pong");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_PINGPONG_EMITTER));
    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG))
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    QMenu* splineShapeMenu = new ContextMenu("Spline shape", this);
    splineMovementMenu->addMenu(splineShapeMenu);
    newaction = splineShapeMenu->addAction("Line");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_LINE));
    if (m_previewModelView->GetSplineMode() == CPreviewModelView::SplineMode::LINE)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splineShapeMenu->addAction("Sine Wave");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_SINE));
    if (m_previewModelView->GetSplineMode() == CPreviewModelView::SplineMode::SINEWAVE)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splineShapeMenu->addAction("Coil");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_COIL));
    if (m_previewModelView->GetSplineMode() == CPreviewModelView::SplineMode::COIL)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    QMenu* splideSpeedMenu = new ContextMenu("spline speed", this);
    splineMovementMenu->addMenu(splideSpeedMenu);
    newaction = splideSpeedMenu->addAction("Speed x1");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_SPEED_X1));
    if (m_previewModelView->GetSplineSpeedMultiplier() == 1)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splideSpeedMenu->addAction("Speed x2");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_SPEED_X2));
    if (m_previewModelView->GetSplineSpeedMultiplier() == 2)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splideSpeedMenu->addAction("Speed x3");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_SPEED_X3));
    if (m_previewModelView->GetSplineSpeedMultiplier() == 3)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    newaction = splideSpeedMenu->addAction("Speed x5");
    newaction->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_SPEED_X5));
    if (m_previewModelView->GetSplineSpeedMultiplier() == 5)
    {
        newaction->setCheckable(true);
        newaction->setChecked(true);
    }
    else
    {
        newaction->setCheckable(false);
    }

    menu->addMenu(splineMovementMenu);
    menu->addAction("Reset spline settings")->setData(static_cast<unsigned int>(MenuActions::PRESET_SPLINE_RESET_DEFAULT));

    //reset everything to default
    menu->addSeparator();
    menu->addAction("Reset to default")->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET_DEFAULT));

    //connect all actions
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));
    connect(splineModeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));
    connect(splineShapeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));
    connect(splideSpeedMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));
    connect(menu, &QMenu::aboutToHide, this, [menu](){ menu->deleteLater(); });
    menu->popup(position);
}

void CPreviewWindowView::BuildPreviewWindowUI()
{
    dockLayout = new QVBoxLayout();
    setLayout(dockLayout);

    m_topFrame = new QFrame(this);
    m_topFrame->setObjectName("PreviewPanelFrame");
    dockLayout->addWidget(m_topFrame, 0, Qt::AlignTop);

    ///////////////////////////////////////////////////////////////////////
    // Our special widget that will allow the CRY renderer to do its thing.
    if (gEnv == nullptr) //Just in case this is not set...
    {
        gEnv = GetIEditor()->GetEnv();
    }
    m_previewModelView = new ParticlePreviewModelView(this);

    m_previewModelView->SetPostUpdateCallback([=]()
        {
            UpdateTimeLabel();
            UpdatePlaybackUI();
            UpdateParticleCount();
        });

    m_previewModelView->SetContextMenuCallback([=](Vec2i mousePos)
        {
            QPoint mpos(mousePos.x, mousePos.y);
            ExecContextMenu(mpos);
        });

    m_previewModelView->setObjectName("PreviewModelView");
    m_previewModelView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dockLayout->addWidget(m_previewModelView);
    ///////////////////////////////////////////////////////////////////////

    m_botFrame = new QFrame(this);
    m_botFrame->setObjectName("PreviewPanelFrame");
    dockLayout->addWidget(m_botFrame, 0, Qt::AlignBottom);

    dockLayout->setMargin(0);
    dockLayout->setSpacing(0);
    m_topFrame->setMaximumHeight(75);
    m_botFrame->setMaximumHeight(75);

    //top Layout
    QHBoxLayout* topFrameLayout = new QHBoxLayout();
    m_topFrame->setLayout(topFrameLayout);

    cameraDropdownButton = new QPushButton(this);
    cameraDropdownButton->setProperty("previewIconButton", true);
    QMenu* cameraDropdownMenu = new QMenu(this);
    m_showEmitterOnly = cameraDropdownMenu->addAction(tr("show Emitter only"));
    m_showEmitterOnly->setCheckable(true);
    m_showEmitterOnly->setChecked(true);
    m_showEmitterOnly->setData(0);
    m_showEmitterChildren = cameraDropdownMenu->addAction(tr("show Emitter with all children"));
    m_showEmitterChildren->setData(1);
    m_showEmitterChildrenParents = cameraDropdownMenu->addAction(tr("show Emitter with all children and parents "));
    m_showEmitterChildrenParents->setData(2);
    cameraDropdownButton->setMenu(cameraDropdownMenu);
    cameraDropdownButton->setIcon(QIcon(":/particleQT/icons/emitteronly.png"));
    topFrameLayout->addWidget(cameraDropdownButton, Qt::AlignLeft);

    //ZoomButton
    m_focusButton = new QPushButton(this);
    m_focusButton->setProperty("previewIconButton", true);
    m_focusButton->setIcon(QIcon(":/particleQT/icons/Zoom.png"));
    topFrameLayout->addWidget(m_focusButton, Qt::AlignLeft);

    //Time of day button
    m_timeOfDayButton = new QPushButton(this);
    m_timeOfDayButton->setProperty("previewIconButton", true);
    m_timeOfDayButton->setIcon(QIcon(":/particleQT/icons/TimeOfDay.png"));
    topFrameLayout->addWidget(m_timeOfDayButton, Qt::AlignLeft);

    m_numOfParticle = new QLabel("0P", this);
    m_numOfParticle->setAlignment(Qt::AlignRight);
    QFont particleNumberFont = m_numOfParticle->font();
    particleNumberFont.setPointSize(14);
    m_numOfParticle->setFont(particleNumberFont);
    topFrameLayout->addWidget(m_numOfParticle);

    m_wireframeButton = new QPushButton(this);
    m_wireframeButton->setProperty("previewIconButton", true);
    m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframe.png"));
    topFrameLayout->addWidget(m_wireframeButton);

    //bottom layout
    QGridLayout* bottomFrameLayout = new QGridLayout();
    m_botFrame->setLayout(bottomFrameLayout);

    m_resetButton = new QPushButton(this);
    m_resetButton->setProperty("previewIconButton", true);
    m_resetButton->setIcon(QIcon(":/particleQT/icons/reset.png"));
    bottomFrameLayout->addWidget(m_resetButton, 1, 0, Qt::AlignLeft);

    QLabel* speedButton = new QLabel(this);
    QPixmap pix(":/particleQT/icons/speed.png");
    speedButton->setPixmap(pix);
    speedButton->setMaximumWidth(28);
    bottomFrameLayout->addWidget(speedButton, 0, 0);

    m_playSpeedButton = new QAmazonDoubleSpinBox(this);
    m_playSpeedButton->setValue(1.0f);
    m_playSpeedButton->setMinimum(0);
    m_playSpeedButton->setMaximum(10.0f);
    m_playSpeedButton->setSingleStep(0.01);
    m_playSpeedButton->setMinimumWidth(50);
    m_playSpeedButton->setMaximumWidth(50);
    connect(m_playSpeedButton, SIGNAL(valueChanged(double)), this, SLOT(OnPlaySpeedButtonValueChanged(double)));
    bottomFrameLayout->addWidget(m_playSpeedButton, 0, 1);

    m_timeLabel = new QLabel("00:00:000", this);
    QFont timeLabelFont = m_timeLabel->font();
    timeLabelFont.setPointSize(14);
    m_timeLabel->setFont(timeLabelFont);
    m_timeLabel->setMaximumWidth(73);
    bottomFrameLayout->addWidget(m_timeLabel, 0, 5);

    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setProperty("previewIconButton", true);
    m_playPauseButton->setIcon(QIcon(":/particleQT/icons/play.png"));
    bottomFrameLayout->addWidget(m_playPauseButton, 1, 3, Qt::AlignRight);

    m_stepButton = new QPushButton(this);
    m_stepButton->setProperty("previewIconButton", true);
    m_stepButton->setIcon(QIcon(":/particleQT/icons/stepforward.png"));
    bottomFrameLayout->addWidget(m_stepButton, 1, 4, 1, 2, Qt::AlignLeft);

    m_loopButton = new QPushButton(this);
    m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
    m_loopButton->setProperty("previewIconButton", true);
    bottomFrameLayout->addWidget(m_loopButton, 1, 5, Qt::AlignRight);

    m_timeSlider = new QSlider(Qt::Horizontal, this);
    m_timeSlider->setSingleStep(1);
    bottomFrameLayout->addWidget(m_timeSlider, 0, 3, 1, 2);

    connect(m_playPauseButton, SIGNAL(clicked()), this, SLOT(OnPlayPauseClicked()));
    connect(m_stepButton, SIGNAL(clicked()), this, SLOT(OnStepClicked()));
    connect(m_loopButton, SIGNAL(clicked()), this, SLOT(OnLoopClicked()));
    connect(m_resetButton, SIGNAL(clicked()), this, SLOT(OnResetClicked()));
    connect(m_wireframeButton, SIGNAL(clicked()), this, SLOT(OnWireframeClicked()));
    connect(m_timeOfDayButton, SIGNAL(clicked()), this, SLOT(OnTimeOfDayClicked()));
    connect(m_focusButton, &QPushButton::clicked, this, &CPreviewWindowView::FocusCameraOnEmitter);

    connect(cameraDropdownMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnCameraMenuActionClicked(QAction*)));
    PostBuildUI();
}

void CPreviewWindowView::OnTimeOfDayClicked()
{
    m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::ENABLE_TIME_OF_DAY);
    RefreshUIStates();
}

void CPreviewWindowView::OnWireframeClicked()
{
    HandleMenuAction(MenuActions::PRESET_WIREFRAME_VIEW);
}

void CPreviewWindowView::OnResetClicked()
{
    HandleMenuAction(MenuActions::PRESET_RESET);
}

void CPreviewWindowView::OnPlayPauseClicked()
{
    HandleMenuAction(MenuActions::PRESET_TOGGLE_PLAYPAUSE);
}

void CPreviewWindowView::OnStepClicked()
{
    HandleMenuAction(MenuActions::PRESET_STEP_FORWARD);
}

void CPreviewWindowView::OnLoopClicked()
{
    HandleMenuAction(MenuActions::PRESET_LOOP);
}

void CPreviewWindowView::SpawnAllEffect(IParticleEffect* effect, CParticleItem* pParticles)
{
    if (effect->GetParent() != nullptr)
    {
        SpawnAllEffect(effect->GetParent(), pParticles);
    }
    else
    {
        m_previewModelView->LoadParticleEffect(effect, pParticles, m_activeLod);
    }
}

void CPreviewWindowView::SetActiveEffect(CParticleItem* pParticle, SLodInfo* lod)
{
    CRY_ASSERT(m_previewModelView);

    m_pActiveItem = pParticle;
    m_activeLod = lod;

    //if no particle is set the clear the emitter from the previewer
    if (pParticle == nullptr || (pParticle && !pParticle->IsParticleItem))
    {
        m_previewModelView->ClearEmitter();
        return;
    }
    if (ShowEmitterSettingState == ShowEmitterSetting::EmitterOnly)
    {
        m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_FIRST_CONTAINER);
        if (pParticle->GetEffect())
        {
            m_previewModelView->LoadParticleEffect(pParticle->GetEffect(), pParticle, m_activeLod);
        }
    }
    else if (ShowEmitterSettingState == ShowEmitterSetting::EmitterAndAllChildren)
    {
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_FIRST_CONTAINER);
        m_previewModelView->LoadParticleEffect(pParticle->GetEffect(), pParticle, m_activeLod);
    }
    else if (ShowEmitterSettingState == ShowEmitterSetting::EmitterAndAllChildrenAndParents)
    {
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_FIRST_CONTAINER);
        SpawnAllEffect(pParticle->GetEffect(), pParticle);
    }
    else
    {
        m_previewModelView->LoadParticleEffect(pParticle->GetEffect(), pParticle, m_activeLod);
    }
}

void CPreviewWindowView::UpdateParticleCount()
{
    m_numOfParticle->setText(QString::number(m_previewModelView->GetAliveParticleCount()) + "P");
}

void CPreviewWindowView::UpdatePlaybackUI()
{
    CRY_ASSERT(m_previewModelView);
    if (m_previewModelView->HasValidParticleEmitter())
    {
        if (m_previewModelView->GetPlayState() == CPreviewModelView::PlayState::PLAY && m_previewModelView->IsParticleEmitterAlive())
        {
            m_playPauseButton->setIcon(QIcon(":/particleQT/icons/pauseOrange.png"));
        }
        else
        {
            m_playPauseButton->setIcon(QIcon(":/particleQT/icons/play.png"));
        }
    }
}

void CPreviewWindowView::OnCameraMenuActionClicked(QAction* action)
{
    HandleEmitterViewChange(static_cast<ShowEmitterSetting>(action->data().toInt()));
}

bool CPreviewWindowView::eventFilter(QObject* obj, QEvent* event)
{
    QWidget* objWidget = static_cast<QWidget*>(obj);
    QRect widgetRect = QRect(QPoint(0, 0), QSize(0, 0));
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* e = static_cast<QHelpEvent*>(event);
        QPoint ep = e->globalPos();
        if (objWidget == m_playPauseButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Play/Pause", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_playPauseButton->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_playPauseButton->size());
        }
        else if (objWidget == m_loopButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Loop", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_loopButton->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_loopButton->size());
        }
        else if (objWidget == m_playSpeedButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Play Speed", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_playSpeedButton->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_playSpeedButton->size());
        }
        else if (objWidget == m_timeSlider)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Time", "Slider");
            widgetRect.setTopLeft(mapToGlobal(m_timeSlider->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_timeSlider->size());
        }
        else if (objWidget == m_numOfParticle)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Num of Particles", "");
            widgetRect.setTopLeft(mapToGlobal(m_numOfParticle->pos()));
            widgetRect.setSize(m_numOfParticle->size());
        }
        else if (objWidget == m_timeLabel)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Time", "");
            widgetRect.setTopLeft(mapToGlobal(m_timeLabel->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_timeLabel->size());
        }
        else if (objWidget == cameraDropdownButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Camera Dropdown", "Button");
            widgetRect.setTopLeft(mapToGlobal(cameraDropdownButton->pos()));
            widgetRect.setSize(cameraDropdownButton->size());
        }
        else if (objWidget == m_wireframeButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Display Wireframe", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_wireframeButton->pos()));
            widgetRect.setSize(m_wireframeButton->size());
        }
        else if (objWidget == m_timeOfDayButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Enable Time Of Day", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_timeOfDayButton->pos()));
            widgetRect.setSize(m_timeOfDayButton->size());
        }
        else if (objWidget == m_focusButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Focus", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_focusButton->pos()));
            widgetRect.setSize(m_focusButton->size());
        }
        else if (objWidget == m_resetButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Reset", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_resetButton->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_resetButton->size());
        }
        else if (objWidget == m_stepButton)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preview Control.Step Forward", "Button");
            widgetRect.setTopLeft(mapToGlobal(m_stepButton->pos()));
            widgetRect.setTop(widgetRect.top() + m_botFrame->pos().y());
            widgetRect.setSize(m_stepButton->size());
        }

        m_tooltip->TryDisplay(ep, widgetRect, QToolTipWidget::ArrowDirection::ARROW_LEFT);

        return true;
    }
    if (event->type() == QEvent::Leave)
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
    }

    //Hotkey handling for the previewModelView
    if (obj == m_previewModelView)
    {
        if (event->type() == QEvent::KeyRelease)
        {
            HandleHotkeys(static_cast<QKeyEvent*>(event));
        }
    }
    return QWidget::eventFilter(obj, event);
}

void CPreviewWindowView::PostBuildUI()
{
    m_playPauseButton->installEventFilter(this);
    m_loopButton->installEventFilter(this);
    m_playSpeedButton->installEventFilter(this);
    m_timeSlider->installEventFilter(this);
    m_numOfParticle->installEventFilter(this);
    m_timeLabel->installEventFilter(this);
    cameraDropdownButton->installEventFilter(this);
    m_wireframeButton->installEventFilter(this);
    m_resetButton->installEventFilter(this);
    m_stepButton->installEventFilter(this);
    m_previewModelView->installEventFilter(this);
    m_timeOfDayButton->installEventFilter(this);
    m_focusButton->installEventFilter(this);
}

bool CPreviewWindowView::event(QEvent* e)
{
    if (e->type() == QEvent::Shortcut)
    {
        if (HandleShortcuts((QShortcutEvent*)e))
        {
            return true;
        }
    }

    return QWidget::event(e);
}

void CPreviewWindowView::keyPressEvent(QKeyEvent* e)
{
    HandleHotkeys(e);
}


void CPreviewWindowView::UpdateTimeLabel()
{
    bool useDefault = true;
    if (m_previewModelView->HasValidParticleEmitter())
    {
        float maxLifeTime = m_previewModelView->GetParticleEffectMaxLifeTime();
        float currentAge = m_previewModelView->GetCurrentParticleEffectAge();

        //maxLifeTime of -1 means effect is continuous
        //currentAge of -1 means the effect is not active
        if (maxLifeTime != -1.0f && currentAge != -1.0f)
        {
            //This is here to catch the case where someone toggled continuous on an effect ... doing this will now cause a restart of the emitter.
            if (maxLifeTime < currentAge)
            {
                m_previewModelView->ForceParticleEmitterRestart();
            }
            else
            {
                int t_minute, t_second, t_milliseconds;
                if (currentAge >= 3600.0f)
                {
                    currentAge = currentAge - 3600.0f;
                }
                t_minute = static_cast<int>(currentAge) / 60;
                t_second = static_cast<int>(currentAge) % 60;
                t_milliseconds = static_cast<int>((currentAge * 1000.0f)) % 1000;

                QTime time(0, t_minute, t_second, t_milliseconds);
                QString sTime = time.toString("mm:ss:zzz");
                m_timeLabel->setText(sTime);
                m_timeSlider->setMaximum(maxLifeTime * 30);
                m_timeSlider->setValue(currentAge * 30);
                useDefault = false;
            }
        }
    }

    if (useDefault)
    {
        m_timeLabel->setText("00:00:000");
        m_timeSlider->setValue(0);
        m_timeSlider->setMaximum(1);
    }
    m_timeSlider->setMinimum(0);
}

void CPreviewWindowView::FocusCameraOnEmitter()
{
    HandleMenuAction(MenuActions::PRESET_RESET_CAMERA_POSITION);
}

void CPreviewWindowView::HandleHotkeys(QKeyEvent* e)
{
    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Focus"))
    {
        HandleMenuAction(MenuActions::PRESET_RESET_CAMERA_POSITION);
        e->setAccepted(true);
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Play/Pause Toggle"))
    {
        HandleMenuAction(MenuActions::PRESET_TOGGLE_PLAYPAUSE);
        e->setAccepted(true);
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Loop Toggle"))
    {
        HandleMenuAction(MenuActions::PRESET_LOOP);
        e->setAccepted(true);
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Step forward through time"))
    {
        HandleMenuAction(MenuActions::PRESET_STEP_FORWARD);
        e->setAccepted(true);
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Reset Playback"))
    {
        HandleMenuAction(MenuActions::PRESET_RESET);
        e->setAccepted(true);
    }
}

bool CPreviewWindowView::HandleShortcuts(QShortcutEvent* e)
{
    bool handled = false;

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Focus"))
    {
        HandleMenuAction(MenuActions::PRESET_RESET_CAMERA_POSITION);
        handled = true;
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Play/Pause Toggle"))
    {
        HandleMenuAction(MenuActions::PRESET_TOGGLE_PLAYPAUSE);
        handled = true;
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Loop Toggle"))
    {
        HandleMenuAction(MenuActions::PRESET_LOOP);
        handled = true;
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Step forward through time"))
    {
        HandleMenuAction(MenuActions::PRESET_STEP_FORWARD);
        handled = true;
    }

    if (GetIEditor()->GetParticleUtils()->HotKey_IsPressed(e, "Previewer.Reset Playback"))
    {
        HandleMenuAction(MenuActions::PRESET_RESET);
        handled = true;
    }

    return handled;
}

void CPreviewWindowView::OnPlaySpeedButtonValueChanged(double value)
{
    CRY_ASSERT(m_previewModelView);
    m_previewModelView->SetTimeScale(static_cast<float>(value));
}

void CPreviewWindowView::OnMenuActionClicked(QAction* action)
{
    HandleMenuAction(static_cast<MenuActions>(action->data().toInt()));
}

QMenu* CPreviewWindowView::ExecPreviewMenu()
{
    CRY_ASSERT(m_PreviewMenu);

    m_PreviewMenu->clear();
    m_PreviewMenu->setVisible(false);
    m_PreviewMenu->addAction(tr("Import Mesh"))->setData(static_cast<unsigned int>(MenuActions::PRESET_IMPORT_MESH));
    m_PreviewMenu->addSeparator();

    QAction* play = m_PreviewMenu->addAction(tr("Play"));
    play->setData(static_cast<unsigned int>(MenuActions::PRESET_PLAY));
    play->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Play/Pause Toggle"));
    QAction* pause = m_PreviewMenu->addAction(tr("Pause"));
    pause->setData(static_cast<unsigned int>(MenuActions::PRESET_PAUSE));
    pause->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Play/Pause Toggle"));
    QAction* stepForward = m_PreviewMenu->addAction(tr("Step forward"));
    stepForward->setData(static_cast<unsigned int>(MenuActions::PRESET_STEP_FORWARD));
    stepForward->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Step forward through time"));
    QAction* playLoop = m_PreviewMenu->addAction(tr("Play loop"));
    playLoop->setData(static_cast<unsigned int>(MenuActions::PRESET_LOOP));
    playLoop->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Loop Toggle"));
    QAction* reset = m_PreviewMenu->addAction(tr("Reset"));
    reset->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET));
    reset->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Reset Playback"));
    m_PreviewMenu->addSeparator();
    QAction* resetCam = m_PreviewMenu->addAction(tr("Reset camera position"));
    resetCam->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET_CAMERA_POSITION));
    resetCam->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Focus"));
    m_PreviewMenu->addSeparator();
    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW))
    {
        m_PreviewMenu->addAction(tr("Hide overdraw"))->setData(static_cast<unsigned int>(MenuActions::PRESET_SHOW_OVERDRAW));
    }
    else
    {
        m_PreviewMenu->addAction(tr("Show overdraw"))->setData(static_cast<unsigned int>(MenuActions::PRESET_SHOW_OVERDRAW));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO))
    {
        m_PreviewMenu->addAction(tr("Hide gizmo"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GIZMO));
    }
    else
    {
        m_PreviewMenu->addAction(tr("Show gizmo"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GIZMO));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID))
    {
        m_PreviewMenu->addAction(tr("Hide grid"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GRID));
    }
    else
    {
        m_PreviewMenu->addAction(tr("Show grid"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_GRID));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX))
    {
        m_PreviewMenu->addAction(tr("Hide bounding box"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_BOUNDING_BOX));
    }
    else
    {
        m_PreviewMenu->addAction(tr("Show bounding box"))->setData(static_cast<unsigned int>(MenuActions::PRESET_HIDE_BOUNDING_BOX));
    }

    m_PreviewMenu->addSeparator();
    m_PreviewMenu->addAction(tr("Background color..."))->setData(static_cast<unsigned int>(MenuActions::PRESET_BACKGROUND_COLOR));
    m_PreviewMenu->addAction(tr("Grid color..."))->setData(static_cast<unsigned int>(MenuActions::PRESET_GRID_COLOR));
    m_PreviewMenu->addAction(tr("Reset colors"))->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET_COLOR));
    m_PreviewMenu->addAction(tr("Reset to default"))->setData(static_cast<unsigned int>(MenuActions::PRESET_RESET_DEFAULT));

    return m_PreviewMenu;
}

void CPreviewWindowView::HandleEmitterViewChange(ShowEmitterSetting showState)
{
    ShowEmitterSettingState = showState;
    if (m_pActiveItem)
    {
        SetActiveEffect(m_pActiveItem, m_activeLod); //should be after ShowEmitterSettingState assignment so the value is correct ... this refreshes the current effect that is drawing.
    }

    //Update the UI state
    switch (ShowEmitterSettingState)
    {
    case ShowEmitterSetting::EmitterOnly:
    {
        m_showEmitterOnly->setCheckable(true);
        m_showEmitterOnly->setChecked(true);
        m_showEmitterChildren->setCheckable(false);
        m_showEmitterChildrenParents->setCheckable(false);
        cameraDropdownButton->setIcon(QIcon(":/particleQT/icons/emitteronly.png"));
        break;
    }
    case ShowEmitterSetting::EmitterAndAllChildren:
    {
        m_showEmitterOnly->setCheckable(false);
        m_showEmitterChildren->setCheckable(true);
        m_showEmitterChildren->setChecked(true);
        m_showEmitterChildrenParents->setCheckable(false);
        cameraDropdownButton->setIcon(QIcon(":/particleQT/icons/emitterchildren.png"));
        break;
    }
    case ShowEmitterSetting::EmitterAndAllChildrenAndParents:
    {
        m_showEmitterOnly->setCheckable(false);
        m_showEmitterChildren->setCheckable(false);
        m_showEmitterChildrenParents->setCheckable(true);
        m_showEmitterChildrenParents->setChecked(true);
        cameraDropdownButton->setIcon(QIcon(":/particleQT/icons/allrelated.png"));
        break;
    }
    default:
        break;
    }
}

void CPreviewWindowView::HandleMenuAction(MenuActions action)
{
    switch (action)
    {
    case MenuActions::PRESET_IMPORT_MESH:
    {
        m_previewModelView->ImportModel();
    }
    break;
    case MenuActions::PRESET_PLAY:
        if (m_previewModelView->HasValidParticleEmitter())
        {
            if (m_previewModelView->GetPlayState() == CPreviewModelView::PlayState::PLAY)
            {
                if (!m_previewModelView->IsParticleEmitterAlive())
                {
                    m_previewModelView->ForceParticleEmitterRestart();
                }
            }
            else
            {
                m_previewModelView->SetPlayState(CPreviewModelView::PlayState::PLAY);
                m_playPauseButton->setIcon(QIcon(":/particleQT/icons/pauseOrange.png"));
            }
        }
        break;
    case MenuActions::PRESET_PAUSE:
        if (m_previewModelView->HasValidParticleEmitter())
        {
            m_previewModelView->SetPlayState(CPreviewModelView::PlayState::PAUSE);
        }
        break;
    case MenuActions::PRESET_STEP_FORWARD:
        if (m_previewModelView->HasValidParticleEmitter())
        {
            m_previewModelView->SetPlayState(CPreviewModelView::PlayState::STEP);
        }
        break;
    case MenuActions::PRESET_RESET:
        if (m_previewModelView->HasValidParticleEmitter())
        {
            m_previewModelView->SetPlayState(CPreviewModelView::PlayState::RESET);
        }
        break;
    case MenuActions::PRESET_LOOP:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY);
        RefreshUIStates();
    }
    break;
    case MenuActions::PRESET_WIREFRAME_VIEW:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME);
        RefreshUIStates();
    }
    break;
    case MenuActions::PRESET_RESET_CAMERA_POSITION:
    {
        m_previewModelView->ResetCamera();
    }
    break;
    case MenuActions::PRESET_SHOW_OVERDRAW:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW);
    }
    break;
    case MenuActions::PRESET_HIDE_GIZMO:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO);
    }
    break;
    case MenuActions::PRESET_HIDE_GRID:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID);
    }
    break;
    case MenuActions::PRESET_HIDE_BOUNDING_BOX:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX);
    }
    break;
    case MenuActions::PRESET_BACKGROUND_COLOR:
    {
        //If the grid color change
        int nGridAlpha = 40;
        ColorF color = m_previewModelView->GetGridColor();
        QColor temp(color.r * 255, color.g * 255, color.b * 255);
        QCustomColorDialog* dlg = UIFactory::GetColorPicker(temp);
        if (dlg->exec() == QDialog::Accepted)
        {
            m_previewModelView->SetBackgroundColor(ColorF(dlg->GetColor().redF(), dlg->GetColor().greenF(), dlg->GetColor().blueF(), nGridAlpha));
        }
    }
    break;
    case MenuActions::PRESET_GRID_COLOR:
    {
        //If the grid color change
        int nGridAlpha = 40;
        ColorF color = m_previewModelView->GetGridColor();
        QColor temp(color.r * 255, color.g * 255, color.b * 255);
        QCustomColorDialog* dlg = UIFactory::GetColorPicker(temp);
        if (dlg->exec() == QDialog::Accepted)
        {
            m_previewModelView->SetGridColor(ColorF(dlg->GetColor().redF(), dlg->GetColor().greenF(), dlg->GetColor().blueF(), nGridAlpha));
        }
    }
    break;
    case MenuActions::PRESET_RESET_COLOR:
    {
        m_previewModelView->ResetBackgroundColor();
        m_previewModelView->ResetGridColor();
    }
    break;
    case MenuActions::PRESET_RESET_DEFAULT:
    {
        ResetToDefaultLayout();
    }
    break;
    case MenuActions::PRESET_RESET_PLAYBACK_CONTROL:
    {
        m_previewModelView->ResetPlaybackControls();
        m_botFrame->show();
        m_playSpeedButton->setValue(m_previewModelView->GetTimeScale());
        m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
    }
    break;
    case MenuActions::PRESET_TOGGLE_PLAYPAUSE:
    {
        if (m_previewModelView->HasValidParticleEmitter())
        {
            if (m_previewModelView->GetPlayState() == CPreviewModelView::PlayState::PLAY)
            {
                if (m_previewModelView->IsParticleEmitterAlive())
                {
                    m_previewModelView->SetPlayState(CPreviewModelView::PlayState::PAUSE);
                    m_playPauseButton->setIcon(QIcon(":/particleQT/icons/play.png"));
                }
                else
                {
                    m_previewModelView->ForceParticleEmitterRestart();
                }
            }
            else
            {
                m_previewModelView->SetPlayState(CPreviewModelView::PlayState::PLAY);
                m_playPauseButton->setIcon(QIcon(":/particleQT/icons/pauseOrange.png"));
            }
        }
    }
    break;
    case MenuActions::PRESET_SPLINE_SPEED_X1:
    {
        m_previewModelView->SetSplineSpeedMultiplier(1);
    }
    break;
    case MenuActions::PRESET_SPLINE_SPEED_X2:
    {
        m_previewModelView->SetSplineSpeedMultiplier(2);
    }
    break;
    case MenuActions::PRESET_SPLINE_SPEED_X3:
    {
        m_previewModelView->SetSplineSpeedMultiplier(3);
    }
    break;
    case MenuActions::PRESET_SPLINE_SPEED_X5:
    {
        m_previewModelView->SetSplineSpeedMultiplier(5);
    }
    break;
    case MenuActions::PRESET_SINGLEPATH_EMITTER:
    {
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
        m_previewModelView->RestartSplineMovement();
    }
    break;
    case MenuActions::PRESET_LOOP_EMITTER:
    {
        m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
        m_previewModelView->RestartSplineMovement();
    }
    break;
    case MenuActions::PRESET_PINGPONG_EMITTER:
    {
        m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
        m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
        m_previewModelView->RestartSplineMovement();
    }
    break;
    case MenuActions::PRESET_SPLINE_LINE:
    {
        m_previewModelView->SetSplineMode(CPreviewModelView::SplineMode::LINE);
    }
    break;
    case MenuActions::PRESET_SPLINE_SINE:
    {
        m_previewModelView->SetSplineMode(CPreviewModelView::SplineMode::SINEWAVE);
    }
    break;
    case MenuActions::PRESET_SPLINE_COIL:
    {
        m_previewModelView->SetSplineMode(CPreviewModelView::SplineMode::COIL);
    }
    break;
    case MenuActions::PRESET_TOGGLE_EMITTERSHAPE:
    {
        m_previewModelView->ToggleFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE);
    }
    break;
    case MenuActions::PRESET_HIDE_PARTICLE_COUNT:
    {
        if (m_numOfParticle->isHidden())
        {
            m_numOfParticle->show();
        }
        else
        {
            m_numOfParticle->hide();
        }
    }
    break;
    case MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS:
    {
        if (m_botFrame->isHidden())
        {
            m_botFrame->show();
        }
        else
        {
            m_botFrame->hide();
        }
    }
    break;
    case MenuActions::PRESET_SPLINE_RESET_DEFAULT:
    {
        m_previewModelView->ResetSplineSetting();
    }
    break;
    default:
    {
        CRY_ASSERT(0 && "Invalid CPreviewWindowView::MenuActions action being used");
    }
    break;
    }
}

void CPreviewWindowView::CorrectLayout(QByteArray& data)
{
    QMessageBox::warning(this, tr("Warning"), tr("The previewer layout format is out of date. Please export a new one.\n"),QMessageBox::Button::Close, QMessageBox::Button::Cancel);
}

void CPreviewWindowView::RefreshUIStates()
{
    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME))
    {
        m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframeOrange.png"));
    }
    else
    {
        m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframe.png"));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY))
    {
        m_loopButton->setIcon(QIcon(":/particleQT/icons/loopOrange.png"));
    }
    else
    {
        m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
    }

    if (m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::ENABLE_TIME_OF_DAY))
    {
        m_timeOfDayButton->setIcon(QIcon(":/particleQT/icons/TimeOfDayOrange.png"));
    }
    else
    {
        m_timeOfDayButton->setIcon(QIcon(":/particleQT/icons/TimeOfDay.png"));
    }
}

void CPreviewWindowView::SaveSessionState(QByteArray& out)
{
    QDataStream stream(&out, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 fileVersion = PARTICLE_EDITOR_LAYOUT_VERSION;
    stream << fileVersion;
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
    stream << m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
    stream << static_cast<quint64>(m_previewModelView->GetSplineMode());
    stream << m_previewModelView->GetTimeScale();
    stream << static_cast<quint64>(m_previewModelView->GetSplineSpeedMultiplier());

    ColorF grid = m_previewModelView->GetGridColor();
    stream << grid.a;
    stream << grid.r;
    stream << grid.g;
    stream << grid.b;

    ColorF back = m_previewModelView->GetBackgroundColor();
    stream << back.a;
    stream << back.r;
    stream << back.g;
    stream << back.b;

    stream << static_cast<quint64>(ShowEmitterSettingState);
    stream << m_numOfParticle->isHidden();
    stream << m_botFrame->isHidden();
}

void CPreviewWindowView::SaveSessionState(QString path)
{
    if (!path.isEmpty())
    {
        QFile file(path);
        QByteArray output;
        file.open(QIODevice::WriteOnly);
        SaveSessionState(output);
        file.write(output);
        file.close();
    }
    else
    {
        //Save to settings so previewer recalls previous mode
        CRY_ASSERT(m_previewModelView);
        QSettings settings("Amazon", "Lumberyard");
        settings.beginGroup(PARTICLE_EDITOR_PREVIEWER_SETTINGS);
        {
            //REMOVE OLD SETTINGS
            settings.remove("");
            settings.sync();

            settings.beginGroup("ModelView");
            {
                settings.setValue("DRAW_WIREFRAME", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME));
                settings.setValue("SHOW_BOUNDINGBOX", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX));
                settings.setValue("SHOW_GIZMO", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO));
                settings.setValue("SHOW_GRID", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID));
                settings.setValue("SHOW_EMITTER_SHAPE", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE));
                settings.setValue("SHOW_OVERDRAW", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW));
                settings.setValue("LOOPING_PLAY", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY));
                settings.setValue("SPLINE_LOOPING", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING));
                settings.setValue("SPLINE_PINGPONG", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG));

                settings.setValue("SplineMode", static_cast<int>(m_previewModelView->GetSplineMode()));
                settings.setValue("PlaySpeed", m_previewModelView->GetTimeScale());
                settings.setValue("SplineSpeedMultiplier", m_previewModelView->GetSplineSpeedMultiplier());

                ColorF grid = m_previewModelView->GetGridColor();
                settings.setValue("GridColorAlpha", grid.a);
                settings.setValue("GridColorRed", grid.r);
                settings.setValue("GridColorGreen", grid.g);
                settings.setValue("GridColorBlue", grid.b);

                ColorF back = m_previewModelView->GetBackgroundColor();
                settings.setValue("BackgroundColorAlpha", back.a);
                settings.setValue("BackgroundColorRed", back.r);
                settings.setValue("BackgroundColorGreen", back.g);
                settings.setValue("BackgroundColorBlue", back.b);
            }
            settings.endGroup();

            //WindowView
            settings.beginGroup("WindowView");
            {
                settings.setValue("ShowEmitterSettingState", static_cast<int>(ShowEmitterSettingState));
                settings.setValue("HideParticleCount", m_numOfParticle->isHidden());
                settings.setValue("HidePlaybackControls", m_botFrame->isHidden());
            }
            settings.endGroup();
        }
        settings.endGroup();
        settings.sync();
    }
}


void CPreviewWindowView::LoadSessionState(QByteArray data)
{
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    bool flagIsTrue = false;
    quint64 enumValue = 0;
    float floatingValue = 0.0f;
    quint32 fileVersion;
    stream >> fileVersion;

#define SET_FLAG(flag, value) value ? m_previewModelView->SetFlag(flag) : m_previewModelView->UnSetFlag(flag);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING, flagIsTrue);
    stream >> flagIsTrue;
    SET_FLAG(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG, flagIsTrue);
#undef SET_FLAG


    switch (fileVersion)
    {
    case 0x1:
    case 0x2:
    case 0x3:
    {
        if (fileVersion != PARTICLE_EDITOR_LAYOUT_VERSION)
        {
            GetIEditor()->GetLogFile()->Warning("The attribute layout format is out of date. Please export a new one.\n");
        }

        stream >> enumValue;
        m_previewModelView->SetSplineMode(static_cast<CPreviewModelView::SplineMode>(enumValue));
        stream >> floatingValue;
        m_previewModelView->SetTimeScale(floatingValue);
        stream >> enumValue;
        m_previewModelView->SetSplineSpeedMultiplier(enumValue);
        float a, r, g, b;
        ColorF colorValue;
        stream >> a;
        stream >> r;
        stream >> g;
        stream >> b;
        colorValue.Set(r, g, b, a);
        m_previewModelView->SetGridColor(colorValue);

        stream >> a;
        stream >> r;
        stream >> g;
        stream >> b;
        colorValue.Set(r, g, b, a);
        m_previewModelView->SetBackgroundColor(colorValue);

        stream >> enumValue;
        HandleEmitterViewChange(static_cast<ShowEmitterSetting>(enumValue));

        stream >> flagIsTrue;
        m_numOfParticle->setHidden(flagIsTrue);

        stream >> flagIsTrue;
        m_botFrame->setHidden(flagIsTrue);

        RefreshUIStates();
        break;
    }
    default:
        CorrectLayout(data);
        break;
    }
}

void CPreviewWindowView::LoadSessionState(QString path)
{
    if (!path.isEmpty())
    {
        QFile file(path);
        QByteArray data;
        file.open(QIODevice::ReadOnly);
        data = file.readAll();
        file.close();
        LoadSessionState(data);
    }
    else
    {
        //Load from settings
        QSettings settings("Amazon", "Lumberyard");

        settings.beginGroup(PARTICLE_EDITOR_PREVIEWER_SETTINGS);
        {
            settings.beginGroup("ModelView");
            {
                if (settings.value("DRAW_WIREFRAME", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::DRAW_WIREFRAME);
                }

                if (settings.value("SHOW_BOUNDINGBOX", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_BOUNDINGBOX);
                }

                if (settings.value("SHOW_GIZMO", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GIZMO);
                }

                if (settings.value("SHOW_GRID", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID);
                }

                if (settings.value("SHOW_EMITTER_SHAPE", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_EMITTER_SHAPE);
                }

                if (settings.value("SHOW_OVERDRAW", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_OVERDRAW);
                }

                if (settings.value("LOOPING_PLAY", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::LOOPING_PLAY);
                }

                if (settings.value("SPLINE_LOOPING", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_LOOPING);
                }

                if (settings.value("SPLINE_PINGPONG", m_previewModelView->IsFlagSet(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG)).toBool())
                {
                    m_previewModelView->SetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
                }
                else
                {
                    m_previewModelView->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SPLINE_PINGPONG);
                }

                m_previewModelView->SetSplineMode(static_cast<CPreviewModelView::SplineMode>(settings.value("SplineMode", static_cast<int>(m_previewModelView->GetSplineMode())).toInt()));
                m_previewModelView->SetTimeScale(settings.value("PlaySpeed", m_previewModelView->GetTimeScale()).toFloat());
                m_previewModelView->SetSplineSpeedMultiplier(settings.value("SplineSpeedMultiplier", m_previewModelView->GetSplineSpeedMultiplier()).toFloat());

                float a, r, g, b;

                ColorF grid = m_previewModelView->GetGridColor();
                a = settings.value("GridColorAlpha", grid.a).toFloat();
                r = settings.value("GridColorRed", grid.r).toFloat();
                g = settings.value("GridColorGreen", grid.g).toFloat();
                b = settings.value("GridColorBlue", grid.b).toFloat();
                grid.Set(r, g, b, a);
                m_previewModelView->SetGridColor(grid);

                ColorF back = m_previewModelView->GetBackgroundColor();
                a = settings.value("BackgroundColorAlpha", back.a).toFloat();
                r = settings.value("BackgroundColorRed", back.r).toFloat();
                g = settings.value("BackgroundColorGreen", back.g).toFloat();
                b = settings.value("BackgroundColorBlue", back.b).toFloat();
                back.Set(r, g, b, a);
                m_previewModelView->SetBackgroundColor(back);
            }
            settings.endGroup();

            //WindowView
            settings.beginGroup("WindowView");
            {
                HandleEmitterViewChange(static_cast<ShowEmitterSetting>(settings.value("ShowEmitterSettingState", static_cast<int>(ShowEmitterSettingState)).toInt()));
                if (settings.value("HideParticleCount", m_numOfParticle->isHidden()).toBool())
                {
                    m_numOfParticle->hide();
                }
                else
                {
                    m_numOfParticle->show();
                }
                if (settings.value("HidePlaybackControls", m_botFrame->isHidden()).toBool())
                {
                    m_botFrame->hide();
                }
                else
                {
                    m_botFrame->show();
                }
            }
            settings.endGroup();
        }
        settings.endGroup();
    }
    RefreshUIStates();
}


void CPreviewWindowView::ForceParticleEmitterRestart()
{
    CRY_ASSERT(m_previewModelView);
    m_previewModelView->ForceParticleEmitterRestart();
}

void CPreviewWindowView::ResetToDefaultLayout()
{
    HandleEmitterViewChange(ShowEmitterSetting::EmitterOnly);
    m_playSpeedButton->setValue(m_previewModelView->GetTimeScale());
    m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
    m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframe.png"));
    m_botFrame->show();
    m_numOfParticle->show();
    m_previewModelView->ResetAll();
}

#include <PreviewWindowView.moc>

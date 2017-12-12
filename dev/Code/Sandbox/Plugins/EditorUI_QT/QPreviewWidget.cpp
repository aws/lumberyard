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
#include "QPreviewWidget.h"

#include "QWidgetAction"
#include "QCustomSpinbox.h"
#include "ContextMenu.h"
#include "PreviewWindowView.h"
#include "PreviewModelView.h"
QPreviewWidget::QPreviewWidget(CPreviewModelView* ModelViewPtr, CPreviewWindowView* WindowViewPtr)
    : QWidget((QWidget*)WindowViewPtr)
{
    customEmitterMenu = new ContextMenu(this);
    loadEmitterMenu = new ContextMenu(this);
    m_PreviewMenu = new ContextMenu(this);
    connect(m_PreviewMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnMenuActionClicked(QAction*)));

    previewWindowPtr = WindowViewPtr;
    previewModelPtr = ModelViewPtr;
}

QPreviewWidget::~QPreviewWidget()
{
}

void QPreviewWidget::buildPreviewMenu(QMenu* previewMenu)
{
    previewMenu->clear();
    previewMenu->setVisible(false);
    previewMenu->addAction("Import Mesh")->setData(MenuActions::PRESET_IMPORT_MESH);
    previewMenu->addSeparator();

    QAction* play = previewMenu->addAction("Play");
    play->setData(MenuActions::PRESET_PLAY);
    play->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Play/Pause Toggle"));
    QAction* pause = previewMenu->addAction("Pause");
    pause->setData(MenuActions::PRESET_PAUSE);
    pause->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Play/Pause Toggle"));
    QAction* stepForward = previewMenu->addAction("Step forward");
    stepForward->setData(MenuActions::PRESET_STEP_FORWARD);
    stepForward->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Step forward through time"));
    QAction* playLoop = previewMenu->addAction("Play loop");
    playLoop->setData(MenuActions::PRESET_LOOP);
    playLoop->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Loop Toggle"));
    QAction* reset = previewMenu->addAction("Reset");
    reset->setData(MenuActions::PRESET_RESET);
    reset->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Reset Playback"));
    previewMenu->addSeparator();
    QAction* resetCam = previewMenu->addAction("Reset camera position");
    resetCam->setData(MenuActions::PRESET_RESET_CAMERA_POSITION);
    resetCam->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Focus"));
    previewMenu->addSeparator();
    if (previewModelPtr->m_bOverdraw)
    {
        previewMenu->addAction("Hide overdraw")->setData(MenuActions::PRESET_SHOW_OVERDRAW);
    }
    else
    {
        previewMenu->addAction("Show overdraw")->setData(MenuActions::PRESET_SHOW_OVERDRAW);
    }
    if (previewModelPtr->m_bshowGizmo)
    {
        previewMenu->addAction("Hide gizmo")->setData(MenuActions::PRESET_HIDE_GIZMO);
    }
    else
    {
        previewMenu->addAction("Show gizmo")->setData(MenuActions::PRESET_HIDE_GIZMO);
    }
    if (previewModelPtr->m_bshowGrid)
    {
        previewMenu->addAction("Hide grid")->setData(MenuActions::PRESET_HIDE_GRID);
    }
    else
    {
        previewMenu->addAction("Show grid")->setData(MenuActions::PRESET_HIDE_GRID);
    }
    if (previewModelPtr->m_bshowBoundingBox)
    {
        previewMenu->addAction("Hide bounding box")->setData(MenuActions::PRESET_HIDE_BOUDING_BOX);
    }
    else
    {
        previewMenu->addAction("Show bounding box")->setData(MenuActions::PRESET_HIDE_BOUDING_BOX);
    }
    previewMenu->addSeparator();
    previewMenu->addAction("Background color...")->setData(MenuActions::PRESET_BACKGROUND_COLOR);
    previewMenu->addAction("Grid color...")->setData(MenuActions::PRESET_GRID_COLOR);
    previewMenu->addAction("Reset colors")->setData(MenuActions::PRESET_RESET_COLOR);
    previewMenu->addAction("Reset to default")->setData(MenuActions::PRESET_RESET_DEFAULT);
}

void QPreviewWidget::buildGizmoMenu(QMenu* gizmoMenu)
{
    gizmoMenu->clear();
    gizmoMenu->setVisible(false);
    QMenu* splineMovementMenu = new ContextMenu(this);
    splineMovementMenu->setTitle("Move on spline");

    QMenu* splineModeMenu = new ContextMenu("Spline mode", this);
    splineMovementMenu->addMenu(splineModeMenu);
    splineModeMenu->addAction("Single path")->setData(MenuActions::PRESET_SINGLEPATH_EMITTER);
    splineModeMenu->addAction("Loop")->setData(MenuActions::PRESET_LOOP_EMITTER);
    splineModeMenu->addAction("Ping-pong")->setData(MenuActions::PRESET_PINGPONG_EMITTER);

    QMenu* splineShapeMenu = new ContextMenu("Spline shape", this);
    splineMovementMenu->addMenu(splineShapeMenu);
    splineShapeMenu->addAction("Line")->setData(MenuActions::PRESET_SPLINE_LINE);
    splineShapeMenu->addAction("Sine Wave")->setData(MenuActions::PRESET_SPLINE_SINE);
    splineShapeMenu->addAction("Coil")->setData(MenuActions::PRESET_SPLINE_COIL);

    gizmoMenu->addMenu(splineMovementMenu);
    gizmoMenu->addSeparator();
    gizmoMenu->addAction("Reset default")->setData(MenuActions::PRESET_GIZMO_RESET_DEFAULT);

    connect(gizmoMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnGizmoMenuActionClicked(QAction*)));
    connect(splineModeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnSplineModeMenuActionClicked(QAction*)));
    connect(splineShapeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnSplineShapeMenuActionClicked(QAction*)));
}

void QPreviewWidget::buildContextMenu(QMenu* contextMenu)
{
    contextMenu->clear();
    contextMenu->setVisible(false);
    contextMenu->addAction("Wireframe view")->setData(MenuActions::PRESET_WIREFRAME_VIEW);
    if (previewModelPtr->m_bDisplayParticleCount)
    {
        contextMenu->addAction("Hide particle count")->setData(MenuActions::PRESET_HIDE_PARTICLE_COUNT);
    }
    else
    {
        contextMenu->addAction("Show particle count")->setData(MenuActions::PRESET_HIDE_PARTICLE_COUNT);
    }
    if (previewModelPtr->m_bshowBoundingBox)
    {
        contextMenu->addAction("Hide bounding box")->setData(MenuActions::PRESET_HIDE_BOUDING_BOX);
    }
    else
    {
        contextMenu->addAction("Show bounding box")->setData(MenuActions::PRESET_HIDE_BOUDING_BOX);
    }
    if (previewModelPtr->m_bshowGizmo)
    {
        contextMenu->addAction("Hide gizmo")->setData(MenuActions::PRESET_HIDE_GIZMO);
    }
    else
    {
        contextMenu->addAction("Show gizmo")->setData(MenuActions::PRESET_HIDE_GIZMO);
    }
    if (previewModelPtr->m_bShowPlaybackControl)
    {
        contextMenu->addAction("Hide playback controls")->setData(MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS);
    }
    else
    {
        contextMenu->addAction("Show playback controls")->setData(MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS);
    }
    if (previewModelPtr->m_bshowGrid)
    {
        contextMenu->addAction("Hide grid")->setData(MenuActions::PRESET_HIDE_GRID);
    }
    else
    {
        contextMenu->addAction("Show grid")->setData(MenuActions::PRESET_HIDE_GRID);
    }
    if (previewModelPtr->m_bshowEmitterShape)
    {
        contextMenu->addAction("Hide Emitter Shape")->setData(MenuActions::PRESET_TOGGLE_EMITTERSHAPE);
    }
    else
    {
        contextMenu->addAction("Show Emitter Shape")->setData(MenuActions::PRESET_TOGGLE_EMITTERSHAPE);
    }

    contextMenu->addSeparator();
    contextMenu->addAction("Background color...")->setData(MenuActions::PRESET_BACKGROUND_COLOR);
    contextMenu->addAction("Grid color...")->setData(MenuActions::PRESET_GRID_COLOR);
    contextMenu->addAction("Reset colors")->setData(MenuActions::PRESET_RESET_COLOR);

    /************************Spline Sub-menu********************************************************/
    contextMenu->addSeparator();
    QMenu* splineMovementMenu = new ContextMenu(this);
    splineMovementMenu->setTitle("Move on spline");

    QMenu* splineModeMenu = new ContextMenu("Spline mode", this);
    splineMovementMenu->addMenu(splineModeMenu);
    splineModeMenu->addAction("Single path")->setData(MenuActions::PRESET_SINGLEPATH_EMITTER);
    splineModeMenu->addAction("Loop")->setData(MenuActions::PRESET_LOOP_EMITTER);
    splineModeMenu->addAction("Ping-pong")->setData(MenuActions::PRESET_PINGPONG_EMITTER);

    QMenu* splineShapeMenu = new ContextMenu("Spline shape", this);
    splineMovementMenu->addMenu(splineShapeMenu);
    splineShapeMenu->addAction("Line")->setData(MenuActions::PRESET_SPLINE_LINE);
    splineShapeMenu->addAction("Sine Wave")->setData(MenuActions::PRESET_SPLINE_SINE);
    splineShapeMenu->addAction("Coil")->setData(MenuActions::PRESET_SPLINE_COIL);

    QMenu* splideSpeedMenu = new ContextMenu("spline speed", this);
    splineMovementMenu->addMenu(splideSpeedMenu);
    splideSpeedMenu->addAction("Speed x1")->setData(MenuActions::PRESET_SPLINE_SPEED_X1);
    splideSpeedMenu->addAction("Speed x2")->setData(MenuActions::PRESET_SPLINE_SPEED_X2);
    splideSpeedMenu->addAction("Speed x3")->setData(MenuActions::PRESET_SPLINE_SPEED_X3);
    splideSpeedMenu->addAction("Speed x5")->setData(MenuActions::PRESET_SPLINE_SPEED_X5);

    contextMenu->addMenu(splineMovementMenu);
    contextMenu->addAction("Reset spline settings")->setData(MenuActions::PRESET_GIZMO_RESET_DEFAULT);

    //reset everything to default
    contextMenu->addSeparator();
    contextMenu->addAction("Reset to default")->setData(MenuActions::PRESET_RESET_DEFAULT);

    //connect all actions
    connect(contextMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnContextMenuActionClicked(QAction*)));
    connect(splineModeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnSplineModeMenuActionClicked(QAction*)));
    connect(splineShapeMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnSplineShapeMenuActionClicked(QAction*)));
    connect(splideSpeedMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnSplineSpeedMenuActionClicked(QAction*)));
}

QMenu* QPreviewWidget::execPreviewMenu()
{
    CRY_ASSERT(m_PreviewMenu);
    buildPreviewMenu(m_PreviewMenu);
    return m_PreviewMenu;
}

void QPreviewWidget::execContextMenu(QPoint position)
{
    ContextMenu menu(this);
    buildContextMenu(&menu);
    menu.exec(position);
}

void QPreviewWidget::execGizmoMenu(QPoint position)
{
    ContextMenu menu(this);
    buildGizmoMenu(&menu);
    menu.exec(position);
}

void QPreviewWidget::OnContextMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_WIREFRAME_VIEW:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_bDrawWireFrame = !previewModelPtr->m_bDrawWireFrame;
            if (previewModelPtr->m_bDrawWireFrame)
            {
                previewWindowPtr->m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframeOrange.png"));
            }
            else
            {
                previewWindowPtr->m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframe.png"));
            }
        }
        break;
    case MenuActions::PRESET_HIDE_PARTICLE_COUNT:
        previewWindowPtr->toggleDisplayParticleCount();
        break;
    case MenuActions::PRESET_HIDE_BOUDING_BOX:
        previewModelPtr->m_bshowBoundingBox = !previewModelPtr->m_bshowBoundingBox;
        break;
    case MenuActions::PRESET_SHOW_GIZMO:
        previewModelPtr->m_bshowGizmo = !previewModelPtr->m_bshowGizmo;
        break;
    case MenuActions::PRESET_HIDE_PLAYBACK_CONTROLS:
        previewWindowPtr->togglePlaybackControl();
        break;
    case MenuActions::PRESET_BACKGROUND_COLOR:
        previewModelPtr->setBackgroundColor();
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_GRID_COLOR:
        previewModelPtr->setGridColor();
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_HIDE_GRID:
        //turn on/off grid
        previewModelPtr->m_bshowGrid = !previewModelPtr->m_bshowGrid;
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_TOGGLE_EMITTERSHAPE:
        previewModelPtr->m_bshowEmitterShape = !previewModelPtr->m_bshowEmitterShape;
        break;
    case MenuActions::PRESET_RESET_COLOR:
        previewModelPtr->gridColor = ColorF(150, 150, 150, 40);
        previewModelPtr->m_clearColor.Set(0.5f, 0.5f, 0.5f);
        break;
    case MenuActions::PRESET_RESET_DEFAULT:
        previewModelPtr->gridColor = ColorF(150, 150, 150, 40);
        previewModelPtr->m_clearColor.Set(0.5f, 0.5f, 0.5f);
        previewModelPtr->m_bIsPlayLoop = false;
        previewWindowPtr->m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
        previewModelPtr->m_timeScale = 1.0f;
        previewWindowPtr->m_playSpeedButton->setValue(1.00);
        previewWindowPtr->m_playSpeedButton->setValue(1.0f);
        previewModelPtr->toggleOverDraw(false);
        previewModelPtr->resetSplineSetting();
        previewModelPtr->ReleaseMesh();
        previewWindowPtr->loadAll(previewWindowPtr->m_defaultSettings);
        break;
    case MenuActions::PRESET_HIDE_GIZMO:
        previewModelPtr->m_bshowGizmo = !previewModelPtr->m_bshowGizmo;
        break;
    case MenuActions::PRESET_GIZMO_RESET_DEFAULT:
        previewModelPtr->resetSplineSetting();
        break;
    default:
        break;
    }
}

void QPreviewWidget::OnMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_IMPORT_MESH:
        previewModelPtr->importMesh();
        break;
    case MenuActions::PRESET_PLAY:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_playState = CPreviewModelView::PLAY;
            previewWindowPtr->m_playPauseButton->setIcon(QIcon(":/particleQT/icons/pauseOrange.png"));
        }
        break;
    case MenuActions::PRESET_PAUSE:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_playState = CPreviewModelView::PAUSE;
            previewWindowPtr->m_playPauseButton->setIcon(QIcon(":/particleQT/icons/play.png"));
        }
        break;
    case MenuActions::PRESET_STEP_FORWARD:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_stepForward = true;
        }
        break;
    case MenuActions::PRESET_LOOP:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_bIsPlayLoop = !previewModelPtr->m_bIsPlayLoop;
            if (previewModelPtr->m_bIsPlayLoop)
            {
                previewWindowPtr->m_loopButton->setIcon(QIcon(":/particleQT/icons/loopOrange.png"));
            }
            else
            {
                previewWindowPtr->m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
            }
        }
        break;
    case MenuActions::PRESET_RESET:
        if (previewModelPtr->m_pEmitter != NULL)
        {
            previewModelPtr->m_playState = CPreviewModelView::RESET;
            previewWindowPtr->m_playPauseButton->setIcon(QIcon(":/particleQT/icons/play.png"));
        }
        break;
    case MenuActions::PRESET_RESET_CAMERA_POSITION:
        //change to default camera
        previewModelPtr->resetCamera();
        break;
    case MenuActions::PRESET_SHOW_OVERDRAW:
        previewModelPtr->toggleOverDraw();
        break;
    case MenuActions::PRESET_HIDE_GIZMO:
        previewModelPtr->m_bshowGizmo = !previewModelPtr->m_bshowGizmo;
        break;
    case MenuActions::PRESET_HIDE_GRID:
        //turn on/off grid
        previewModelPtr->m_bshowGrid = !previewModelPtr->m_bshowGrid;
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_HIDE_BOUDING_BOX:
        previewModelPtr->m_bshowBoundingBox = !previewModelPtr->m_bshowBoundingBox;
        break;
    case MenuActions::PRESET_BACKGROUND_COLOR:
        previewModelPtr->setBackgroundColor();
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_GRID_COLOR:
        previewModelPtr->setGridColor();
        previewModelPtr->Invalidate();
        break;
    case MenuActions::PRESET_RESET_COLOR:
        previewModelPtr->gridColor = ColorF(150, 150, 150, 40);
        previewModelPtr->m_clearColor.Set(0.5f, 0.5f, 0.5f);
        break;
    case MenuActions::PRESET_RESET_DEFAULT:
        previewModelPtr->gridColor = ColorF(150, 150, 150, 40);
        previewModelPtr->m_clearColor.Set(0.5f, 0.5f, 0.5f);
        previewModelPtr->m_bIsPlayLoop = false;
        previewWindowPtr->m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
        previewModelPtr->m_timeScale = 1.0f;
        previewWindowPtr->m_playSpeedButton->setValue(1.0f);
        previewModelPtr->m_bShowPlaybackControl = true;
        previewModelPtr->m_bDrawWireFrame = false;
        previewModelPtr->toggleOverDraw(false);
        previewModelPtr->resetSplineSetting();
        previewModelPtr->Invalidate();
        previewModelPtr->m_bshowBoundingBox = false;
        previewModelPtr->ReleaseMesh();
        previewWindowPtr->m_wireframeButton->setIcon(QIcon(":/particleQT/icons/wireframe.png"));
        break;
    case MenuActions::PRESET_RESET_PLAYBACK_CONTROL:
        previewModelPtr->m_bIsPlayLoop = false;
        previewWindowPtr->m_loopButton->setIcon(QIcon(":/particleQT/icons/loop.png"));
        previewModelPtr->m_timeScale = 1.0f;
        previewWindowPtr->m_playSpeedButton->setValue(1.00);
        previewModelPtr->m_bShowPlaybackControl = true;
        break;
    default:
        break;
    }
}

void QPreviewWidget::OnGizmoMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_GIZMO_RESET_DEFAULT:
        previewModelPtr->resetSplineSetting();
        break;
    default:
        break;
    }
}

void QPreviewWidget::buildEmitterMenu()
{
    customEmitterMenu->clear();
    customEmitterMenu = new ContextMenu(this);
    customEmitterMenu->setTitle("Customize default emitter");
    QAction* action;
    buildLoadEmitterMenu();
    customEmitterMenu->addMenu(loadEmitterMenu);

    action = customEmitterMenu->addAction(tr("Change color..."));
    connect(action, &QAction::triggered, this, [=]()
        {
            previewModelPtr->onSetColor();
        });
    action = customEmitterMenu->addAction(tr("Change Texture..."));
    connect(action, &QAction::triggered, this, [=]()
        {
            previewModelPtr->onSetTexture();
        });
    action = customEmitterMenu->addAction(tr("Export..."));
    connect(action, &QAction::triggered, this, [=]()
        {
            previewModelPtr->onExportEmitter();
        });
    action = customEmitterMenu->addAction(tr("Save"));
    connect(action, &QAction::triggered, this, [=]()
        {
            previewModelPtr->onSaveEmitter();
            buildLoadEmitterMenu();
        });
}

void QPreviewWidget::buildLoadEmitterMenu()
{
    loadEmitterMenu->clear();
    loadEmitterMenu->setTitle(tr("Load Emitter"));
    DefaultEmitter* emitters = (DefaultEmitter*)GetIEditor()->GetParticleUtils()->DefaultEmitter_GetAllSavedEmitters();
    int emitterCount = GetIEditor()->GetParticleUtils()->DefaultEmitter_GetCount();
    QAction* action;

    for (int i = 0; i < emitterCount; i++)
    {
        if (emitters[i].current)
        {
            action = loadEmitterMenu->addAction(emitters[i].name);
            connect(action, &QAction::triggered, this, [this, action]()
                {
                    previewModelPtr->onSelectEmitter(action->text().toStdString().c_str());
                });
        }
        else
        {
            loadEmitterMenu->addSeparator();
            action = loadEmitterMenu->addAction(emitters[i].name);
            connect(action, &QAction::triggered, this, [this, action]()
                {
                    previewModelPtr->onSelectEmitter(action->text().toStdString().c_str());
                });
            loadEmitterMenu->addSeparator();
        }
    }


    loadEmitterMenu->addSeparator();
    action = loadEmitterMenu->addAction(tr("Import Emitter..."));
    connect(action, &QAction::triggered, this, [this, action]()
        {
            previewModelPtr->onImportEmitter();
        });
}

void QPreviewWidget::OnSplineModeMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_SINGLEPATH_EMITTER:
        previewModelPtr->m_splineLoop = false;
        previewModelPtr->m_splinePingpong = false;
        break;
    case MenuActions::PRESET_LOOP_EMITTER:
        previewModelPtr->m_splineLoop = true;
        previewModelPtr->m_splinePingpong = false;
        break;
    case MenuActions::PRESET_PINGPONG_EMITTER:
        previewModelPtr->m_splinePingpong = true;
        break;
    }
}

void QPreviewWidget::OnSplineShapeMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_SPLINE_LINE:
        previewModelPtr->m_splineMode = 0;
        break;
    case MenuActions::PRESET_SPLINE_SINE:
        previewModelPtr->m_splineMode = 1;
        break;
    case MenuActions::PRESET_SPLINE_COIL:
        previewModelPtr->m_splineMode = 2;
        break;
    }
    previewModelPtr->m_splineTimer = 0;
    previewModelPtr->m_isSplinePlayForward = true;
}

void QPreviewWidget::OnSplineSpeedMenuActionClicked(QAction* action)
{
    int type = action->data().toInt();
    switch (type)
    {
    case MenuActions::PRESET_SPLINE_SPEED_X1:
        previewModelPtr->m_splineSpeed = 1;
        break;
    case MenuActions::PRESET_SPLINE_SPEED_X2:
        previewModelPtr->m_splineSpeed = 2;
        break;
    case MenuActions::PRESET_SPLINE_SPEED_X3:
        previewModelPtr->m_splineSpeed = 3;
        break;
    case MenuActions::PRESET_SPLINE_SPEED_X5:
        previewModelPtr->m_splineSpeed = 5;
        break;
    }
}

#ifdef EDITOR_QT_UI_EXPORTS
#include <QPreviewWidget.moc>
#endif

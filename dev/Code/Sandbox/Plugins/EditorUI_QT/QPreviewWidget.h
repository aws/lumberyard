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
#pragma once

#include <QWidget>
#include "QMenu"
#include "VariableWidgets/QCustomColorDialog.h"
#include "ContextMenu.h"

class CPreviewWindowView;
class CPreviewModelView;
class QPreviewWidget
    : public QWidget
{
    Q_OBJECT
public:
    QPreviewWidget(CPreviewModelView* ModelViewPtr, CPreviewWindowView* WindowViewPtr);
    virtual ~QPreviewWidget();

    QMenu* customEmitterMenu;
    QMenu* loadEmitterMenu;
    ///////////////////////////
    void buildPreviewMenu(QMenu* previewMenu);
    void buildGizmoMenu(QMenu* gizmoMenu);
    void buildContextMenu(QMenu* contextMenu);
    void buildEmitterMenu();
    void buildLoadEmitterMenu();
    CPreviewWindowView* previewWindowPtr;
    CPreviewModelView* previewModelPtr;
    //menu
    enum MenuActions
    {
        NONE = 0,
        PRESET_IMPORT_MESH,
        PRESET_PLAY,
        PRESET_PAUSE,
        PRESET_STEP_FORWARD,
        PRESET_LOOP,
        PRESET_RESET,
        PRESET_RESET_CAMERA_POSITION,
        PRESET_ADD_COLLISION_OBJECT,
        PRESET_SHOW_OVERDRAW,
        PRESET_HIDE_GIZMO,
        PRESET_HIDE_PARTICLE_COUNT,
        PRESET_HIDE_GRID,
        PRESET_HIDE_BOUDING_BOX,
        PRESET_BACKGROUND_COLOR,
        PRESET_GRID_COLOR,
        PRESET_RESET_COLOR,
        PRESET_RESET_DEFAULT,
        PRESET_RESET_PLAYBACK_CONTROL,
        PRESET_SCALE,
        PRESET_MOVE,
        PRESET_ROTATE,
        PRESET_SINGLEPATH_EMITTER,
        PRESET_LOOP_EMITTER,
        PRESET_PINGPONG_EMITTER,
        PRESET_SPLINE_MOVEMENT,
        PRESET_CHANGE_EMITTER_COLOR,
        PRESET_GIZMO_RESET_DEFAULT,
        PRESET_SPLINE_LINE,
        PRESET_SPLINE_SINE,
        PRESET_SPLINE_COIL,
        PRESET_WIREFRAME_VIEW,
        PRESET_SHOW_GIZMO,
        PRESET_HIDE_PLAYBACK_CONTROLS,
        PRESET_TOGGLE_EMITTERSHAPE,
        PRESET_SHOW_SPEEDDIALOG,
        PRESET_SPLINE_SPEED_X1,
        PRESET_SPLINE_SPEED_X2,
        PRESET_SPLINE_SPEED_X3,
        PRESET_SPLINE_SPEED_X5
    };


    QMenu* execPreviewMenu();
    void execContextMenu(QPoint position);
    void execGizmoMenu(QPoint position);
protected slots:
    void OnContextMenuActionClicked(QAction* action);
    void OnGizmoMenuActionClicked(QAction* action);
    void OnMenuActionClicked(QAction* action);
    void OnSplineModeMenuActionClicked(QAction* action);
    void OnSplineShapeMenuActionClicked(QAction* action);
    void OnSplineSpeedMenuActionClicked(QAction* action);
private:
    ContextMenu* m_PreviewMenu;
};


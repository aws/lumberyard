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
#include "api.h"
#include <QWidget>
#include <QSettings>

struct IParticleEffect;
class QAmazonDoubleSpinBox;
class ParticlePreviewModelView;
class CParticleItem;
class QToolTipWidget;
class QFrame;
class QVBoxLayout;
class QPushButton;
class QLabel;
class QAction;
class QSlider;
class QMenu;
class QShortcutEvent;
struct SLodInfo;

class EDITOR_QT_UI_API CPreviewWindowView
    : public QWidget
{
    Q_OBJECT
public:
    CPreviewWindowView(QWidget* parent);
    ~CPreviewWindowView();

    void ExecContextMenu(QPoint position);
    QMenu* ExecPreviewMenu();

    void HandleHotkeys(QKeyEvent* e);
    bool HandleShortcuts(QShortcutEvent* e);
    void FocusCameraOnEmitter();

    void SetActiveEffect(CParticleItem* pParticle, SLodInfo* lod);

    void SaveSessionState(QString filepath);
    void SaveSessionState(QByteArray& out);
    void LoadSessionState(QString filepath);
    void LoadSessionState(QByteArray data);

    void ToData(QByteArray& data);
    void FromData(QByteArray& data);

    void ForceParticleEmitterRestart();
    void ResetToDefaultLayout();

private Q_SLOTS:
    void OnWireframeClicked();
    void OnTimeOfDayClicked();
    void OnResetClicked();
    void OnPlayPauseClicked();
    void OnStepClicked();
    void OnLoopClicked();
    void OnCameraMenuActionClicked(QAction* action);
    void OnMenuActionClicked(QAction* action);
    void OnPlaySpeedButtonValueChanged(double value);
private:

    void CorrectLayout(QByteArray& data);
    void UpdateTimeLabel();
    void UpdateParticleCount();
    void UpdatePlaybackUI();

    void SpawnAllEffect(IParticleEffect* effect, CParticleItem* pParticles);
    void RefreshUIStates();
    void PostBuildUI();
    void BuildPreviewWindowUI();
    virtual bool eventFilter(QObject* obj, QEvent* event) override;
    virtual QSize   sizeHint() const Q_DECL_OVERRIDE { return QSize(256, 256); }
    virtual bool event(QEvent*) override;
    virtual void keyPressEvent(QKeyEvent*) override;
    QFrame* m_topFrame;
    QFrame* m_botFrame;
    QVBoxLayout* dockLayout;
    ////////////////////////////////////////////////////////

    void BuildPreviewMenu(QMenu* previewMenu);
    void BuildContextMenu(QMenu* contextMenu);
    void BuildEmitterMenu();
    void BuildLoadEmitterMenu();
    ////////////////////////////////////////////////////////

    ParticlePreviewModelView* m_previewModelView;
    CParticleItem* m_pActiveItem;
    SLodInfo*   m_activeLod;
    QToolTipWidget* m_tooltip;
    QPushButton* m_playPauseButton;
    QPushButton* m_loopButton;
    QPushButton* m_resetButton;
    QAmazonDoubleSpinBox* m_playSpeedButton;
    QLabel* m_numOfParticle;
    QLabel* m_timeLabel;
    QSlider* m_timeSlider;
    QPushButton* cameraDropdownButton;
    QPushButton* m_wireframeButton;
    QPushButton* m_timeOfDayButton;
    QPushButton* m_focusButton;
    QAction* m_showEmitterOnly;
    QAction* m_showEmitterChildren;
    QAction* m_showEmitterChildrenParents;
    QPushButton* m_stepButton;
    QPushButton* playSpeedButton;
    QSlider* comboSlider;

    enum class ShowEmitterSetting
    {
        EmitterOnly,
        EmitterAndAllChildren,
        EmitterAndAllChildrenAndParents
    };

    void HandleEmitterViewChange(ShowEmitterSetting showState);

    ShowEmitterSetting ShowEmitterSettingState;

    ////////////////////////////////////////////////////////
    enum class MenuActions
    {
        NONE = 0,
        PRESET_IMPORT_MESH,
        PRESET_PLAY,
        PRESET_PAUSE,
        PRESET_STEP_FORWARD,
        PRESET_LOOP,
        PRESET_RESET,
        PRESET_TOGGLE_PLAYPAUSE,
        PRESET_RESET_CAMERA_POSITION,
        PRESET_ADD_COLLISION_OBJECT,
        PRESET_SHOW_OVERDRAW,
        PRESET_HIDE_GIZMO,
        PRESET_HIDE_PARTICLE_COUNT,
        PRESET_HIDE_GRID,
        PRESET_HIDE_BOUNDING_BOX,
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
        PRESET_SPLINE_RESET_DEFAULT,
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

    void HandleMenuAction(MenuActions action);

    QMenu* m_PreviewMenu;
    QMenu* customEmitterMenu;
    QMenu* loadEmitterMenu;
};


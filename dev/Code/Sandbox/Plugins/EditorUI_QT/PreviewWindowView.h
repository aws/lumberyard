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
#include <AzQtComponents/Components/Widgets/Slider.h>
#include "AzQtComponents/Components/Widgets/SpinBox.h"

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

class PreviewToolButton
    : public QToolButton
{
    Q_OBJECT
public:
    explicit PreviewToolButton(QWidget* parent = nullptr);

    void SetNormalIcon(const QIcon& icon) { m_normalIcon[0] = icon; }
    void SetHoverIcon(const QIcon& icon) { m_hoverIcon[0] = icon; }
    void SetSelectedIcon(const QIcon& icon) { m_selectedIcon[0] = icon; }
    void SetIconsForType(const QString& iconBaseName, int level = 0);
    void SetClicked(bool clicked) { m_clicked = clicked; }
    void RefreshIcon();
    void SetLevel(int level);
    void HandlePressedEvent();
    void HandleReleasedEvent();

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QVector<QIcon> m_normalIcon = (QVector<QIcon>(3));
    QVector <QIcon> m_hoverIcon = (QVector<QIcon>(3));
    QVector <QIcon> m_selectedIcon = (QVector<QIcon>(3));
    int m_iconLevelSet = 0;
    bool m_hovering = false;
    bool m_clicked = false;
};

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
    PreviewToolButton* m_playPauseButton;
    PreviewToolButton* m_loopButton;
    PreviewToolButton* m_resetButton;
    AzQtComponents::DoubleSpinBox* m_playSpeedButton;
    QLabel* m_numOfParticle;
    QLabel* m_timeLabel;
    AzQtComponents::SliderDouble* m_timeSlider;
    PreviewToolButton* m_cameraDropdownButton;
    PreviewToolButton* m_wireframeButton;
    PreviewToolButton* m_timeOfDayButton;
    PreviewToolButton* m_focusButton;
    QAction* m_showEmitterOnly;
    QAction* m_showEmitterChildren;
    QAction* m_showEmitterChildrenParents;
    PreviewToolButton* m_stepButton;

    enum class ShowEmitterSetting
    {
        EmitterOnly,
        EmitterAndAllChildren,
        EmitterAndAllChildrenAndParents
    };

    void HandleEmitterViewChange(ShowEmitterSetting showState);

    ShowEmitterSetting m_showEmitterSettingState;

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
    QMenu* m_customEmitterMenu;
    QMenu* m_loadEmitterMenu;
};


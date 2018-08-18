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
#include "InfoBar.h"
#include "DisplaySettings.h"
#include "GameEngine.h"
#include "Include/ITransformManipulator.h"
#include "Objects/EntityObject.h"
#include <IConsole.h>
#include <HMDBus.h>
#include "MainWindow.h"
#include "ActionManager.h"

#include "MathConversion.h"
#include <AzFramework/Math/MathUtils.h>

#include <QFontMetrics>

#include <ui_InfoBar.h>

void BeautifyEulerAngles(Vec3& v)
{
    if (v.x + v.y + v.z >= 360.0f)
    {
        v.x = 180.0f - v.x;
        v.y = 180.0f - v.y;
        v.z = 180.0f - v.z;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CInfoBar dialog
CInfoBar::CInfoBar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CInfoBar)
{
    ui->setupUi(this);

    foreach(auto buttons, findChildren<QAbstractButton*>())
    {
        buttons->setProperty("class", "tiny");
    }

    m_enabledVector = false;
    m_bVectorLock = false;
    m_prevEditMode = 0;
    m_bSelectionLocked = false;
    m_bSelectionChanged = false;
    m_editTool = 0;
    m_bDragMode = false;
    m_prevMoveSpeed = 0;
    m_currValue = Vec3(-111, +222, -333); //this wasn't initialized. I don't know what a good value is
    m_oldMasterVolume = 1.0f;

    GetIEditor()->RegisterNotifyListener(this);

    //audio request setup
    m_oMuteAudioRequest.pData       = &m_oMuteAudioRequestData;
    m_oUnmuteAudioRequest.pData = &m_oUnmuteAudioRequestData;

    OnInitDialog();

    connect(ui->m_vectorLock, &QToolButton::clicked, this, &CInfoBar::OnVectorLock);
    connect(ui->m_lockSelection, &QToolButton::clicked, this, &CInfoBar::OnLockSelection);

    auto doubleSpinBoxValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(ui->m_moveSpeed, doubleSpinBoxValueChanged, this, &CInfoBar::OnUpdateMoveSpeed);

    connect(ui->m_posCtrlX, &QNumberCtrl::valueChanged, this, &CInfoBar::OnVectorChanged);
    connect(ui->m_posCtrlY, &QNumberCtrl::valueChanged, this, &CInfoBar::OnVectorChanged);
    connect(ui->m_posCtrlZ, &QNumberCtrl::valueChanged, this, &CInfoBar::OnVectorChanged);

    connect(ui->m_posCtrlX, &QNumberCtrl::valueUpdated, this, &CInfoBar::OnVectorUpdateX);
    connect(ui->m_posCtrlY, &QNumberCtrl::valueUpdated, this, &CInfoBar::OnVectorUpdateY);
    connect(ui->m_posCtrlZ, &QNumberCtrl::valueUpdated, this, &CInfoBar::OnVectorUpdateZ);

    connect(ui->m_posCtrlX, &QNumberCtrl::dragStarted, this, &CInfoBar::OnBeginVectorUpdate);
    connect(ui->m_posCtrlY, &QNumberCtrl::dragStarted, this, &CInfoBar::OnBeginVectorUpdate);
    connect(ui->m_posCtrlZ, &QNumberCtrl::dragStarted, this, &CInfoBar::OnBeginVectorUpdate);

    connect(ui->m_posCtrlX, &QNumberCtrl::dragFinished, this, &CInfoBar::OnEndVectorUpdate);
    connect(ui->m_posCtrlY, &QNumberCtrl::dragFinished, this, &CInfoBar::OnEndVectorUpdate);
    connect(ui->m_posCtrlZ, &QNumberCtrl::dragFinished, this, &CInfoBar::OnEndVectorUpdate);

    connect(ui->m_posCtrlX, &QNumberCtrl::mouseReleased, this, &CInfoBar::OnEnableIdleUpdate);
    connect(ui->m_posCtrlY, &QNumberCtrl::mouseReleased, this, &CInfoBar::OnEnableIdleUpdate);
    connect(ui->m_posCtrlZ, &QNumberCtrl::mouseReleased, this, &CInfoBar::OnEnableIdleUpdate);
    connect(ui->m_posCtrlX, &QNumberCtrl::mousePressed, this, &CInfoBar::OnDisableIdleUpdate);
    connect(ui->m_posCtrlY, &QNumberCtrl::mousePressed, this, &CInfoBar::OnDisableIdleUpdate);
    connect(ui->m_posCtrlZ, &QNumberCtrl::mousePressed, this, &CInfoBar::OnDisableIdleUpdate);

    connect(ui->m_setVector, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSetVector);
    connect(ui->m_terrainCollision, &QToolButton::clicked, this, &CInfoBar::OnBnClickedTerrainCollision);
    connect(ui->m_physicsBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedPhysics);
    connect(ui->m_physSingleStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSingleStepPhys);
    connect(ui->m_physDoStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedDoStepPhys);
    connect(ui->m_syncPlayerBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSyncplayer);
    connect(ui->m_gotoPos, &QToolButton::clicked, this, &CInfoBar::OnBnClickedGotoPosition);
    connect(ui->m_speed_01, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSpeed01);
    connect(ui->m_speed_1, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSpeed1);
    connect(ui->m_speed_10, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSpeed10);
    connect(ui->m_muteBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedMuteAudio);
    connect(ui->m_vrBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedEnableVR);

    connect(this, &CInfoBar::ActionTriggered, MainWindow::instance()->GetActionManager(), &ActionManager::ActionTriggered);

    connect(ui->m_lockSelection, &QAbstractButton::toggled, ui->m_lockSelection, [this](bool checked) {
        ui->m_lockSelection->setToolTip(checked ? tr("Unlock Object Selection") : tr("Lock Object Selection"));
    });
    connect(ui->m_vectorLock, &QAbstractButton::toggled, ui->m_vectorLock, [this](bool checked) {
        ui->m_vectorLock->setToolTip(checked ? tr("Unlock Axis Vectors") : tr("Lock Axis Vectors"));
    });
    connect(ui->m_terrainCollision, &QAbstractButton::toggled, ui->m_terrainCollision, [this](bool checked) {
        ui->m_terrainCollision->setToolTip(checked ? tr("Disable Terrain Camera Collision (Q)") : tr("Enable Terrain Camera Collision (Q)"));
    });
    connect(ui->m_physicsBtn, &QAbstractButton::toggled, ui->m_physicsBtn, [this](bool checked) {
        ui->m_physicsBtn->setToolTip(checked ? tr("Disable Physics/AI (Ctrl+P)") : tr("Enable Physics/AI (Ctrl+P)"));
    });
    connect(ui->m_physSingleStepBtn, &QAbstractButton::toggled, ui->m_physSingleStepBtn, [this](bool checked) {
        ui->m_physSingleStepBtn->setToolTip(checked ? tr("Disable Physics/AI Single-step Mode ('<' in Game Mode)") : tr("Enable Physics/AI Single-step Mode ('<' in Game Mode)"));
    });
    connect(ui->m_syncPlayerBtn, &QAbstractButton::toggled, ui->m_syncPlayerBtn, [this](bool checked) {
        ui->m_syncPlayerBtn->setToolTip(checked ? tr("Synchronize Player with Camera") : tr("Move Player and Camera Separately"));
    });
    connect(ui->m_muteBtn, &QAbstractButton::toggled, ui->m_muteBtn, [this](bool checked) {
        ui->m_muteBtn->setToolTip(checked ? tr("Un-mute Audio") : tr("Mute Audio"));
    });
    connect(ui->m_vrBtn, &QAbstractButton::toggled, ui->m_vrBtn, [this](bool checked) {
        ui->m_vrBtn->setToolTip(checked ? tr("Disable VR Preview") : tr("Enable VR Preview"));
    });
}

//////////////////////////////////////////////////////////////////////////
CInfoBar::~CInfoBar()
{
    GetIEditor()->UnregisterNotifyListener(this);

    AZ::VR::VREventBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        IdleUpdate();
    }
    else if (event == eNotify_OnBeginGameMode || event == eNotify_OnEndGameMode)
    {
        // Audio: determine muted state of audio
        //m_bMuted = gEnv->pAudioSystem->GetMasterVolume() == 0.f;
        ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
    }
    else if (event == eNotify_OnBeginLoad || event == eNotify_OnCloseScene)
    {
        // make sure AI/Physics is disabled on level load (CE-4229)
        if (GetIEditor()->GetGameEngine()->GetSimulationMode())
        {
            OnBnClickedPhysics();
        }

        ui->m_physicsBtn->setEnabled(false);
        ui->m_physSingleStepBtn->setEnabled(false);
        ui->m_physDoStepBtn->setEnabled(false);
    }
    else if (event == eNotify_OnEndLoad || event == eNotify_OnEndNewScene)
    {
        ui->m_physicsBtn->setEnabled(true);
        ui->m_physSingleStepBtn->setEnabled(true);
        ui->m_physDoStepBtn->setEnabled(true);
    }
    else if (event == eNotify_OnSelectionChange)
    {
        m_bSelectionChanged = true;
    }
    else if (event == eNotify_OnEditModeChange)
    {
        int emode = GetIEditor()->GetEditMode();
        switch (emode)
        {
        case eEditModeMove:
            ui->m_setVector->setToolTip(tr("Set Position of Selected Objects"));
            break;
        case eEditModeRotate:
            ui->m_setVector->setToolTip(tr("Set Rotation of Selected Objects"));
            break;
        case eEditModeScale:
            ui->m_setVector->setToolTip(tr("Set Scale of Selected Objects"));
            break;
        default:
            ui->m_setVector->setToolTip(tr("Set Position/Rotation/Scale of Selected Objects (None Selected)"));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnVectorUpdateX()
{
    if (ui->m_posCtrlX->IsDragging() || m_currValue.x != ui->m_posCtrlX->value())
    {
        if (m_bVectorLock)
        {
            Vec3 v = GetVector();
            SetVector(Vec3(v.x, v.x, v.x));
        }
        OnVectorUpdate(false);
    }
}

void CInfoBar::OnVectorUpdateY()
{
    if (ui->m_posCtrlY->IsDragging() || m_currValue.y != ui->m_posCtrlY->value())
    {
        if (m_bVectorLock)
        {
            Vec3 v = GetVector();
            SetVector(Vec3(v.y, v.y, v.y));
        }
        OnVectorUpdate(false);
    }
}

void CInfoBar::OnVectorUpdateZ()
{
    if (ui->m_posCtrlZ->IsDragging() || m_currValue.z != ui->m_posCtrlZ->value())
    {
        if (m_bVectorLock)
        {
            Vec3 v = GetVector();
            SetVector(Vec3(v.z, v.z, v.z));
        }
        OnVectorUpdate(false);
    }
}

void CInfoBar::OnVectorChanged()
{
    SetVector(GetVector());
    OnVectorUpdate(false);
}

void CInfoBar::OnVectorUpdate(bool followTerrain)
{
    int emode = GetIEditor()->GetEditMode();
    if (emode != eEditModeMove && emode != eEditModeRotate && emode != eEditModeScale)
    {
        return;
    }


    Vec3 v = GetVector();

    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();
    if (pManipulator)
    {
        CEditTool* pEditTool = GetIEditor()->GetEditTool();

        if (pEditTool)
        {
            Vec3 diff = v - m_lastValue;
            if (emode == eEditModeMove)
            {
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
            if (emode == eEditModeRotate)
            {
                diff = DEG2RAD(diff);
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
            if (emode == eEditModeScale)
            {
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
        }
        return;
    }

    CSelectionGroup* selection = GetIEditor()->GetObjectManager()->GetSelection();
    if (selection->IsEmpty())
    {
        return;
    }

    GetIEditor()->RestoreUndo();

    int referenceCoordSys = GetIEditor()->GetReferenceCoordSys();

    CBaseObject* obj = GetIEditor()->GetSelectedObject();

    Matrix34 tm;
    AffineParts ap;
    if (obj)
    {
        tm = obj->GetWorldTM();
        ap.SpectralDecompose(tm);
    }

    if (emode == eEditModeMove)
    {
        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm.SetTranslation(v);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetPos(v);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->MoveTo(v, followTerrain ? CSelectionGroup::eMS_FollowTerrain : CSelectionGroup::eMS_None, referenceCoordSys);
        }
    }
    if (emode == eEditModeRotate)
    {
        if (obj)
        {
            Quat qrot = AZQuaternionToLYQuaternion(AZ::ConvertEulerDegreesToQuaternion(LYVec3ToAZVec3(v)));

            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(ap.scale, qrot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetRotation(qrot);
            }
        }
        else
        {
            CBaseObject *refObj;
            CSelectionGroup* pGroup = GetIEditor()->GetSelection();
            if (pGroup && pGroup->GetCount() > 0)
            {
                refObj = pGroup->GetObject(0);
                AffineParts ap;
                ap.SpectralDecompose(refObj->GetWorldTM());
                Vec3 oldEulerRotation = AZVec3ToLYVec3(AZ::ConvertQuaternionToEulerDegrees(LYQuaternionToAZQuaternion(ap.rot)));
                Vec3 diff = v - oldEulerRotation;
                GetIEditor()->GetSelection()->Rotate((Ang3)diff, referenceCoordSys);
            }
        }
    }
    if (emode == eEditModeScale)
    {
        if (v.x == 0 || v.y == 0 || v.z == 0)
        {
            return;
        }

        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(v, ap.rot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetScale(v);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->SetScale(v, referenceCoordSys);
        }
    }

    if (obj)
    {
        obj->UpdateGroup();
    }
}

void CInfoBar::IdleUpdate()
{
    if (!m_idleUpdateEnabled)
    {
        return;
    }

    bool updateUI = false;
    // Update Width/Height of selection rectangle.
    AABB box;
    GetIEditor()->GetSelectedRegion(box);
    float width = box.max.x - box.min.x;
    float height = box.max.y - box.min.y;
    if (m_width != width || m_height != height)
    {
        m_width = width;
        m_height = height;
        updateUI = true;
    }

    Vec3 marker = GetIEditor()->GetMarkerPosition();

    /*
    // Get active viewport.
    int hx = marker.x / 2;
    int hy = marker.y / 2;
    if (m_heightMapX != hx || m_heightMapY != hy)
    {
        m_heightMapX = hx;
        m_heightMapY = hy;
        updateUI = true;
    }
    */

    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    bool bWorldSpace = GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD;

    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() != m_numSelected)
    {
        m_numSelected = selection->GetCount();
        updateUI = true;
    }

    if (GetIEditor()->GetEditTool() != m_editTool)
    {
        updateUI = true;
        m_editTool = GetIEditor()->GetEditTool();
    }

    QString str;
    if (m_editTool)
    {
        str = m_editTool->GetStatusText();
        if (str != m_sLastText)
        {
            updateUI = true;
        }
    }

    if (updateUI)
    {
        if (!m_editTool)
        {
            if (m_numSelected == 0)
            {
                str = tr("None Selected");
            }
            else if (m_numSelected == 1)
            {
                str = tr("1 Object Selected");
            }
            else
            {
                str = tr("%1 Objects Selected").arg(m_numSelected);
            }
        }
        ui->m_statusText->setText(str);
        m_sLastText = str;
    }

    if (gSettings.cameraMoveSpeed != m_prevMoveSpeed)
    {
        m_prevMoveSpeed = gSettings.cameraMoveSpeed;
        ui->m_moveSpeed->setValue(gSettings.cameraMoveSpeed);
    }

    {
        int settings = GetIEditor()->GetDisplaySettings()->GetSettings();
        bool noCollision = settings & SETTINGS_NOCOLLISION;
        if ((!ui->m_terrainCollision->isChecked() && !noCollision) ||
            (ui->m_terrainCollision->isChecked() && noCollision))
        {
            ui->m_terrainCollision->setChecked(!noCollision);
        }

        bool bPhysics = GetIEditor()->GetGameEngine()->GetSimulationMode();
        if ((ui->m_physicsBtn->isChecked() && !bPhysics) ||
            (!ui->m_physicsBtn->isChecked() && bPhysics))
        {
            ui->m_physicsBtn->setChecked(bPhysics);
        }

        bool bSingleStep = (gEnv->pPhysicalWorld->GetPhysVars()->bSingleStepMode != 0);
        if (ui->m_physSingleStepBtn->isChecked() != bSingleStep)
        {
            ui->m_physSingleStepBtn->setChecked(bSingleStep);
        }

        bool bSyncPlayer = GetIEditor()->GetGameEngine()->IsSyncPlayerPosition();
        if ((!ui->m_syncPlayerBtn->isChecked() && !bSyncPlayer) ||
            (ui->m_syncPlayerBtn->isChecked() && bSyncPlayer))
        {
            ui->m_syncPlayerBtn->setChecked(!bSyncPlayer);
        }
    }

    bool bSelLocked = GetIEditor()->IsSelectionLocked();
    if (bSelLocked != m_bSelectionLocked)
    {
        m_bSelectionLocked = bSelLocked;
        ui->m_lockSelection->setChecked(m_bSelectionLocked);
    }


    if (GetIEditor()->GetSelection()->IsEmpty())
    {
        if (ui->m_lockSelection->isEnabled())
        {
            ui->m_lockSelection->setEnabled(false);
        }
    }
    else
    {
        if (!ui->m_lockSelection->isEnabled())
        {
            ui->m_lockSelection->setEnabled(true);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Update vector.
    //////////////////////////////////////////////////////////////////////////
    Vec3 v(0, 0, 0);
    bool enable = false;
    float min = 0, max = 10000;

    int emode = GetIEditor()->GetEditMode();
    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();

    if (pManipulator)
    {
        AffineParts ap;
        ap.SpectralDecompose(pManipulator->GetTransformation(coordSys));

        if (emode == eEditModeMove)
        {
            v = ap.pos;
            enable = true;

            min = -64000;
            max = 64000;
        }
        if (emode == eEditModeRotate)
        {
            v = Vec3(RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(ap.rot))));
            enable = true;
            min = -10000;
            max = 10000;
        }
        if (emode == eEditModeScale)
        {
            v = ap.scale;
            enable = true;
            min = -10000;
            max = 10000;
        }
    }
    else
    {
        if (selection->IsEmpty())
        {
            // Show marker position.
            EnableVector(false);
            SetVector(marker);
            SetVectorRange(-100000, 100000);
            return;
        }

        CBaseObject* obj = GetIEditor()->GetSelectedObject();
        if (!obj)
        {
            CSelectionGroup* pGroup = GetIEditor()->GetSelection();
            if (pGroup && pGroup->GetCount() > 0)
            {
                obj = pGroup->GetObject(0);
            }
        }

        if (obj)
        {
            v = obj->GetWorldPos();
        }

        if (emode == eEditModeMove)
        {
            if (obj)
            {
                if (bWorldSpace)
                {
                    v = obj->GetWorldTM().GetTranslation();
                }
                else
                {
                    v = obj->GetPos();
                }
            }
            enable = true;
            min = -64000;
            max = 64000;
        }
        if (emode == eEditModeRotate)
        {
            if (obj)
            {
                Quat objRot;
                if (bWorldSpace)
                {
                    AffineParts ap;
                    ap.SpectralDecompose(obj->GetWorldTM());
                    objRot = ap.rot;
                }
                else
                {
                    objRot = obj->GetRotation();
                }

                // Always convert objRot to v in order to ensure that the inspector and info bar are always in sync
                v = AZVec3ToLYVec3(AZ::ConvertQuaternionToEulerDegrees(LYQuaternionToAZQuaternion(objRot)));
            }
            enable = true;
            min = -10000;
            max = 10000;
        }
        if (emode == eEditModeScale)
        {
            if (obj)
            {
                if (bWorldSpace)
                {
                    AffineParts ap;
                    ap.SpectralDecompose(obj->GetWorldTM());
                    v = ap.scale;
                }
                else
                {
                    v = obj->GetScale();
                }
            }
            enable = true;
            min = -10000;
            max = 10000;
        }
    }

    bool updateDisplayVector = (m_currValue != v);

    // If Edit mode changed.
    if (m_prevEditMode != emode)
    {
        // Scale mode enables vector lock.
        SetVectorLock(emode == eEditModeScale);

        // Change undo strings.
        QString undoString("Modify Object(s)");
        int mode = GetIEditor()->GetEditMode();
        switch (mode)
        {
        case eEditModeMove:
            undoString = QStringLiteral("Move Object(s)");
            break;
        case eEditModeRotate:
            undoString = QStringLiteral("Rotate Object(s)");
            break;
        case eEditModeScale:
            undoString = QStringLiteral("Scale Object(s)");
            break;
        }
        ui->m_posCtrlX->EnableUndo(undoString);
        ui->m_posCtrlY->EnableUndo(undoString);
        ui->m_posCtrlZ->EnableUndo(undoString);

        // edit mode changed, we must update the number values
        updateDisplayVector = true;
    }

    SetVectorRange(min, max);
    EnableVector(enable);

    // if our selection changed, or if our display values are out of date
    if (m_bSelectionChanged)
    {
        updateDisplayVector = true;
        m_bSelectionChanged = false;
    }

    // If there is a drag operation in progress, always update the number values
    updateDisplayVector |= m_bDragMode && (ui->m_posCtrlX->IsDragging() || ui->m_posCtrlY->IsDragging() || ui->m_posCtrlZ->IsDragging());
    if (updateDisplayVector)
    {
        SetVector(v);
    }

    m_prevEditMode = emode;
}

inline double Round(double fVal, double fStep)
{
    if (fStep > 0.f)
    {
        fVal = int_round(fVal / fStep) * fStep;
    }
    return fVal;
}

void CInfoBar::SetVector(const Vec3& v)
{
    if (!m_bDragMode)
    {
        m_lastValue = m_currValue;
    }

    if (m_currValue != v)
    {
        static const double fPREC = 1e-4;
        ui->m_posCtrlX->setValue(Round(v.x, fPREC));
        ui->m_posCtrlY->setValue(Round(v.y, fPREC));
        ui->m_posCtrlZ->setValue(Round(v.z, fPREC));
        m_currValue = v;
    }
}

Vec3 CInfoBar::GetVector()
{
    Vec3 v;
    v.x = ui->m_posCtrlX->value();
    v.y = ui->m_posCtrlY->value();
    v.z = ui->m_posCtrlZ->value();
    m_currValue = v;
    return v;
}

void CInfoBar::EnableVector(bool enable)
{
    if (m_enabledVector != enable)
    {
        m_enabledVector = enable;
        ui->m_posCtrlX->setEnabled(enable);
        ui->m_posCtrlY->setEnabled(enable);
        ui->m_posCtrlZ->setEnabled(enable);

        ui->m_vectorLock->setEnabled(enable);
        ui->m_setVector->setEnabled(enable);
    }
}

void CInfoBar::SetVectorLock(bool bVectorLock)
{
    m_bVectorLock = bVectorLock;
    ui->m_vectorLock->setChecked(bVectorLock);
    GetIEditor()->SetAxisVectorLock(bVectorLock);
}

void CInfoBar::SetVectorRange(float min, float max)
{
    // We call our custom override of SetRange so that we can avoid generating Qt messages.
    ui->m_posCtrlX->SetRange(min, max);
    ui->m_posCtrlY->SetRange(min, max);
    ui->m_posCtrlZ->SetRange(min, max);
}

void CInfoBar::OnVectorLock()
{
    // TODO: Add your control notification handler code here
    SetVectorLock(ui->m_vectorLock->isChecked());
}

void CInfoBar::OnLockSelection()
{
    // TODO: Add your control notification handler code here
    bool locked = ui->m_lockSelection->isChecked();
    ui->m_lockSelection->setChecked(locked);
    m_bSelectionLocked = (locked) ? true : false;
    GetIEditor()->LockSelection(m_bSelectionLocked);
}

void CInfoBar::OnUpdateMoveSpeed()
{
    gSettings.cameraMoveSpeed = ui->m_moveSpeed->value();
}

void CInfoBar::OnInitDialog()
{
    QFontMetrics metrics({});
    int width = metrics.boundingRect("-9999.99").width() * 1.1;

    ui->m_posCtrlX->setEnabled(false);
    ui->m_posCtrlX->setMaximumWidth(width);
    ui->m_posCtrlY->setEnabled(false);
    ui->m_posCtrlY->setMaximumWidth(width);
    ui->m_posCtrlZ->setEnabled(false);
    ui->m_posCtrlZ->setMaximumWidth(width);
    ui->m_setVector->setEnabled(false);

    ui->m_physicsBtn->setEnabled(false);
    ui->m_physSingleStepBtn->setEnabled(false);
    ui->m_physDoStepBtn->setEnabled(false);

    ui->m_moveSpeed->setRange(0.01, 100);
    ui->m_moveSpeed->setSingleStep(0.1);

    ui->m_moveSpeed->setValue(gSettings.cameraMoveSpeed);

    ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, gSettings.bMuteAudio ? m_oMuteAudioRequest : m_oUnmuteAudioRequest);
    ui->m_moveSpeed->setValue(gSettings.cameraMoveSpeed);

    //This is here just in case this class hasn't been created before
    //a VR headset was initialized
    ui->m_vrBtn->setEnabled(false);
    if (AZ::VR::HMDDeviceRequestBus::GetTotalNumOfEventHandlers() != 0)
    {
        ui->m_vrBtn->setEnabled(true);
    }

    AZ::VR::VREventBus::Handler::BusConnect();
}

void CInfoBar::OnEnableIdleUpdate()
{
    m_idleUpdateEnabled = true;
}
void CInfoBar::OnDisableIdleUpdate()
{
    m_idleUpdateEnabled = false;
}

void CInfoBar::OnHMDInitialized()
{
    ui->m_vrBtn->setEnabled(true);
}

void CInfoBar::OnHMDShutdown()
{
    ui->m_vrBtn->setEnabled(false);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBeginVectorUpdate()
{
    m_lastValue = GetVector();
    m_bDragMode = true;
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnEndVectorUpdate()
{
    m_bDragMode = false;
}

void CInfoBar::OnBnClickedTerrainCollision()
{
    emit ActionTriggered(ID_TERRAIN_COLLISION);
}

void CInfoBar::OnBnClickedPhysics()
{
    bool bPhysics = GetIEditor()->GetGameEngine()->GetSimulationMode();
    ui->m_physicsBtn->setChecked(bPhysics);
    emit ActionTriggered(ID_SWITCH_PHYSICS);

    if (bPhysics && ui->m_physSingleStepBtn->isChecked())
    {
        OnBnClickedSingleStepPhys();
    }
}

void CInfoBar::OnBnClickedSingleStepPhys()
{
    ui->m_physSingleStepBtn->setChecked(gEnv->pPhysicalWorld->GetPhysVars()->bSingleStepMode ^= 1);
}

void CInfoBar::OnBnClickedDoStepPhys()
{
    gEnv->pPhysicalWorld->GetPhysVars()->bDoStep = 1;
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSyncplayer()
{
    emit ActionTriggered(ID_GAME_SYNCPLAYER);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedGotoPosition()
{
    emit ActionTriggered(ID_DISPLAY_GOTOPOSITION);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSetVector()
{
    emit ActionTriggered(ID_DISPLAY_SETVECTOR);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed01()
{
    ui->m_moveSpeed->setValue(0.1);
    OnUpdateMoveSpeed();
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed1()
{
    ui->m_moveSpeed->setValue(1);
    OnUpdateMoveSpeed();
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSpeed10()
{
    ui->m_moveSpeed->setValue(10);
    OnUpdateMoveSpeed();
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedMuteAudio()
{
    gSettings.bMuteAudio = !gSettings.bMuteAudio;

    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, gSettings.bMuteAudio ? m_oMuteAudioRequest : m_oUnmuteAudioRequest);

    ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
}

void CInfoBar::OnBnClickedEnableVR()
{
    gSettings.bEnableGameModeVR = !gSettings.bEnableGameModeVR;
    ui->m_vrBtn->setChecked(gSettings.bEnableGameModeVR);
}

#include <InfoBar.moc>

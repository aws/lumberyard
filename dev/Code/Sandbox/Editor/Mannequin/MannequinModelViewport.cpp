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
#include "MannequinModelViewport.h"

#include "../../CryEngine/CryEntitySystem/Components/ComponentRender.h"
#include "../objects/EntityObject.h"
#include "Components/IComponentPhysics.h"
#include "ICryMannequin.h"
#include "IGameFramework.h"
#include "MannequinDialog.h"
#include "ThumbnailGenerator.h"
#include "FileTypeUtils.h"
#include "Material/MaterialManager.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QFileInfo>
#include <QMessageBox>

#include "QtUtil.h"
#include "QtUtilWin.h"

#define PROXYINDICATTION (0x4000)
#define MAX_ORBIT_DISTANCE (2000.0f)
extern uint32 g_ypos;

// Undo object stored when track is modified.
class CUndoAttachmentModifyObject
    : public IUndoObject
{
public:
    CUndoAttachmentModifyObject(int32 attachmentIdx, CMannequinModelViewport* pViewPort)
    {
        // Stores the current state of this track.
        assert(attachmentIdx != -1);

        m_AttachmentIdx = attachmentIdx;
        m_pViewPort = pViewPort;

        // Store undo info.
        m_undo.GetData(m_AttachmentIdx, m_pViewPort);
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Attachments Modify"; };
    virtual void Undo(bool bUndo)
    {
        if (!m_undo.m_ok)
        {
            return;
        }

        if (bUndo)
        {
            m_redo.GetData(m_AttachmentIdx, m_pViewPort);
        }
        // Undo state.
        m_undo.SetData(m_AttachmentIdx, m_pViewPort);
    }
    virtual void Redo()
    {
        if (!m_redo.m_ok)
        {
            return;
        }

        // Redo state.
        m_redo.SetData(m_AttachmentIdx, m_pViewPort);
    }

private:
    int32 m_AttachmentIdx;
    CMannequinModelViewport* m_pViewPort;

    struct TAttachmentModifyStep
    {
        TAttachmentModifyStep()
            : m_ok(false) {}
        void GetData(int32 attachmentIdx, CMannequinModelViewport* pViewPort)
        {
            IAttachmentManager* pAM = pViewPort->GetCharacterBase()->GetIAttachmentManager();
            const int32 numAttachments = pAM->GetAttachmentCount();
            if (attachmentIdx < numAttachments)
            {
                IAttachment* pAttachment = pAM->GetInterfaceByIndex(attachmentIdx);
                m_abs = pAttachment->GetAttAbsoluteDefault();
                m_rel = pAttachment->GetAttRelativeDefault();
                m_ok = true;
            }
        }
        void SetData(int32 attachmentIdx, CMannequinModelViewport* pViewPort)
        {
            IAttachmentManager* pAM = pViewPort->GetCharacterBase()->GetIAttachmentManager();
            const int32 numAttachments = pAM->GetAttachmentCount();
            if (attachmentIdx < numAttachments)
            {
                pViewPort->m_SelectedAttachment = attachmentIdx;

                pViewPort->m_ArcBall.sphere.center = m_abs.t;
                pViewPort->m_ArcBall.ObjectRotation = m_abs.q;

                IAttachment* pAttachment = pAM->GetInterfaceByIndex(attachmentIdx);
                pAttachment->SetAttAbsoluteDefault(m_abs);
                pAttachment->SetAttRelativeDefault(m_rel);
            }
        }
        QuatT m_abs;
        QuatT m_rel;
        bool m_ok;
    };

    TAttachmentModifyStep m_undo;
    TAttachmentModifyStep m_redo;
};

namespace
{
    QString GetUserOptionsRegKeyName(EMannequinEditorMode editorMode)
    {
        QString keyName("Settings\\Mannequin\\");
        switch (editorMode)
        {
        case eMEM_FragmentEditor:
            keyName += "FragmentEditor";
            break;
        case eMEM_Previewer:
            keyName += "Previewer";
            break;
        case eMEM_TransitionEditor:
            keyName += "TransitionEditor";
            break;
        }
        return keyName + "UserOptions";
    }
}

const float CMannequinModelViewport::s_maxTweenTime = 0.5f;

CMannequinModelViewport::CMannequinModelViewport(EMannequinEditorMode editorMode, QWidget* parent)
    : CModelViewport(GetUserOptionsRegKeyName(editorMode).toUtf8().data(), parent)
    , m_locMode(LM_Translate)
    , m_selectedLocator(0xffffffff)
    , m_viewmode(eVM_Unknown)
    , m_draggingLocator(false)
    , m_dragStartPoint(0, 0)
    , m_LeftButtonDown(false)
    , m_lookAtCamera(false)
    , m_showSceneRoots(false)
    , m_cameraKeyDown(false)
    , m_playbackMultiplier(1.0f)
    , m_tweenToFocusStart(ZERO)
    , m_tweenToFocusDelta(ZERO)
    , m_tweenToFocusTime(0.0f)
    , m_editorMode(editorMode)
    , m_pActionController(NULL)
    , m_piGroundPlanePhysicalEntity(NULL)
    , m_TickerMode(SEQTICK_INFRAMES)
    , m_attachCameraToEntity(NULL)
    , m_lastEntityPos(ZERO)
    , m_pHoverBaseObject(NULL)
{
    primitives::box box;
    box.center = Vec3(0.f, 0.f, -0.2f);
    box.size = Vec3(1000.f, 1000.f, 0.2f);
    box.Basis.SetIdentity();

    IGeometry* pPrimGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &box);
    phys_geometry* pGeom = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(pPrimGeom, 0);

    m_piGroundPlanePhysicalEntity = gEnv->pPhysicalWorld->CreatePhysicalEntity(PE_STATIC);

    pe_geomparams geomParams;
    m_piGroundPlanePhysicalEntity->AddGeometry(pGeom, &geomParams);

    pe_params_pos posParams;
    posParams.pos = Vec3(-50.f, -50.f, 0.f);
    posParams.q = IDENTITY;
    m_piGroundPlanePhysicalEntity->SetParams(&posParams);

    pPrimGeom->Release();

    gEnv->pParticleManager->AddEventListener(this);
    gEnv->pGame->GetIGameFramework()->GetMannequinInterface().AddMannequinGameListener(this);

    m_alternateCamera = GetCamera();

    // ModelViewportCE
    m_animEventPosition = false;
    m_bRenderStats = false;
    m_Button_MOVE = 0;
    m_Button_IK = 0;
    m_Button_ROTATE = 0;
    m_highlightedBoneID = -1;
    m_pDefaultMaterial = nullptr;
    m_MouseOnBoneID = -1;
    m_MouseOnAttachment = -1;
    m_MouseButtonL = false;
    m_MouseButtonR = false;
    m_SelectionUpdate = 0;
    m_SelectedAttachment = 0;

    m_ArcBall.InitArcBall();
}

CMannequinModelViewport::~CMannequinModelViewport()
{
    gEnv->pParticleManager->RemoveEventListener(this);
    m_piGroundPlanePhysicalEntity->Release();
    gEnv->pPhysicalWorld->RemoveEventClient(EventPhysPostStep::id, OnPostStepLogged, 1);
    gEnv->pGame->GetIGameFramework()->GetMannequinInterface().RemoveMannequinGameListener(this);
}

int CMannequinModelViewport::OnPostStepLogged(const EventPhys* pEvent)
{
    const EventPhysPostStep* pPostStep = static_cast<const EventPhysPostStep*>(pEvent);
    IEntity* piEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPostStep->pEntity);
    piEntity->SetPosRotScale(pPostStep->pos, pPostStep->q, Vec3(1.f, 1.f, 1.f));
    return 1;
}

bool CMannequinModelViewport::UseAnimationDrivenMotionForEntity(const IEntity* piEntity)
{
    static bool addedClientEvent = false;
    if (piEntity)
    {
        IPhysicalEntity* piPhysicalEntity = piEntity->GetPhysics();
        if (piPhysicalEntity && (piPhysicalEntity->GetType() == PE_ARTICULATED))
        {
            if (!addedClientEvent)
            {
                addedClientEvent = true;
                piPhysicalEntity->GetWorld()->AddEventClient(EventPhysPostStep::id, OnPostStepLogged, 1);
            }

            return false;
        }
    }

    addedClientEvent = false;
    return true;
}

void CMannequinModelViewport::Update()
{
    CModelViewport::Update();

    if (IsKeyDown(Qt::Key_E))
    {
        m_locMode = LM_Rotate;
    }
    if (IsKeyDown(Qt::Key_W))
    {
        m_locMode = LM_Translate;
    }
    if (IsKeyDown(Qt::Key_L))
    {
        m_cameraKeyDown = true;
    }
    else if (m_cameraKeyDown)
    {
        m_lookAtCamera = !m_lookAtCamera;
        m_cameraKeyDown = false;
    }
    if (IsKeyDown(Qt::Key_P))
    {
        AttachToEntity();
    }
    else if (IsKeyDown(Qt::Key_O))
    {
        DetachFromEntity();
    }

    if (m_tweenToFocusTime > 0.0f)
    {
        m_tweenToFocusTime = MAX(0.0f, m_tweenToFocusTime - gEnv->pTimer->GetFrameTime());
        float fProgress = (s_maxTweenTime - m_tweenToFocusTime) / s_maxTweenTime;
        m_Camera.SetPosition((m_tweenToFocusDelta * fProgress) + m_tweenToFocusStart);
    }
}

void CMannequinModelViewport::mousePressEvent(QMouseEvent* event)
{
    const auto& point = event->pos();

    CModelViewport::mousePressEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        CELButtonDown(point);

        m_HitContext.view = this;
        m_HitContext.b2DViewport = false;
        m_HitContext.point2d = point;
        ViewToWorldRay(point, m_HitContext.raySrc, m_HitContext.rayDir);
        m_HitContext.distanceTolerance = 0;
        m_LeftButtonDown = true;

        if (m_locMode == LM_Translate)
        {
            const uint32 numLocators = m_locators.size();
            for (uint32 i = 0; i < numLocators; ++i)
            {
                SLocator& locator = m_locators[i];

                Matrix34 m34 = GetLocatorWorldMatrix(locator);

                SetConstructionMatrix(COORDS_LOCAL, m34);

                if (locator.m_AxisHelper.HitTest(m34, GetIEditor()->GetGlobalGizmoParameters(), m_HitContext))
                {
                    m_draggingLocator = true;
                    m_selectedLocator = i;
                    m_dragStartPoint = point;
                }
            }
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        m_MouseButtonR = true;
    }
    else if (event->button() == Qt::MiddleButton)
    {
        CEMButtonDown(event, point);
    }
}

void CMannequinModelViewport::mouseReleaseEvent(QMouseEvent* event)
{
    const auto& point = event->pos();

    CModelViewport::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
    {
        CELButtonUp(point);

        if (!m_draggingLocator)
        {
            HitContext hc;
            hc.view = this;
            hc.point2d = point;
            hc.camera = &m_Camera;
            Vec3 raySrc(0, 0, 0), rayDir(1, 0, 0);
            ViewToWorldRay(point, hc.raySrc, hc.rayDir);
            HitTest(hc, true);
        }
        m_draggingLocator = false;
        m_LeftButtonDown = false;

        m_selectedLocator = 0xffffffff;
    }
    else if (event->button() == Qt::RightButton)
    {
        m_MouseButtonR = false;
    }
}

void CMannequinModelViewport::mouseMoveEvent(QMouseEvent* event)
{
    const auto& point = event->pos();

    CModelViewport::mouseMoveEvent(event);

    //calculate the Ray for camera-pos to mouse-cursor
    m_WinRect = rect();
    f32 RresX = (f32)(m_WinRect.width());
    f32 RresY = (f32)(m_WinRect.height());
    f32 MiddleX = RresX / 2.0f;
    f32 MiddleY = RresY / 2.0f;
    Vec3 MoursePos3D = Vec3(point.x() - MiddleX, m_Camera.GetEdgeP().y, -point.y() + MiddleY) * Matrix33(m_Camera.GetViewMatrix()) + m_Camera.GetPosition();
    Ray mray(m_Camera.GetPosition(), (MoursePos3D - m_Camera.GetPosition()).GetNormalized());

    if (m_MouseButtonR == false && GetCharacterBase() != NULL && m_SelectionUpdate)
    {
        m_SelectionUpdate = 0;
        ClosestPoint ct;

#ifdef EDITOR_PCDEBUGCODE
        if (GetCharacterBase()->GetResetMode() || m_Button_IK)
#else
        if (m_Button_IK)
#endif
        {
            //-----------------------------------------------------------------------
            //---            find closest point on base-mesh                 ---
            //-----------------------------------------------------------------------
            ct.m_nBaseModel = 0;
            ct.m_fDistance = Picking_BaseMesh(mray);
            if (ct.m_fDistance < 10000.0f)
            {
                ct.m_nBaseModel = 1;
            }

            //-----------------------------------------------------------------------
            //---            find closest point on arcball                        ---
            //-----------------------------------------------------------------------
            IAttachmentManager* pIAttachmentManager = GetCharacterBase()->GetIAttachmentManager();
            uint32 numAttachment = pIAttachmentManager->GetAttachmentCount();
            if (m_Button_ROTATE && numAttachment > 0)
            {
                Vec3 output;
                uint32 i = Intersect::Ray_SphereFirst(mray, m_ArcBall.sphere, output);
                if (i)
                {
                    f32 d = (output - m_Camera.GetPosition()).GetLength();
                    if (ct.m_fDistance > d)
                    {
                        ct.m_nBaseModel = 0;
                        ct.m_fDistance = d;
                    }
                    //ct.distance=0;
                    for (uint32 s = 0; s < numAttachment; s++)
                    {
                        IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
                        uint32 type = pIAttachment->GetType();
                        if (type == CA_BONE || type == CA_FACE)
                        {
                            if (pIAttachment->GetAttAbsoluteDefault().t == m_ArcBall.sphere.center)
                            {
                                ct.m_nAttachmentIdx = s;
                            }
                        }
                    } //for loop
                }
            }

            //----------------------------------------------------------------------
            //----------------------------------------------------------------------
            //----------------------------------------------------------------------

            SetCurrentCursor(STD_CURSOR_DEFAULT, QStringLiteral(""));

            if (numAttachment || m_animEventPosition  || m_Button_IK)
            {
                if (m_Button_ROTATE)
                {
                    m_ArcBall.ArcControl(Matrix34(IDENTITY), mray, m_MouseButtonL);
                }

                if (m_Button_MOVE || m_Button_IK || m_animEventPosition)
                {
                    Matrix34 m34 = Matrix34(Matrix33(m_ArcBall.ObjectRotation), m_ArcBall.sphere.center);
                    m_HitContext.view = this;
                    m_HitContext.b2DViewport = false;
                    m_HitContext.point2d = point;
                    m_HitContext.rayDir = mray.direction;
                    m_HitContext.raySrc = mray.origin;
                    ViewToWorldRay(point, m_HitContext.raySrc, m_HitContext.rayDir);
                    m_HitContext.distanceTolerance = 0;
                    if (m_AxisHelper.HitTest(m34, GetIEditor()->GetGlobalGizmoParameters(), m_HitContext))
                    {
                        if (m_Button_MOVE)
                        {
                            SetCurrentCursor(STD_CURSOR_MOVE, QStringLiteral("Attachment"));
                            ct.m_fDistance = 0;
                            for (uint32 s = 0; s < numAttachment; s++)
                            {
                                IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
                                uint32 type = pIAttachment->GetType();
                                if (type == CA_BONE || type == CA_FACE)
                                {
                                    if (pIAttachment->GetAttAbsoluteDefault().t == m_ArcBall.sphere.center)
                                    {
                                        ct.m_nAttachmentIdx = s;
                                    }
                                }
                            }
                        }
                        else if (m_Button_IK)
                        {
                            SetCurrentCursor(STD_CURSOR_MOVE, QStringLiteral("IK Target"));
                        }
                    }
                }
            }

            //-----------------------------------------------------------------------
            //---       loop over all attachments and do raycasting               ---
            //-----------------------------------------------------------------------
            m_MouseOnAttachment = -1;
            for (uint32 s = 0; s < numAttachment; s++)
            {
                IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
                Matrix34 m34 = Matrix34(pIAttachment->GetAttWorldAbsolute());
                f32 distance = Picking_AttachedMeshes(mray, pIAttachment, m34);
                if (ct.m_fDistance > distance)
                {
                    ct.m_nBaseModel =   0;
                    ct.m_fDistance = distance;
                    ct.m_nAttachmentIdx = s;
                }
            } //for loop
        }

        //-----------------------------------------------------------------------
        //---       loop over all joints and do raycasting                                      ---
        //-----------------------------------------------------------------------
        if (!m_Button_IK)
        {
            const uint32 nJointCount = GetCharacterBase()->GetIDefaultSkeleton().GetJointCount();
            f32 nearestDotP = FLT_MIN;
            f32 nearestDistance = FLT_MAX;
            static const float EPSILON = 0.0000000001f;
            for (uint32 boneID = 0; boneID < nJointCount; boneID++)
            {
                const QuatT m34 = GetCharacterBase()->GetISkeletonPose()->GetAbsJointByID(boneID);

                const Vec3 originTojoint = (m34.t - mray.origin);
                const f32 dp = mray.direction.dot(originTojoint.GetNormalized());
                const f32 distance = originTojoint.GetLength();

                // joints have no "volume" to intersect with
                if ((dp > 0.99f) && dp >= nearestDotP)
                {
                    //if( (fabs(dp-nearestDotP) <= EPSILON) && distance < nearestDistance )
                    {
                        nearestDistance = distance;
                        nearestDotP = dp;
                        ct.m_nJointIdx = boneID;
                        ct.m_fDistance = distance;
                        ct.m_nBaseModel = 0;
                    }
                }
            }

            m_ClosestPoint = ct;
            m_MouseOnAttachment = m_ClosestPoint.m_nAttachmentIdx;
            m_MouseOnBoneID = m_ClosestPoint.m_nJointIdx;
        }
        if (m_opMode == MoveMode)
        {
            Vec3 p1 = MapViewToCP(m_cMouseDownPos);
            Vec3 p2 = MapViewToCP(point);
            if (p1.IsZero() || p2.IsZero())
            {
                return;
            }
            Vec3 v = GetCPVector(p1, p2, GetAxisConstrain());
            m_cMouseDownPos = point;
            ;
            m_ArcBall.sphere.center += v;
        }
    }

    if (m_bInOrbitMode)
    {
        UpdateOrbitPosition();
    }

    if (m_bInMoveMode)
    {
        // End the focus tween if in progress and the middle mouse button has been pressed
        m_tweenToFocusTime = 0.0f;
    }

    if (m_locMode == LM_Rotate)
    {
        //calculate the Ray for camera-pos to mouse-cursor
        m_WinRect = rect();
        f32 RresX = (f32)(m_WinRect.width());
        f32 RresY = (f32)(m_WinRect.height());
        f32 MiddleX = RresX / 2.0f;
        f32 MiddleY = RresY / 2.0f;
        Vec3 MoursePos3D = Vec3(point.x() - MiddleX, m_Camera.GetEdgeP().y, -point.y() + MiddleY) * Matrix33(m_Camera.GetViewMatrix()) + m_Camera.GetPosition();
        Ray mray(m_Camera.GetPosition(), (MoursePos3D - m_Camera.GetPosition()).GetNormalized());

        const uint32 numLocators = m_locators.size();
        for (uint32 i = 0; i < numLocators; i++)
        {
            SLocator& locator = m_locators[i];

            if (locator.m_ArcBall.ArcControl(GetLocatorReferenceMatrix(locator), mray, m_LeftButtonDown))
            {
                Matrix34 m34 = GetLocatorWorldMatrix(locator);

                SetConstructionMatrix(COORDS_LOCAL, m34);

                CMannequinDialog::GetCurrentInstance()->OnMoveLocator(m_editorMode, locator.m_refID, locator.m_name.toUtf8().data(), QuatT(locator.m_ArcBall.ObjectRotation, locator.m_ArcBall.sphere.center));
            }
        }
    }
    else
    {
        if (m_draggingLocator && m_selectedLocator < m_locators.size())
        {
            SLocator& locator = m_locators[m_selectedLocator];

            int axis = locator.m_AxisHelper.GetHighlightAxis();

            if (m_locMode == LM_Translate)
            {
                Matrix34 constructionMatrix = GetLocatorWorldMatrix(locator);

                SetConstructionMatrix(COORDS_LOCAL, constructionMatrix);

                Vec3 p1 = MapViewToCP(m_dragStartPoint);
                Vec3 p2 = MapViewToCP(point, axis);
                if (!p1.IsZero() && !p2.IsZero())
                {
                    Vec3 v = GetCPVector(p1, p2, axis);

                    m_dragStartPoint = point;

                    Vec3 pos = constructionMatrix.GetTranslation() + v;

                    Matrix34 mInvert = GetLocatorReferenceMatrix(locator).GetInverted();
                    pos = mInvert * pos;
                    locator.m_ArcBall.sphere.center = pos;

                    CMannequinDialog::GetCurrentInstance()->OnMoveLocator(m_editorMode, locator.m_refID, locator.m_name.toUtf8().data(), QuatT(locator.m_ArcBall.ObjectRotation, locator.m_ArcBall.sphere.center));
                }
            }
        }
        else
        {
            PreWidgetRendering();
            HitContext hc;
            hc.view = this;
            hc.point2d = point;
            hc.camera = &m_Camera;
            Vec3 raySrc(0, 0, 0), rayDir(1, 0, 0);
            ViewToWorldRay(point, hc.raySrc, hc.rayDir);
            HitTest(hc, false);
            PostWidgetRendering();
        }
    }
}

void CMannequinModelViewport::keyPressEvent(QKeyEvent* event)
{
    // special case the escape key so that it doesn't bubble up to the main window;
    // don't want the escape key in Mannequin to affect the currently active tool in the main viewport/rollup bar
    if (event->key() != Qt::Key_Escape)
    {
        CModelViewport::keyPressEvent(event);
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

void CMannequinModelViewport::keyReleaseEvent(QKeyEvent* event)
{
    // we special case the key press event for escape, so special case the key release event as well,
    // so that we don't get the ModelViewport / RenderViewport code into a weird state,
    // getting a release key event that they never saw a press key event for.
    if (event->key() != Qt::Key_Escape)
    {
        CModelViewport::keyReleaseEvent(event);
    }
    else
    {
        QWidget::keyReleaseEvent(event);
    }
}

void CMannequinModelViewport::UpdatePropEntities(SMannequinContexts* pContexts, SMannequinContexts::SProp& prop)
{
    for (uint32 em = 0; em < eMEM_Max; em++)
    {
        const SMannequinContextViewData::TBackgroundObjects& backgroundObjects = pContexts->viewData[em].backgroundObjects;
        const uint32 numBGObjects = backgroundObjects.size();
        for (uint32 i = 0; i < numBGObjects; i++)
        {
            const CBaseObject* pBaseObject = backgroundObjects[i];
            if (qobject_cast<const CEntityObject*>(pBaseObject))
            {
                if (prop.entity == pBaseObject->GetName())
                {
                    CEntityObject* pEntObject = (CEntityObject*)pBaseObject;
                    prop.entityID[em] = pEntObject->GetIEntity()->GetId();
                    break;
                }
            }
        }
    }
}

bool CMannequinModelViewport::HitTest(HitContext& hc, const bool bIsClick)
{
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    SMannequinContextViewData::TBackgroundObjects& backgroundObjects = pContexts->viewData[m_editorMode].backgroundObjects;
    const uint32 numBGObjects = backgroundObjects.size();
    float minDist = FLT_MAX;
    CBaseObject* pBestObject = NULL;

    for (uint32 i = 0; i < numBGObjects; i++)
    {
        CBaseObject* pBaseObject = backgroundObjects[i];
        if (qobject_cast<CEntityObject*>(pBaseObject))
        {
            if (pBaseObject->HitTest(hc))
            {
                if (hc.dist < minDist)
                {
                    minDist = hc.dist;
                    pBestObject = pBaseObject;
                }
            }
        }
    }

    if (pBestObject)
    {
        if (bIsClick)
        {
            QString initialProp = "";
            for (uint32 i = 0; i < pContexts->backgroundProps.size(); i++)
            {
                SMannequinContexts::SProp& prop = pContexts->backgroundProps[i];
                if (prop.entity == pBestObject->GetName())
                {
                    initialProp = prop.name;
                    break;
                }
            }

            QString pickerTitle = tr("Set Prop Name for %1").arg(pBestObject->GetName());
            QString m_loadedFile;

            QString newName = QInputDialog::getText(this, tr("Property Name"), pickerTitle, QLineEdit::Normal, initialProp);
            if (!newName.isNull())
            {
                bool inserted = newName.isEmpty();

                for (uint32 i = 0; i < pContexts->backgroundProps.size(); i++)
                {
                    SMannequinContexts::SProp& prop = pContexts->backgroundProps[i];
                    if (prop.entity == pBestObject->GetName())
                    {
                        if (inserted)
                        {
                            pContexts->backgroundProps.erase(pContexts->backgroundProps.begin() + i);
                            break;
                        }
                        else
                        {
                            prop.name = newName;
                            inserted = true;
                        }
                    }
                    else if (prop.name == newName)
                    {
                        if (inserted)
                        {
                            pContexts->backgroundProps.erase(pContexts->backgroundProps.begin() + i);
                            break;
                        }
                        else
                        {
                            prop.entity = pBestObject->GetName();
                            inserted = true;
                            UpdatePropEntities(pContexts, prop);
                        }
                    }
                }

                if (!inserted)
                {
                    SMannequinContexts::SProp prop;
                    prop.entity = pBestObject->GetName();
                    prop.name = newName;
                    UpdatePropEntities(pContexts, prop);
                    pContexts->backgroundProps.push_back(prop);
                }

                pMannequinDialog->ResavePreviewFile();
            }
        }
    }

    m_pHoverBaseObject = pBestObject;

    return false;
}

void CMannequinModelViewport::OnRender()
{
    if (CMannequinDialog::GetCurrentInstance())
    {
        CMannequinDialog::GetCurrentInstance()->OnRender();
    }

    IRenderer* renderer = GetIEditor()->GetRenderer();
    IRenderAuxGeom* pAuxGeom = renderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags previousFlags = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
    renderFlags.SetFillMode(e_FillModeSolid);
    pAuxGeom->SetRenderFlags(renderFlags);

    int mode = (m_locMode == LM_Translate) ? CAxisHelper::MOVE_MODE : CAxisHelper::ROTATE_MODE;
    const uint32 numLocators = m_locators.size();
    for (uint32 i = 0; i < numLocators; i++)
    {
        SLocator& locator = m_locators[i];

        if (locator.m_pEntity != NULL)
        {
            pAuxGeom->DrawSphere(locator.m_pEntity->GetWorldTM().GetTranslation(), 0.1f, ColorB());
        }

        if (locator.m_AxisHelper.GetHighlightAxis() == 0)
        {
            locator.m_AxisHelper.SetHighlightAxis(GetAxisConstrain());
        }

        locator.m_AxisHelper.SetMode(mode);

        if (m_locMode == LM_Rotate)
        {
            locator.m_ArcBall.DrawSphere(GetLocatorReferenceMatrix(locator), GetCamera(), pAuxGeom);
        }
        else
        {
            Matrix34 m34 = GetLocatorWorldMatrix(locator);

            if (GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD)
            {
                m34.SetRotation33(IDENTITY);
            }

            Vec3 groundProj(m34.GetTranslation());
            groundProj.z = 0.0f;
            pAuxGeom->DrawLine(m34.GetTranslation(), RGBA8(0x00, 0x00, 0xff, 0x00), groundProj, RGBA8(0xff, 0x00, 0x00, 0x00));
            pAuxGeom->DrawLine(groundProj - Vec3(0.2f, 0.0f, 0.0f), RGBA8(0xff, 0x00, 0x00, 0x00), groundProj + Vec3(0.2f, 0.0f, 0.0f), RGBA8(0xff, 0x00, 0x00, 0x00));
            pAuxGeom->DrawLine(groundProj - Vec3(0.0f, 0.2f, 0.0f), RGBA8(0xff, 0x00, 0x00, 0x00), groundProj + Vec3(0.0f, 0.2f, 0.0f), RGBA8(0xff, 0x00, 0x00, 0x00));

            locator.m_AxisHelper.DrawAxis(m34, GetIEditor()->GetGlobalGizmoParameters(), m_displayContext);
        }
    }

    if (mv_AttachCamera)
    {
        int screenWidth = max(renderer->GetWidth(), 1);
        int screenHeight = max(renderer->GetHeight(), 1);
        static bool showCrosshair = true;
        static bool showSafeZone = true;
        const float targetRatio = 16.0f / 9.0f;
        static float crossHairLen = 25.0f;

        const ColorB crosshairColour = ColorB(192, 192, 192);
        const ColorB edgeColour = ColorB(192, 192, 192, 60);

        renderFlags.SetDepthTestFlag(e_DepthTestOff);
        renderFlags.SetMode2D3DFlag(e_Mode2D);
        pAuxGeom->SetRenderFlags(renderFlags);

        if (showCrosshair)
        {
            float adjustedHeight = crossHairLen / (float)screenHeight;
            float adjustedWidth = crossHairLen / (float)screenWidth;

            pAuxGeom->DrawLine(
                Vec3(0.5f - adjustedWidth, 0.5f, 0.0f), crosshairColour,
                Vec3(0.5f + adjustedWidth, 0.5f, 0.0f), crosshairColour);
            pAuxGeom->DrawLine(
                Vec3(0.5f, 0.5f - adjustedHeight, 0.0f), crosshairColour,
                Vec3(0.5f, 0.5f + adjustedHeight, 0.0f), crosshairColour);
        }

        if (showSafeZone)
        {
            float targetX = (float)screenHeight * targetRatio;
            float edgeWidth = (float)screenWidth - targetX;
            if (edgeWidth > 0.0f)
            {
                float edgePercent = (edgeWidth / (float)screenWidth) / 2.0f;

                float innerX = edgePercent;
                float upperX = 1.0f - edgePercent;
                pAuxGeom->DrawLine(
                    Vec3(innerX, 0.0f, 0.0f), crosshairColour,
                    Vec3(innerX, 1.0f, 0.0f), crosshairColour);
                pAuxGeom->DrawLine(
                    Vec3(upperX, 0.0f, 0.0f), crosshairColour,
                    Vec3(upperX, 1.0f, 0.0f), crosshairColour);

                renderFlags.SetAlphaBlendMode(e_AlphaBlended);
                pAuxGeom->SetRenderFlags(renderFlags);

                pAuxGeom->DrawTriangle(Vec3(0.0f, 1.0f, 0.0f), edgeColour, Vec3(innerX, 1.0f, 0.0f), edgeColour, Vec3(0.0f, 0.0f, 0.0f), edgeColour);
                pAuxGeom->DrawTriangle(Vec3(innerX, 1.0f, 0.0f), edgeColour, Vec3(innerX, 0.0f, 0.0f), edgeColour, Vec3(0.0f, 0.0f, 0.0f), edgeColour);

                pAuxGeom->DrawTriangle(Vec3(upperX, 1.0f, 0.0f), edgeColour, Vec3(1.0f, 1.0f, 0.0f), edgeColour, Vec3(upperX, 0.0f, 0.0f), edgeColour);
                pAuxGeom->DrawTriangle(Vec3(upperX, 0.0f, 0.0f), edgeColour, Vec3(1.0f, 1.0f, 0.0f), edgeColour, Vec3(1.0f, 0.0f, 0.0f), edgeColour);
            }
        }
    }
    pAuxGeom->SetRenderFlags(previousFlags);

    float x = (float)gEnv->pRenderer->GetWidth();
    float y = (float)gEnv->pRenderer->GetHeight();
    x -= 5.0f;

    if (m_TickerMode == SEQTICK_INFRAMES)
    {
        gEnv->p3DEngine->DrawTextRightAligned(x, y - 20, "Timeline: Frames");
    }
    else
    {
        gEnv->p3DEngine->DrawTextRightAligned(x, y - 20, "Timeline: Seconds");
    }

    int ypos = y - 20;
    if (m_lookAtCamera)
    {
        ypos -= 20;
        gEnv->p3DEngine->DrawTextRightAligned(x, ypos, "Looking at camera");
    }
    if (m_attachCameraToEntity)
    {
        ypos -= 20;
        gEnv->p3DEngine->DrawTextRightAligned(x, ypos, "Tracking %s", m_attachCameraToEntity->GetName());
    }


    if (m_pHoverBaseObject)
    {
        const char* currentPropName = "Click to Assign Prop";
        Vec3 pos = m_pHoverBaseObject->GetPos();
        if (qobject_cast<CEntityObject*>(m_pHoverBaseObject))
        {
            CEntityObject* pEntObject = (CEntityObject*)m_pHoverBaseObject;
            if (pEntObject->GetIEntity())
            {
                pos = pEntObject->GetIEntity()->GetPos();

                CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
                SMannequinContexts* pContexts = pMannequinDialog->Contexts();
                const uint32 numProps = pContexts->backgroundProps.size();
                for (uint32 i = 0; i < numProps; i++)
                {
                    if (pContexts->backgroundProps[i].entityID[m_editorMode] == pEntObject->GetEntityId())
                    {
                        currentPropName = pContexts->backgroundProps[i].name.toUtf8().data();
                        break;
                    }
                }

                ColorB boxCol(0, 255, 255, 255);
                AABB bbox;
                pEntObject->GetLocalBounds(bbox);
                pAuxGeom->DrawAABB(bbox, pEntObject->GetIEntity()->GetWorldTM(), false, boxCol, eBBD_Faceted);
            }
        }

        float colour[] = {0.0f, 1.0f, 1.0f, 1.0f};
        renderer->DrawLabelEx(pos, 1.5f, colour, true, true, "%s\n%s", m_pHoverBaseObject->GetName(), currentPropName);
    }

    UpdateAnimation(m_playbackMultiplier * gEnv->pTimer->GetFrameTime());

    if (m_AxisHelper.GetHighlightAxis() == 0)
    {
        m_AxisHelper.SetHighlightAxis(GetAxisConstrain());
    }

    if (GetCharacterBase())
    {
        if (mv_showJointsValues)
        {
            QString bonename;
            int32 idx = GetCharacterBase()->GetIDefaultSkeleton().GetJointIDByName(bonename.toUtf8().data());

            if (idx >= 0)
            {
                uint32 ypos = 600;
                float color1[4] = {1, 1, 1, 1};
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "bonename: %s %d", bonename, idx);
                ypos += 10;

                QuatT absQuat = QuatT(m_PhysicalLocation * GetCharacterBase()->GetISkeletonPose()->GetAbsJointByID(idx));
                QuatT relQuat = GetCharacterBase()->GetISkeletonPose()->GetRelJointByID(idx);

                Vec3 WPos           =   absQuat.t;
                Vec3 RPos           = relQuat.t;
                Vec3 AxisX      =   absQuat.GetColumn0();
                Vec3 AxisY      =   absQuat.GetColumn1();
                Vec3 AxisZ      =   absQuat.GetColumn2();

                SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
                pAuxGeom->SetRenderFlags(renderFlags);
                pAuxGeom->DrawLine(WPos, RGBA8(0xff, 0x00, 0x00, 0x00), WPos + AxisX * 29.0f, RGBA8(0x00, 0x00, 0x00, 0x00));
                pAuxGeom->DrawLine(WPos, RGBA8(0x00, 0xff, 0x00, 0x00), WPos + AxisY * 29.0f, RGBA8(0x00, 0x00, 0x00, 0x00));
                pAuxGeom->DrawLine(WPos, RGBA8(0x00, 0x00, 0xff, 0x00), WPos + AxisZ * 29.0f, RGBA8(0x00, 0x00, 0x00, 0x00));

                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "absQuat: %15.10f %15.10f %15.10f %15.10f  --- %15.10f %15.10f %15.10f", absQuat.q.w, absQuat.q.v.x, absQuat.q.v.y, absQuat.q.v.z,  absQuat.t.x, absQuat.t.y, absQuat.t.z);
                ypos += 10;
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "relQuat: %15.10f %15.10f %15.10f %15.10f  --- %15.10f %15.10f %15.10f", relQuat.q.w, relQuat.q.v.x, relQuat.q.v.y, relQuat.q.v.z,  relQuat.t.x, relQuat.t.y, relQuat.t.z);
                ypos += 10;
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "WPos: %15.10f %15.10f %15.10f", WPos.x, WPos.y, WPos.z);
                ypos += 10;
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "RPos: %15.10f %15.10f %15.10f", RPos.x, RPos.y, RPos.z);
                ypos += 10;

                //  DrawArrowAim(QuatT(abs34),abs34.GetColumn1(),1.5f,RGBA8(0xff,0xff,0x00,0x00) ); // Replaced by DrawArrowArc
            }
        }

        if (mv_showJointNames)
        {
            const IDefaultSkeleton& rIDefaultSkeleton = GetCharacterBase()->GetIDefaultSkeleton();
            ISkeletonPose* pSkeletonPose = GetCharacterBase()->GetISkeletonPose();
            if (pSkeletonPose)
            {
                uint32 numJoints = rIDefaultSkeleton.GetJointCount();
                for (int j = 0; j < numJoints; j++)
                {
                    const char* pJointName = rIDefaultSkeleton.GetJointNameByID(j);
                    QuatT jointTM = QuatT(m_PhysicalLocation * pSkeletonPose->GetAbsJointByID(j));
                    if (renderer)
                    {
                        renderer->DrawLabel(jointTM.t, 1, "%s", pJointName);
                    }
                }
            }
        }

        //-------------------------------------------------------------------------------------
        //-------------------------------------------------------------------------------------
        //-------------------------------------------------------------------------------------

        if (m_Button_IK)
        {
            DrawMoveTool(pAuxGeom);
        }

#ifdef EDITOR_PCDEBUGCODE
        ICharacterInstance* charBase = GetCharacterBase();
        uint32 resetMode = 0;
        if (charBase)
        {
            resetMode = charBase->GetResetMode();
        }
#endif 

        if (resetMode)
        {
            //-----------------------------------------------------------------------
            //---                    resolve selection                            ---
            //-----------------------------------------------------------------------
            IAttachmentManager* pIAttachmentManager = GetCharacterBase()->GetIAttachmentManager();
            uint32 numAttachment = pIAttachmentManager->GetAttachmentCount();
            if (numAttachment == 0)
            {
                m_SelectedAttachment = -1;
            }
            if (numAttachment == 1)
            {
                m_SelectedAttachment = 0;
            }

            m_MouseOnBoneID = -1;


            //-------------------------------------------------------------------------
            //--- loop over all attachments and draw a box for the empty sockets    ---
            //-------------------------------------------------------------------------
            for (uint32 s = 0; s < numAttachment; s++)
            {
                IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);

                IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
                if (pIAttachmentObject == 0)
                {
                    Matrix34 m34 = Matrix34(pIAttachment->GetAttWorldAbsolute());
                    AABB caabb = AABB(Vec3(-0.01f, -0.01f, -0.01f), Vec3(+0.01f, +0.01f, +0.01f));
                    OBB obb = OBB::CreateOBBfromAABB(Matrix33(m34), caabb);
                    pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
                    pAuxGeom->DrawOBB(obb, m34.GetTranslation(), 0, RGBA8(0x00, 0x00, 0xff, 0xff), eBBD_Extremes_Color_Encoded);
                }
            } //for loop


            //-----------------------------------------------------------------------
            // draw wireframe over closest selection
            //-----------------------------------------------------------------------
            uint32 ypos = 90;
            float color1[4] = {1, 1, 1, 1};
            renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "m_ClosestPoint.m_nAttachmentIdx: %d  m_ClosestPoint.distance: %f", m_ClosestPoint.m_nAttachmentIdx, m_ClosestPoint.m_fDistance);
            ypos += 10;

            if (m_ClosestPoint.m_nAttachmentIdx != -1)
            {
                IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(m_ClosestPoint.m_nAttachmentIdx);

                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "Attachment: %s", pIAttachment->GetName());
                ypos += 10;

                IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();

                if (pIAttachmentObject)
                {
                    QuatTS wpos = pIAttachment->GetAttWorldAbsolute();
                    Matrix34 m34 = Matrix34(wpos);

                    IStatObj* pIStaticObject = pIAttachmentObject->GetIStatObj();
                    if (pIStaticObject)
                    {
                        IRenderMesh* pRenderMesh = pIStaticObject->GetRenderMesh();
                        if (pRenderMesh)
                        {
                            IRenderMesh::ThreadAccessLock lock(pRenderMesh);

                            uint32 NumVertices = pRenderMesh->GetVerticesCount();

                            int numIndices = pRenderMesh->GetIndicesCount();
                            vtx_idx* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
                            int PosStride = 0;
                            byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

                            static Vec3 LineVertices[0x5000];
                            for (uint32 i = 0; i < NumVertices; i++)
                            {
                                Vec3 v  = *(Vec3*)pVertices_strided;
                                LineVertices[i]     = m34 * v;
                                pVertices_strided += PosStride;
                            }
                            SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
                            renderFlags.SetFillMode(e_FillModeWireframe);
                            renderFlags.SetDrawInFrontMode(e_DrawInFrontOn);
                            renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
                            pAuxGeom->SetRenderFlags(renderFlags);
                            pAuxGeom->DrawTriangles(LineVertices, NumVertices, pIndices, numIndices, RGBA8(0x00, 0x1f, 0x00, 0x00));
                        }
                    }
                    else
                    {
                        //maybe object is an attached character
                        ICharacterInstance* pICharacterInstance = pIAttachmentObject->GetICharacterInstance();
                        if (pICharacterInstance)
                        {
                            pICharacterInstance->DrawWireframeStatic(m34, 0, RGBA8(0x00, 0x1f, 0x00, 0x00));
                        }
                        IAttachmentSkin* pIAttachmentSkin = pIAttachmentObject->GetIAttachmentSkin();
                        if (pIAttachmentSkin)
                        {
                            pIAttachmentSkin->DrawWireframeStatic(m34, 0, RGBA8(0x00, 0x1f, 0x00, 0x00));
                        }
                    }
                }
            }
            else
            {
                if (m_ClosestPoint.m_nBaseModel)
                {
                    f32 fUniformScale = GetCharacterBase()->GetUniformScale();
                    //uint32 ypos = 300;
                    //float color1[4] = {1,1,1,1};
                    //renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"base scale: %15.10f",fUniformScale);
                    //ypos+=10;
                    GetCharacterBase()->DrawWireframeStatic(Matrix34(IDENTITY) * fUniformScale, 0, RGBA8(0x1f, 0x00, 0x00, 0x00));
                }
            }


            //-----------------------------------------------------------------------
            //---            use arcball to re-orient attachment                  ---
            //-----------------------------------------------------------------------
            if (numAttachment)
            {
                if (m_SelectedAttachment != -1)
                {
                    if (m_SelectedAttachment & PROXYINDICATTION)
                    {
                        IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByIndex(m_SelectedAttachment & (PROXYINDICATTION - 1));
                        if (pIProxy)
                        {
                            Quat WRot = m_ArcBall.DragRotation * m_ArcBall.ObjectRotation;
                            Vec3 WPos = m_ArcBall.sphere.center;
                            pIProxy->SetProxyAbsoluteDefault(QuatT(WRot, WPos));

                            uint32 ypos = 300;
                            float color1[4] = {1, 1, 1, 1};
                            renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "Proxy rotation: %15.10f (%15.10f %15.10f %15.10f)", WRot.w, WRot.v.x, WRot.v.y, WRot.v.z);
                            ypos += 10;
                            renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "Proxy position: %15.10f %15.10f %15.10f", WPos.x, WPos.y, WPos.z);
                            ypos += 10;

                            pIProxy->ProjectProxy();

                            if (m_Button_ROTATE)
                            {
                                m_ArcBall.DrawSphere(Matrix34(IDENTITY), GetCamera(), pAuxGeom);
                            }
                            else if (m_Button_MOVE || m_Button_IK)
                            {
                                DrawMoveTool(pAuxGeom);
                            }
                        }
                    }
                    else
                    {
                        IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(m_SelectedAttachment);
                        if (pIAttachment)
                        {
                            uint32 type = pIAttachment->GetType();
                            if (type == CA_BONE || type == CA_FACE)
                            {
                                Quat WRot = m_ArcBall.DragRotation * m_ArcBall.ObjectRotation;
                                Vec3 WPos = m_ArcBall.sphere.center;
                                pIAttachment->SetAttAbsoluteDefault(QuatT(WRot, WPos));
                                if (m_Button_ROTATE)
                                {
                                    m_ArcBall.DrawSphere(Matrix34(IDENTITY), GetCamera(), pAuxGeom);
                                }
                                else if (m_Button_MOVE || m_Button_IK)
                                {
                                    DrawMoveTool(pAuxGeom);
                                }
                            }
                        }
                    }
                }
            }

            if (m_animEventPosition)
            {
                SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
                renderFlags.SetFillMode(e_FillModeSolid);
                pAuxGeom->SetRenderFlags(renderFlags);
                Matrix34 m34 = Matrix34(Matrix33(m_ArcBall.DragRotation * m_ArcBall.ObjectRotation), m_ArcBall.sphere.center);
                m_AxisHelper.DrawAxis(m34, GetIEditor()->GetGlobalGizmoParameters(), m_displayContext);
            }
        } //if resetmode
        else
        {
            //-----------------------------------------------------------------------
            // draw wireframe highlight at closest bone
            //-----------------------------------------------------------------------
            uint32 ypos = 50;
            float color1[4] = {1, 1, 1, 1};
            renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "m_ClosestPoint.m_nJointIdx: %d  m_ClosestPoint.distance: %f", m_ClosestPoint.m_nJointIdx, m_ClosestPoint.m_fDistance), ypos += 10;
            if (m_SelectedAttachment & PROXYINDICATTION)
            {
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "m_SelectedProxy: %d", m_SelectedAttachment & (PROXYINDICATTION - 1)), ypos += 10;
            }
            else
            {
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "m_SelectedAttachment: %d", m_SelectedAttachment), ypos += 10;
            }

            const uint32 nJointCount = GetCharacterBase()->GetIDefaultSkeleton().GetJointCount();
            // display the currently selected bone/joint
            if (m_highlightedBoneID < nJointCount)
            {
                renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "Selected Bone: %s", GetCharacterBase()->GetIDefaultSkeleton().GetJointNameByID(m_highlightedBoneID));
                ypos += 10;

                Matrix34 m34 = Matrix34(GetCharacterBase()->GetISkeletonPose()->GetAbsJointByID(m_highlightedBoneID));

                AABB caabb = AABB(Vec3(-0.01f, -0.01f, -0.01f), Vec3(+0.01f, +0.01f, +0.01f));
                OBB obb = OBB::CreateOBBfromAABB(Matrix33(m34), caabb);
                SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
                renderFlags.SetFillMode(e_FillModeSolid);
                pAuxGeom->SetRenderFlags(renderFlags);
                m_displayContext.DepthTestOff();
                pAuxGeom->DrawOBB(obb, m34.GetTranslation(), 0, RGBA8(0xff, 0x00, 0x00, 0xff), eBBD_Faceted);
                m_displayContext.DepthTestOn();
            }

            if (m_ClosestPoint.m_nJointIdx != 0xffffffff)
            {
                // display the nearest bone/joint to the mouse
                if (m_ClosestPoint.m_nJointIdx < nJointCount)
                {
                    renderer->Draw2dLabel(12, ypos, 1.2f, color1, false, "Mouse Joint/Bone: %s", GetCharacterBase()->GetIDefaultSkeleton().GetJointNameByID(m_ClosestPoint.m_nJointIdx));
                    ypos += 10;

                    Matrix34 m34 = Matrix34(GetCharacterBase()->GetISkeletonPose()->GetAbsJointByID(m_ClosestPoint.m_nJointIdx));

                    AABB caabb = AABB(Vec3(-0.01f, -0.01f, -0.01f), Vec3(+0.01f, +0.01f, +0.01f));
                    OBB obb = OBB::CreateOBBfromAABB(Matrix33(m34), caabb);
                    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
                    renderFlags.SetFillMode(e_FillModeSolid);
                    pAuxGeom->SetRenderFlags(renderFlags);
                    m_displayContext.DepthTestOff();
                    pAuxGeom->DrawOBB(obb, m34.GetTranslation(), 0, RGBA8(0x00, 0xff, 0x00, 0xff), eBBD_Faceted);
                    m_displayContext.DepthTestOn();

                    Vec3 offsetPos = m34.GetTranslation();

                    Matrix34 m = GetViewTM();
                    Vec3 xdir = m.GetColumn0().GetNormalized();
                    offsetPos   =   offsetPos   +   (0.1f * xdir);

                    renderer->DrawLabel(offsetPos, 1.2f, "%s", GetCharacterBase()->GetIDefaultSkeleton().GetJointNameByID(m_ClosestPoint.m_nJointIdx));
                }
            }
        }
    } //if character

    m_SelectionUpdate = 1;
    CModelViewport::OnRender();
}

void CMannequinModelViewport::SetTimelineUnits(ESequencerTickMode mode)
{
    m_TickerMode = mode;
}

void CMannequinModelViewport::Focus(AABB& pBoundingBox)
{
    // Parent class handles focusing on bones...if about to happen, abort
    if (m_highlightedBoneID >= 0)
    {
        return;
    }

    // If currently panning the view, then abort
    if (m_bInMoveMode)
    {
        return;
    }

    // Don't want to move the first person camera
    if (IsCameraAttached())
    {
        ToggleCamera();
    }

    float fRadius = pBoundingBox.GetRadius();
    float fFov = m_Camera.GetFov();
    float fDistance = fRadius / (tan(fFov / 2));
    Vec3 center = pBoundingBox.GetCenter();
    Vec3 viewDir = m_Camera.GetViewdir();

    m_tweenToFocusStart = m_Camera.GetPosition();
    m_tweenToFocusDelta = (center + (-(viewDir.normalized()) * fDistance)) - m_tweenToFocusStart;
    m_tweenToFocusTime = s_maxTweenTime;
}

void CMannequinModelViewport::ClearLocators()
{
    m_locators.resize(0);
}

void CMannequinModelViewport::AddLocator(uint32 refID, const char* name, const QuatT& transform, IEntity* pEntity, int16 refJointId, IAttachment* pAttachment, uint32 paramCRC, string helperName)
{
    const uint32 numLocators = m_locators.size();
    SLocator* pLocator = NULL;
    for (uint32 i = 0; i < numLocators; i++)
    {
        SLocator& locator = m_locators[i];
        if (locator.m_refID == refID)
        {
            pLocator = &locator;
            break;
        }
    }

    if (pLocator == NULL)
    {
        m_locators.resize(numLocators + 1);
        pLocator = &m_locators[numLocators];
        pLocator->m_name = QString::fromLatin1(name);
        pLocator->m_refID = refID;
        pLocator->m_pEntity = pEntity;
        pLocator->m_jointId = refJointId;
        pLocator->m_pAttachment = pAttachment;
        pLocator->m_paramCRC = paramCRC;
        pLocator->m_helperName = helperName;
        pLocator->m_ArcBall.InitArcBall();
    }

    pLocator->m_ArcBall.DragRotation.SetIdentity();
    pLocator->m_ArcBall.ObjectRotation = transform.q;
    pLocator->m_ArcBall.sphere.center = transform.t;
}

Matrix34 CMannequinModelViewport::GetLocatorReferenceMatrix(const SLocator& locator)
{
    if (locator.m_pAttachment != NULL)
    {
        if (!locator.m_helperName.empty())
        {
            IAttachmentObject* pAttachObject = locator.m_pAttachment->GetIAttachmentObject();
            IStatObj* pStatObj = pAttachObject ? pAttachObject->GetIStatObj() : NULL;
            if (!pStatObj && pAttachObject && (pAttachObject->GetAttachmentType() == IAttachmentObject::eAttachment_Entity))
            {
                CEntityAttachment* pEntAttachment = (CEntityAttachment*)pAttachObject;
                IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pEntAttachment->GetEntityId());
                if (pEntity)
                {
                    pStatObj = pEntity->GetStatObj(0);
                }
            }
            if (pStatObj)
            {
                return Matrix34(locator.m_pAttachment->GetAttWorldAbsolute()) * pStatObj->GetHelperTM(locator.m_helperName);
            }
        }
        return Matrix34(locator.m_pAttachment->GetAttWorldAbsolute());
    }
    else if (locator.m_jointId > -1 && locator.m_pEntity != NULL)
    {
        ICharacterInstance* pCharInstance = locator.m_pEntity->GetCharacter(0);
        CRY_ASSERT(pCharInstance);

        ISkeletonPose& skeletonPose = *pCharInstance->GetISkeletonPose();

        Matrix34 world = Matrix34(locator.m_pEntity->GetWorldTM());
        Matrix34 joint = Matrix34(skeletonPose.GetAbsJointByID(locator.m_jointId));

        return world * joint;
    }
    else if (m_pActionController && locator.m_paramCRC != 0)
    {
        if (!locator.m_helperName.empty())
        {
            EntityId attachEntityId;
            m_pActionController->GetParam(locator.m_paramCRC, attachEntityId);

            IEntity* pEntity = gEnv->pEntitySystem->GetEntity(attachEntityId);
            if (pEntity)
            {
                IStatObj* pStatObj = pEntity->GetStatObj(0);

                return pEntity->GetWorldTM() * pStatObj->GetHelperTM(locator.m_helperName);
                ;
            }
        }

        QuatT location;
        if (m_pActionController->GetParam(locator.m_paramCRC, location))
        {
            return Matrix34(location);
        }
        else
        {
            return Matrix34(IDENTITY);
        }
    }
    else
    {
        return Matrix34(IDENTITY);
    }
}

Matrix34 CMannequinModelViewport::GetLocatorWorldMatrix(const SLocator& locator)
{
    return GetLocatorReferenceMatrix(locator) * Matrix34(Matrix33(locator.m_ArcBall.DragRotation * locator.m_ArcBall.ObjectRotation), locator.m_ArcBall.sphere.center);
}

void CMannequinModelViewport::UpdateCharacter(IEntity* pEntity, ICharacterInstance* pInstance, float deltaTime)
{
    ISkeletonAnim& skeletonAnimation = *pInstance->GetISkeletonAnim();
    ISkeletonPose& skeletonPose = *pInstance->GetISkeletonPose();

    pInstance->SetCharEditMode(CA_CharacterTool);
    skeletonPose.SetForceSkeletonUpdate(1);

    const bool useAnimationDrivenMotion = UseAnimationDrivenMotionForEntity(pEntity);
    skeletonAnimation.SetAnimationDrivenMotion(useAnimationDrivenMotion);

    QuatT physicalLocation(pEntity->GetRotation(), pEntity->GetPos());

    const CCamera& camera = GetCamera();
    float fDistance = (camera.GetPosition() - physicalLocation.t).GetLength();
    float fZoomFactor = 0.001f + 0.999f * (RAD2DEG(camera.GetFov()) / 60.f);

    SAnimationProcessParams params;
    params.locationAnimation = physicalLocation;
    params.bOnRender = 0;
    params.zoomAdjustedDistanceFromCamera = fDistance * fZoomFactor;
    params.overrideDeltaTime = deltaTime;
    pInstance->StartAnimationProcessing(params);
    QuatT relMove = skeletonAnimation.GetRelMovement();
    pInstance->FinishAnimationComputations();

    if (mv_InPlaceMovement)
    {
        Matrix34 m = GetViewTM();
        m.SetTranslation(m.GetTranslation() - m_PhysicalLocation.t);
        SetViewTM(m);
        relMove.t.zero();
        m_PhysicalLocation.t.zero();
    }

    physicalLocation = physicalLocation * relMove;
    physicalLocation.q.Normalize();

    pEntity->SetRotation(physicalLocation.q);
    pEntity->SetPos(physicalLocation.t);

    if (pEntity == m_attachCameraToEntity)
    {
        const Vec3 movement = pEntity->GetPos() - m_lastEntityPos;
        m_lastEntityPos = pEntity->GetPos();
        Matrix34 viewTM = GetViewTM();
        viewTM.SetTranslation(viewTM.GetTranslation() + movement);
        SetViewTM(viewTM);
    }
}

void CMannequinModelViewport::UpdateAnimation(float timePassed)
{
    gEnv->pGame->GetIGameFramework()->GetMannequinInterface().SetSilentPlaybackMode(m_bPaused);
    if (m_pActionController)
    {
        m_pActionController->Update(timePassed);
    }
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const uint32 numContextDatums = pContexts->m_contextData.size();
    for (uint32 i = 0; i < numContextDatums; i++)
    {
        SScopeContextData& contextData = pContexts->m_contextData[i];
        if (contextData.viewData[m_editorMode].enabled && contextData.viewData[m_editorMode].m_pActionController)
        {
            contextData.viewData[m_editorMode].m_pActionController->Update(timePassed);
        }
    }

    uint32 numChars = m_entityList.size();

    //--- Update the inputs after any animation installations
    if (numChars > 0)
    {
        IEntity* pEntity = m_entityList[0].GetEntity();
        if (pEntity != NULL)
        {
            ICharacterInstance* pInstance = pEntity->GetCharacter(0);
            if (pInstance)
            {
                UpdateInput(pInstance, timePassed, m_pActionController);
            }
        }
    }

    for (uint32 i = 0; i < numChars; i++)
    {
        IEntity* pEntity = m_entityList[i].GetEntity();
        if (pEntity && !pEntity->IsHidden())
        {
            ICharacterInstance* pInstance = pEntity->GetCharacter(0);
            if (pInstance)
            {
                UpdateCharacter(pEntity, pInstance, timePassed);

                IAttachmentManager* pIAttachmentManager = pInstance->GetIAttachmentManager();
                if (pIAttachmentManager && (i == 0))
                {
                    IAttachment*  pIAttachment = pIAttachmentManager->GetInterfaceByName("#camera");
                    if (pIAttachment)
                    {
                        const QuatT physicalLocation(pEntity->GetRotation(), pEntity->GetPos());
                        QuatTS qt = physicalLocation * pIAttachment->GetAttModelRelative();
                        if (mv_AttachCamera)
                        {
                            SetViewTM(Matrix34(qt));
                            SetFirstperson(pIAttachmentManager, eVM_FirstPerson);
                        }
                        else
                        {
                            //--- TODO: Add in an option to render this!
                            //IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
                            //pAuxGeom->DrawCone(qt.t, qt.q.GetColumn1(), 0.15f, 0.5f, RGBA8(0x00,0xff,0x00,0x60));

                            SetFirstperson(pIAttachmentManager, eVM_ThirdPerson);
                        }
                    }
                }

                if (m_lookAtCamera && !mv_AttachCamera)
                {
                    IAnimationPoseBlenderDir* pAim = pInstance->GetISkeletonPose()->GetIPoseBlenderAim();

                    const Vec3 lookTarget = GetViewTM().GetTranslation();
                    if (pAim)
                    {
                        pAim->SetTarget(lookTarget);
                    }

                    QuatT lookTargetQT(Quat(IDENTITY), lookTarget);
                    if (m_pActionController)
                    {
                        m_pActionController->SetParam("AimTarget", lookTargetQT);
                        m_pActionController->SetParam("LookTarget", lookTargetQT);
                    }

                    for (uint32 i = 0; i < numContextDatums; i++)
                    {
                        SScopeContextData& contextData = pContexts->m_contextData[i];
                        if (contextData.viewData[m_editorMode].enabled && contextData.viewData[m_editorMode].m_pActionController)
                        {
                            contextData.viewData[m_editorMode].m_pActionController->SetParam("AimTarget", lookTargetQT);
                            contextData.viewData[m_editorMode].m_pActionController->SetParam("LookTarget", lookTargetQT);
                        }
                    }
                }
            }
        }
    }
    gEnv->pGame->GetIGameFramework()->GetMannequinInterface().SetSilentPlaybackMode(false);
}

void CMannequinModelViewport::SetFirstperson(IAttachmentManager* pAttachmentManager, EMannequinViewMode viewmode)
{
    if (viewmode != m_viewmode)
    {
        m_viewmode = viewmode;
        bool isFirstPerson = viewmode == eVM_FirstPerson;
        int numAttachments = pAttachmentManager->GetAttachmentCount();

        for (int i = 0; i < numAttachments; i++)
        {
            if (IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(i))
            {
                const char* pAttachmentName = pAttachment->GetName();

                if (strstr(pAttachmentName, "_tp") != 0)
                {
                    pAttachment->HideAttachment(isFirstPerson);
                }
                else if (strstr(pAttachmentName, "_fp") != 0)
                {
                    pAttachment->HideAttachment(!isFirstPerson);
                }
            }
        }
    }
}

void CMannequinModelViewport::DrawEntityAndChildren(CEntityObject* pEntityObject, const SRendParams& rp, const SRenderingPassInfo& passInfo)
{
    IEntity* pEntity = pEntityObject->GetIEntity();
    if (pEntity)
    {
        if (IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>())
        {
            if (IRenderNode* pRenderNode = pRenderComponent->GetRenderNode())
            {
                pRenderNode->Render(rp, passInfo);
            }
        }

        const int childEntityCount = pEntity->GetChildCount();
        for (int idx = 0; idx < childEntityCount; ++idx)
        {
            if (IEntity* pChild = pEntity->GetChild(idx))
            {
                if (IComponentRenderPtr pRenderComponent = pChild->GetComponent<IComponentRender>())
                {
                    if (IRenderNode* pRenderNode = pRenderComponent->GetRenderNode())
                    {
                        pRenderNode->Render(rp, passInfo);
                    }
                }
            }
        }
    }

    const int childObjectCount = pEntityObject->GetChildCount();
    for (int idx = 0; idx < childObjectCount; ++idx)
    {
        if (CBaseObject* pObjectChild = pEntityObject->GetChild(idx))
        {
            if (qobject_cast<CEntityObject*>(pObjectChild))
            {
                DrawEntityAndChildren((CEntityObject*)pObjectChild, rp, passInfo);
            }

            IRenderNode* pRenderNode = pObjectChild->GetEngineNode();
            if (pRenderNode)
            {
                pRenderNode->Render(rp, passInfo);
            }
        }
    }
}

void CMannequinModelViewport::DrawCharacter(ICharacterInstance* pInstance, const SRendParams& rRP, const SRenderingPassInfo& passInfo)
{
    f32 FrameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
    m_AverageFrameTime = pInstance->GetAverageFrameTime();

    if (mv_showGrid)
    {
        DrawGrid(Quat(IDENTITY), Vec3(ZERO), Vec3(ZERO), m_GroundOBB.m33);
    }

    GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_DrawLocator")->Set(mv_showLocator);
    GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_DrawSkeleton")->Set(mv_showSkeleton);

    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    SMannequinContextViewData::TBackgroundObjects& backgroundObjects = pContexts->viewData[m_editorMode].backgroundObjects;
    const uint32 numBGObjects = backgroundObjects.size();
    for (uint32 i = 0; i < numBGObjects; i++)
    {
        CBaseObject* pBaseObject = backgroundObjects[i];
        if (qobject_cast<CEntityObject*>(pBaseObject))
        {
            CEntityObject* pEntObject = (CEntityObject*)pBaseObject;
            DrawEntityAndChildren(pEntObject, rRP, passInfo);
        }
        else
        {
            IRenderNode* pRenderNode = pBaseObject->GetEngineNode();
            if (pRenderNode)
            {
                pRenderNode->Render(rRP, passInfo);
            }
        }
    }

    const uint32 numChars = m_entityList.size();
    for (uint32 i = 0; i < numChars; i++)
    {
        IEntity* pEntity = m_entityList[i].GetEntity();
        if (pEntity && !pEntity->IsHidden())
        {
            ICharacterInstance* charInst = pEntity->GetCharacter(0);
            IStatObj* statObj = pEntity->GetStatObj(0);
            if (charInst || statObj)
            {
                DrawCharacter(pEntity, charInst, statObj, rRP, passInfo);
            }
        }
    }

    if (m_particleEmitters.empty() == false)
    {
        for (std::vector<IParticleEmitter*>::iterator itEmitters = m_particleEmitters.begin(); itEmitters != m_particleEmitters.end(); )
        {
            IParticleEmitter* pEmitter = *itEmitters;
            if (pEmitter->IsAlive())
            {
                pEmitter->Update();
                pEmitter->Render(rRP, passInfo);
                ++itEmitters;
            }
            else
            {
                itEmitters = m_particleEmitters.erase(itEmitters);
            }
        }
    }
}

void CMannequinModelViewport::OnCreateEmitter(IParticleEmitter* pEmitter, QuatTS const& qLoc, const IParticleEffect* pEffect, uint32 uEmitterFlags)
{
}

void CMannequinModelViewport::OnDeleteEmitter(IParticleEmitter* pEmitter)
{
    m_particleEmitters.erase(std::remove(m_particleEmitters.begin(), m_particleEmitters.end(), pEmitter), m_particleEmitters.end());
}

void CMannequinModelViewport::OnSpawnParticleEmitter(IParticleEmitter* pEmitter, IActionController& actionController)
{
    if (&actionController == m_pActionController)
    {
        pEmitter->SetEmitterFlags(pEmitter->GetEmitterFlags() | ePEF_Nowhere);
        m_particleEmitters.push_back(pEmitter);
    }
}

void CMannequinModelViewport::DrawCharacter(IEntity* pEntity, ICharacterInstance* pInstance, IStatObj* pStatObj, const SRendParams& rRP, const SRenderingPassInfo& passInfo)
{
    const QuatT physicalLocation(pEntity->GetRotation(), pEntity->GetPos());

    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
    pAuxGeom->SetRenderFlags(renderFlags);

    if (mv_showBase)
    {
        DrawCoordSystem(QuatT(physicalLocation), 7.0f);
    }

    Matrix34 localEntityMat = Matrix34(physicalLocation);
    SRendParams rp = rRP;
    rp.pRenderNode = pEntity->GetComponent<IComponentRender>()->GetRenderNode();
    assert(rp.pRenderNode != NULL);
    rp.pMatrix = &localEntityMat;
    rp.pPrevMatrix = &localEntityMat;
    rp.fDistance = (physicalLocation.t - m_Camera.GetPosition()).GetLength();

    if (pInstance)
    {
        gEnv->p3DEngine->PrecacheCharacter(NULL, 1.f, pInstance, pInstance->GetIMaterial(), localEntityMat, 0, 1.f, 4, true, passInfo);
        pInstance->Render(rp, QuatTS(IDENTITY), passInfo);

        if (m_showSceneRoots)
        {
            int numAnims = pInstance->GetISkeletonAnim()->GetNumAnimsInFIFO(0);
            if (numAnims > 0)
            {
                CAnimation& anim = pInstance->GetISkeletonAnim()->GetAnimFromFIFO(0, numAnims - 1);
                QuatT startLocation;
                if (pInstance->GetIAnimationSet()->GetAnimationDCCWorldSpaceLocation(&anim, startLocation, pInstance->GetIDefaultSkeleton().GetControllerIDByID(0)))
                {
                    static float AXIS_LENGTH = 0.75f;
                    static float AXIS_RADIUS = 0.01f;
                    static float AXIS_CONE_RADIUS = 0.02f;
                    static float AXIS_CONE_LENGTH = 0.1f;
                    QuatT invStart = startLocation.GetInverted();
                    QuatT location = physicalLocation * invStart;
                    Vec3 xDir = location.q * Vec3(1.0f, 0.0f, 0.0f);
                    Vec3 yDir = location.q * Vec3(0.0f, 1.0f, 0.0f);
                    Vec3 zDir = location.q * Vec3(0.0f, 0.0f, 1.0f);
                    Vec3 pos = location.t;
                    pAuxGeom->DrawCylinder(pos + (xDir * (AXIS_LENGTH * 0.5f)), xDir, AXIS_RADIUS, AXIS_LENGTH, ColorB(255, 0, 0));
                    pAuxGeom->DrawCylinder(pos + (yDir * (AXIS_LENGTH * 0.5f)), yDir, AXIS_RADIUS, AXIS_LENGTH, ColorB(0, 255, 0));
                    pAuxGeom->DrawCylinder(pos + (zDir * (AXIS_LENGTH * 0.5f)), zDir, AXIS_RADIUS, AXIS_LENGTH, ColorB(0, 0, 255));
                    pAuxGeom->DrawCone(pos + (xDir * AXIS_LENGTH), xDir, AXIS_CONE_RADIUS, AXIS_CONE_LENGTH, ColorB(255, 0, 0));
                    pAuxGeom->DrawCone(pos + (yDir * AXIS_LENGTH), yDir, AXIS_CONE_RADIUS, AXIS_CONE_LENGTH, ColorB(0, 255, 0));
                    pAuxGeom->DrawCone(pos + (zDir * AXIS_LENGTH), zDir, AXIS_CONE_RADIUS, AXIS_CONE_LENGTH, ColorB(0, 0, 255));
                }
            }
        }
    }
    else
    {
        pStatObj->Render(rp, passInfo);
    }


    //-------------------------------------------------
    //---      draw path of the past
    //-------------------------------------------------
    Matrix33 m33 = Matrix33(physicalLocation.q);
    Matrix34 m34 = Matrix34(physicalLocation);


    uint32 numEntries = m_arrAnimatedCharacterPath.size();
    for (int32 i = (numEntries - 2); i > -1; i--)
    {
        m_arrAnimatedCharacterPath[i + 1] = m_arrAnimatedCharacterPath[i];
    }
    m_arrAnimatedCharacterPath[0] = physicalLocation.t;


    Vec3 axis = physicalLocation.q.GetColumn0();
    Matrix33 SlopeMat33 = m_GroundOBB.m33;
    // [MichaelS 13/2/2007] Stop breaking the facial editor! Don't assume m_pCharPanel_Animation is valid! You (and I) know who you are!

    //gdc
    for (uint32 i = 0; i < numEntries; i++)
    {
        AABB aabb;
        aabb.min = Vec3(-0.01f, -0.01f, -0.01f) + SlopeMat33 * (m_arrAnimatedCharacterPath[i]);
        aabb.max = Vec3(+0.01f, +0.01f, +0.01f) + SlopeMat33 * (m_arrAnimatedCharacterPath[i]);
        pAuxGeom->DrawAABB(aabb, 1, RGBA8(0x00, 0x00, 0xff, 0x00), eBBD_Extremes_Color_Encoded);
    }

    if (pInstance)
    {
        ISkeletonAnim& skeletonAnim = *pInstance->GetISkeletonAnim();
        Vec3 vCurrentLocalMoveDirection = skeletonAnim.GetCurrentVelocity(); //.GetNormalizedSafe( Vec3(0,1,0) );;
        Vec3 CurrentVelocity = m_PhysicalLocation.q * skeletonAnim.GetCurrentVelocity();
        if (mv_printDebugText)
        {
            float color1[4] = {1, 1, 1, 1};
            m_renderer->Draw2dLabel(12, g_ypos, 1.2f, color1, false, "CurrTravelDirection: %f %f", CurrentVelocity.x, CurrentVelocity.y);
            g_ypos += 10;
            m_renderer->Draw2dLabel(12, g_ypos, 1.2f, color1, false, "vCurrentLocalMoveDirection: %f %f", vCurrentLocalMoveDirection.x, vCurrentLocalMoveDirection.y);
            g_ypos += 10;

            //draw the diagonal lines;
            //  pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3( 1, 1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
            //  pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3(-1, 1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
            //  pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3( 1,-1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
            //  pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3(-1,-1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
        }
    }
}

void CMannequinModelViewport::OnScrubTime(float timePassed)
{
}

void CMannequinModelViewport::OnSequenceRestart(float timePassed)
{
    OnScrubTime(timePassed);
    m_GridOrigin = Vec3(ZERO);
    m_PhysicalLocation.SetIdentity();
    SetPlayerPos();

    ResetLockedValues();

    const uint32 numChars = m_entityList.size();
    for (uint32 i = 0; i < numChars; i++)
    {
        IEntity* pEntity = m_entityList[i].GetEntity();
        if (pEntity)
        {
            pEntity->SetPosRotScale(m_entityList[i].startLocation.t, m_entityList[i].startLocation.q, Vec3(1.0f, 1.0f, 1.0f));
            ICharacterInstance* charInst = pEntity->GetCharacter(0);
            if (charInst)
            {
                if (pEntity->GetPhysics())
                {
                    // Physicalizing with default params destroys the physical entity in the physical proxy.
                    SEntityPhysicalizeParams params;
                    pEntity->Physicalize(params);
                }

                charInst->GetISkeletonPose()->DestroyCharacterPhysics();
                charInst->GetISkeletonPose()->SetDefaultPose();

                SAnimationProcessParams params;
                params.locationAnimation = m_entityList[i].startLocation;
                params.bOnRender = 0;
                params.zoomAdjustedDistanceFromCamera = 0.0f;
                charInst->StartAnimationProcessing(params);
                charInst->FinishAnimationComputations();
            }
        }
    }

    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();

    //--- Flush ActionControllers
    if (m_pActionController)
    {
        m_pActionController->Flush();
    }
    const uint32 numContextDatums = pContexts->m_contextData.size();
    for (uint32 i = 0; i < numContextDatums; i++)
    {
        SScopeContextData& contextData = pContexts->m_contextData[i];
        SScopeContextViewData& viewData = contextData.viewData[m_editorMode];
        if (viewData.enabled && viewData.m_pActionController)
        {
            viewData.m_pActionController->Flush();

            //--- Temporary workaround for visualising lookposes in the editor
            if (contextData.database)
            {
                FragmentID fragID = contextData.pControllerDef->m_fragmentIDs.Find("LookPose");
                if (fragID != FRAGMENT_ID_INVALID)
                {
                    TAction<SAnimationContext>* pAction = new TAction<SAnimationContext>(-1, fragID, TAG_STATE_EMPTY, IAction::Interruptable);
                    viewData.m_pActionController->Queue(*pAction);
                }
            }
        }
    }

    //--- Reset background objects
    SMannequinContextViewData::TBackgroundObjects& backgroundObjects = pContexts->viewData[m_editorMode].backgroundObjects;
    const uint32 numBGObjects = backgroundObjects.size();
    for (uint32 i = 0; i < numBGObjects; i++)
    {
        CBaseObject* pBaseObject = backgroundObjects[i];
        if (qobject_cast<CEntityObject*>(pBaseObject))
        {
            CEntityObject* pEntObject = (CEntityObject*)pBaseObject;
            Vec3 oldPos = pEntObject->GetPos();
            pEntObject->SetPos(Vec3(ZERO));
            pEntObject->SetPos(oldPos);
        }
    }
}

void CMannequinModelViewport::UpdateDebugParams()
{
    if (!m_pActionController)
    {
        return;
    }

    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();

    const uint32 numContextDatums = pContexts->m_contextData.size();
    for (uint32 i = 0; i < numContextDatums; i++)
    {
        const SScopeContextData& contextData = pContexts->m_contextData[i];
        const SScopeContextViewData& contextDataViewInfo = contextData.viewData[m_editorMode];
        if (contextDataViewInfo.enabled && contextDataViewInfo.entity)
        {
            if (contextData.contextID != CONTEXT_DATA_NONE)
            {
                m_pActionController->SetParam(pContexts->m_controllerDef->m_scopeContexts.GetTagName(contextData.contextID), contextDataViewInfo.entity->GetId());
            }
            else
            {
                m_pActionController->SetParam(contextData.name.toLocal8Bit().data(), contextDataViewInfo.entity->GetId());
            }
        }
    }
    const uint32 numProps = pContexts->backgroundProps.size();
    for (uint32 i = 0; i < numProps; i++)
    {
        m_pActionController->SetParam(pContexts->backgroundProps[i].name.toUtf8().data(), pContexts->backgroundProps[i].entityID[m_editorMode]);
    }
}

void CMannequinModelViewport::DrawGrid(const Quat& tmRotation, const Vec3& MotionTranslation, const Vec3& FootSlide, const Matrix33& rGridRot)
{
    CModelViewport::DrawFloorGrid(tmRotation, MotionTranslation, rGridRot);
}

void CMannequinModelViewport::LoadObject(const QString& fileName, float)
{
    GetIEditor()->FlushUndo(false); // need to clear out any CUndo operations which might affect this character

    QWaitCursor waitCursor;
    const ICVar* pShowErrorDialogOnLoad = gEnv->pConsole->GetCVar("ed_showErrorDialogOnLoad");
    CErrorsRecorder errorsRecorder(pShowErrorDialogOnLoad && (pShowErrorDialogOnLoad->GetIVal() != 0));

    m_bPaused = false;

    // Load object.
    QString file = fileName;

    bool reload = false;
    if (m_loadedFile == file)
    {
        reload = true;
    }
    m_loadedFile = file;

    m_pPhysicalEntity = NULL;

    SetName(tr("Model View - %1").arg(file));
    ReleaseObject();

    // Initialize these with some values
    m_AABB.min = Vec3(-1.0f, -1.0f, -1.0f);
    m_AABB.max = Vec3(1.0f, 1.0f, 1.0f);

    //assert(m_pCharPanel_Animation);
    if (IsPreviewableFileType(file.toUtf8().data()))
    {
        m_pCharacterBase = 0;
        //  m_pAnimationSystem->ClearResources(0);
        const QString fileExt = QFileInfo(file).completeSuffix();

        bool IsCGA = (0 == fileExt.compare("cga", Qt::CaseInsensitive));
        if (IsCGA)
        {
            CLogFile::WriteLine("Loading CGA Model...");
            m_pCharacterBase = m_pAnimationSystem->CreateInstance(file.toUtf8().data(), CA_CharEditModel);
        }

        bool IsCDF = (0 == fileExt.compare("cdf", Qt::CaseInsensitive));
        if (IsCDF)
        {
            CLogFile::WriteLine("Importing Character Definitions...");
            m_pCharacterBase = m_pAnimationSystem->CreateInstance(file.toUtf8().data(), CA_CharEditModel);
            if (m_pCharacterBase)
            {
                m_pAnimationSystem->CreateDebugInstances(file.toUtf8().data());
            }

            /*      for (uint32 i=0; i<NUM_INSTANCE; i++)
                            arrCharacterBase[i] = m_pAnimationSystem->CreateInstance( file ); */
        }

        bool IsSKEL = (0 == fileExt.compare(CRY_SKEL_FILE_EXT, Qt::CaseInsensitive));
        bool IsSKIN = (0 == fileExt.compare(CRY_SKIN_FILE_EXT, Qt::CaseInsensitive));
        if (IsSKEL || IsSKIN)
        {
            CLogFile::WriteLine("Loading Character Model...");
            m_pCharacterBase = m_pAnimationSystem->CreateInstance(file.toUtf8().data(), CA_CharEditModel);
            if (m_pCharacterBase)
            {
                m_pAnimationSystem->CreateDebugInstances(file.toUtf8().data());
            }
        }

        //-------------------------------------------------------

        if (GetCharacterBase())
        {
            CThumbnailGenerator thumbGen;
            thumbGen.GenerateForFile(file);
            m_PhysicalLocation.SetIdentity();

            f32  radius = GetCharacterBase()->GetAABB().GetRadius();        //m_pCompoundCharacter->GetRadius();
            Vec3 center = GetCharacterBase()->GetAABB().GetCenter();        //m_pCompoundCharacter->GetCenter();

            m_AABB.min = center - Vec3(radius, radius, radius);
            m_AABB.max = center + Vec3(radius, radius, radius);
            if (!reload)
            {
                m_camRadius = center.z + radius;
            }

            m_Button_MOVE = 0;
            m_Button_ROTATE = 0;
            m_Button_IK = 0;
        }
        else if (!IsSKEL && !IsSKIN && !IsCGA && !IsCDF)
        {
            LoadStaticObject(file);
        }
    }
    else
    {
        QMessageBox::critical(this, tr("Preview Error"), tr("Preview of this file type not supported"));
        return;
    }

    //--------------------------------------------------------------------------------

    //  m_objectAngles(0,0,0);
    m_camRadius = (m_AABB.max - m_AABB.min).GetLength();

    Matrix34 VMat;
    VMat.SetRotationZ(gf_PI);
    VMat.SetTranslation(Vec3(0, m_camRadius, (m_AABB.max.z + m_AABB.min.z) / 2));
    SetViewTM(VMat);

    Physicalize();

    //------------------------------------------------------------------------------
    // Save default material

    _smart_ptr<IMaterial> pMtl = 0;
    if (m_object)
    {
        pMtl = m_object->GetMaterial();
    }
    else if (GetCharacterBase())
    {
        pMtl = GetCharacterBase()->GetIMaterial();
    }

    m_pDefaultMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pMtl);

    if (!m_groundAlignment)
    {
        ::CryCreateClassInstance<IAnimationGroundAlignment>("AnimationPoseModifier_GroundAlignment", m_groundAlignment);
    }

    m_pAnimationSystem->ClearCDFCache();
}

f32 CMannequinModelViewport::Picking_BaseMesh(const Ray& mray)
{
    f32 distance = 9999999.0f;

    IRenderer* renderer = GetIEditor()->GetRenderer();
    ICharacterInstance* pICharacterInstance = GetCharacterBase();
    if (pICharacterInstance == 0)
    {
        return distance;
    }

    IRenderMesh* pRenderMesh = pICharacterInstance->GetIDefaultSkeleton().GetIRenderMesh();
    if (pRenderMesh == 0)
    {
        return distance;
    }

    IRenderMesh::ThreadAccessLock lock(pRenderMesh);
    uint32 NumVertices = pRenderMesh->GetVerticesCount();

    int pIndicesCount = pRenderMesh->GetIndicesCount();
    vtx_idx* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
    int PosStride = 0;
    byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

    static Vec3 LineVertices[0x10000];
    f32 fUniformScale = pICharacterInstance->GetUniformScale();

    for (uint32 i = 0; i < NumVertices; i++)
    {
        Vec3 v = *(Vec3*)pVertices_strided;
        LineVertices[i] = v * fUniformScale;
        pVertices_strided += PosStride;
    }

    for (uint32 i = 0; i < pIndicesCount; i = i + 3)
    {
        uint32 i0 = pIndices[i + 0];
        uint32 i1 = pIndices[i + 1];
        uint32 i2 = pIndices[i + 2];
        Vec3 output;
        bool t = Intersect::Ray_Triangle(mray, LineVertices[i1], LineVertices[i0], LineVertices[i2], output);
        if (t)
        {
            f32 d = (output - m_Camera.GetPosition()).GetLength();
            if (distance > d)
            {
                distance = d;
            }
        }
    }

    return distance;
}

f32 CMannequinModelViewport::Picking_AttachedMeshes(const Ray& mray, IAttachment* pIAttachment, const Matrix34& m34)
{
    f32 distance = 9999999.0f;
    Vec3 output(ZERO);
    IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
    if (pIAttachmentObject)
    {
        IAttachmentSkin* pIAttachmentSkin = pIAttachmentObject->GetIAttachmentSkin();
        AABB caabb = pIAttachmentObject->GetAABB();
        OBB obb = OBB::CreateOBBfromAABB(Matrix33(m34), caabb);

        uint32 intersection = Intersect::Ray_OBB(mray, m34.GetTranslation(), obb, output);
        if (intersection || pIAttachmentSkin)
        {
            IStatObj* pIStaticObject = pIAttachmentObject->GetIStatObj();
            if (pIStaticObject)
            {
                IRenderMesh* pRenderMesh = pIStaticObject->GetRenderMesh();
                if (pRenderMesh)
                {
                    IRenderMesh::ThreadAccessLock lock(pRenderMesh);

                    uint32 NumVertices = pRenderMesh->GetVerticesCount();
                    int pIndicesCount = pRenderMesh->GetIndicesCount();
                    vtx_idx* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
                    int PosStride = 0;
                    byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

                    static Vec3 LineVertices[0x5000];
                    for (uint32 i = 0; i < NumVertices; i++)
                    {
                        Vec3  v = *(Vec3*)pVertices_strided;
                        LineVertices[i] = m34 * v;
                        pVertices_strided += PosStride;
                    }

                    for (uint32 i = 0; i < pIndicesCount; i = i + 3)
                    {
                        uint32 i0 = pIndices[i + 0];
                        uint32 i1 = pIndices[i + 1];
                        uint32 i2 = pIndices[i + 2];
                        Vec3 output;
                        bool t = Intersect::Ray_Triangle(mray, LineVertices[i1], LineVertices[i0], LineVertices[i2], output);
                        if (t)
                        {
                            f32 d = (output - m_Camera.GetPosition()).GetLength();
                            if (distance > d)
                            {
                                distance = d;
                            }
                        }
                    }
                }
            }
            else
            {
                //check if SKEL or SKIN
                ICharacterInstance* pICharacterInstance = pIAttachmentObject->GetICharacterInstance();
                IAttachmentSkin* pISkinInstance = pIAttachmentObject->GetIAttachmentSkin();
                if (pICharacterInstance == 0 && pISkinInstance == 0)
                {
                    return distance;
                }

                Vec3 vRenderOffset = Vec3(ZERO);
                IRenderMesh* pRenderMesh = 0;
                if (pISkinInstance)
                {
                    pRenderMesh = pISkinInstance->GetISkin()->GetIRenderMesh(0);
                    vRenderOffset = pISkinInstance->GetISkin()->GetRenderMeshOffset(0);
                }
                if (pICharacterInstance)
                {
                    pRenderMesh = pICharacterInstance->GetIDefaultSkeleton().GetIRenderMesh();
                    vRenderOffset = pICharacterInstance->GetIDefaultSkeleton().GetRenderMeshOffset();
                }

                if (pRenderMesh == 0)
                {
                    return distance;
                }

                IRenderMesh::ThreadAccessLock lock(pRenderMesh);
                uint32 NumVertices = pRenderMesh->GetVerticesCount();

                int pIndicesCount = pRenderMesh->GetIndicesCount();
                vtx_idx* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
                int PosStride = 0;
                byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);


                Vec3* pLineVertices = (Vec3*)alloca(NumVertices * sizeof(Vec3));
                for (uint32 i = 0; i < NumVertices; i++)
                {
                    Vec3 v = *(Vec3*)pVertices_strided;
                    pLineVertices[i] = m34 * (v + vRenderOffset);
                    pVertices_strided += PosStride;
                }

                for (uint32 i = 0; i < pIndicesCount; i = i + 3)
                {
                    uint32 i0 = pIndices[i + 0];
                    uint32 i1 = pIndices[i + 1];
                    uint32 i2 = pIndices[i + 2];
                    Vec3 output;
                    bool t = Intersect::Ray_Triangle(mray, pLineVertices[i1], pLineVertices[i0], pLineVertices[i2], output);
                    if (t)
                    {
                        f32 d = (output - m_Camera.GetPosition()).GetLength();
                        if (distance > d)
                        {
                            distance = d;
                        }
                    }
                }
            }
        }    //if mouse on obb
    }
    else
    {
        //picking of empty attachments
        AABB caabb = AABB(Vec3(-0.01f, -0.01f, -0.01f), Vec3(+0.01f, +0.01f, +0.01f));
        OBB obb = OBB::CreateOBBfromAABB(Matrix33(m34), caabb);
        uint32 intersection = Intersect::Ray_OBB(mray, m34.GetTranslation(), obb, output);
        if (intersection)
        {
            distance = (output - m_Camera.GetPosition()).GetLength();
        }
    }

    return distance;
}

bool CMannequinModelViewport::UpdateOrbitPosition()
{
    ICharacterInstance* pCharacter = GetCharacterBase();
    if (pCharacter)
    {
        if (m_highlightedBoneID >= 0)
        {
            const uint32 nJointCount = pCharacter->GetIDefaultSkeleton().GetJointCount();
            if (m_highlightedBoneID < nJointCount)
            {
                const QuatT m34 = pCharacter->GetISkeletonPose()->GetAbsJointByID(m_highlightedBoneID);
                m_orbitTarget = m34.t;
                return true;
            }
        }
    }
    return false;
}

/// Helper method that draws move tool (3 axis)
void CMannequinModelViewport::DrawMoveTool(IRenderAuxGeom* pAuxGeom)
{
    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
    renderFlags.SetFillMode(e_FillModeSolid);
    pAuxGeom->SetRenderFlags(renderFlags);
    Matrix34 m34 = Matrix34(Matrix33(m_ArcBall.DragRotation * m_ArcBall.ObjectRotation), m_ArcBall.sphere.center);
    m_AxisHelper.DrawAxis(m34, GetIEditor()->GetGlobalGizmoParameters(), m_displayContext);
}

void CMannequinModelViewport::CELButtonDown(QPoint point)
{
    ICharacterInstance* pCharacter = GetCharacterBase();
    if (pCharacter)
    {
        if (pCharacter->GetResetMode())
        {
            IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

            m_opMode = SelectMode;
            m_MouseButtonL = true;
            m_cMouseDownPos = point;

            if (m_Button_MOVE || m_animEventPosition)
            {
                if (RepositionMoveTool(point))
                {
                    return;
                }
            }

            Matrix34 m34 = Matrix34(Matrix33(m_ArcBall.ObjectRotation), m_ArcBall.sphere.center);
            SetConstructionMatrix(COORDS_LOCAL, m34);
        }
        if (m_Button_IK)
        {
            m_opMode = MoveMode;
            m_MouseButtonL = true;
            m_cMouseDownPos = point;

            if (RepositionMoveTool(point))
            {
                return;
            }
        }

        if (m_MouseOnBoneID >= 0)
        {
            m_highlightedBoneID = m_MouseOnBoneID;
        }
        else
        {
            m_highlightedBoneID = -1;
        }
    }
}

void CMannequinModelViewport::CELButtonUp(QPoint point)
{
    if (CUndo::IsRecording())
    {
        GetIEditor()->AcceptUndo("Moved Attachment");
    }

    m_MouseButtonL = false;
    m_opMode = SelectMode;
}

void CMannequinModelViewport::CEMButtonDown(QMouseEvent* event, QPoint point)
{
    if (GetIEditor()->IsInGameMode())
    {
        return;
    }

    if (!(event->modifiers() & Qt::ControlModifier) && !(event->modifiers() & Qt::ShiftModifier))
    {
        bool bAlt = ((event->modifiers() & Qt::AltModifier));
        if (bAlt)
        {
            m_bInOrbitMode = true;

            bool needOrbitDistance = true;
            ICharacterInstance* pCharacter = GetCharacterBase();
            if (pCharacter)
            {
                if (m_highlightedBoneID >= 0)
                {
                    const uint32 nJointCount = pCharacter->GetIDefaultSkeleton().GetJointCount();
                    if (m_highlightedBoneID < nJointCount)
                    {
                        const QuatT m34 = pCharacter->GetISkeletonPose()->GetAbsJointByID(m_highlightedBoneID);
                        m_orbitTarget = m34.t;
                        needOrbitDistance = false;
                    }
                }
            }

            if (needOrbitDistance)
            {
                const QRect rc = contentsRect();
                // Distance to the center of the screen.
                const float orbitDistance = std::min(ViewToWorld(QPoint(rc.width() / 2, rc.height() / 2)).GetDistance(GetViewTM().GetTranslation()), MAX_ORBIT_DISTANCE);
                m_orbitTarget = GetViewTM().GetTranslation() + GetViewTM().TransformVector(FORWARD_DIRECTION) * orbitDistance;
            }
        }
        else
        {
            m_bInMoveMode = true;
        }
        m_mousePos = point;

        HideCursor();
    }
}

bool CMannequinModelViewport::RepositionMoveTool(const QPoint& point)
{
    Matrix34 m34 = Matrix34(Matrix33(m_ArcBall.ObjectRotation), m_ArcBall.sphere.center);
    m_HitContext.view = this;
    m_HitContext.b2DViewport = false;
    m_HitContext.point2d = point;
    ViewToWorldRay(point, m_HitContext.raySrc, m_HitContext.rayDir);
    m_HitContext.distanceTolerance = 0;

    if (m_AxisHelper.HitTest(m34, GetIEditor()->GetGlobalGizmoParameters(), m_HitContext))
    {
        SetAxisConstrain(m_HitContext.axis);
        GetIEditor()->SetAxisConstraints((AxisConstrains)m_HitContext.axis);
        if (m_Button_MOVE || m_Button_IK || m_animEventPosition)
        {
            m_opMode = MoveMode;
        }
        SetConstructionMatrix(COORDS_LOCAL, m34);
        return true;
    }
    return false;
}

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

#include "EditorUI_QT_Precompiled.h"

//AZCore
#include <AzCore/Math/MathUtils.h>

//CRY
#include <IParticles.h>
#include <ParticleParams.h>

//Editor
#include <GameEngine.h>
#include <IEditorParticleUtils.h>
#include <Include/HitContext.h>
#include <Gizmo/RotateAxisHelper.h>
#include <Gizmo/GizmoMoveHandler.h>
#include <Gizmo/GizmoRotationHandler.h>
#include <RenderHelpers/AxisHelper.h>
#include <QViewportEvents.h>
#include <DisplayViewportAdapter.h>
#include <Objects/DisplayContext.h>

//Local
#include "ParticlePreviewModelView.h"

//QT
#include <qapplication.h>

// These constants were found experimentally.
static const float kGizmoLineScale = 0.3f;
static const float kGizmoRotationRadiusScale = 4.0f;
static const float kGizmoRotationStepRadians = DEG2RAD(5.0f);
static const float kFocusRadiusMultiplier = 2.5f;
static const float kViewDistanceScaleFactor = 0.06f; // This constant is used with GetScreenScaleFactor when drawing the emittershape mirroring RotationDrawHelper::Axis value.

//Spline
static const int kSplineNumFullSineWaves = 3;
static const float kSplineArcDrawAngularStepDegrees = 1.0f;
static const int kSplineNumFullRotations = 3;
static const float kSplineRadiusIterations = 12.0f; // how many radius lengths the effect is to move along the spline. (NOTE: using X radius as we are only moving along the x Axis)
static const int kSplineDrawLineWidth = 3.0f;
static float kSplineMaxMoveTime = 5.0f; // only move for 5 seconds

//Menu Threshold
static const int kMenuThreshold = 36; // 6 pixel radius squared

//Axis constants
static const Vec3 kAxisX(1.0f, 0.0f, 0.0f);
static const Vec3 kAxisY(0.0f, 1.0f, 0.0f);
static const Vec3 kAxisZ(0.0f, 0.0f, 1.0f);

//Color Float
static const float fColRed[4] = { Col_Red.r, Col_Red.g, Col_Red.b, Col_Red.a };
static const float fColGreen[4] = { Col_Green.r, Col_Green.g, Col_Green.b, Col_Green.a };
static const float fColBlue[4] = { Col_Blue.r, Col_Blue.g, Col_Blue.b, Col_Blue.a };

// Values for styling the emitter shape
static const float EmitterShapeLineWidth = 5; //Set line width to get a better visual style
static const float EmitterShapeOffset = 0.5; // A value to style the X,Y,Z axis behavior;
static const float TextLabelSize = 300; //font size
static const Vec3 TextLabelOffset(0, EmitterShapeOffset, EmitterShapeOffset * 1.5); // Use a hard-coded offset to move the label text to a better position
static const float EmitterShapeCircleAngularStep = 2.5f; // A angle step to draw a better circle.

ParticlePreviewModelView::ParticlePreviewModelView(QWidget* parent)
    : CPreviewModelView(parent)
    , m_EmitterAABB(2.0f)
    , m_RightClickMousePos(0, 0)
{
    //By default all emitters are spawning in a Y up world so we need to rotate or emitter so that UP is Z.
    m_EmitterDefault.SetRotationXYZ(Ang3(0, 0, 0));

    //Move Gizmo
    m_pMoveGizmo = new CAxisHelper();
    m_pMoveGizmo->SetMode(CAxisHelper::MOVE_MODE);

    //Rotate Gizmo
    m_pRotateAxes = new RotationDrawHelper::Axis[3]; //The Axes that are supported are X, Y, and Z
    m_pRotateAxes[AXIS_X_ID].SetColors(Col_Red);//X
    m_pRotateAxes[AXIS_Y_ID].SetColors(Col_Green);//Y
    m_pRotateAxes[AXIS_Z_ID].SetColors(Col_Blue);//Z
}

ParticlePreviewModelView::~ParticlePreviewModelView()
{
    ReleaseEmitter();

    if (m_pMoveGizmo)
    {
        delete m_pMoveGizmo;
        m_pMoveGizmo = nullptr;
    }

    if (m_pRotateAxes)
    {
        delete[] m_pRotateAxes;
        m_pRotateAxes = nullptr;
    }

    if (m_pMouseDragHandler)
    {
        delete m_pMouseDragHandler;
        m_pMouseDragHandler = nullptr;
    }
}

void ParticlePreviewModelView::SetFlag(PreviewModelViewFlag flag)
{
    CPreviewModelView::SetFlag(flag);

    //Special case here cause we need to set rendering flags.
    if (flag == PreviewModelViewFlag::SHOW_OVERDRAW)
    {
        CRY_ASSERT(GetIEditor());
        IEditorParticleUtils* partUtils = GetIEditor()->GetParticleUtils();
        CRY_ASSERT(partUtils);
        int accumulated = partUtils->PreviewWindow_GetDisplaySettingsDebugFlags(GetIEditor()->GetDisplaySettings());
        accumulated |= DBG_RENDERER_OVERDRAW;
        partUtils->PreviewWindow_SetDisplaySettingsDebugFlags(GetIEditor()->GetDisplaySettings(), accumulated);
    }
}

void ParticlePreviewModelView::UnSetFlag(PreviewModelViewFlag flag)
{
    CPreviewModelView::UnSetFlag(flag);

    //Special case here cause we need to set rendering flags.
    if (flag == PreviewModelViewFlag::SHOW_OVERDRAW)
    {
        CRY_ASSERT(GetIEditor());
        IEditorParticleUtils* partUtils = GetIEditor()->GetParticleUtils();
        CRY_ASSERT(partUtils);
        int accumulated = partUtils->PreviewWindow_GetDisplaySettingsDebugFlags(GetIEditor()->GetDisplaySettings());
        accumulated &= ~DBG_RENDERER_OVERDRAW;
        partUtils->PreviewWindow_SetDisplaySettingsDebugFlags(GetIEditor()->GetDisplaySettings(), accumulated);
    }
}

void ParticlePreviewModelView::ResetSplineSetting()
{
    UnSetFlag(PreviewModelViewFlag::SPLINE_LOOPING);
    UnSetFlag(PreviewModelViewFlag::SPLINE_PINGPONG);
    m_SplineTimer = 0.0f;
    m_SplineMode = SplineMode::NONE;
    m_IsSplinePlayForward = true;
    m_SplineSpeedMultiplier = 1;
    m_EmitterSplineOffset.SetIdentity();
}

void ParticlePreviewModelView::ResetGizmoSetting()
{
    m_EmitterGizmoOffset.SetIdentity();
}

void ParticlePreviewModelView::ResetCamera()
{
    FocusOnParticle();
}

void ParticlePreviewModelView::ResetAll()
{
    ResetSplineSetting();
    ResetGizmoSetting();
    UpdateEmitterMatrix(); //Do this update here so the Camera has an updated location to focus on.
    ResetCamera();

    CPreviewModelView::ResetAll();
}

void ParticlePreviewModelView::SetSplineMode(SplineMode mode)
{
    m_SplineMode = mode;
    RestartSplineMovement();

    //Grab the size of the bound at this point so we dont cause our spline to grow and grow as we move along it (ie bound box changes based on particle positions)
    if (m_SplineMode != SplineMode::NONE)
    {
        m_SplineEmitterBoundRadiusAtStart = GetEmitterAABB().GetSize().x; //Using X here because all movemnt is down the X Axis
    }

    ForceParticleEmitterRestart(); //restart the effect so the bounding box is updated correctly.
}

void ParticlePreviewModelView::SetSplineSpeedMultiplier(unsigned char multiplier)
{
    m_SplineSpeedMultiplier = static_cast<unsigned int>(multiplier);
}

ParticlePreviewModelView::SplineMode ParticlePreviewModelView::GetSplineMode() const
{
    return m_SplineMode;
}

unsigned int ParticlePreviewModelView::GetSplineSpeedMultiplier() const
{
    return m_SplineSpeedMultiplier;
}

bool ParticlePreviewModelView::HasValidParticleEmitter() const
{
    return m_pEmitter != nullptr;
}

bool ParticlePreviewModelView::IsParticleEmitterAlive() const
{
    bool retval = false;
    if (HasValidParticleEmitter())
    {
        retval = m_pEmitter->IsAlive();
    }
    return retval;
}

unsigned int ParticlePreviewModelView::GetAliveParticleCount() const
{
    return m_AliveParticleCount;
}


float ParticlePreviewModelView::GetParticleEffectMaxLifeTime() const
{
    float retval = -1.0f;
    if (IsParticleEmitterAlive())
    {
        retval = FindMaxLifeTime(static_cast<const IParticleEffect*>(m_pEmitter->GetEffect()));
    }
    return retval;
}

float ParticlePreviewModelView::GetCurrentParticleEffectAge() const
{
    float retval = -1.0f;
    if (GetPlayState() != PlayState::NONE)
    {
        if (m_pEmitter && m_pEmitter->IsAlive())
        {
            retval = m_pEmitter->GetEmitterAge();
        }
    }
    return retval;
}

void ParticlePreviewModelView::RestartSplineMovement()
{
    m_SplineTimer = 0.0f;
    m_IsSplinePlayForward = true;
    m_SplineEmitterBoundRadiusAtStart = 1.0f;
    //Place the emitter back in original location.
    m_EmitterSplineOffset.SetIdentity();
}

void ParticlePreviewModelView::ForceParticleEmitterRestart()
{
    if (m_pEmitter)
    {
        m_pEmitter->Restart();
    }
}

void ParticlePreviewModelView::LoadParticleEffect(IParticleEffect* pEffect, CParticleItem* particle, SLodInfo* lod)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());

    if (pEffect && particle)
    {
        ReleaseEmitter();        

        if (lod)
        {
            IParticleEffect* lodEffect = pEffect->GetLodParticle(lod);
            if (lodEffect)
            {
                pEffect = lodEffect;
            }
        }

        m_pEmitter = pEffect->Spawn(m_EmitterDefault, ePEF_Nowhere| ePEF_DisableLOD);

        if (m_pEmitter)
        {
            m_pEmitter->AddRef();
            SetPlayState(PlayState::PLAY);
            ResetSplineSetting();

            if (m_IsFirstEffectLoad)
            {
                FocusOnParticle();
                m_IsFirstEffectLoad = false; //No longer first load ...
            }
        }
    }
}

const AABB& ParticlePreviewModelView::GetEmitterAABB()
{
    if (HasValidParticleEmitter())
    {
        //Doing this because when the emitter is being moved particles trailing behind the moved effect cause the bounding box to be invalid, but the emitter bound box is not updated
        // right away resulting in a time period where the AABB is invalid ... so we are using m_EmitterAABB as a way to cache off the last non-reset state.
        AABB temp = m_pEmitter->GetBBox();
        if (!temp.IsReset())
        {
            //update the AABB
            m_EmitterAABB = temp;
        }
    }
    return m_EmitterAABB;
}

void ParticlePreviewModelView::OnViewportRender(const SRenderContext& rc)
{
    //Update
    if (HasValidParticleEmitter())
    {
        // When a Level is present in the Editor, the main RenderViewport updates the ParticleManager
        // via C3DEngine::RenderWorld(). When there is no Level, we need to process ParticleManager
        // ourselves to ensure the preview Emitter remains active.
        CGameEngine* gameEngine = GetIEditor()->GetGameEngine();
        if (!gameEngine->IsLevelLoaded())
        {
            GetIEditor()->GetEnv()->pParticleManager->Update();
        }

        UpdateEmitter();
        UpdateSplineMovement();
        UpdateEmitterMatrix();
    }

    CPreviewModelView::OnViewportRender(rc);

    RenderEmitters(*rc.renderParams, *rc.passInfo);
}

void ParticlePreviewModelView::OnViewportMouse(const SMouseEvent& ev)
{
    if (ev.type == SMouseEvent::MOVE)
    {
        Vec2i currentMouse = Vec2i(ev.x, ev.y);
        if ((currentMouse - m_RightClickMousePos).GetLengthSquared() > kMenuThreshold)
        {
            m_IsRequestingMenu = false;
        }
        //If we have a mouse drag handler then we should update that instead of performing hit tests on the gizmo
        if (m_pMouseDragHandler)
        {
            UpdateMouseHandler(ev);
        }
        else
        {
            UpdateGizmo(ev);
        }
    }
    if (ev.button == SMouseEvent::BUTTON_RIGHT)
    {
        if (ev.type == SMouseEvent::PRESS)
        {
            m_RightClickMousePos = Vec2i(ev.x, ev.y);
            m_IsRequestingMenu = true;
        }
        else if (ev.type == SMouseEvent::RELEASE)
        {
            if (!m_IsRequestingMenu)
            {
                return;
            }
            m_IsRequestingMenu = false;
            Vec2i current = Vec2i(ev.x, ev.y);
            //If the mouse has not moved ... trigger the right click menu.
            if (m_RightClickMousePos == current)
            {
                if (m_ContextMenuCallback)
                {
                    QApplication::restoreOverrideCursor(); //setting this here so we have a mouse cursor when selecting menu options and other things.
                    QPoint temp = mapToGlobal(QPoint(m_RightClickMousePos.x, m_RightClickMousePos.y));
                    m_ContextMenuCallback(Vec2i(temp.x(), temp.y()));
                }
                m_RightClickMousePos = current;
            }
        }
    }
    else if (ev.button == SMouseEvent::BUTTON_LEFT)
    {
        if (ev.type == SMouseEvent::PRESS)
        {
            ReleaseMouseHandler(ev);
            SelectMouseHandler(ev);
        }
        else if (ev.type == SMouseEvent::RELEASE)
        {
            ReleaseMouseHandler(ev);
        }
    }
}

//IEditorNotifyListener
void ParticlePreviewModelView::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnCloseScene)
    {
        ReleaseEmitter();
    }

    CPreviewModelView::OnEditorNotifyEvent(event);
}

void ParticlePreviewModelView::ClearEmitter()
{
    ReleaseEmitter();
    SetPlayState(PlayState::NONE);
}

void ParticlePreviewModelView::UpdateEmitterMatrix()
{
    //NOTE: order is important here ... splitting up the calculation so that things get applied the the correct order.
    Matrix33 gizmo, spline;
    m_EmitterGizmoOffset.GetRotation33(gizmo);
    m_EmitterSplineOffset.GetRotation33(spline);

    Matrix33 rotation = spline * gizmo;
    Vec3 translation = m_EmitterSplineOffset.GetTranslation() + m_EmitterGizmoOffset.GetTranslation();

    m_EmitterCurrent.SetRotation33(rotation);
    m_EmitterCurrent.SetTranslation(translation);
}

void ParticlePreviewModelView::UpdateEmitter()
{
    CRY_ASSERT(m_pEmitter);

    switch (m_PlayState)
    {
    case PlayState::PLAY:
    {
        if (!m_pEmitter->IsAlive())
        {
            if (IsFlagSet(PreviewModelViewFlag::LOOPING_PLAY))
            {
                m_pEmitter->Restart();
            }
        }
        m_pEmitter->Activate(true);
        SpawnParams sPram;
        sPram.fTimeScale = m_TimeScale;
        m_pEmitter->SetSpawnParams(sPram);
        break;
    }
    case PlayState::STEP:
    {
        if (!m_pEmitter->IsAlive())
        {
            m_pEmitter->Restart();
        }
        m_pEmitter->Activate(true);
        SpawnParams sPram;
        sPram.fStepValue = 0.033f;
        sPram.fTimeScale = 0.0f;
        m_pEmitter->SetSpawnParams(sPram);
        SetPlayState(PlayState::PAUSE);
        break;
    }
    case PlayState::PAUSE:
    {
        SpawnParams sPram;
        sPram.fTimeScale = 0.0;
        m_pEmitter->SetSpawnParams(sPram);
        break;
    }
    case PlayState::RESET:
    {
        //Set the play state before restart emitter, otherwise emitter will restart immediately.
        if (IsFlagSet(PreviewModelViewFlag::LOOPING_PLAY))
        {
            SetPlayState(PlayState::PLAY);
        }
        else
        {
            SetPlayState(PlayState::PAUSE);
        }
        m_pEmitter->Restart();
        break;
    }
    default:
        break;
    }
}



void ParticlePreviewModelView::UpdateSplineMovement()
{
    if (!IsParticleEmitterAlive())
    {
        return;
    }

    PlayState playstate = GetPlayState();
    if (playstate != PlayState::PLAY && playstate != PlayState::STEP)
    {
        return;
    }

    SplineMode splinemode = GetSplineMode();
    if (splinemode != SplineMode::NONE)
    {
        CRY_ASSERT(GetIEditor());
        CRY_ASSERT(GetIEditor()->GetSystem());
        CRY_ASSERT(GetIEditor()->GetSystem()->GetITimer());
        CRY_ASSERT(m_pEmitter);
        float frameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime() * m_SplineSpeedMultiplier;

        if (!m_IsSplinePlayForward)
        {
            frameTime *= -1.0f;
        }
        m_SplineTimer += frameTime; //increment time

        if ((m_SplineTimer <= kSplineMaxMoveTime && m_SplineTimer >= 0.0f))
        {
            float totalDistanceToCover = m_SplineEmitterBoundRadiusAtStart * kSplineRadiusIterations;
            float timeRatio = m_SplineTimer / kSplineMaxMoveTime;

            //Always move along the X axis
            Vec3 nowpos = kAxisX * totalDistanceToCover * timeRatio;

            SplineMode smode = GetSplineMode();
            if (smode == SplineMode::SINEWAVE)
            {
                float totalRotationTodo = AZ::Constants::Pi * kSplineNumFullSineWaves;
                float angle = totalRotationTodo * timeRatio;
                nowpos += Vec3(0.0f, 0.0f, m_SplineEmitterBoundRadiusAtStart * sin(angle));
            }

            if (smode == SplineMode::COIL)
            {
                float totalRotationTodo = AZ::Constants::TwoPi * kSplineNumFullRotations;
                float angle = totalRotationTodo * timeRatio;
                nowpos += Vec3(0.0f, m_SplineEmitterBoundRadiusAtStart * cos(angle), m_SplineEmitterBoundRadiusAtStart * sin(angle));
            }
            m_EmitterSplineOffset.SetTranslation(nowpos);
        }
        else if (IsFlagSet(PreviewModelViewFlag::SPLINE_PINGPONG))
        {
            //Reset for ping pong
            m_IsSplinePlayForward = !m_IsSplinePlayForward;
        }
        else if (IsFlagSet(PreviewModelViewFlag::SPLINE_LOOPING))
        {
            //Reset for looping
            RestartSplineMovement();
        }
        else
        {
            //Done playing so lets stop trying to play ...
            SetSplineMode(SplineMode::NONE);
        }
    }
}

void ParticlePreviewModelView::UpdateGizmo(const SMouseEvent& ev)
{
    if (GetSplineMode() != SplineMode::NONE)
    {
        //Dont update if running along a spline
        return;
    }

    HitContext hc;
    hc.point2d = QPoint(ev.x, ev.y);

    CDisplayViewportAdapter view(this); //NOTE: CPreviewModelView derives from QViewport
    hc.view = &view;

    UpdateMoveGizmo(hc, m_EmitterCurrent);
    UpdateRotateGizmo(hc, m_EmitterCurrent);
}

void ParticlePreviewModelView::UpdateMoveGizmo(HitContext& hc, const Matrix34& mat)
{
    CRY_ASSERT(m_pMoveGizmo);

    SGizmoParameters gizmoParameters;
    gizmoParameters.axisGizmoScale = kGizmoLineScale;
    m_pMoveGizmo->HitTest(mat, gizmoParameters, hc);
    m_pMoveGizmo->SetHighlightAxis(hc.axis);
}

void ParticlePreviewModelView::UpdateRotateGizmo(HitContext& hc, const Matrix34& mat)
{
    CRY_ASSERT(hc.view);
    CRY_ASSERT(m_pRotateAxes);

    Vec3 position = mat.GetTranslation();
    //NOTE: need to take into account orientation changes ...
    Matrix33 gizmoOrientation(IDENTITY);
    mat.GetRotation33(gizmoOrientation);
    float screenScale = hc.view->GetScreenScaleFactor(position);

    //Make a ray into the world
    Ray ray;
    hc.view->ViewToWorldRay(hc.point2d, ray.origin, ray.direction);
    hc.raySrc = ray.origin;
    hc.rayDir = ray.direction;
    m_highlightedRotAxis = -1;

    if (m_pRotateAxes[AXIS_X_ID].HitTest(position, hc, kGizmoRotationRadiusScale, kGizmoRotationStepRadians, gizmoOrientation * kAxisX, screenScale))
    {
        m_highlightedRotAxis = AXIS_X_ID;
    }

    if (m_pRotateAxes[AXIS_Y_ID].HitTest(position, hc, kGizmoRotationRadiusScale, kGizmoRotationStepRadians, gizmoOrientation * kAxisY, screenScale))
    {
        m_highlightedRotAxis = AXIS_Y_ID;
    }

    if (m_pRotateAxes[AXIS_Z_ID].HitTest(position, hc, kGizmoRotationRadiusScale, kGizmoRotationStepRadians, gizmoOrientation * kAxisZ, screenScale))
    {
        m_highlightedRotAxis = AXIS_Z_ID;
    }
}

void ParticlePreviewModelView::RenderEmitters(SRendParams& rendParams, SRenderingPassInfo& passInfo)
{
    m_AliveParticleCount = 0;
    //Only one emitter currently ...
    if (m_pEmitter)
    {
        if (IsFlagSet(PreviewModelViewFlag::PRECACHE_MATERIAL))
        {
            _smart_ptr<IMaterial>  pCurMat = m_pEmitter->GetMaterial();
            if (pCurMat)
            {
                pCurMat->PrecacheMaterial(0.0f, nullptr, true, true);
            }
        }

        //NOTE: m_EmitterDefault is a rotation of 90 degrees around the X axis as a way to make the emitter's up be Z instead of Y
        // The viewports are a Z up world so need to make sure the emitter points accordingly.
        //NOTE: this should not be used for other things unless truly needed ... everything else is assuming a Z up world.
        Matrix34 emitterFinal = m_EmitterCurrent * m_EmitterDefault;
        //m_pEmitter->SetLocation(QuatT(emitterFinal));
        m_pEmitter->SetMatrix(emitterFinal);
        rendParams.bIsShowWireframe = IsFlagSet(PreviewModelViewFlag::DRAW_WIREFRAME);
        rendParams.mRenderFirstContainer = IsFlagSet(PreviewModelViewFlag::SHOW_FIRST_CONTAINER);
        //use a temporary rendering pass that substitutes our camera instead of using the Level's camera.
        m_pEmitter->Render(rendParams, SRenderingPassInfo::CreateTempRenderingInfo(*Camera(), passInfo));

        DisplayContext dc;
        CDisplayViewportAdapter view(this); //NOTE: CPreviewModelView derives from QViewport
        dc.SetView(&view);
        dc.camera = Camera();

        if (IsFlagSet(PreviewModelViewFlag::SHOW_EMITTER_SHAPE))
        {
            RenderEmitterShape(dc, m_EmitterCurrent);
        }

        if (IsFlagSet(PreviewModelViewFlag::SHOW_BOUNDINGBOX))
        {
            CRY_ASSERT(GetIEditor());
            CRY_ASSERT(GetIEditor()->GetEnv());
            CRY_ASSERT(GetIEditor()->GetEnv()->pRenderer);
            IRenderAuxGeom* aux = GetIEditor()->GetEnv()->pRenderer->GetIRenderAuxGeom();
            CRY_ASSERT(aux);
            aux->DrawAABB(GetEmitterAABB(), false, ColorB(1.0f), eBBD_Faceted);
        }

        if (IsFlagSet(PreviewModelViewFlag::SHOW_GIZMO)) // if this emitter is the selected one ...
        {
            RenderMoveGizmo(dc, m_EmitterCurrent);
            RenderRotateGizmo(dc, m_EmitterCurrent);
        }

        //Spline Visualization
        RenderSplineVisualization(dc);

        //Only valid during a render ...
        m_AliveParticleCount = m_pEmitter->GetAliveParticleCount();
    }
}

void ParticlePreviewModelView::RenderEmitterShape(DisplayContext& dc, const Matrix34& mat)
{
    dc.SetLineWidth(EmitterShapeLineWidth);

    const ParticleParams& params = m_pEmitter->GetEffect()->GetParticleParams();
    dc.PushMatrix(mat);
    
    Vec3 pos = params.vSpawnPosOffset;

    Vec3 screenPos = dc.GetView()->GetScreenTM().TransformPoint(pos);


    float maxOffsetToEdge = Vec2(params.fSizeX, params.fSizeY).GetLength() * 0.5;//used to extend shape to encompass entire particle
    switch (params.GetEmitterShape())
    {
        case ParticleParams::EEmitterShapeType::CIRCLE:
        {
            float radius = params.fEmitterSizeDiameter * 0.5f + maxOffsetToEdge;
            float zLength = params.fEmitterSizeDiameter * 0.5f + maxOffsetToEdge;

            dc.DrawDottedCircle(pos, radius, kAxisZ, 0, 10);
            dc.SetColor(Col_Red);
            DrawLineWithArrow(dc, pos, radius, EmitterShapeOffset, kAxisX);
            Vec3 drawPosition = pos + radius * kAxisX;
            GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(drawPosition + TextLabelOffset), TextLabelSize, fColRed, false, true, "%s", "X");

            dc.SetColor(Col_Blue);
            DrawLineWithArrow(dc, pos - Vec3(0, radius, 0), zLength, 0, kAxisZ);
            DrawLineWithArrow(dc, pos - Vec3(0, radius, 0), zLength, 0, -kAxisZ);
            GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(pos - Vec3(0, radius, 0) + TextLabelOffset), TextLabelSize, fColBlue, false, true, "%s", "Z");

            // We shift the center point up so the shape indicator does not overlap with the emitter shape.
            // 5*EmitterShapeOffset is the offset we would like to shift up. It gives user a better visual result.
            dc.SetColor(Col_Green);
            dc.DrawArcWithArrow(pos + Vec3(0, 5 * EmitterShapeOffset, 0), radius, 90, 45, EmitterShapeCircleAngularStep, kAxisZ);
            dc.DrawArcWithArrow(pos + Vec3(0, 5 * EmitterShapeOffset, 0), radius, 180, 45, EmitterShapeCircleAngularStep, -kAxisZ);
            drawPosition = GetPointOnArc(pos, radius, 90, kAxisZ);
            GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(drawPosition + TextLabelOffset), TextLabelSize, fColGreen, false, true, "%s", "Y");

            break;
        }
        case ParticleParams::EEmitterShapeType::SPHERE:
        {
            float radius = params.fEmitterSizeDiameter * 0.5f + maxOffsetToEdge;
            RenderEmitterShapeSphere(dc, pos, radius);
            break;
        }
        case ParticleParams::EEmitterShapeType::BOX:
        {
            Vec3 bounds(params.vEmitterSizeXYZ.fX * 0.5f + maxOffsetToEdge, 
                        params.vEmitterSizeXYZ.fY * 0.5f + maxOffsetToEdge, 
                        params.vEmitterSizeXYZ.fZ * 0.5f + maxOffsetToEdge);
            RenderEmitterShapeBox(dc, pos, bounds.x, bounds.y, bounds.z);
            break;
        }
        default:
        {
            break;
        }
    }

    dc.PopMatrix();
}


/* The box axis indicator will be drawn as follow:
** <------[length=size]-----[offset]TextLabel[offset]---------[length=size]-------->
*/
void ParticlePreviewModelView::RenderEmitterShapeBox(DisplayContext& dc, const Vec3& center, const float x, const float y, const float z)
{
    Vec3 boxMin(center.x - x, center.y - y, center.z - z);
    Vec3 boxMax(center.x + x, center.y + y, center.z + z);

    // Draw bording box
    Vec3 P0 = Vec3(boxMin.x, boxMax.y, boxMin.z);
    Vec3 P1 = Vec3(boxMax.x, boxMax.y, boxMin.z);
    Vec3 P2 = boxMin;
    Vec3 P3 = Vec3(boxMax.x, boxMin.y, boxMin.z);
    Vec3 P4 = Vec3(boxMin.x, boxMax.y, boxMax.z);
    Vec3 P5 = boxMax;
    Vec3 P6 = Vec3(boxMin.x, boxMin.y, boxMax.z);
    Vec3 P7 = Vec3(boxMax.x, boxMin.y, boxMax.z);

    // draw box
    dc.DrawDottedLine(P0, P1, Col_White, Col_White);
    dc.DrawDottedLine(P0, P2, Col_White, Col_White);
    dc.DrawDottedLine(P2, P3, Col_White, Col_White);
    dc.DrawDottedLine(P1, P3, Col_White, Col_White);
    dc.DrawDottedLine(P0, P4, Col_White, Col_White);
    dc.DrawDottedLine(P1, P5, Col_White, Col_White);
    dc.DrawDottedLine(P2, P6, Col_White, Col_White);
    dc.DrawDottedLine(P3, P7, Col_White, Col_White);
    dc.DrawDottedLine(P4, P5, Col_White, Col_White);
    dc.DrawDottedLine(P4, P6, Col_White, Col_White);
    dc.DrawDottedLine(P7, P5, Col_White, Col_White);
    dc.DrawDottedLine(P7, P6, Col_White, Col_White);

    // draw XYZ indicator
    Vec3 posX = center - Vec3(0, 0, boxMin.z);
    float xlineLength = x - EmitterShapeOffset;
    dc.SetColor(Col_Red);
    DrawLineWithArrow(dc, posX, xlineLength, EmitterShapeOffset, kAxisX);
    DrawLineWithArrow(dc, posX, xlineLength, EmitterShapeOffset, -(kAxisX));
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(posX + TextLabelOffset), TextLabelSize, fColRed, false, true, "%s", "X");

    Vec3 posY = center - Vec3(boxMin.x, 0, 0);
    float ylineLength = y - EmitterShapeOffset;
    dc.SetColor(Col_Green);
    DrawLineWithArrow(dc, posY, ylineLength, EmitterShapeOffset, kAxisY);
    DrawLineWithArrow(dc, posY, ylineLength, EmitterShapeOffset, -(kAxisY));
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(posY + TextLabelOffset), TextLabelSize, fColGreen, false, true, "%s", "Y");

    Vec3 posZ = center - Vec3(0, boxMin.y, 0);
    float zlineLength = z - EmitterShapeOffset;
    dc.SetColor(Col_Blue);
    DrawLineWithArrow(dc, posZ, zlineLength, EmitterShapeOffset, kAxisZ);
    DrawLineWithArrow(dc, posZ, zlineLength, EmitterShapeOffset, -(kAxisZ));
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(posZ + TextLabelOffset), TextLabelSize, fColBlue, false, true, "%s", "Z");
}

void ParticlePreviewModelView::RenderEmitterShapeSphere(DisplayContext& dc, const Vec3& pos, const float radius)
{
    if (radius <= 0) // Don't draw the shape if the size is smaller than 0
    {
        return;
    }

    // White Emitter Shape
    dc.SetColor(Col_White);
    dc.DrawDottedCircle(pos, radius, kAxisX, 0, 10);
    dc.DrawBall(pos, EmitterShapeOffset);

    dc.SetColor(Col_Green);
    dc.DrawDottedCircle(pos, radius, kAxisZ, 3, 10);
    Vec3 labelPosition = GetPointOnArc(pos, radius, 90, kAxisZ);
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(labelPosition + TextLabelOffset), TextLabelSize, fColGreen, false, true, "%s", "Y");


    dc.SetColor(Col_Blue);
    dc.DrawDottedCircle(pos, radius, kAxisY, 3, 10);
    labelPosition = GetPointOnArc(pos, radius, 90, kAxisY);
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(labelPosition + TextLabelOffset), TextLabelSize, fColBlue, false, true, "%s", "Z");


    dc.SetColor(Col_Red);
    DrawLineWithArrow(dc, pos, radius, EmitterShapeOffset, kAxisX);
    labelPosition = pos + radius * kAxisX;
    GetIEditor()->GetEnv()->pRenderer->DrawLabelEx(dc.ToWorldSpacePosition(labelPosition + TextLabelOffset), TextLabelSize, fColRed, false, true, "%s", "X");
}

void ParticlePreviewModelView::RenderEmitterShapeAngle(DisplayContext& dc, const Vec3& pos, const Matrix34& mat, float radius)
{
    const AABB box = GetEmitterAABB();

    Vec3 centerOfBox = box.GetCenter();
    //if centered on x and z but lowest 25% on y
    Vec3 radiusOfBoundsCheck = box.GetSize() * 0.25f;
    Vec3 checkVec = Vec3(abs(pos.x - centerOfBox.x), abs(pos.y - centerOfBox.y), abs(pos.z - (box.GetCenter().z - (box.GetSize().z / 2.0f))));
    float widthToHeightRatioOfBox = ((box.GetSize().x > box.GetSize().y) ? box.GetSize().x : box.GetSize().y) / box.GetSize().z;

    if (checkVec.z < radiusOfBoundsCheck.z)   //we are in the bottom 25% of box
    {
        if (widthToHeightRatioOfBox <= 0.5f)  //if pos is within bottom 25% on the y, and box width/height <= 0.5f it's a cylinder
        {
            dc.DrawCircle(Vec3(0.0f, 0.0f, radius * 2), radius);
            dc.DrawCircle(pos, radius);

            dc.DrawLine(Vec3(radius, 0.0f, 0.0f), Vec3(radius, 0.0f, radius * 2));
            dc.DrawLine(Vec3(-radius, 0.0f, 0.0f), Vec3(-radius, 0.0f, radius * 2));
            dc.DrawLine(Vec3(0.0f, radius, 0.0f), Vec3(0.0f, radius, radius * 2));
            dc.DrawLine(Vec3(0.0f, -radius, 0.0f), Vec3(0.0f, -radius, radius * 2));
        }
        else //if pos is within bottom 25% on the y, and box width/height > 0.5f
        {
            dc.DrawCircle(pos, radius * 2);
            dc.DrawCircle(pos, radius * 2 / 3.0f);

            dc.DrawArc(pos, radius * 2, 0.0f, 180.0f, 1, Vec3(0.0f, -1.0f, 0.0f));
            dc.DrawArc(pos, radius * 2, 90.0f, 180.0f, 1, kAxisX);
        }
    }
    else    //if pos is within 25% of the center of the in all directions it's a sphere
    {
        dc.DrawWireSphere(pos, radius * 2);
    }
}


void ParticlePreviewModelView::DrawLineWithArrow(DisplayContext& dc, const Vec3& startPos /*= Vec3(0, 0, 0)*/, const float length /*= 1*/, const float positionOffset /*= 0.0f*/, const Vec3 direction /*= kAxisX */)
{
    if (length > 0)
    {
        float offset = positionOffset;
        if (length <= offset) // if offset is too large, ignored the offset
        {
            offset = 0;
        }
        float arrowSize = EmitterShapeLineWidth;
        if (EmitterShapeLineWidth >= length)// if the arrow size is too big
        {
            arrowSize = length / 2;
        }

        dc.DrawLine(startPos + direction * offset, startPos + direction * (length + offset));
        dc.DrawArrow(startPos + direction * (length - 1), startPos + direction * (length + offset), arrowSize);
    }
}

Vec3 ParticlePreviewModelView::GetPointOnArc(const Vec3& center, float radius, float Degrees, const Vec3& fixedAxis)
{
    Vec3 a;
    Vec3 b;
    GetBasisVectors(fixedAxis, a, b);

    float angle = DEG2RAD(Degrees);

    float cosAngle = cos(angle) * radius;
    float sinAngle = sin(angle) * radius;

    Vec3 p0;
    p0.x = center.x + cosAngle * a.x + sinAngle * b.x;
    p0.y = center.y + cosAngle * a.y + sinAngle * b.y;
    p0.z = center.z + cosAngle * a.z + sinAngle * b.z;

    return p0;
}


void ParticlePreviewModelView::RenderSplineVisualization(DisplayContext& dc)
{
    CRY_ASSERT(m_pEmitter);
    CRY_ASSERT(dc.view);
    Vec3 startPos = m_EmitterGizmoOffset.GetTranslation();//spline does not take into account gizmo rotations ...
    float totalDistanceToCover = m_SplineEmitterBoundRadiusAtStart * kSplineRadiusIterations;

    dc.SetLineWidth(kSplineDrawLineWidth);
    dc.SetColor(Col_Orange);

    SplineMode smode = GetSplineMode();
    if (smode == SplineMode::LINE)
    {
        dc.DrawLine(startPos, startPos + kAxisX * totalDistanceToCover);
    }
    else if (smode == SplineMode::SINEWAVE)
    {
        float diameterOfFullWave = totalDistanceToCover / kSplineNumFullSineWaves;
        float diameterOfHalfWave = diameterOfFullWave * 0.5f;
        float radiusOfWave = diameterOfHalfWave * 0.5f;

        for (int w = 0; w < kSplineNumFullSineWaves; w++)
        {
            float upOffset = diameterOfFullWave * w + radiusOfWave;
            float downOffset = diameterOfFullWave * w + diameterOfHalfWave + radiusOfWave;

            //Up arc
            dc.DrawArc(startPos + (kAxisX * upOffset), radiusOfWave, 0.0f, 180.0f, kSplineArcDrawAngularStepDegrees, kAxisY * -1.0f);

            //Down arc
            dc.DrawArc(startPos + (kAxisX * downOffset), radiusOfWave, 0.0f, 180.0f, kSplineArcDrawAngularStepDegrees, kAxisY);
        }
    }
    else if (smode == SplineMode::COIL)
    {
        Vec3 lastOff(0.0f, m_SplineEmitterBoundRadiusAtStart, 0.0f); //NOTE: this will be the first point this is being done to remove extra line from emitter origin so coil looks smooth.
        float totalRotationTodo = AZ::Constants::TwoPi * kSplineNumFullRotations;
        static const float step = 0.01f;
        for (float ratio = step; ratio <= 1.0f; ratio += step)
        {
            float angle = totalRotationTodo * ratio;
            float distance = totalDistanceToCover * ratio;
            //NOTE: this moves along the kAxisX
            Vec3 nowOff(distance, m_SplineEmitterBoundRadiusAtStart * cos(angle), m_SplineEmitterBoundRadiusAtStart * sin(angle));
            dc.DrawLine(startPos + lastOff, startPos + nowOff);
            lastOff = nowOff;
        }
    }
}

void ParticlePreviewModelView::RenderMoveGizmo(DisplayContext& dc, const Matrix34& mat)
{
    //any mode other then none means we're moving along a spline
    if (GetSplineMode() != SplineMode::NONE)
    {
        //Shouldn't render gizmo while moving along spline
        return;
    }
    CRY_ASSERT(m_pMoveGizmo);

    SGizmoParameters gizmoParameters;
    gizmoParameters.axisGizmoScale = kGizmoLineScale;
    m_pMoveGizmo->DrawAxis(mat, gizmoParameters, dc);
}

void ParticlePreviewModelView::RenderRotateGizmo(DisplayContext& dc, const Matrix34& mat)
{
    //any mode other then none means we're moving along a spline
    if (GetSplineMode() != SplineMode::NONE)
    {
        //Shouldn't render gizmo while moving along spline
        return;
    }

    CRY_ASSERT(m_pRotateAxes);

    Vec3 position = mat.GetTranslation();
    //NOTE: need to take into account orientation changes ...
    Matrix33 gizmoOrientation(IDENTITY);
    mat.GetRotation33(gizmoOrientation);
    float screenScale = dc.view->GetScreenScaleFactor(position);
    Vec3 cameraPos = dc.camera->GetPosition();

    // X axis arc
    Vec3 cameraViewDir = (cameraPos - position).GetNormalized();
    float cameraAngle = atan2f(cameraViewDir.y, cameraViewDir.x);
    m_pRotateAxes[AXIS_X_ID].Draw(dc, position, gizmoOrientation * kAxisX, cameraAngle, kGizmoRotationStepRadians, kGizmoRotationRadiusScale, m_highlightedRotAxis == AXIS_X_ID, screenScale, true);

    // Y axis arc
    cameraAngle = atan2f(-cameraViewDir.z, cameraViewDir.x);
    m_pRotateAxes[AXIS_Y_ID].Draw(dc, position, gizmoOrientation * kAxisY, cameraAngle, kGizmoRotationStepRadians, kGizmoRotationRadiusScale, m_highlightedRotAxis == AXIS_Y_ID, screenScale, true);

    // Z axis arc
    cameraAngle = atan2f(cameraViewDir.y, cameraViewDir.x);
    m_pRotateAxes[AXIS_Z_ID].Draw(dc, position, gizmoOrientation * kAxisZ, cameraAngle, kGizmoRotationStepRadians, kGizmoRotationRadiusScale, m_highlightedRotAxis == AXIS_Z_ID, screenScale, true);
}


void ParticlePreviewModelView::ReleaseMouseHandler(const SMouseEvent& ev)
{
    if (m_pMouseDragHandler)
    {
        m_pMouseDragHandler->End(ev);
        delete m_pMouseDragHandler;
        m_pMouseDragHandler = nullptr;
    }
}

void ParticlePreviewModelView::UpdateMouseHandler(const SMouseEvent& ev)
{
    CRY_ASSERT(m_pMouseDragHandler);
    m_pMouseDragHandler->Update(ev);
    m_EmitterGizmoOffset = m_pMouseDragHandler->GetMatrix();
}

void ParticlePreviewModelView::SelectMouseHandler(const SMouseEvent& ev)
{
    CRY_ASSERT(m_pMouseDragHandler == nullptr);

    if (GetSplineMode() != SplineMode::NONE)
    {
        //Cant select a mouse handler when running along a spline
        return;
    }

    Vec3 hitPoint = m_EmitterGizmoOffset.GetTranslation();
    int highlightedMoveAxis = m_pMoveGizmo->GetHighlightAxis();
    if (highlightedMoveAxis != AxisConstrains::AXIS_NONE)
    {
        //NOTE: need to take into account orientation changes ...
        Matrix33 gizmoOrientation(IDENTITY);
        m_EmitterCurrent.GetRotation33(gizmoOrientation);

        //Select the move axis and prepare the mouse handler
        GizmoTransformConstraint constraint;
        switch (highlightedMoveAxis)
        {
        case AXIS_X:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = gizmoOrientation * kAxisX;
        }
        break;
        case AXIS_Y:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = gizmoOrientation * kAxisY;
        }
        break;
        case AXIS_Z:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = gizmoOrientation * kAxisZ;
        }
        break;
        case AXIS_XY:
        {
            constraint.type = GizmoTransformConstraint::PLANE;
            constraint.plane.SetPlane(gizmoOrientation * kAxisZ, hitPoint);
            constraint.axis = Vec3(1.0f, 1.0f, 0.0f);
        }
        break;
        case AXIS_XZ:
        {
            constraint.type = GizmoTransformConstraint::PLANE;
            constraint.plane.SetPlane(gizmoOrientation * kAxisY, hitPoint);
            constraint.axis = Vec3(1.0f, 0.0f, 1.0f);
        }
        break;
        case AXIS_YZ:
        {
            constraint.type = GizmoTransformConstraint::PLANE;
            constraint.plane.SetPlane(gizmoOrientation * kAxisX, hitPoint);
            constraint.axis = Vec3(0.0f, 1.0f, 1.0f);
        }
        break;
        case AXIS_XYZ:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = Vec3(1.0f, 1.0f, 1.0f);
        }
        break;
        }

        m_pMouseDragHandler = new GizmoMoveHandler(constraint);
    }
    else if (m_highlightedRotAxis != -1)
    {
        //Select the rotate axis and prepare the mouse handler
        GizmoTransformConstraint constraint;
        switch (m_highlightedRotAxis)
        {
        case AXIS_X_ID:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = kAxisX;
        }
        break;
        case AXIS_Y_ID:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = kAxisY;
        }
        break;
        case AXIS_Z_ID:
        {
            constraint.type = GizmoTransformConstraint::AXIS;
            constraint.axis = kAxisZ;
        }
        break;
        }

        m_pMouseDragHandler = new GizmoRotationHandler(constraint);
    }

    if (m_pMouseDragHandler)
    {
        if (!m_pMouseDragHandler->Begin(ev, m_EmitterGizmoOffset))
        {
            ReleaseMouseHandler(ev);
        }
    }
}

void ParticlePreviewModelView::ReleaseEmitter()
{
    if (m_pEmitter)
    {
        m_pEmitter->Activate(false);
        m_pEmitter->Release();
        m_pEmitter = nullptr;
    }
}

float ParticlePreviewModelView::FindMaxLifeTime(const IParticleEffect* effect) const
{
    float maxLife = -1.0f; // by default this means that the particle effect is continuous
    FindMaxLifeTimeRecurse(effect, maxLife);
    return maxLife;
}

void ParticlePreviewModelView::FindMaxLifeTimeRecurse(const IParticleEffect* effect, float& inOut) const
{
    const ParticleParams& params = effect->GetParticleParams();
    float totalMaxTime = 0;
    int numChildren = effect->GetChildCount();

    if (effect->IsEnabled())
    {
        if (params.fEmitterLifeTime != 0)
        {
            totalMaxTime = params.GetMaxEmitterLife() + params.GetMaxParticleLife();
        }
        else
        {
            totalMaxTime = params.GetMaxParticleLife();
        }
        if (params.fPulsePeriod.GetMaxValue() > 0 || totalMaxTime == 0 || (params.bContinuous && params.fEmitterLifeTime == 0))
        {
            inOut = -1; //particle is continuous
            return;
        }
        if (totalMaxTime > inOut)
        {
            inOut = totalMaxTime;
        }
    }

    for (int i = 0; i < numChildren; i++)
    {
        FindMaxLifeTimeRecurse(effect->GetChild(i), inOut);
        if (inOut == -1)
        {
            break;
        }
    }
}

void ParticlePreviewModelView::FocusOnParticle()
{
    if (HasValidParticleEmitter())
    {
        CCamera* camera = Camera();
        if (camera)
        {
            Vec3 fromDir(1.0f, 1.0f, -0.5f);
            Vec3 target = GetEmitterAABB().GetCenter();
            float bbRadius = GetEmitterAABB().GetRadius() * kFocusRadiusMultiplier;

            Vec3 dir = fromDir.GetNormalized();
            Matrix34 tm = Matrix33::CreateRotationVDir(dir, 0);
            tm.SetTranslation(target - dir * bbRadius);
            CameraMoved(QuatT(tm), true);
        }
    }
    else
    {
        CPreviewModelView::FocusOnScreen();
    }
}
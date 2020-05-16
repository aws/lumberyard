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

#include "PreviewModelView.h"
#include "Cry_Geo.h"

struct IParticleEffect;
struct IParticleEmitter;

class EDITOR_QT_UI_API ParticlePreviewModelView
    : public CPreviewModelView
{
public:
    enum class EmitterShape
    {
        UNKNOWN,
        ANGLE,
        POINT,
        CIRCLE,
        SPHERE,
        BOX,
    };

    explicit ParticlePreviewModelView(QWidget* parent);
    virtual ~ParticlePreviewModelView();

    //Flags
    void SetFlag(PreviewModelViewFlag flag) override;
    void UnSetFlag(PreviewModelViewFlag flag) override;

    //Resets
    void ResetSplineSetting();
    void ResetGizmoSetting();
    void ResetCamera() override;
    void ResetAll() override;

    void SetSplineMode(SplineMode mode);
    void SetSplineSpeedMultiplier(unsigned char multiplier);

    SplineMode GetSplineMode() const;
    unsigned int GetSplineSpeedMultiplier() const;

    bool HasValidParticleEmitter() const;
    bool IsParticleEmitterAlive() const;
    unsigned int GetAliveParticleCount() const;
    float GetParticleEffectMaxLifeTime() const;
    float GetCurrentParticleEffectAge() const;

    void RestartSplineMovement();
    void ForceParticleEmitterRestart();
    void LoadParticleEffect(IParticleEffect* pEffect, CParticleItem* particle, SLodInfo* lod = nullptr);
    const AABB& GetEmitterAABB();

    //QViewportConsumer
    void OnViewportRender(const SRenderContext& rc) override;
    void OnViewportMouse(const SMouseEvent& ev) override;

    //IEditorNotifyListener
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    //clears previewer information
    //use when no preview should be displayed
    void ClearEmitter();
private:
    static const char AXIS_X_ID = 0;
    static const char AXIS_Y_ID = 1;
    static const char AXIS_Z_ID = 2;

    //Update
    void UpdateEmitterMatrix();
    void UpdateEmitter();
    void UpdateSplineMovement();
    void UpdateGizmo(const SMouseEvent& ev);
    void UpdateMoveGizmo(HitContext& hc, const Matrix34& mat);
    void UpdateRotateGizmo(HitContext& hc, const Matrix34& mat);

    //Render
    void RenderEmitters(SRendParams& rendParams, SRenderingPassInfo& passInfo);
    void RenderEmitterShape(DisplayContext& dc, const Matrix34& mat);
    void RenderEmitterShapeAngle(DisplayContext& dc, const Vec3& pos, const Matrix34& mat, float radius);
    void RenderEmitterShapeBox(DisplayContext& dc, const Vec3& center, const float x, const float y, const float z);
    void RenderEmitterShapeSphere(DisplayContext& dc, const Vec3& pos, const float radius);
    void RenderSplineVisualization(DisplayContext& dc);
    virtual void RenderMoveGizmo(DisplayContext& dc, const Matrix34& mat);
    virtual void RenderRotateGizmo(DisplayContext& dc, const Matrix34& mat);

    //Render Helper
    Vec3 GetPointOnArc(const Vec3& center, float radius, float Degrees, const Vec3& fixedAxis);
    void DrawLineWithArrow(DisplayContext& dc, const Vec3& startPos = Vec3(0, 0, 0), const float length = 1, const float offset = 0.0f, const Vec3 direction = Vec3(1.0f, 0.0f, 0.0f));

    //Gizmo mouse handler
    void SelectMouseHandler(const SMouseEvent& ev);
    void UpdateMouseHandler(const SMouseEvent& ev);
    void ReleaseMouseHandler(const SMouseEvent& ev);

    //Misc
    void ReleaseEmitter();
    float FindMaxLifeTime(const IParticleEffect* effect) const;
    void FindMaxLifeTimeRecurse(const IParticleEffect* effect, float& inOut) const;
    void FocusOnParticle();

    //Emitter
    IParticleEmitter* m_pEmitter = nullptr;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Matrix34 m_EmitterDefault = IDENTITY;
    Matrix34 m_EmitterSplineOffset = IDENTITY;
    Matrix34 m_EmitterGizmoOffset = IDENTITY;
    Matrix34 m_EmitterCurrent = IDENTITY;
    AABB m_EmitterAABB; //Using this to cache off the last valid bounding box for the emitter.
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    EmitterShape m_EmitterShape = EmitterShape::UNKNOWN;

    CAxisHelper* m_pMoveGizmo = nullptr;
    RotationDrawHelper::Axis* m_pRotateAxes = nullptr;
    IGizmoMouseDragHandler* m_pMouseDragHandler = nullptr;
    char m_highlightedRotAxis = -1;

    unsigned int m_AliveParticleCount = 0;
    bool m_IsFirstEffectLoad = true;

    //Spline
    SplineMode m_SplineMode = SplineMode::NONE;
    float m_SplineTimer = 0.0f;
    float m_SplineEmitterBoundRadiusAtStart = 1.0f;
    unsigned int m_SplineSpeedMultiplier = 1;
    bool m_IsSplinePlayForward = true;

    bool m_IsRequestingMenu = false;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Vec2i m_RightClickMousePos;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

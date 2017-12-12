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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINMODELVIEWPORT_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINMODELVIEWPORT_H
#pragma once

#include "ModelViewport.h"
#include "MannequinBase.h"
#include "ICryMannequinEditor.h"
#include "SequencerDopeSheetBase.h"
#include "Util/ArcBall.h"
#include "RenderHelpers/AxisHelper.h"

class IActionController;
class CMannequinDialog;
class CCharacterPropertiesDlg;

class CMannequinModelViewport
    : public CModelViewport
    , public IParticleEffectListener
    , public IMannequinGameListener
    , public CActionInputHandler
{
public:

    enum ELocatorMode
    {
        LM_Translate,
        LM_Rotate,
    };

    enum EMannequinViewMode
    {
        eVM_Unknown = 0,
        eVM_FirstPerson = 1,
        eVM_ThirdPerson = 2
    };

    CMannequinModelViewport(EMannequinEditorMode editorMode = eMEM_FragmentEditor, QWidget* parent = nullptr);
    virtual ~CMannequinModelViewport();

    virtual void Update();
    void UpdateAnimation(float timePassed);

    virtual void OnRender();

    void SetActionController(IActionController* pActionController)
    {
        m_pActionController = pActionController;
    }

    void ToggleCamera()
    {
        mv_AttachCamera = !mv_AttachCamera;

        CCamera oldCamera(GetCamera());
        SetCamera(m_alternateCamera);
        m_alternateCamera = oldCamera;
    }

    bool IsCameraAttached() const { return mv_AttachCamera; }

    void ClearLocators();
    void AddLocator(uint32 refID, const char* name, const QuatT& transform, IEntity* pEntity = NULL, int16 refJointId = -1, IAttachment* pAttachment = NULL, uint32 paramCRC = 0, string helperName = "");

    const QuatTS& GetPhysicalLocation() const
    {
        return m_PhysicalLocation;
    }

    virtual void DrawCharacter(ICharacterInstance* pInstance, const SRendParams& rRP, const SRenderingPassInfo& passInfo);

    //--- Override base level as we don't want the animation control there
    void SetPlaybackMultiplier(float multiplier)
    {
        m_bPaused = (multiplier == 0.0f);
        m_playbackMultiplier = multiplier;
    }

    void OnScrubTime(float timePassed);
    void OnSequenceRestart(float timePassed);
    void UpdateDebugParams();

    void OnLoadPreviewFile()
    {
        if (m_attachCameraToEntity)
        {
            m_attachCameraToEntity = NULL;
            AttachToEntity();
        }

        m_viewmode = eVM_Unknown;
        m_pHoverBaseObject = NULL;
        update();
    }

    void ClearCharacters()
    {
        m_entityList.resize(0);
    }

    void AddCharacter(IEntity* entity, const QuatT& startPosition)
    {
        m_entityList.push_back(SCharacterEntity(entity, startPosition));
    }

    static int OnPostStepLogged(const EventPhys* pEvent);

    void OnCreateEmitter(IParticleEmitter* pEmitter, QuatTS const& qLoc, const IParticleEffect* pEffect, uint32 uEmitterFlags) override;
    void OnDeleteEmitter(IParticleEmitter* pEmitter) override;
    void OnSpawnParticleEmitter(IParticleEmitter* pEmitter, IActionController& actionController) override;
    void SetTimelineUnits(ESequencerTickMode mode);

    bool ToggleLookingAtCamera() { (m_lookAtCamera = !m_lookAtCamera); return m_lookAtCamera; }
    void SetLocatorRotateMode() {   m_locMode = LM_Rotate; }
    void SetLocatorTranslateMode() { m_locMode = LM_Translate; }
    void SetShowSceneRoots(bool showSceneRoots)  { m_showSceneRoots = showSceneRoots; }
    void AttachToEntity()
    {
        if (m_entityList.size() > 0)
        {
            m_attachCameraToEntity = m_entityList[0].GetEntity();
            if (m_attachCameraToEntity != NULL)
            {
                m_lastEntityPos = m_attachCameraToEntity->GetPos();
            }
        }
    }
    void DetachFromEntity() { m_attachCameraToEntity = NULL; }

    void Focus(AABB& pBoundingBox);

    bool IsLookingAtCamera() const { return m_lookAtCamera; }
    bool IsTranslateLocatorMode() const { return (LM_Translate == m_locMode); }
    bool IsAttachedToEntity() const { return NULL != m_attachCameraToEntity; }
    bool IsShowingSceneRoots() const { return m_showSceneRoots; }

    // ModelViewportCE
    enum ViewModeCE
    {
        NothingMode = 0,
        SelectMode,
        MoveMode,
        RotateMode,
        ScrollZoomMode,
        ScrollMode,
        ZoomMode,
    };

    struct ClosestPoint
    {
        uint32 m_nBaseModel;
        uint32 m_nAttachmentIdx;
        uint32 m_nJointIdx;
        f32 m_fDistance;
        ClosestPoint()
        {
            m_nBaseModel = 0;
            m_nAttachmentIdx = 0xffffffff;
            m_nJointIdx = 0xffffffff;
            m_fDistance = 9999999.0f;
        }
    };

    virtual void DrawGrid(const Quat& tmRotation, const Vec3& MotionTranslation, const Vec3& FootSlide, const Matrix33& rGridRot);
    void DrawMoveTool(IRenderAuxGeom* pAuxGeom);
    void LoadObject(const QString& fileName, float scale = 1.f);
    f32 Picking_BaseMesh(const Ray& mray);
    f32 Picking_AttachedMeshes(const Ray& mray, IAttachment* pIAttachment, const Matrix34& m34);
    bool UpdateOrbitPosition();

    ViewModeCE m_opMode;
    int32 m_MouseOnAttachment;
    QRect m_WinRect;
    HitContext m_HitContext;
    int32 m_highlightedBoneID;
    CArcBall3D m_ArcBall;
    int32 m_Button_MOVE;
    int32 m_Button_ROTATE;
    int32 m_Button_IK;
    IAnimationGroundAlignmentPtr m_groundAlignment;
    CMaterial* m_pDefaultMaterial;
    int32 m_SelectedAttachment;
    int32 m_SelectionUpdate;
    int32 m_MouseOnBoneID;
    bool m_animEventPosition;
    CAxisHelper m_AxisHelper;
    ClosestPoint m_ClosestPoint;
    bool m_MouseButtonL;
    bool m_MouseButtonR;

protected:

    bool UseAnimationDrivenMotionForEntity(const IEntity* piEntity);
    void SetFirstperson(IAttachmentManager* pAttachmentManager, EMannequinViewMode viewmode);

    bool HitTest(HitContext& hc, const bool bIsClick);

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // ModelViewportCE
    void CELButtonDown(QPoint point);
    void CELButtonUp(QPoint point);
    void CEMButtonDown(QMouseEvent* event, QPoint point);

private:
    void UpdateCharacter(IEntity* pEntity, ICharacterInstance* pInstance, float deltaTime);
    void DrawCharacter(IEntity* pEntity, ICharacterInstance* pInstance, IStatObj* pStatObj, const SRendParams& rRP, const SRenderingPassInfo& passInfo);
    void DrawEntityAndChildren(class CEntityObject* pEntityObject, const SRendParams& rp, const SRenderingPassInfo& passInfo);
    void UpdatePropEntities(SMannequinContexts* pContexts, SMannequinContexts::SProp& prop);

    struct SLocator
    {
        uint32       m_refID;
        QString      m_name;
        CArcBall3D   m_ArcBall;
        CAxisHelper  m_AxisHelper;
        IEntity*     m_pEntity;
        _smart_ptr<IAttachment> m_pAttachment;
        string           m_helperName;
        int16        m_jointId;
        uint32           m_paramCRC;
    };

    inline Matrix34 GetLocatorReferenceMatrix(const SLocator& locator);
    inline Matrix34 GetLocatorWorldMatrix(const SLocator& locator);

    // ModelViewportCE
    bool RepositionMoveTool(const QPoint& point);

    ELocatorMode m_locMode;

    struct SCharacterEntity
    {
        SCharacterEntity(IEntity* _entity = NULL, const QuatT& _startLocation = QuatT(IDENTITY))
            : entityId(_entity ? _entity->GetId() : 0)
            , startLocation(_startLocation)
        {}
        EntityId entityId;
        QuatT startLocation;

        IEntity* GetEntity() const { return gEnv->pEntitySystem->GetEntity(entityId); }
    };
    std::vector<SCharacterEntity> m_entityList;

    std::vector<IParticleEmitter*> m_particleEmitters;

    std::vector<SLocator> m_locators;
    uint32 m_selectedLocator;
    EMannequinViewMode m_viewmode;
    bool m_draggingLocator;
    QPoint m_dragStartPoint;
    bool m_LeftButtonDown;
    bool m_lookAtCamera;
    bool m_showSceneRoots;
    bool m_cameraKeyDown;
    float m_playbackMultiplier;
    Vec3 m_tweenToFocusStart;
    Vec3 m_tweenToFocusDelta;
    float m_tweenToFocusTime;
    static const float s_maxTweenTime;

    EMannequinEditorMode m_editorMode;

    IActionController* m_pActionController;

    IPhysicalEntity* m_piGroundPlanePhysicalEntity;

    ESequencerTickMode m_TickerMode;

    IEntity* m_attachCameraToEntity;
    Vec3 m_lastEntityPos;

    CCamera m_alternateCamera;

    CBaseObject* m_pHoverBaseObject;
};


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINMODELVIEWPORT_H

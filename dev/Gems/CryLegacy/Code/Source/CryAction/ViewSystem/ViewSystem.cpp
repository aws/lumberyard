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

#include "CryLegacy_precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include <Cry_Camera.h>
#include <ILevelSystem.h>
#include "ViewSystem.h"
#include "GameObjects/GameObject.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include "PNoise3.h"
#include <IAISystem.h>
#include <DebugCamera/DebugCamera.h>

#define VS_CALL_LISTENERS(func)                                                                                     \
    {                                                                                                               \
        size_t count = m_listeners.size();                                                                          \
        if (count > 0)                                                                                              \
        {                                                                                                           \
            const size_t memSize = count * sizeof(IViewSystemListener*);                                            \
            PREFAST_SUPPRESS_WARNING(6255) IViewSystemListener * *pArray = (IViewSystemListener**) alloca(memSize); \
            memcpy(pArray, &*m_listeners.begin(), memSize);                                                         \
            while (count--)                                                                                         \
            {                                                                                                       \
                (*pArray)->func; ++pArray;                                                                          \
            }                                                                                                       \
        }                                                                                                           \
    }

//------------------------------------------------------------------------
CViewSystem::CViewSystem(ISystem* pSystem)
    : m_pSystem(pSystem)
    , m_activeViewId(0)
    , m_nextViewIdToAssign(1000)
    , m_preSequenceViewId(0)
    , m_cutsceneViewId(0)
    , m_cutsceneCount(0)
    , m_bOverridenCameraRotation(false)
    , m_bActiveViewFromSequence(false)
    , m_fBlendInPosSpeed(0.0f)
    , m_fBlendInRotSpeed(0.0f)
    , m_bPerformBlendOut(false)
    , m_useDeferredViewSystemUpdate(false)
    , m_bControlsAudioListeners(true)
{
    REGISTER_CVAR2("cl_camera_noise", &m_fCameraNoise, -1, 0,
        "Adds hand-held like camera noise to the camera view. \n The higher the value, the higher the noise.\n A value <= 0 disables it.");
    REGISTER_CVAR2("cl_camera_noise_freq", &m_fCameraNoiseFrequency, 2.5326173f, 0,
        "Defines camera noise frequency for the camera view. \n The higher the value, the higher the noise.");

    REGISTER_CVAR2("cl_ViewSystemDebug", &m_nViewSystemDebug, 0, VF_CHEAT,
        "Sets Debug information of the ViewSystem.");

    REGISTER_CVAR2("cl_DefaultNearPlane", &m_fDefaultCameraNearZ, DEFAULT_NEAR, VF_CHEAT,
        "The default camera near plane. ");

    //Register as level system listener
    if (CCryAction::GetCryAction()->GetILevelSystem())
    {
        CCryAction::GetCryAction()->GetILevelSystem()->AddListener(this);
    }

    Camera::CameraSystemRequestBus::Handler::BusConnect();
}

//------------------------------------------------------------------------
CViewSystem::~CViewSystem()
{
    Camera::CameraSystemRequestBus::Handler::BusDisconnect();

    ClearAllViews();

    IConsole* pConsole = gEnv->pConsole;
    CRY_ASSERT(pConsole);
    pConsole->UnregisterVariable("cl_camera_noise", true);
    pConsole->UnregisterVariable("cl_camera_noise_freq", true);
    pConsole->UnregisterVariable("cl_ViewSystemDebug", true);
    pConsole->UnregisterVariable("cl_DefaultNearPlane", true);

    //Remove as level system listener
    if (CCryAction::GetCryAction()->GetILevelSystem())
    {
        CCryAction::GetCryAction()->GetILevelSystem()->RemoveListener(this);
    }
}

//------------------------------------------------------------------------
void CViewSystem::Update(float frameTime)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    if (gEnv->IsDedicated())
    {
        return;
    }

#ifndef _RELEASE
    DebugCamera* debugCamera = CCryAction::GetCryAction()->GetDebugCamera();
    if (debugCamera)
    {
        debugCamera->Update();
    }
#endif // !_RELEASE

    CView* const pActiveView = static_cast<CView*>(GetActiveView());

    TViewMap::const_iterator Iter(m_views.begin());
    TViewMap::const_iterator const IterEnd(m_views.end());

    for (; Iter != IterEnd; ++Iter)
    {
        IView* const pView = Iter->second;

        bool const bIsActive = (pView == pActiveView);

        pView->Update(frameTime, bIsActive);

        if (bIsActive)
        {
            CCamera& rCamera = pView->GetCamera();
#ifndef _RELEASE
            if (!debugCamera || (debugCamera && !debugCamera->IsEnabled()))
#endif // !_RELEASE
            {
                pView->UpdateAudioListener(rCamera.GetMatrix());
            }

            if (const SViewParams* currentParams = pView->GetCurrentParams())
            {
                SViewParams copyCurrentParams = *currentParams;
                rCamera.SetJustActivated(copyCurrentParams.justActivated);

                copyCurrentParams.justActivated = false;
                pView->SetCurrentParams(copyCurrentParams);
            }

            if (m_bOverridenCameraRotation)
            {
                // When camera rotation is overridden.
                Vec3 pos = rCamera.GetMatrix().GetTranslation();
                Matrix34 camTM(m_overridenCameraRotation);
                camTM.SetTranslation(pos);
                rCamera.SetMatrix(camTM);
            }
            else
            {
                // Normal setting of the camera

                if (m_fCameraNoise > 0)
                {
                    Matrix33 m = Matrix33(rCamera.GetMatrix());
                    m.OrthonormalizeFast();
                    Ang3 aAng1 = Ang3::GetAnglesXYZ(m);
                    //Ang3 aAng2 = RAD2DEG(aAng1);

                    Matrix34 camTM = rCamera.GetMatrix();
                    Vec3 pos = camTM.GetTranslation();
                    camTM.SetIdentity();

                    const float fScale = 0.1f;
                    CPNoise3* pNoise = m_pSystem->GetNoiseGen();
                    float fRes = pNoise->Noise1D(gEnv->pTimer->GetCurrTime() * m_fCameraNoiseFrequency);
                    aAng1.x += fRes * m_fCameraNoise * fScale;
                    pos.z -= fRes * m_fCameraNoise * fScale;
                    fRes = pNoise->Noise1D(17 + gEnv->pTimer->GetCurrTime() * m_fCameraNoiseFrequency);
                    aAng1.y -= fRes * m_fCameraNoise * fScale;

                    //aAng1.z+=fRes*0.025f; // left / right movement should be much less visible

                    camTM.SetRotationXYZ(aAng1);
                    camTM.SetTranslation(pos);
                    rCamera.SetMatrix(camTM);
                }
            }

            m_pSystem->SetViewCamera(rCamera);
        }
    }

#ifndef _RELEASE
    if (debugCamera)
    {
        debugCamera->PostUpdate();
    }
#endif // !_RELEASE

    // Display debug info on screen
    if (m_nViewSystemDebug)
    {
        DebugDraw();
    }

    // perform dynamic color grading
    // Beni - Commented for console demo stuff
    /******************************************************************************************
    if (!IsPlayingCutScene() && gEnv->pAISystem)
    {
        SAIDetectionLevels detection;
        gEnv->pAISystem->GetDetectionLevels(NULL, detection);
        float factor = detection.puppetThreat;

        // marcok: magic numbers taken from Maciej's tweaked values (final - initial)
        {
            float percentage = (0.0851411f - 0.0509998f)/0.0509998f * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_GrainAmount", amount);
            gEnv->p3DEngine->SetPostEffectParam("ColorGrading_GrainAmount_Offset", amount*percentage);
        }
        {
            float percentage = (0.405521f - 0.256739f)/0.256739f * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_SharpenAmount", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_SharpenAmount_Offset", amount*percentage);
        }
        {
            float percentage = (0.14f - 0.11f)/0.11f * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_PhotoFilterColorDensity", amount);
            gEnv->p3DEngine->SetPostEffectParam("ColorGrading_PhotoFilterColorDensity_Offset", amount*percentage);
        }
        {
            float percentage = (234.984f - 244.983f)/244.983f * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_maxInput", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_maxInput_Offset", amount*percentage);
        }
        {
            float percentage = (239.984f - 247.209f)/247.209f * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_maxOutput", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_maxOutput_Offset", amount*percentage);
        }
        {
            Vec4 dest(0.0f/255.0f, 22.0f/255.0f, 33.0f/255.0f, 1.0f);
            Vec4 initial(2.0f/255.0f, 154.0f/255.0f, 226.0f/255.0f, 1.0f);
            Vec4 percentage = (dest - initial)/initial * factor;
            Vec4 amount;
            gEnv->p3DEngine->GetPostEffectParamVec4("clr_ColorGrading_SelectiveColor", amount);
            //gEnv->p3DEngine->SetPostEffectParamVec4("clr_ColorGrading_SelectiveColor_Offset", amount*percentage);
        }
        {
            float percentage = (-5.0083 - -1.99999)/-1.99999 * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_SelectiveColorCyans", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_SelectiveColorCyans_Offset", amount*percentage);
        }
        {
            float percentage = (10.0166 - -5.99999)/-5.99999 * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_SelectiveColorMagentas", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_SelectiveColorMagentas_Offset", amount*percentage);
        }
        {
            float percentage = (10.0166 - 1.99999)/1.99999 * factor;
            float amount = 0.0f;
            gEnv->p3DEngine->GetPostEffectParam("ColorGrading_SelectiveColorYellows", amount);
            //gEnv->p3DEngine->SetPostEffectParam("ColorGrading_SelectiveColorYellows_Offset", amount*percentage);
        }
    }
    ********************************************************************************************/
}

//------------------------------------------------------------------------
IView* CViewSystem::CreateView()
{
    CView* newView = new CView(m_pSystem);

    if (newView)
    {
        AddView(newView);
    }

    return newView;
}

unsigned int CViewSystem::AddView(IView* pView)
{
    assert(pView);

    m_views.insert(TViewMap::value_type(m_nextViewIdToAssign, pView));
    return m_nextViewIdToAssign++;
}

void CViewSystem::RemoveView(IView* pView)
{
    RemoveViewById(GetViewId(pView));
}

void CViewSystem::RemoveView(unsigned int viewId)
{
    RemoveViewById(viewId);
}

void CViewSystem::RemoveViewById(unsigned int viewId)
{
    TViewMap::iterator iter = m_views.find(viewId);

    if (iter != m_views.end())
    {
        if (viewId == m_activeViewId)
        {
            m_activeViewId = 0;
        }
        if (viewId == m_preSequenceViewId)
        {
            m_preSequenceViewId = 0;
        }
        SAFE_RELEASE(iter->second);
        m_views.erase(iter);
    }
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveView(IView* pView)
{
    if (pView != NULL)
    {
        IView* const pPrevView = GetView(m_activeViewId);

        if (pPrevView != pView)
        {
            if (pPrevView != NULL)
            {
                pPrevView->SetActive(false);
            }

            pView->SetActive(true);
            m_activeViewId = GetViewId(pView);
        }
    }
    else
    {
        m_activeViewId = ~0;
    }

    m_bActiveViewFromSequence = false;
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveView(unsigned int viewId)
{
    IView* const pPrevView = GetView(m_activeViewId);

    if (pPrevView != NULL)
    {
        pPrevView->SetActive(false);
    }

    IView* const pView = GetView(viewId);

    if (pView != NULL)
    {
        pView->SetActive(true);
        m_activeViewId = viewId;
        m_bActiveViewFromSequence = false;
    }
}

//------------------------------------------------------------------------
IView* CViewSystem::GetView(unsigned int viewId)
{
    TViewMap::iterator it = m_views.find(viewId);

    if (it != m_views.end())
    {
        return it->second;
    }

    return NULL;
}

//------------------------------------------------------------------------
IView* CViewSystem::GetActiveView()
{
    return GetView(m_activeViewId);
}

//------------------------------------------------------------------------
unsigned int CViewSystem::GetViewId(IView* pView)
{
    for (TViewMap::iterator it = m_views.begin(); it != m_views.end(); ++it)
    {
        IView* tView = it->second;

        if (tView == pView)
        {
            return it->first;
        }
    }

    return 0;
}

//------------------------------------------------------------------------
unsigned int CViewSystem::GetActiveViewId()
{
    // cutscene can override the games id of the active view
    if (m_cutsceneCount && m_cutsceneViewId)
    {
        return m_cutsceneViewId;
    }
    return m_activeViewId;
}

//------------------------------------------------------------------------
IView* CViewSystem::GetViewByEntityId(const AZ::EntityId& id, bool forceCreate)
{
    for (TViewMap::iterator it = m_views.begin(); it != m_views.end(); ++it)
    {
        IView* tView = it->second;

        if (tView && tView->GetLinkedId() == id)
        {
            return tView;
        }
    }

    if (forceCreate)
    {
        if (IsLegacyEntityId(id))
        {
            // Legacy Camera
            if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(id)))
            {
                if (CGameObjectPtr pGameObject = pEntity->GetComponent<CGameObject>())
                {
                    if (IView* pNew = CreateView())
                    {
                        pNew->LinkTo(pGameObject.get());
                        return pNew;
                    }
                }
                else
                {
                    if (IView* pNew = CreateView())
                    {
                        pNew->LinkTo(pEntity);
                        return pNew;
                    }
                }
            }
        }
        else
        {
            // Component Camera
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
            if (entity)
            {
                if (IView* pNew = CreateView())
                {
                    pNew->LinkTo(entity);
                    return pNew;
                }
            }          
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
void CViewSystem::SetActiveCamera(const SCameraParams& params)
{
    IView* pView = NULL;

    if (params.cameraEntityId.IsValid())
    {
        pView = GetViewByEntityId(params.cameraEntityId, true);
        if (pView)
        {
            SViewParams viewParams = *pView->GetCurrentParams();
            viewParams.fov = params.fov;
            viewParams.nearplane = params.nearZ;

            if (m_bActiveViewFromSequence == false && m_preSequenceViewId == 0)
            {
                m_preSequenceViewId = m_activeViewId;
                IView* pPrevView = GetView(m_activeViewId);
                if (pPrevView && m_fBlendInPosSpeed > 0.0f && m_fBlendInRotSpeed > 0.0f)
                {
                    viewParams.blendPosSpeed = m_fBlendInPosSpeed;
                    viewParams.blendRotSpeed = m_fBlendInRotSpeed;
                    viewParams.BlendFrom(*pPrevView->GetCurrentParams());
                }
            }

            if (m_activeViewId != GetViewId(pView) && params.justActivated)
            {
                viewParams.justActivated = true;
            }

            pView->SetCurrentParams(viewParams);
            // make this one the active view
            SetActiveView(pView);
            m_bActiveViewFromSequence = true;
        }
    }
    else
    {
        if (m_preSequenceViewId != 0)
        {
            // Restore m_preSequenceViewId view

            IView* pActiveView = GetView(m_activeViewId);
            IView* pNewView = GetView(m_preSequenceViewId);
            if (pActiveView && pNewView && m_bPerformBlendOut)
            {
                SViewParams activeViewParams = *pActiveView->GetCurrentParams();
                SViewParams newViewParams = *pNewView->GetCurrentParams();
                newViewParams.BlendFrom(activeViewParams);
                newViewParams.blendPosSpeed = activeViewParams.blendPosSpeed;
                newViewParams.blendRotSpeed = activeViewParams.blendRotSpeed;

                if (m_activeViewId != m_preSequenceViewId && params.justActivated)
                {
                    newViewParams.justActivated = true;
                }

                pNewView->SetCurrentParams(newViewParams);
                SetActiveView(m_preSequenceViewId);
            }
            else if (pActiveView && m_activeViewId != m_preSequenceViewId && params.justActivated)
            {
                SViewParams activeViewParams = *pActiveView->GetCurrentParams();
                activeViewParams.justActivated = true;
                if (m_nViewSystemDebug)
                {
                    GameWarning("CViewSystem::SetActiveCamera(): %d", m_preSequenceViewId);
                    if (pNewView == NULL)
                    {
                        GameWarning("CViewSystem::SetActiveCamera(): Switching to pre-sequence view failed!");
                    }
                    else
                    {
                        GameWarning("CViewSystem::SetActiveCamera(): Switching to pre-sequence view OK!");
                    }
                }

                if (pNewView)
                {
                    pNewView->SetCurrentParams(activeViewParams);
                    SetActiveView(m_preSequenceViewId);
                }
            }

            m_preSequenceViewId = 0;
            m_bActiveViewFromSequence = false;
        }
    }
    m_cutsceneViewId = GetViewId(pView);

    VS_CALL_LISTENERS(OnCameraChange(params));
}

//------------------------------------------------------------------------
void CViewSystem::BeginCutScene(IAnimSequence* pSeq, unsigned long dwFlags, bool bResetFX)
{
    m_cutsceneCount++;

    IConsole* pCon = gEnv->pConsole;

    if (m_cutsceneCount == 1)
    {
        gEnv->p3DEngine->ResetPostEffects();
    }

    VS_CALL_LISTENERS(OnBeginCutScene(pSeq, bResetFX));

    /* TODO: how do we pause the game
        IScriptSystem *pSS=m_pGame->GetScriptSystem();
        _SmartScriptObject pClientStuff(pSS,true);
        if(pSS->GetGlobalValue("ClientStuff",pClientStuff)){
            pSS->BeginCall("ClientStuff","OnPauseGame");
            pSS->PushFuncParam(pClientStuff);
            pSS->EndCall();
        }
    */

    /* TODO: how do we block keys
        // do not allow the player to mess around with player's keys
        // during a cutscene
        GetISystem()->GetIInput()->GetIKeyboard()->ClearKeyState();
        m_pGame->m_pIActionMapManager->SetActionMap("player_dead");
        m_pGame->AllowQuicksave(false);
    */


    /* TODO: how do we pause sounds?
        // Sounds are not stopped or cut automatically, code needs to listen to cutscene begin/end event
        // Cutscenes have dedicated foley sounds so to lower the volume of the rest a soundmood is used
    */

    // Audio: notify the audio system?

    /* TODO: how do we reset FX?
        if (bResetFx)
        {
            m_pGame->m_p3DEngine->ResetScreenFx();
            ICVar *pResetScreenEffects=pCon->GetCVar("r_ResetScreenFx");
            if(pResetScreenEffects)
            {
            pResetScreenEffects->Set(1);
            }
        }
    */

    //  if(gEnv->p3DEngine)
    //  gEnv->p3DEngine->ProposeContentPrecache();
}

//------------------------------------------------------------------------
void CViewSystem::EndCutScene(IAnimSequence* pSeq, unsigned long dwFlags)
{
    m_cutsceneCount -= (m_cutsceneCount > 0);

    IConsole* pCon = gEnv->pConsole;

    if (m_cutsceneCount == 0)
    {
        gEnv->p3DEngine->ResetPostEffects();
    }

    ClearCutsceneViews();

    VS_CALL_LISTENERS(OnEndCutScene(pSeq));

    // TODO: reimplement
    //  m_pGame->AllowQuicksave(true);

    /* TODO: how to resume game
        if (!m_pGame->IsServer())
        {
            IScriptSystem *pSS=m_pGame->GetScriptSystem();
            _SmartScriptObject pClientStuff(pSS,true);
            if(pSS->GetGlobalValue("ClientStuff",pClientStuff)){
                pSS->BeginCall("ClientStuff","OnResumeGame");
                pSS->PushFuncParam(pClientStuff);
                pSS->EndCall();
            }
        }
    */

    /* TODO: how do we pause sounds?
    // Sounds are not stopped or cut automatically, code needs to listen to cutscene begin/end event
    // Cutscenes have dedicated foley sounds so to lower the volume of the rest a soundmood is used

    if (m_bSoundsPaused)
    {
    }
*/

    // Audio: notify the audio system?

    /* TODO: resolve input difficulties
        m_pGame->m_pIActionMapManager->SetActionMap("default");
        GetISystem()->GetIInput()->GetIKeyboard()->ClearKeyState();
    */

    /* TODO: weird game related stuff
        // we regenerate stamina fpr the local payer on cutsceen end - supposendly he was idle long enough to get it restored
        if (m_pGame->GetMyPlayer())
        {
            CPlayer *pPlayer;
            if (m_pGame->GetMyPlayer()->GetContainer()->QueryContainerInterface(CIT_IPLAYER,(void**)&pPlayer))
            {
                pPlayer->m_stats.stamina = 100;
            }
        }

        // reset subtitles
        m_pGame->m_pClient->ResetSubtitles();
    */
}

void CViewSystem::SendGlobalEvent(const char* pszEvent)
{
    // TODO: broadcast to flowgraph/script system
}

//////////////////////////////////////////////////////////////////////////
void CViewSystem::SetOverrideCameraRotation(bool bOverride, Quat rotation)
{
    m_bOverridenCameraRotation = bOverride;
    m_overridenCameraRotation = rotation;
}

//////////////////////////////////////////////////////////////////////////
void CViewSystem::UpdateSoundListeners()
{
    assert(gEnv->IsEditor() && !gEnv->IsEditorGameMode());

    // In Editor we may want to control global listeners outside of the game view.
    if (m_bControlsAudioListeners)
    {
        IView* const pActiveView = static_cast<IView*>(GetActiveView());
        TViewMap::const_iterator Iter(m_views.begin());
        TViewMap::const_iterator const IterEnd(m_views.end());

        for (; Iter != IterEnd; ++Iter)
        {
            IView* const pView = Iter->second;
            bool const bIsActive = (pView == pActiveView);
            CCamera const& rCamera = bIsActive ? gEnv->pSystem->GetViewCamera() : pView->GetCamera();
            pView->UpdateAudioListener(rCamera.GetMatrix());
        }
    }
}

//////////////////////////////////////////////////////////////////
void CViewSystem::OnLoadingStart(ILevelInfo* pLevel)
{
    //If the level is being restarted (IsSerializingFile() == 1)
    //views should not be cleared, because the main view (player one) won't be recreated in this case
    //Views will only be cleared when loading a new map, or loading a saved game (IsSerizlizingFile() == 2)
    bool shouldClearViews = gEnv->pSystem ? (gEnv->pSystem->IsSerializingFile() != 1) : false;

    if (shouldClearViews)
    {
        ClearAllViews();
    }
}

/////////////////////////////////////////////////////////////////////
void CViewSystem::OnUnloadComplete(ILevel* pLevel)
{
    bool shouldClearViews = gEnv->pSystem ? (gEnv->pSystem->IsSerializingFile() != 1) : false;

    if (shouldClearViews)
    {
        ClearAllViews();
    }

    assert(m_listeners.empty());
    stl::free_container(m_listeners);
}

/////////////////////////////////////////////////////////////////////
void CViewSystem::ClearCutsceneViews()
{
    //First switch to previous camera if available
    //In practice, the camera should be already restored before reaching this point, but just in case.
    if (m_preSequenceViewId != 0)
    {
        SCameraParams camParams;
        camParams.cameraEntityId.SetInvalid();  //Setting to invalid will try to switch to previous camera
        camParams.fov = 60.0f;
        camParams.nearZ = DEFAULT_NEAR;
        camParams.justActivated = true;
        SetActiveCamera(camParams);
    }
}

///////////////////////////////////////////
void CViewSystem::ClearAllViews()
{
    TViewMap::iterator end = m_views.end();
    for (TViewMap::iterator it = m_views.begin(); it != end; ++it)
    {
        SAFE_RELEASE(it->second);
    }
    m_views.clear();
    m_preSequenceViewId = 0;
    m_activeViewId = 0;
}

////////////////////////////////////////////////////////////////////
void CViewSystem::DebugDraw()
{
    IRenderer* pRenderer = gEnv->pRenderer;
    if (pRenderer)
    {
        float xpos = 20;
        float ypos = 15;
        float fColor[4]             = {1.0f, 1.0f, 1.0f, 0.7f};
        float fColorRed[4]      = {1.0f, 0.0f, 0.0f, 0.7f};
        float fColorGreen[4]    = {0.0f, 1.0f, 0.0f, 0.7f};

        pRenderer->Draw2dLabel(xpos, 5, 1.35f, fColor, false, "ViewSystem Stats: %" PRISIZE_T " Views ", m_views.size());

        IView* pActiveView = GetActiveView();
        for (TViewMap::iterator it = m_views.begin(); it != m_views.end(); ++it)
        {
            IView* pView = it->second;
            const CCamera& cam = pView->GetCamera();
            bool isActive = (pView == pActiveView);
            Vec3 pos = cam.GetPosition();
            Ang3 ang = cam.GetAngles();
            pRenderer->Draw2dLabel(xpos, ypos, 1.35f, isActive ? fColorGreen : fColorRed, false, "View Camera: %p . View Id: %d, pos (%f, %f, %f), ang (%f, %f, %f)", &cam, it->first, pos.x, pos.y, pos.z, ang.x, ang.y, ang.z);

            ypos += 11;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewSystem::GetMemoryUsage(ICrySizer* s) const
{
    SIZER_SUBCOMPONENT_NAME(s, "ViewSystem");
    s->Add(*this);
    s->AddContainer(m_views);
}

void CViewSystem::Serialize(TSerialize ser)
{
    TViewMap::iterator iter = m_views.begin();
    TViewMap::iterator iterEnd = m_views.end();
    while (iter != iterEnd)
    {
        iter->second->Serialize(ser);
        ++iter;
    }
}

void CViewSystem::PostSerialize()
{
    TViewMap::iterator iter = m_views.begin();
    TViewMap::iterator iterEnd = m_views.end();
    while (iter != iterEnd)
    {
        iter->second->PostSerialize();
        ++iter;
    }
}

///////////////////////////////////////////////////////////////////////////
void CViewSystem::SetControlAudioListeners(bool bActive)
{
    m_bControlsAudioListeners = bActive;

    TViewMap::const_iterator Iter(m_views.begin());
    TViewMap::const_iterator const IterEnd(m_views.end());

    for (; Iter != IterEnd; ++Iter)
    {
        Iter->second->SetActive(bActive);
    }
}



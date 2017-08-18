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
#include "StdAfx.h"
#include <IFlowSystem.h>
#include <IGameFramework.h>
#include "FlowBaseNode.h"

class CHUDFader
{
public:
    CHUDFader()
    {
        m_pRenderer = gEnv->pRenderer;
        assert (m_pRenderer != 0);

        m_currentColor = Col_Black;
        m_targetColor = Col_Black;
        m_drawColor = Col_Black;
        m_pTexture = 0;
        m_bActive = false;
        m_duration = 0.0f;
        m_curTime = 0.0f;
        m_direction = 0;
        m_ticket = 0;
        m_lastTime = 0;
    }

    virtual ~CHUDFader()
    {
        SAFE_RELEASE(m_pTexture);
    }


    const char* GetDebugName() const
    {
        if (m_pTexture)
        {
            return m_pTexture->GetName();
        }
        return "<no texture>";
    }

    void Stop()
    {
        m_bActive = false;
    }

    void Reset()
    {
        m_ticket = 0;
        m_bActive = false;
    }

    bool IsActive() const { return m_bActive; }
    bool IsPlaying(int ticket) const { return m_ticket > 0 && m_ticket == ticket; }
    ColorF GetCurrentColor()
    {
        float curVal = 1.0;
        if (m_duration != 0.0f)
        {
            curVal = m_curTime / m_duration;
            curVal = CLAMP(curVal, 0.0f, 1.0f);
        }
        ColorF col;
        col.lerpFloat(m_currentColor, m_targetColor, curVal);
        return col;
    }

    int FadeIn(const ColorF& targetColor, float fDuration, bool bUseCurrentColor = true, bool bUpdateAlways = false)
    {
        if (m_ticket == 0 || bUseCurrentColor == false)
        {
            m_currentColor = targetColor;
            m_currentColor.a = 1.0f;
        }
        else
        {
            m_currentColor = GetCurrentColor();
        }
        m_targetColor = targetColor;
        m_targetColor.a = 0.0f;
        m_duration = fDuration > 0.0f ? fDuration : 0.0f;
        m_curTime = 0.0f;
        m_direction = -1;
        m_bActive = true;
        m_bUpdateAlways = bUpdateAlways;
        m_lastTime = gEnv->pTimer->GetAsyncCurTime();
        return ++m_ticket;
    }

    int FadeOut(const ColorF& targetColor, float fDuration, const char* textureName = "", bool bUseCurrentColor = true, bool bUpdateAlways = false)
    {
        SetTexture(textureName);
        if (m_ticket == 0 || bUseCurrentColor == false)
        {
            m_currentColor = targetColor;
            m_currentColor.a = 0.0f;
        }
        else
        {
            m_currentColor = GetCurrentColor();
        }

        m_targetColor = targetColor;
        m_targetColor.a = 1.0f;
        m_duration = fDuration > 0.0 ? fDuration : 0.0f;
        m_curTime = 0.0f;
        m_direction = 1;
        m_bActive = true;
        m_bUpdateAlways = bUpdateAlways;
        m_lastTime = gEnv->pTimer->GetAsyncCurTime();
        return ++m_ticket;
    }


    virtual void Update(float fDeltaTime)
    {
        if (m_bActive == false)
        {
            return;
        }

        if (m_bUpdateAlways)
        {
            float fCurrTime = gEnv->pTimer->GetAsyncCurTime();
            fDeltaTime = fCurrTime - m_lastTime;
            m_lastTime = fCurrTime;
        }

        if (m_curTime >= m_duration)
        {
            // when we're logically supposed to fade 'in' (meaning, to go away), do so
            if (m_direction < 0)
            {
                m_bActive = false;
                m_currentColor = m_targetColor;
            }
            m_currentColor = m_targetColor;
            m_ticket = 0;
        }
        m_drawColor = GetCurrentColor();
        m_curTime += fDeltaTime;
    }

    virtual void Draw()
    {
        if (m_bActive == false)
        {
            return;
        }

        m_pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
        m_pRenderer->Draw2dImage(0.0f, 0.0f, 800.0f, 600.0f,
            m_pTexture ? m_pTexture->GetTextureID() : -1,
            0.0f, 1.0f, 1.0f, 0.0f, // tex coords
            0.0f, // angle
            m_drawColor.r, m_drawColor.g, m_drawColor.b, m_drawColor.a,
            0.0f);
    }

    static ITexture* LoadTexture(const char* textureName)
    {
        if (gEnv->pRenderer)
        {
            return gEnv->pRenderer->EF_LoadTexture(textureName, FT_DONT_STREAM);
        }
        else
        {
            return 0;
        }
    }
protected:
    void SetTexture(const char* textureName)
    {
        if (m_pTexture && textureName && strcmp(textureName, m_pTexture->GetName()) == 0)
        {
            return;
        }

        SAFE_RELEASE(m_pTexture);
        if (textureName == 0 || *textureName == 0)
        {
            return;
        }

        m_pTexture = LoadTexture(textureName);
        if (m_pTexture != 0)
        {
            m_pTexture->SetClamp(true);
        }
    }


protected:
    IRenderer* m_pRenderer;
    ColorF         m_currentColor;
    ColorF         m_targetColor;
    ColorF         m_drawColor;
    ITexture* m_pTexture;
    float          m_duration;
    float          m_curTime;
    int            m_direction;
    int            m_ticket;
    bool           m_bActive;
    bool           m_bUpdateAlways;
    float          m_lastTime;
};

static const int NUM_FADERS = 8;
static const int MFX_FADER_OFFSET = 0;
static const int MFX_FADER_END = 3;
static const int GAME_FADER_OFFSET = 4;


class CMasterFader
    : public IGameFrameworkListener
{
public:
    CMasterFader()
    {
        m_bRegistered = false;
        memset(m_pHUDFader, 0, sizeof(m_pHUDFader));
    }

    ~CMasterFader()
    {
        UnRegister();
        for (int i = 0; i < NUM_FADERS; ++i)
        {
            SAFE_DELETE(m_pHUDFader[i]);
        }
    }

    // IGameFrameworkListener
    virtual void OnPostUpdate(float fDeltaTime)
    {
        Update(fDeltaTime);
    }

    virtual void OnSaveGame(ISaveGame* pSaveGame) {}
    virtual void OnLoadGame(ILoadGame* pLoadGame) {}
    virtual void OnLevelEnd(const char* nextLevel) {}
    virtual void OnActionEvent(const SActionEvent& event)
    {
        if (event.m_event == eAE_unloadLevel)
        {
            OnHUDToBeDestroyed();
        }
    }
    // ~IGameFrameworkListener

    CHUDFader* GetHUDFader(int group)
    {
        if (m_bRegistered == false)
        {
            Register();
        }
        if (group < 0 || group >= NUM_FADERS)
        {
            return 0;
        }
        if (m_pHUDFader[group] == 0)
        {
            m_pHUDFader[group] = new CHUDFader;
        }
        return m_pHUDFader[group];
    }


    virtual void Update(float fDeltaTime)
    {
        int nActive = 0;
        for (int i = 0; i < NUM_FADERS; ++i)
        {
            if (m_pHUDFader[i])
            {
                m_pHUDFader[i]->Update(fDeltaTime);
                m_pHUDFader[i]->Draw();
                if (m_pHUDFader[i]->IsActive())
                {
                    ++nActive;
                }
            }
        }
    }

    virtual void OnHUDToBeDestroyed()
    {
        m_bRegistered = false;
        for (int i = 0; i < NUM_FADERS; ++i)
        {
            SAFE_DELETE(m_pHUDFader[i]);
        }
    }

    virtual void Serialize(TSerialize ser)
    {
        if (ser.IsReading())
        {
            for (int i = 0; i < NUM_FADERS; ++i)
            {
                if (m_pHUDFader[i])
                {
                    m_pHUDFader[i]->Reset();
                }
            }
        }
    }
    // ~CHUDObject

    void Register()
    {
        if (m_bRegistered)
        {
            return;
        }

        gEnv->pGame->GetIGameFramework()->RegisterListener(this, "HUD_Master_Fader",  FRAMEWORKLISTENERPRIORITY_HUD);
        m_bRegistered = true;
    }

    void UnRegister()
    {
        if (!m_bRegistered)
        {
            return;
        }

        gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
        m_bRegistered = false;
    }

    bool m_bRegistered;
    CHUDFader* m_pHUDFader[NUM_FADERS];
};

CMasterFader* g_pMasterFader = 0;

CHUDFader* GetHUDFader(int group)
{
    if (g_pMasterFader == 0)
    {
        g_pMasterFader = new CMasterFader();
    }
    if (g_pMasterFader)
    {
        return g_pMasterFader->GetHUDFader(group);
    }
    else
    {
        return 0;
    }
}


class CFlowFadeNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    CHUDFader* GetFader(SActivationInfo* pActInfo)
    {
        int group = GetPortInt(pActInfo, EIP_FadeGroup);
        group += m_nFaderOffset;
        return GetHUDFader(group);
    }

public:
    CFlowFadeNode(SActivationInfo* pActInfo)
        : m_bPlaying(false)
        , m_bNeedFaderStop(false)
        , m_direction(0)
        , m_ticket(0)
        , m_postSerializeTrigger(0)
    {
    }

    ~CFlowFadeNode()
    {
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowFadeNode(pActInfo);
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
    {
        ser.Value("m_bPlaying", m_bPlaying);
        ser.Value("m_direction", m_direction);
        ser.Value("m_postSerializeTrigger", m_postSerializeTrigger);

        if (ser.IsReading())
        {
            // in case we were playing before the fader is stopped on load,
            // but still we need to activate outputs. this MUST NOT be done in serialize
            // but in ProcessEvent
            if (m_bPlaying)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                m_postSerializeTrigger = m_direction;
            }
            m_bPlaying = false;
            m_bNeedFaderStop = false;
            m_ticket = 0;
            m_direction = 0;
        }
    }

    void StopFader(SActivationInfo* pActInfo)
    {
        CHUDFader* pFader = GetFader(pActInfo);
        if (pFader)
        {
            pFader->Stop();
        }
        m_ticket = 0;
        m_bNeedFaderStop = false;
    }

    enum EInputPorts
    {
        EIP_FadeGroup = 0,
        EIP_FadeIn,
        EIP_FadeOut,
        EIP_UseCurrentColor,
        EIP_InTime,
        EIP_OutTime,
        EIP_Color,
        EIP_TextureName,
        EIP_UpdateAlways,
    };

    enum EOutputPorts
    {
        EOP_FadedIn = 0,
        EOP_FadedOut,
        EOP_FadeColor
    };

    void StartFader(SActivationInfo* pActInfo)
    {
        CHUDFader* pFader = GetFader(pActInfo);
        if (pFader != 0)
        {
            const float fDuration = GetPortFloat(pActInfo, m_direction < 0 ? EIP_InTime : EIP_OutTime);
            ColorF col;
            const Vec3 fadeColor = GetPortVec3(pActInfo, EIP_Color);
            const bool bUseCurColor = GetPortBool(pActInfo, EIP_UseCurrentColor);
            const bool bUpdateAlways = GetPortBool(pActInfo, EIP_UpdateAlways);
            col.r = fadeColor[0];
            col.g = fadeColor[1];
            col.b = fadeColor[2];
            col.a = m_direction < 0 ? 0.0f : 1.0f;
            if (m_direction < 0)
            {
                m_ticket = pFader->FadeIn(col, fDuration, bUseCurColor, bUpdateAlways);
            }
            else
            {
                m_ticket = pFader->FadeOut(col, fDuration, GetPortString(pActInfo, EIP_TextureName), bUseCurColor, bUpdateAlways);
            }
            m_bNeedFaderStop = true;
        }
    }

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig<int> ("FadeGroup", 0, _HELP("Fade Group [0-3]"), 0, _UICONFIG("enum_int:0=0,1=1,2=2,3=3")),
            InputPortConfig_Void("FadeIn", _HELP("Fade back from the specified color back to normal screen")),
            InputPortConfig_Void("FadeOut", _HELP("Fade the screen to the specified color")),
            InputPortConfig<bool> ("UseCurColor", true, _HELP("If checked, use the current color as Source color. Otherwise use [FadeColor] as Source color and Target color.")),
            InputPortConfig<float>("FadeInTime", 2.0f, _HELP("Duration of fade in")),
            InputPortConfig<float>("FadeOutTime", 2.0f, _HELP("Duration of fade out")),
            InputPortConfig<Vec3> ("color_FadeColor", _HELP("Target Color to fade to")),
            InputPortConfig<string> ("tex_TextureName", _HELP("Texture Name")),
            InputPortConfig<bool> ("UpdateAlways", false, _HELP("If checked, the Fader will be updated always, otherwise only if game is not paused.")),
            {0}
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void("FadedIn", _HELP("FadedIn")),
            OutputPortConfig_Void("FadedOut", _HELP("FadedOut")),
            OutputPortConfig<Vec3> ("CurColor", _HELP("Current Faded Color")),
            {0}
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Controls Screen Fading.");
        config.SetCategory(EFLN_ADVANCED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            if (pActInfo->pGraph->GetGraphEntity(0) == 0 && pActInfo->pGraph->GetGraphEntity(1) == 0)
            {
                m_nFaderOffset = MFX_FADER_OFFSET;
            }
            else
            {
                m_nFaderOffset = GAME_FADER_OFFSET;
            }

            if (gEnv->pCryPak->GetLvlResStatus())
            {
                const string& texName = GetPortString(pActInfo, EIP_TextureName);
                if (texName.empty() == false)
                {
                    ITexture* pTexture = CHUDFader::LoadTexture(texName.c_str());
                    if (pTexture)
                    {
                        pTexture->Release();
                    }
                }
            }

            if (m_bNeedFaderStop)
            {
                StopFader(pActInfo);
                m_bPlaying = false;
                m_bNeedFaderStop = false;
                m_ticket = 0;
            }
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
            break;
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, EIP_FadeIn))
            {
                StopFader(pActInfo);
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                m_direction = -1;
                StartFader(pActInfo);
                m_bPlaying = true;
                m_bNeedFaderStop = true;
                m_postSerializeTrigger = 0;
            }
            if (IsPortActive(pActInfo, EIP_FadeOut))
            {
                StopFader(pActInfo);
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
                m_direction = 1;
                StartFader(pActInfo);
                m_bPlaying = true;
                m_bNeedFaderStop = true;
                m_postSerializeTrigger = 0;
            }
        }
        break;
        case eFE_Update:
        {
            if (m_postSerializeTrigger)
            {
                ActivateOutput(pActInfo, m_postSerializeTrigger < 0 ? EOP_FadedIn : EOP_FadedOut, true);
                m_postSerializeTrigger = 0;
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                return;
            }

            CHUDFader* pFader = GetFader(pActInfo);
            if (pFader == 0 || m_bPlaying == false)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                m_bPlaying = false;
                m_ticket = 0;
                return;
            }

            ColorF col = pFader->GetCurrentColor();
            Vec3 vCol (col.r, col.g, col.b);
            ActivateOutput(pActInfo, EOP_FadeColor, vCol);
            if (pFader->IsPlaying(m_ticket) == false)
            {
                if (m_direction < 0.0f)
                {
                    ActivateOutput(pActInfo, EOP_FadedIn, true);
                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                    m_bNeedFaderStop = false;
                }
                else
                {
                    ActivateOutput(pActInfo, EOP_FadedOut, true);
                    pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                    m_bNeedFaderStop = true;     // but needs a stop, if we're faded out (fader is still active then!)
                }
                m_bPlaying = false;
                m_ticket = 0;
            }
        }
        break;
        }
    }
protected:
    int   m_ticket;
    int   m_direction;
    int   m_nFaderOffset;
    int   m_postSerializeTrigger;
    bool  m_bPlaying;
    bool  m_bNeedFaderStop;
};

REGISTER_FLOW_NODE("Image:ScreenFader", CFlowFadeNode);


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
#include "CryLegacy_precompiled.h"
#include <IFlowSystem.h>
#include <IGameFramework.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <Graphics/ScreenFader.h>

static const int MFX_FADER_OFFSET = 0;
static const int MFX_FADER_END = 3;
static const int GAME_FADER_OFFSET = 4;

class CFlowFadeNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    Graphics::CHUDFader* GetFader(SActivationInfo* pActInfo)
    {
        int group = GetPortInt(pActInfo, EIP_FadeGroup);
        group += m_nFaderOffset;
        return Graphics::ScreenFaderManager::GetFader(group);
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
        Graphics::CHUDFader* pFader = GetFader(pActInfo);
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
        Graphics::CHUDFader* pFader = GetFader(pActInfo);
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
                    ITexture* pTexture = Graphics::CHUDFader::LoadTexture(texName.c_str());
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

            Graphics::CHUDFader* pFader = GetFader(pActInfo);
            if (pFader == 0 || m_bPlaying == false)
            {
                pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
                m_bPlaying = false;
                m_ticket = 0;
                return;
            }

            ColorF col = AZColorToLYColorF(pFader->GetCurrentColor());
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


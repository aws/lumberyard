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
// Original file (from FadeNode.cpp) Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include <IGameFramework.h>
#include "ScreenFader.h"
#include <MathConversion.h>

namespace Graphics
{

CHUDFader::CHUDFader(int faderId) : m_faderId(faderId)
{
    m_pRenderer = gEnv->pRenderer;
    assert (m_pRenderer != 0);

    const ColorF zeroColor(0, 0, 0, 0);

    m_startColor = zeroColor;
    m_targetColor = zeroColor;
    m_drawColor = zeroColor;
    m_pTexture = 0;
    m_screenCoordinates = AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
    m_bActive = false;
    m_duration = 0.0f;
    m_curTime = 0.0f;
    m_direction = 0;
    m_ticket = 0;
    m_lastTime = 0;

    AZ::ScreenFaderNotificationBus::Bind(m_notificationBus, m_faderId);
    AZ::ScreenFaderRequestBus::Handler::BusConnect(m_faderId);
}

CHUDFader::~CHUDFader()
{
    AZ::ScreenFaderRequestBus::Handler::BusDisconnect();
    SAFE_RELEASE(m_pTexture);
}


const char* CHUDFader::GetDebugName() const
{
    if (m_pTexture)
    {
        return m_pTexture->GetName();
    }
    return "<no texture>";
}

void CHUDFader::Stop()
{
    m_bActive = false;
}

void CHUDFader::Reset()
{
    m_ticket = 0;
    m_bActive = false;
}

bool CHUDFader::IsActive() const
{
    return m_bActive; 
}

bool CHUDFader::IsPlaying() const
{
    return m_ticket > 0;
}

bool CHUDFader::IsPlaying(int ticket) const 
{ 
    return m_ticket > 0 && m_ticket == ticket; 
}

AZ::Color CHUDFader::GetCurrentColor()
{
    float curVal = 1.0;
    if (m_duration != 0.0f)
    {
        curVal = m_curTime / m_duration;
        curVal = CLAMP(curVal, 0.0f, 1.0f);
    }
    ColorF col;
    col.lerpFloat(m_startColor, m_targetColor, curVal);
    return LYColorFToAZColor(col);
}

void CHUDFader::FadeIn(const AZ::Color& targetColor, float fDuration, bool bUseCurrentColor, bool bUpdateAlways)
{
    if (bUseCurrentColor == false)
    {
        m_startColor = AZColorToLYColorF(targetColor);
        m_startColor.a = 1.0f;
    }
    else
    {
        m_startColor = AZColorToLYColorF(GetCurrentColor());
    }

    m_targetColor = m_startColor;
    m_targetColor.a = 0.0f;

    m_duration = fDuration > 0.0f ? fDuration : 0.0f;
    m_curTime = 0.0f;
    m_direction = -1;
    m_bActive = true;
    m_bUpdateAlways = bUpdateAlways;
    m_lastTime = gEnv->pTimer->GetAsyncCurTime();

    ++m_ticket;
}

void CHUDFader::FadeOut(const AZ::Color& targetColor, float fDuration, bool bUseCurrentColor, bool bUpdateAlways)
{
    if (bUseCurrentColor == false)
    {
        m_startColor = AZColorToLYColorF(targetColor);
        m_startColor.a = 0.0f;
    }
    else
    {
        m_startColor = AZColorToLYColorF(GetCurrentColor());
    }

    m_targetColor = AZColorToLYColorF(targetColor);
    m_targetColor.a = 1.0f;

    m_duration = fDuration > 0.0 ? fDuration : 0.0f;
    m_curTime = 0.0f;
    m_direction = 1;
    m_bActive = true;
    m_bUpdateAlways = bUpdateAlways;
    m_lastTime = gEnv->pTimer->GetAsyncCurTime();

    ++m_ticket;
}

// This function could be deprecated as it is only used by the legacy Flow Graph ScreenFader node (FadeNode.cpp).
int CHUDFader::FadeIn(const ColorF& targetColor, float fDuration, bool bUseCurrentColor, bool bUpdateAlways)
{
    bUseCurrentColor &= m_ticket != 0; // This preserves legacy behavior of the Flow Graph ScreenFader node
    FadeIn(LYColorFToAZColor(targetColor), fDuration, bUseCurrentColor, bUpdateAlways);
    return m_ticket;
}

// This function could be deprecated as it is only used by the legacy Flow Graph ScreenFader node (FadeNode.cpp).
int CHUDFader::FadeOut(const ColorF& targetColor, float fDuration, const char* textureName, bool bUseCurrentColor, bool bUpdateAlways)
{
    bUseCurrentColor &= m_ticket != 0; // This preserves legacy behavior of the Flow Graph ScreenFader node
    SetTexture(textureName);
    FadeOut(LYColorFToAZColor(targetColor), fDuration, bUseCurrentColor, bUpdateAlways);
    return m_ticket;
}

void CHUDFader::Update(float fDeltaTime)
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
            m_startColor = m_targetColor;
        }

        const bool justFinishedFading = IsPlaying();

        m_startColor = m_targetColor;
        m_ticket = 0;

        if (justFinishedFading)
        {
            if (m_direction < 0)
            {
                AZ::ScreenFaderNotificationBus::Event(m_notificationBus, &AZ::ScreenFaderNotifications::OnFadeInComplete);
            }
            else
            {
                AZ::ScreenFaderNotificationBus::Event(m_notificationBus, &AZ::ScreenFaderNotifications::OnFadeOutComplete);
            }
        }

    }
    m_drawColor = AZColorToLYColorF(GetCurrentColor());
    m_curTime += fDeltaTime;
}

void CHUDFader::Draw()
{
    if (m_bActive == false)
    {
        return;
    }

    // CD3D9Renderer::Draw2dImage() is based off of the "virtual screen" dimensions, not the viewport, rendertarget or renderer dimensions.
    const float renderWidth = VIRTUAL_SCREEN_WIDTH;
    const float renderHeight = VIRTUAL_SCREEN_HEIGHT;

    const float screenLeft = m_screenCoordinates.GetX() * renderWidth;
    const float screenTop = m_screenCoordinates.GetY() * renderHeight;
    const float screenWidth = (m_screenCoordinates.GetZ() - m_screenCoordinates.GetX()) * renderWidth;
    const float screenHeight = (m_screenCoordinates.GetW() - m_screenCoordinates.GetY()) * renderHeight;

    gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
    gEnv->pRenderer->Draw2dImage(screenLeft, screenTop, screenWidth, screenHeight,
        m_pTexture ? m_pTexture->GetTextureID() : -1,
        0.0f, 1.0f, 1.0f, 0.0f, // tex coords
        0.0f, // angle
        m_drawColor.r, m_drawColor.g, m_drawColor.b, m_drawColor.a,
        0.0f);

}

void CHUDFader::SetScreenCoordinates(const AZ::Vector4& screenCoordinates)
{
    m_screenCoordinates = screenCoordinates;
}

ITexture* CHUDFader::LoadTexture(const char* textureName)
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

void CHUDFader::SetTexture(const AZStd::string& textureName)
{
    SetTexture(textureName.c_str());
}

void CHUDFader::SetTexture(const char* textureName)
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



static const int NUM_FADERS = 100; // Using this number for now based on a specific customer's needs/feedback. Eventually, we plan to get rid of this fixed limit anyway.


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
            m_pHUDFader[group] = new CHUDFader(group);
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

CMasterFader* ScreenFaderManager::m_masterFader = nullptr;

ScreenFaderManager::ScreenFaderManager()
{
    // Ensure all faders are created so they will be available on the ScreenFaderRequestBus
    for (int i = 0; i < NUM_FADERS; ++i)
    {
        GetFader(i);
    }

    AZ::ScreenFaderManagementRequestBus::Handler::BusConnect();
}

ScreenFaderManager::~ScreenFaderManager()
{
    AZ::ScreenFaderManagementRequestBus::Handler::BusDisconnect();
}

int ScreenFaderManager::GetNumFaderIDs()
{
    return NUM_FADERS;
}

CHUDFader* ScreenFaderManager::GetFader(int group)
{
    if (!m_masterFader)
    {
        m_masterFader = new CMasterFader();
    }

    if (m_masterFader)
    {
        return m_masterFader->GetHUDFader(group);
    }
    else
    {
        return nullptr;
    }
}


} // namespace Graphics


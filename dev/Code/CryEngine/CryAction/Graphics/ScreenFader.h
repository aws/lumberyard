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

#pragma once

#include <ScreenFaderBus.h>
#include <Cry_Math.h>
#include <Cry_Color.h>

class ITexture;
struct IRenderer;

namespace Graphics
{

class CHUDFader : public AZ::ScreenFaderRequestBus::Handler
{
public:
    explicit CHUDFader(int faderId);

    virtual ~CHUDFader();


    const char* GetDebugName() const;

    void Stop();

    void Reset();

    //! Returns true whenever the fader is affecting the rendered output (even when the idling at 100% faded out)
    bool IsActive() const;

    //! Returns true when fading in or fading out
    bool IsPlaying() const;

    //! Returns true when fading in or fading out under the given ticket
    bool IsPlaying(int ticket) const;


    // This function could be deprecated as it is only used by the legacy Flow Graph ScreenFader node (FadeNode.cpp).
    int FadeIn(const ColorF& targetColor, float fDuration, bool bUseCurrentColor = true, bool bUpdateAlways = false);
    // This function could be deprecated as it is only used by the legacy Flow Graph ScreenFader node (FadeNode.cpp).
    int FadeOut(const ColorF& targetColor, float fDuration, const char* textureName = "", bool bUseCurrentColor = true, bool bUpdateAlways = false);

    //////////////////////////////////////////////////////////////////////////
    // AZ::ScreenFaderRequestBus::Handler
    void FadeIn(const AZ::Color& color, float duration, bool useCurrentColor = true, bool updateAlways = false) override;
    void FadeOut(const AZ::Color& color, float duration, bool useCurrentColor = true, bool updateAlways = false) override;
    void SetTexture(const AZStd::string& textureName) override;
    void SetScreenCoordinates(const AZ::Vector4& screenCoordinates) override;
    AZ::Color GetCurrentColor() override;
    //////////////////////////////////////////////////////////////////////////

    void Update(float fDeltaTime);

    void Draw();

    static ITexture* LoadTexture(const char* textureName);

protected:
    void SetTexture(const char* textureName);


protected:
    AZ::ScreenFaderNotificationBus::BusPtr m_notificationBus;    //!< Cached bus pointer to the notification bus.
    IRenderer*     m_pRenderer;
    ColorF         m_startColor;
    ColorF         m_targetColor;
    ColorF         m_drawColor;
    ITexture*      m_pTexture;
    AZ::Vector4    m_screenCoordinates; //!< (left,top,right,bottom). Fullscreen is (0,0,1,1)
    float          m_duration;
    float          m_curTime;
    int            m_direction;
    int            m_ticket;            //!< Tracks individual fade-in/fade-out actions for syncronizing with the legacy Flow Graph ScreenFader Node (FadeNode.cpp)
    bool           m_bActive;
    bool           m_bUpdateAlways;
    float          m_lastTime;
    const int      m_faderId;
};

class CMasterFader;

//! Manages the creation of individual ScreenFader objects.
class ScreenFaderManager : public AZ::ScreenFaderManagementRequestBus::Handler
{
public:

    ScreenFaderManager();
    virtual ~ScreenFaderManager();

    static CHUDFader* GetFader(int faderId);

    //////////////////////////////////////////////////////////////////////////
    // AZ::ScreenFaderManagementRequestBus::Handler
    int GetNumFaderIDs() override;
    //////////////////////////////////////////////////////////////////////////

private:
    static CMasterFader* m_masterFader; //! Legacy fader management implementation
};

} // namespace Graphics
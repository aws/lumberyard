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

#include "Precompiled.h"

#include "ScreenFaderNode.h"
#include <IRenderer.h>
#include <ISystem.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Serialization/Utils.h>

static bool ScreenFaderNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

#include <Source/ScreenFaderNode.generated.cpp>

namespace GraphicsReflectContext
{
    void ScreenFaderNode::OnActivate()
    {
        m_originalTextureName = ScreenFaderNodeProperty::GetTextureName(this);

        if (!m_originalTextureName.empty())
        {
            gEnv->pRenderer->EF_LoadTexture(m_originalTextureName.c_str(), FT_DONT_STREAM);
        }

        m_originalFaderId = ScreenFaderNodeProperty::GetFaderId(this);

        AZ::TickBus::Handler::BusConnect();
    }

    void ScreenFaderNode::OnDeactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ScreenFaderNode::OnInputSignal(const ScriptCanvas::SlotId& slot)
    {
        const ScriptCanvas::SlotId fadeInSlot = ScreenFaderNodeProperty::GetFadeInSlotId(this);
        const ScriptCanvas::SlotId fadeOutSlot = ScreenFaderNodeProperty::GetFadeOutSlotId(this);

        const int faderId = ScreenFaderNodeProperty::GetFaderId(this);
        const AZ::Color color = ScreenFaderNodeProperty::GetColor(this);
        const float fadeInTime = ScreenFaderNodeProperty::GetFadeInTime(this);
        const float fadeOutTime = ScreenFaderNodeProperty::GetFadeOutTime(this);
        const bool useCurrentColor = ScreenFaderNodeProperty::GetUseCurrentColor(this);
        const bool updateAlways = ScreenFaderNodeProperty::GetUpdateAlways(this);
        const AZStd::string textureName = ScreenFaderNodeProperty::GetTextureName(this);
        const AZ::Vector4 screenCoordinates = ScreenFaderNodeProperty::GetScreenCoordinates(this);

        int numFaderIDs = 0;
        AZ::ScreenFaderManagementRequestBus::BroadcastResult(numFaderIDs, &AZ::ScreenFaderManagementRequests::GetNumFaderIDs);
        const int maxFaderID = numFaderIDs - 1;
        AZ_Warning("Script", 0 <= faderId && faderId <= maxFaderID, "FaderID of %d is out of range. Must be 0-%d", faderId, maxFaderID);

        AZ::ScreenFaderRequestBus::Event(faderId, &AZ::ScreenFaderRequests::SetTexture, textureName);
        AZ::ScreenFaderRequestBus::Event(faderId, &AZ::ScreenFaderRequests::SetScreenCoordinates, screenCoordinates);

        if (slot == fadeInSlot)
        {
            AZ::ScreenFaderNotificationBus::Handler::BusConnect(faderId);
            AZ::ScreenFaderRequestBus::Event(faderId, &AZ::ScreenFaderRequests::FadeIn, color, fadeInTime, useCurrentColor, updateAlways);
        }
        else if (slot == fadeOutSlot)
        {
            AZ::ScreenFaderNotificationBus::Handler::BusConnect(faderId);
            AZ::ScreenFaderRequestBus::Event(faderId, &AZ::ScreenFaderRequests::FadeOut, color, fadeOutTime, useCurrentColor, updateAlways);
        }
    }

    void ScreenFaderNode::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        const AZStd::string textureName = ScreenFaderNodeProperty::GetTextureName(this);
        AZ_Warning("Script", m_originalTextureName == textureName, "ScreenFader texture changed from '%s' to '%s'. The new texture might not be preloaded.", m_originalTextureName.c_str(), textureName.c_str());
        m_originalTextureName = textureName;

        const int faderId = ScreenFaderNodeProperty::GetFaderId(this);
        AZ_Warning("Script", m_originalFaderId == faderId, "ScreenFader ID changed from '%d' to '%d'. This may result in unexpected behavior.", m_originalFaderId, faderId);
        m_originalFaderId = faderId;

        AZ::Color currentColor(0.0f);
        AZ::ScreenFaderRequestBus::EventResult(currentColor, faderId, &AZ::ScreenFaderRequests::GetCurrentColor);

        const ScriptCanvas::SlotId currentColorSlot = ScreenFaderNodeProperty::GetCurrentColorSlotId(this);
        ScriptCanvas::Datum outputDatum = ScriptCanvas::Datum(currentColor);
        PushOutput(outputDatum, *GetSlot(currentColorSlot));
    }

    void ScreenFaderNode::OnFadeOutComplete()
    {
        AZ::ScreenFaderNotificationBus::Handler::BusDisconnect();
        const ScriptCanvas::SlotId outSlot = ScreenFaderNodeProperty::GetFadeOutCompleteSlotId(this);
        SignalOutput(outSlot);
    }

    void ScreenFaderNode::OnFadeInComplete()
    {
        AZ::ScreenFaderNotificationBus::Handler::BusDisconnect();
        const ScriptCanvas::SlotId outSlot = ScreenFaderNodeProperty::GetFadeInCompleteSlotId(this);
        SignalOutput(outSlot);
    }

    class ScreenFaderNotificationBusBehaviorHandler : public AZ::ScreenFaderNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ScreenFaderNotificationBusBehaviorHandler, "{B6E6FB22-8ED5-45E3-A5CC-4023268BD30C}", AZ::SystemAllocator
            , OnFadeOutComplete
            , OnFadeInComplete);

        void OnFadeOutComplete() override
        {
            Call(FN_OnFadeOutComplete);
        }

        void OnFadeInComplete() override
        {
            Call(FN_OnFadeInComplete);
        }
    };

    void ReflectScreenFaderBus(AZ::BehaviorContext *behaviorContext)
    {
        behaviorContext->EBus<AZ::ScreenFaderRequestBus>("ScreenFaderRequestBus", nullptr, "Provides controls for basic screen fading to/from a solid color and/or texture")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All) // Exclude from ScriptCanvas (since we have ScreenFaderNode) but still expose to Lua
            ->Event("FadeOut", &AZ::ScreenFaderRequests::FadeOut,
                { {
                    { "color", "Color to fade out to" },
                    { "duration", "Number of seconds the fade should take" },
                    { "useCurrentColor", "If true, the transition begins from the current color left over from any prior fading. Otherwise, the transition begins fully transparent (i.e. the current rendered output)." },
                    { "updateAlways", "Continue fading even when the game is paused" },
                } }
                )
                ->Attribute(AZ::Script::Attributes::ToolTip, "Triggers fading out to a solid color")
            ->Event("FadeIn", &AZ::ScreenFaderRequests::FadeIn,
                { {
                    { "color", "Color to fade through. Should be ignored if useCurrentColor is true." },
                    { "duration", "Number of seconds the fade should take" },
                    { "useCurrentColor", "If true, the transition begins from the current color left over from any prior fading. Otherwise, the transition begins from targetColor." },
                    { "updateAlways", "Continue fading even when the game is paused" },
                } }
                )
                ->Attribute(AZ::Script::Attributes::ToolTip, "Triggers fading back in from a solid color")
            ->Event("SetTexture", &AZ::ScreenFaderRequests::SetTexture)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Sets a texture to be used my the fade mask. Use empty string to clear the texture.")
            ->Event("SetScreenCoordinates", &AZ::ScreenFaderRequests::SetScreenCoordinates)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Sets the screen coordinates where the fade mask will be drawn (left,top,right,bottom). The default is fullscreen (0,0,1,1).")
            ->Event("GetCurrentColor", &AZ::ScreenFaderRequests::GetCurrentColor)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns the current color of the fade mask")
            ;

        behaviorContext->EBus<AZ::ScreenFaderNotificationBus>("ScreenFaderNotificationBus", nullptr, "Provides notifications for basic screen fading to/from a solid color and/or texture")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All) // Exclude from ScriptCanvas (since we have ScreenFaderNode) but still expose to Lua
            ->Handler<ScreenFaderNotificationBusBehaviorHandler>()
            ;

        behaviorContext->EBus<AZ::ScreenFaderManagementRequestBus>("ScreenFaderManagementRequestBus", nullptr, "Supports management of the set of available ScreenFaders")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All) // Exclude from ScriptCanvas (since we have ScreenFaderNode) but still expose to Lua
            ->Event("GetNumFaderIDs", &AZ::ScreenFaderManagementRequests::GetNumFaderIDs)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns the number of available fader IDs (not necessarily the number of Faders that have been created)")
            ;
    }
}

bool ScreenFaderNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
{
    if (rootElement.GetVersion() == 0)
    {
        auto slotNameElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0), AZ_CRC("element", 0x41405e39), AZ_CRC("slotName", 0x817c3511)});

        for (AZ::SerializeContext::DataElementNode* slotNameElement : slotNameElements)
        {
            AZStd::string slotName;
            if (!slotNameElement->GetData(slotName))
            {
                return false;
            }

            // Rename the slot names from the old values to the new values
            if (slotName == "TextureName")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Texture Name"));
            }
            else if (slotName == "FaderId")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fader Id"));
            }
            else if (slotName == "FadeOut")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade Out"));
            }
            else if (slotName == "FadeIn")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade In"));
            }
            else if (slotName == "FadeOutComplete")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade Out Complete"));
            }
            else if (slotName == "FadeInComplete")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade In Complete"));
            }
            else if (slotName == "FadeOutTime")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade Out Time"));
            }
            else if (slotName == "FadeInTime")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade In Time"));
            }
            else if (slotName == "UseCurrentColor")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Use Current Color"));
            }
            else if (slotName == "EvenWhenPaused")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Update Always"));
            }
            else if (slotName == "ScreenCoordinates")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Screen Coordinates"));
            }
            else if (slotName == "CurrentColor")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Current Color"));
            }

        }
    }

    return true;
}




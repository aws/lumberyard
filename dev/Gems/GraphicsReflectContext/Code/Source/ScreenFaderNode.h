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

#include <AzCore/std/string/string.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <AzCore/Math/Color.h>
#include <ScreenFaderBus.h>
#include <AzCore/Component/TickBus.h>

#include <Source/ScreenFaderNode.generated.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphicsReflectContext
{
    //! ScriptCanvas node that controls fading the screen to a color and/or texture
    class ScreenFaderNode : public ScriptCanvas::Node, AZ::ScreenFaderNotificationBus::Handler, AZ::TickBus::Handler
    {
        ScriptCanvas_Node(ScreenFaderNode,
            ScriptCanvas_Node::Uuid("{508EDC8D-5580-4B84-9660-1341DB763E67}")
            ScriptCanvas_Node::Name("Screen Fader") // The localization tool doesn't support custom SC nodes, so we have to put the spaces in here.
            ScriptCanvas_Node::Category("Rendering/Post Effects")
            ScriptCanvas_Node::Description("Controls fading the screen to a color and/or texture")
            ScriptCanvas_Node::Version(1, ScreenFaderNodeVersionConverter)
        );

    public:
        ScriptCanvas_In(ScriptCanvas_In::Name("Fade Out", "Triggers fading out to a color/texture"));
        ScriptCanvas_In(ScriptCanvas_In::Name("Fade In", "Triggers fading back in from a color/texture"));

        ScriptCanvas_Out(ScriptCanvas_Out::Name("Fade Out Complete", "Occurs when fade-out is complete"));
        ScriptCanvas_Out(ScriptCanvas_Out::Name("Fade In Complete", "Occurs when fade-in is complete"));

        ScriptCanvas_Property(int,
            ScriptCanvas_Property::Name("Fader Id", "Which fader to use (allows maintaining separate settings and/or layering fades on top of each other)")
            ScriptCanvas_Property::Input,
            ScriptCanvas_Property::Min(0));

        ScriptCanvas_PropertyWithDefaults(float, 2.0f,
            ScriptCanvas_Property::Name("Fade Out Time", "Number of seconds when fading out")
            ScriptCanvas_Property::Input,
            ScriptCanvas_Property::Min(0.0f));

        ScriptCanvas_PropertyWithDefaults(float, 2.0f,
            ScriptCanvas_Property::Name("Fade In Time", "Number of seconds when fading in")
            ScriptCanvas_Property::Input,
            ScriptCanvas_Property::Min(0.0f));

        ScriptCanvas_PropertyWithDefaults(AZ::Color, AZ::Color(0.0f),
            ScriptCanvas_Property::Name("Color", "The color to fade to/from (the Alpha channel is ignored)")
            ScriptCanvas_Property::Input);

        ScriptCanvas_PropertyWithDefaults(bool, true,
            ScriptCanvas_Property::Name("Use Current Color", "If true, the transition begins from the current color left over from any prior fading. Otherwise, the transition begins from the Color property.")
            ScriptCanvas_Property::Input);

        ScriptCanvas_Property(AZStd::string,
            ScriptCanvas_Property::Name("Texture Name", "The name of a texture to fade to/from (optional)")
            ScriptCanvas_Property::Input);

        ScriptCanvas_Property(bool,
            ScriptCanvas_Property::Name("Update Always", "Continue fading even when the game is paused")
            ScriptCanvas_Property::Input);

        ScriptCanvas_PropertyWithDefaults(AZ::Vector4, AZ::Vector4(0,0,1,1),
            ScriptCanvas_Property::Name("Screen Coordinates", "Sets the screen coordinates where the fade mask will be drawn (left,top,right,bottom). The default is fullscreen (0,0,1,1).")
            ScriptCanvas_Property::Input);

        ScriptCanvas_Property(AZ::Color,
            ScriptCanvas_Property::Name("Current Color", "Outputs the current fade mask color value")
            ScriptCanvas_Property::Output
            ScriptCanvas_Property::OutputStorageSpec);

        //////////////////////////////////////////////////////////////////////////
        // AZ::ScreenFaderNotificationBus::Handler
        void OnFadeOutComplete() override;
        void OnFadeInComplete() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void OnActivate() override;
        void OnDeactivate() override;
        void OnInputSignal(const ScriptCanvas::SlotId& slot) override;
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        AZStd::string m_originalTextureName;
        int m_originalFaderId;
    };

    //! Provides reflection of the ScreenFader bus for access from Lua script
    void ReflectScreenFaderBus(AZ::BehaviorContext *behaviorContext);
}

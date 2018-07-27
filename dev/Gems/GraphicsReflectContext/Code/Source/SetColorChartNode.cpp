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

#include "SetColorChartNode.h"
#include <ColorGradingBus.h>
#include <IRenderer.h>
#include <ISystem.h>
#include <AzCore/Serialization/Utils.h>

static bool SetColorChartNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

#include <Source/SetColorChartNode.generated.cpp>

namespace GraphicsReflectContext
{    

    void  SetColorChartNode::OnActivate()
    {
        m_loadedTextureName = SetColorChartNodeProperty::GetTextureName(this);
        if (!m_loadedTextureName.empty())
        {
            const uint32 COLORCHART_TEXFLAGS = FT_NOMIPS | FT_DONT_STREAM | FT_STATE_CLAMP;
            gEnv->pRenderer->EF_LoadTexture(m_loadedTextureName.c_str(), COLORCHART_TEXFLAGS);
        }
    }

    void  SetColorChartNode::OnDeactivate()
    {
        AZ::ColorGradingRequestBus::Broadcast(&AZ::ColorGradingRequests::Reset);
        m_loadedTextureName = "";
    }


    void  SetColorChartNode::OnInputSignal(const ScriptCanvas::SlotId& slot)
    {
        const AZStd::string textureName = SetColorChartNodeProperty::GetTextureName(this);
        AZ_Warning("Script", m_loadedTextureName == textureName, "Preloaded texture '%s' and current texture '%s' are not the same. The actual texture might not be loaded yet.", m_loadedTextureName.c_str(), textureName.c_str());

        const float fadeTime = SetColorChartNodeProperty::GetFadeTime(this);

        AZ::ColorGradingRequestBus::Broadcast(&AZ::ColorGradingRequests::FadeInColorChart, textureName, fadeTime);

        m_loadedTextureName = textureName;

        const ScriptCanvas::SlotId outSlot = SetColorChartNodeProperty::GetOutSlotId(this);
        SignalOutput(outSlot);
    }
}

bool SetColorChartNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
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
            else if (slotName == "FadeTime")
            {
                slotNameElement->SetData(serializeContext, AZStd::string("Fade Time"));
            }

        }
    }

    return true;
}

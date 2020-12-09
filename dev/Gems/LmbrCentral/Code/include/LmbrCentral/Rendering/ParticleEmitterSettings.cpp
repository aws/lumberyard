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
#include "LmbrCentral_precompiled.h"
#include "ParticleEmitterSettings.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IEntityRenderState.h>

namespace LmbrCentral
{
    bool RenameElement(AZ::SerializeContext::DataElementNode& classElement, AZ::Crc32 srcCrc, const char* newName)
    {
        AZ::SerializeContext::DataElementNode* eleNode = classElement.FindSubElement(srcCrc);

        if (!eleNode)
        {
            return false;
        }

        eleNode->SetName(newName);
        return true;
    }

    const float ParticleEmitterSettings::MaxCountScale = 1000.f;
    const float ParticleEmitterSettings::MaxTimeScale = 1000.f;
    const float ParticleEmitterSettings::MaxSpeedScale = 1000.f;
    const float ParticleEmitterSettings::MaxSizeScale = 100.f;
    const float ParticleEmitterSettings::MaxLifttimeStrength = 1.f;
    const float ParticleEmitterSettings::MinLifttimeStrength = -1.f;
    const float ParticleEmitterSettings::MaxRandSizeScale = 1.f;

    void ParticleEmitterSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ParticleEmitterSettings>()->
                Version(5, &VersionConverter)->

                //Particle
                Field("Visible", &ParticleEmitterSettings::m_visible)->
                Field("Enable", &ParticleEmitterSettings::m_enable)->
                Field("SelectedEmitter", &ParticleEmitterSettings::m_selectedEmitter)->

                //Spawn Properties
                Field("Color", &ParticleEmitterSettings::m_color)->
                Field("Pre-roll", &ParticleEmitterSettings::m_prime)->
                Field("Particle Count Scale", &ParticleEmitterSettings::m_countScale)->
                Field("Time Scale", &ParticleEmitterSettings::m_timeScale)->
                Field("Pulse Period", &ParticleEmitterSettings::m_pulsePeriod)->
                Field("GlobalSizeScale", &ParticleEmitterSettings::m_sizeScale)->
                Field("ParticleSizeX", &ParticleEmitterSettings::m_particleSizeScaleX)->
                Field("ParticleSizeY", &ParticleEmitterSettings::m_particleSizeScaleY)->
                Field("ParticleSizeZ", &ParticleEmitterSettings::m_particleSizeScaleZ)->
                Field("ParticleSizeRandom", &ParticleEmitterSettings::m_particleSizeScaleRandom)->
                Field("Speed Scale", &ParticleEmitterSettings::m_speedScale)->
                Field("Strength", &ParticleEmitterSettings::m_strength)->
                Field("Ignore Rotation", &ParticleEmitterSettings::m_ignoreRotation)->
                Field("Not Attached", &ParticleEmitterSettings::m_notAttached)->
                Field("Register by Bounding Box", &ParticleEmitterSettings::m_registerByBBox)->
                Field("Use LOD", &ParticleEmitterSettings::m_useLOD)->

                Field("Target Entity", &ParticleEmitterSettings::m_targetEntity)->

                //Audio
                Field("Enable Audio", &ParticleEmitterSettings::m_enableAudio)->
                Field("Audio RTPC", &ParticleEmitterSettings::m_audioRTPC)->

                //render node and misc
                Field("View Distance Multiplier", &ParticleEmitterSettings::m_viewDistMultiplier)->
                Field("Use VisArea", &ParticleEmitterSettings::m_useVisAreas)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ParticleEmitterSettings>("ParticleSettings", "")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_prime, "Pre-Roll", "(CPU Only) Set emitter as though it's been running indefinitely.")->

                    DataElement(AZ::Edit::UIHandlers::Color, &ParticleEmitterSettings::m_color, "Color tint", "Particle color tint")->

                    DataElement(0, &ParticleEmitterSettings::m_countScale, "Count scale", "Multiple for particle count")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxCountScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_timeScale, "Time scale", "Multiple for emitter time evolution.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxTimeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_pulsePeriod, "Pulse period", "How often to restart emitter.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->

                    DataElement(0, &ParticleEmitterSettings::m_speedScale, "Speed scale", "Multiple for particle emission speed.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSpeedScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &ParticleEmitterSettings::m_strength, "Strength Curve Time",
                        "Controls all \"Strength Over Emitter Life\" curves.\n\
                The curves will use this Strength Curve Time instead of the actual Emitter life time.\n\
                Negative values will be ignored.\n")->

                    Attribute(AZ::Edit::Attributes::Min, ParticleEmitterSettings::MinLifttimeStrength)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxLifttimeStrength)->
                    Attribute(AZ::Edit::Attributes::Step, 0.05f)->

                    DataElement(0, &ParticleEmitterSettings::m_sizeScale, "Global size scale", "Multiple for all effect sizes.")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleX, "Particle size scale x", "Particle size scale x")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleY, "Particle size scale y", "Particle size scale y")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(0, &ParticleEmitterSettings::m_particleSizeScaleZ, "Particle size scale z", "Particle size scale z for geometry particle")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, ParticleEmitterSettings::MaxSizeScale)->
                    Attribute(AZ::Edit::Attributes::Step, 0.1f)->

                    DataElement(AZ::Edit::UIHandlers::Slider, &ParticleEmitterSettings::m_particleSizeScaleRandom, "Particle size scale random", "(CPU only) Randomize particle size scale")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, 1.f)->
                    Attribute(AZ::Edit::Attributes::Step, 0.05f)->

                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_ignoreRotation, "Ignore rotation", "Ignored the entity's rotation.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_notAttached, "Not attached", "If selected, the entity's position is ignored. Emitter does not follow its entity.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_registerByBBox, "Register by bounding box", "Use the Bounding Box instead of Position to Register in VisArea.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_useLOD, "Use LOD", "Activates LOD if they exist on emitter.")->

                    DataElement(AZ::Edit::UIHandlers::Default, &ParticleEmitterSettings::m_targetEntity, "Target Entity", "Target Entity to be used for emitters with Target Attraction or similar features enabled.")->
                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_viewDistMultiplier, "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default. Set to 100 for infinite visibility ")->
                    Attribute(AZ::Edit::Attributes::Min, 0.f)->
                    Attribute(AZ::Edit::Attributes::Max, static_cast<float>(IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX))->

                    DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleEmitterSettings::m_useVisAreas, "Use VisAreas", "Allow VisAreas to control this component's visibility.")
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<ParticleEmitterSettings>()
                ->Property("ColorTint", BehaviorValueProperty(&ParticleEmitterSettings::m_color))
                ->Property("Preroll", BehaviorValueProperty(&ParticleEmitterSettings::m_prime))
                ->Property("CountScale", BehaviorValueProperty(&ParticleEmitterSettings::m_countScale))
                ->Property("TimeScale", BehaviorValueProperty(&ParticleEmitterSettings::m_timeScale))
                ->Property("SpeedScale", BehaviorValueProperty(&ParticleEmitterSettings::m_speedScale))
                ->Property("PulsePeriod", BehaviorValueProperty(&ParticleEmitterSettings::m_pulsePeriod))
                ->Property("ParticleSizeScaleX", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleX))
                ->Property("ParticleSizeScaleY", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleY))
                ->Property("ParticleSizeScaleZ", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleZ))
                ->Property("ParticleSizeRandom", BehaviorValueProperty(&ParticleEmitterSettings::m_particleSizeScaleRandom))
                ->Property("LifetimeStrength", BehaviorValueProperty(&ParticleEmitterSettings::m_strength))
                ->Property("IgnoreRotation", BehaviorValueProperty(&ParticleEmitterSettings::m_ignoreRotation))
                ->Property("NotAttached", BehaviorValueProperty(&ParticleEmitterSettings::m_notAttached))
                ->Property("RegisterByBBox", BehaviorValueProperty(&ParticleEmitterSettings::m_registerByBBox))
                ->Property("UseLOD", BehaviorValueProperty(&ParticleEmitterSettings::m_useLOD))
                ->Property("TargetEntity", BehaviorValueProperty(&ParticleEmitterSettings::m_targetEntity))
                ->Property("EnableAudio", BehaviorValueProperty(&ParticleEmitterSettings::m_enableAudio))
                ->Property("RTPC", BehaviorValueProperty(&ParticleEmitterSettings::m_audioRTPC))
                ->Property("ViewDistMultiplier", BehaviorValueProperty(&ParticleEmitterSettings::m_viewDistMultiplier))
                ->Property("UseVisAreas", BehaviorValueProperty(&ParticleEmitterSettings::m_useVisAreas));
        }
    }

    // Private Static 
    bool ParticleEmitterSettings::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        bool re = true;
        // conversion from version 1:
        // - Need to rename Emitter Object Type to Attach Type
        // - Need to rename Emission Speed to Speed Scale
        if (classElement.GetVersion() == 1)
        {
            re &= RenameElement(classElement, AZ_CRC("Emitter Object Type", 0xc563146b), "Attach Type");
            re &= RenameElement(classElement, AZ_CRC("Emission Speed", 0xb375c0de), "Speed Scale");
        }
        // conversion from version 2:
        if (classElement.GetVersion() <= 2)
        {

            //Rename
            re &= RenameElement(classElement, AZ_CRC("Prime", 0x544b0f57), "Pre-roll");
            re &= RenameElement(classElement, AZ_CRC("Particle Size Scale", 0x533c8807), "GlobalSizeScale");
            re &= RenameElement(classElement, AZ_CRC("Size X", 0x29925f6f), "ParticleSizeX");
            re &= RenameElement(classElement, AZ_CRC("Size Y", 0x5e956ff9), "ParticleSizeY");
            re &= RenameElement(classElement, AZ_CRC("Size Random X", 0x61eb4b20), "ParticleSizeRandom");

            //Remove
            re &= classElement.RemoveElementByName(AZ_CRC("Attach Type", 0x86d39374));
            re &= classElement.RemoveElementByName(AZ_CRC("Emitter Shape", 0x2c633f81));
            re &= classElement.RemoveElementByName(AZ_CRC("Geometry", 0x95520eab));
            re &= classElement.RemoveElementByName(AZ_CRC("Count Per Unit", 0xc4969296));
            re &= classElement.RemoveElementByName(AZ_CRC("Position Offset", 0xbbc4049f));
            re &= classElement.RemoveElementByName(AZ_CRC("Random Offset", 0x53c41fee));
            re &= classElement.RemoveElementByName(AZ_CRC("Size Random Y", 0x16ec7bb6));
            re &= classElement.RemoveElementByName(AZ_CRC("Init Angles", 0x4b47ebd2));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate X", 0x0356bf40));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Y", 0x74518fd6));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Z", 0xed58de6c));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random X", 0x9d401896));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random Y", 0xea472800));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Rate Random Z", 0x734e79ba));
            re &= classElement.RemoveElementByName(AZ_CRC("Rotation Random Angles", 0x1d5bf41f));
        }

        // conversion from version 4:
        if (classElement.GetVersion() <= 4)
        {
            //convert color tint's type from Vector3 to Color
            int colorIndex = classElement.FindElement(AZ_CRC("Color", 0x665648e9));
            if (colorIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& color = classElement.GetSubElement(colorIndex);
            AZ::Vector3 colorVec;
            color.GetData<AZ::Vector3>(colorVec);
            AZ::Color colorVal(colorVec.GetX(), colorVec.GetY(), colorVec.GetZ(), AZ::VectorFloat(1.0f));
            color.Convert<AZ::Color>(context);
            color.SetData(context, colorVal);
        }

        return re;
    }
}

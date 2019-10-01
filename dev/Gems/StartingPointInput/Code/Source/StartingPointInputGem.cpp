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

#include "StartingPointInput_precompiled.h"
#include <platform_impl.h>

#include "Input.h"
#include "InputNode.h"
#include "InputLibrary.h"
#include "LyToAzInputNameConversions.h"

#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/VectorFloat.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/Component.h>

namespace ClassConverters
{

    static bool ConvertToInput(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    static bool BaseClassDeprecator(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    static bool ConvertHeldVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    static bool ConvertAnalogVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
}

namespace StartingPointInput
{
    // Dummy component to get reflect function
    class StartingPointInputDummyComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(StartingPointInputDummyComponent, "{0F784CD5-C5AB-4673-8651-ABFA66060232}");

        static void Reflect(AZ::ReflectContext* context)
        {
            Input::InputLibrary::Reflect(context);
            Input::Input::Reflect(context);
            Input::ThumbstickInput::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<StartingPointInputDummyComponent, AZ::Component>()
                    ->Version(0)
                    ;

                serializeContext->ClassDeprecate("SingleEventToAction", "{2C93824D-D011-459C-B12B-9F4A6148730C}", &ClassConverters::BaseClassDeprecator);
                serializeContext->ClassDeprecate("Held", "{A3F1D51B-3473-49D6-9131-98D1FBA9003A}", &ClassConverters::ConvertHeldVersion);
                serializeContext->ClassDeprecate("Analog", "{806F21D9-11EA-47FC-8B89-FDB67AADE4FF}", &ClassConverters::ConvertAnalogVersion);
                serializeContext->ClassDeprecate("Pressed", "{8C6868E9-CBFF-4B42-AC72-FD22421F9C6A}", &ClassConverters::ConvertToInput);
                serializeContext->ClassDeprecate("Released", "{8280FA20-7171-41BA-ACA1-4F79190DD60F}", &ClassConverters::ConvertToInput);
                serializeContext->ClassDeprecate("OrderedEventCombination", "{D2D4CF91-392E-4D0E-BD60-67E09981F63D}");
                serializeContext->ClassDeprecate("UnorderedEventCombination", "{A1A67F18-C395-45E0-9648-BE01321EB41A}");
                serializeContext->ClassDeprecate("VectorizedEventCombination", "{2BAB18ED-7CA6-4147-B857-9523E653B2A5}");
            }
        }

        void Init() override
        {
            AZ::EnvironmentVariable<ScriptCanvas::NodeRegistry> nodeRegistryVariable = AZ::Environment::FindVariable<ScriptCanvas::NodeRegistry>(ScriptCanvas::s_nodeRegistryName);
            if (nodeRegistryVariable)
            {
                ScriptCanvas::NodeRegistry& nodeRegistry = nodeRegistryVariable.Get();
                Input::InputLibrary::InitNodeRegistry(nodeRegistry);
            }
        }
        void Activate() override { }
        void Deactivate() override { }
    };

    class StartingPointInputModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(StartingPointInputModule, "{B30D421E-127D-4C46-90B1-AC3DDF3EC1D9}", AZ::Module);

        StartingPointInputModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StartingPointInputDummyComponent::CreateDescriptor(),
            });

            AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(Input::InputLibrary::GetComponentDescriptors());
            m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        { 
            return AZ::ComponentTypeList({ StartingPointInputDummyComponent::RTTI_Type() }); 
        }
    };
}


namespace ClassConverters
{
    // This method will convert an input based on the SingleEventToAction class (Held, Input, Released, Analog) to an Input
    // all of the data is transferring over, the behavior will now be handled in script
    static bool ConvertToInput(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        AZ_Error("Input Conversion", classElement.GetNumSubElements() > 0, "You are trying to convert a Held, Pressed, Released, Analog, that is missing it's base class data");
        if (classElement.GetNumSubElements() > 0)
        {
            AZ::SerializeContext::DataElementNode& singleInputToEvent = classElement.GetSubElement(0);
            // store the old data
            AZStd::string deviceType;
            singleInputToEvent.GetChildData(AZ::Crc32("Input Device Type"), deviceType);
            AZStd::string inputName;
            singleInputToEvent.GetChildData(AZ::Crc32("Input Name"), inputName);
            float eventValueMultiplier;
            singleInputToEvent.GetChildData(AZ::Crc32("Event Value Multiplier"), eventValueMultiplier);
            float deadZone;
            singleInputToEvent.GetChildData(AZ::Crc32("Dead Zone"), deadZone);

            // convert the device and input names in case this data wasn't upgraded before the
            // converter from Input v1->v2 was added in Input::ConvertInputVersion1To2 (Input.cpp)
            deviceType = Input::ConvertInputDeviceName(deviceType);
            inputName = Input::ConvertInputEventName(inputName);

            // convert the class
            classElement.Convert(context, AZ::AzTypeInfo<Input::Input>::Uuid());

            //push the data into the new class
            classElement.AddElementWithData(context, "Input Device Type", deviceType);
            classElement.AddElementWithData(context, "Input Name", inputName);
            classElement.AddElementWithData(context, "Event Value Multiplier", eventValueMultiplier);
            classElement.AddElementWithData(context, "Dead Zone", deadZone);
        }
        return true;
    }

    static const int s_deprecatedSingleEventToActionAtVersion = 2;
    static const int s_singleEventToActionVersion = s_deprecatedSingleEventToActionAtVersion + 1;
    static bool BaseClassDeprecator(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        int currentUpgradedVersion = classElement.GetVersion();
        while (currentUpgradedVersion != s_singleEventToActionVersion)
        {
            switch (currentUpgradedVersion)
            {
                // Going from version 1 to 2 we added the Dead Zone field to the SingleEventToAction class
                case 1:
                {
                    classElement.AddElementWithData(context, "Dead Zone", 0.2f);
                    break;
                }
                case s_deprecatedSingleEventToActionAtVersion:
                {
                    // because we are pulling data out of the base class we need to have a converter for it, and it needs to succeed, or we will lose all of the data
                    return true;
                }
            }
            ++currentUpgradedVersion;
        }
        return true;
    }

    static const int s_deprecatedHeldAtVersion = 2;
    static const int s_heldVersion = s_deprecatedHeldAtVersion + 1;
    static bool ConvertHeldVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        enum class HeldInvokeType
        {
            EveryDuration,
            OncePerRelease,
            EveryFrameAfterDuration,
        };
        int currentUpgradedVersion = classElement.GetVersion();
        while (currentUpgradedVersion != s_heldVersion)
        {
            switch (currentUpgradedVersion)
            {
                // Going from 1 to 2 we switched from an Invoke Once per release to the HeldInvokeType enum.  Depending on
                // what the previous value of Invoke Once Per Release we would set the enum value to the corresponding option
                case 1:
                {
                    AZ::Crc32 invokeOnceCrc("Invoke Once Per Release");
                    int invokeOncePropertyIndex = classElement.FindElement(invokeOnceCrc);
                    if (invokeOncePropertyIndex != -1)
                    {
                        bool invokeOncePerRelease = true;
                        classElement.GetSubElement(invokeOncePropertyIndex).GetData(invokeOncePerRelease);
                        classElement.ReplaceElement<HeldInvokeType>(context, invokeOncePropertyIndex, "Invoke Type");
                        HeldInvokeType invokeType = HeldInvokeType::OncePerRelease;
                        if (!invokeOncePerRelease)
                        {
                            float holdDuration = 0.1f;
                            classElement.GetChildData(AZ::Crc32("Duration To Hold"), holdDuration);
                            classElement.AddElementWithData(context, "Success Pulse Interval", holdDuration);
                            invokeType = HeldInvokeType::EveryDuration;
                        }
                        classElement.GetSubElement(invokeOncePropertyIndex).SetData(context, invokeType);
                        break;
                    }
                    else
                    {
                        AZ_Warning("Input", false, "Unable to convert Held from version 1 to 2, its data will be lost on save");
                        return false;
                    }
                }
                case s_deprecatedHeldAtVersion:
                {
                    return ConvertToInput(context, classElement);
                }
                default:
                    AZ_Warning("Input", false, "Unable to convert Held: unsupported version %i, its data will be lost on save", currentUpgradedVersion);
                    return false;
            }
            ++currentUpgradedVersion;
        }
        return true;
    }

    static const int s_deprecatedAnalogVersion = 2;
    static const int s_analogVersion = s_deprecatedAnalogVersion + 1;
    static bool ConvertAnalogVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        int currentUpgradedVersion = classElement.GetVersion();
        while (currentUpgradedVersion != s_analogVersion)
        {
            switch (currentUpgradedVersion)
            {
                case 1:
                {
                    // Going from version 1 to 2 on Analog we removed the Dead Zone field as it was added to the base class
                    // We needed to set the data in the base class to whatever had been set in the analog originally
                    float currentDeadZone = AZ::g_fltEps;
                    classElement.GetChildData(AZ::Crc32("Dead Zone"), currentDeadZone);
                    int singleEventToActionPropertyIndex = classElement.FindElement(AZ::Crc32("BaseClass1"));
                    if (singleEventToActionPropertyIndex != -1)
                    {
                        auto& baseClass = classElement.GetSubElement(singleEventToActionPropertyIndex);
                        if (baseClass.GetVersion() == 2)
                        {
                            int deadZoneIndex = baseClass.FindElement(AZ::Crc32("Dead Zone"));
                            if (deadZoneIndex != -1)
                            {
                                // in case the base class updates before us
                                baseClass.GetSubElement(deadZoneIndex).SetData(context, currentDeadZone);
                            }
                        }
                        break;
                    }
                    else
                    {
                        AZ_Warning("Input", false, "Unable to convert Analog from version 1 to 2, its data will be lost on save");
                        return false;
                    }
                }
                case s_deprecatedAnalogVersion:
                {
                    return ConvertToInput(context, classElement);
                }
                default:
                    AZ_Warning("Input", false, "Unable to convert Analog: unsupported version %i, its data will be lost on save", currentUpgradedVersion);
                    return false;
            }
            ++currentUpgradedVersion;
        }
        return true;
    }
}


// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(StartingPointInput_09f4bedeee614358bc36788e77f97e51, StartingPointInput::StartingPointInputModule)

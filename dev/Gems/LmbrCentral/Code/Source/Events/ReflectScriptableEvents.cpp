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
#include "Events/ReflectScriptableEvents.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <GameplayEventBus.h>
#include <InputEventBus.h>
#include <InputRequestBus.h>
#include <AzCore/Math/Vector3.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzFramework/Math/MathUtils.h>


namespace LmbrCentral
{
    /// BahaviorContext forwarder for FloatGameplayNotificationBus
    class BehaviorInputEventNotificationBusHandler : public AZ::InputEventNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorInputEventNotificationBusHandler, "{8AAEEB1A-21E2-4D2E-A719-73552D41F506}", AZ::SystemAllocator,
            OnPressed, OnHeld, OnReleased);

        void OnPressed(float value) override
        {
            Call(FN_OnPressed, value);
        }

        void OnHeld(float value) override
        {
            Call(FN_OnHeld, value);
        }

        void OnReleased(float value) override
        {
            Call(FN_OnReleased, value);
        }
    };

    /// BahaviorContext forwarder for FloatGameplayNotificationBus
    class BehaviorGameplayNotificationBusHandler : public AZ::GameplayNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorGameplayNotificationBusHandler, "{227DCFE6-B527-4FED-8A4D-5D723B07EAA5}", AZ::SystemAllocator,
            OnEventBegin, OnEventUpdating, OnEventEnd);

        void OnEventBegin(const AZStd::any& value) override
        {
            Call(FN_OnEventBegin, value);
        }

        void OnEventUpdating(const AZStd::any& value) override
        {
            Call(FN_OnEventUpdating, value);
        }

        void OnEventEnd(const AZStd::any& value) override
        {
            Call(FN_OnEventEnd, value);
        }
    };

    class MathUtils
    {
    public:
        AZ_TYPE_INFO(MathUtils, "{BB7F7465-B355-4435-BB9D-44D8F586EE8B}");
        AZ_CLASS_ALLOCATOR(MathUtils, AZ::SystemAllocator, 0);

        MathUtils() = default;
        ~MathUtils() = default;
    };

    class AxisWrapper
    {
    public:
        AZ_TYPE_INFO(AxisWrapper, "{86817913-7D0C-4883-8EDC-2B0DE643392B}");
        AZ_CLASS_ALLOCATOR(AxisWrapper, AZ::SystemAllocator, 0);

        AxisWrapper() = default;
        ~AxisWrapper() = default;
    };

    void InputEventNonIntrusiveConstructor(AZ::InputEventNotificationId* thisOutPtr, AZ::ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 0)
        {
            // Use defaults.
        }
        else if (dc.GetNumArguments() == 1 && dc.IsString(0))
        {

            const char* currentProfileName = nullptr;
            EBUS_EVENT_RESULT(currentProfileName, AZ::PlayerProfileRequestBus, GetCurrentProfileForCurrentUser);
            thisOutPtr->m_profileIdCrc = Input::ProfileId(currentProfileName);
            const char* actionName = nullptr;
            dc.ReadArg(0, actionName);
            thisOutPtr->m_actionNameCrc = Input::ProcessedEventName(actionName);
        }
        else if (dc.GetNumArguments() == 2 && dc.IsClass<AZ::Crc32>(0) && dc.IsString(1))
        {
            Input::ProfileId profileId = 0;
            dc.ReadArg(0, profileId);
            thisOutPtr->m_profileIdCrc = profileId;
            const char* actionName = nullptr;
            dc.ReadArg(1, actionName);
            thisOutPtr->m_actionNameCrc = Input::ProcessedEventName(actionName);
        }
        else
        {
            AZ_Error("InputEventNotificationId", false, "The InputEventNotificationId takes one or two args. 1 argument: a string representing the input events name (determined by the event group). 2 arguments: a Crc of the profile channel, and a string representing the input event's name");
        }
    }

    void GameplayEventIdNonIntrusiveConstructor(AZ::GameplayNotificationId* outData, AZ::ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 0)
        {
            // Use defaults.
            outData->m_channel.SetInvalid();
            outData->m_actionNameCrc = 0;
            outData->m_payloadTypeId = AZ::Uuid::CreateNull();
        }
        else if (dc.GetNumArguments() == 2 && dc.IsClass<AZ::EntityId>(0) && dc.IsString(1))
        {
            AZ::EntityId channel(0);
            dc.ReadArg(0, channel);
            outData->m_channel = channel;
            const char* actionName = nullptr;
            dc.ReadArg(1, actionName);
            outData->m_actionNameCrc = AZ_CRC(actionName);
            outData->m_payloadTypeId = AZ::Uuid::CreateNull();
            AZ_WarningOnce(AZStd::string::format("GameplayNotificationId %s", outData->ToString().c_str()).c_str(), false, "This constructor has been deprecated.  Please add the name of the type you wish to send/receive, example 'float'");
        }
        else if (dc.GetNumArguments() == 3 && dc.IsClass<AZ::EntityId>(0) 
            && (dc.IsString(1) || dc.IsClass<AZ::Crc32>(1))
            && (dc.IsString(2) || dc.IsClass<AZ::Crc32>(2) || dc.IsClass<AZ::Uuid>(2)))
        {
            dc.ReadArg(0, outData->m_channel);
            if (dc.IsString(1))
            {
                const char* actionName = nullptr;
                dc.ReadArg(1, actionName);
                outData->m_actionNameCrc = AZ_CRC(actionName);
            }
            else
            {
                dc.ReadArg(1, outData->m_actionNameCrc);
            }

            if (dc.IsClass<AZ::Uuid>(2))
            {
                dc.ReadArg(2, outData->m_payloadTypeId);
            }
            else
            {
                const char* payloadClassName = nullptr;
                AZ::Crc32 payloadClassCrc;
                if (dc.IsString(2))
                {
                    dc.ReadArg(2, payloadClassName);
                    payloadClassCrc = AZ::Crc32(payloadClassName);
                }
                else
                {
                    dc.ReadArg(2, payloadClassCrc);
                }

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZStd::vector<AZ::Uuid> classIds = serializeContext->FindClassId(payloadClassCrc);
                if (classIds.size() == 1)
                {
                    outData->m_payloadTypeId = classIds[0];
                }
                else
                {
                    AZStd::string errorClassName = payloadClassName ? payloadClassName : AZStd::to_string(payloadClassCrc);
                    if (classIds.size() == 0)
                    {
                        outData->m_payloadTypeId = AZ::Uuid::CreateNull();
                        AZ_Warning("GameplayNotificationId", false, "No class found with key %s", errorClassName.c_str());
                    }
                    else
                    {
                        outData->m_payloadTypeId = AZ::Uuid::CreateNull();
                        AZStd::string errorOutput = AZStd::string::format("Too many classes with key %s.  You may need to create a Uiid via Uuid.CreateString() using one of the following uuids: ", errorClassName.c_str());
                        for (AZ::Uuid classId : classIds)
                        {
                            errorOutput += AZStd::string::format("%s, ", classId.ToString<AZStd::string>().c_str());
                        }
                        AZ_Warning("GameplayNotificationId", false, errorOutput.c_str());
                    }
                }
            }
        }
        else
        {
            AZ_Error("GameplayNotificationId", false, "The GameplayNotificationId takes 3 arguments: an entityId representing the channel, a string or crc representing the action's name, and a string or crc or uuid for the type");
        }
    }

    void ReflectScriptableEvents::Reflect(AZ::BehaviorContext* behaviorContext)
    {
        if (behaviorContext)
        {
            behaviorContext->Class<AZ::GameplayNotificationId>("GameplayNotificationId")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Constructor<AZ::EntityId, AZ::Crc32>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &GameplayEventIdNonIntrusiveConstructor)
                ->Property("actionNameCrc", BehaviorValueProperty(&AZ::GameplayNotificationId::m_actionNameCrc))
                ->Property("channel", BehaviorValueProperty(&AZ::GameplayNotificationId::m_channel))
                ->Property("payloadTypeId", BehaviorValueProperty(&AZ::GameplayNotificationId::m_payloadTypeId))
                ->Method("ToString", &AZ::GameplayNotificationId::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("Equal", &AZ::GameplayNotificationId::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("Clone", &AZ::GameplayNotificationId::Clone);

            behaviorContext->Class<AZ::InputEventNotificationId>("InputEventNotificationId")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Constructor<const char*>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &InputEventNonIntrusiveConstructor)
                ->Property("actionNameCrc", BehaviorValueProperty(&AZ::InputEventNotificationId::m_actionNameCrc))
                ->Property("profileCrc", BehaviorValueProperty(&AZ::InputEventNotificationId::m_profileIdCrc))
                ->Method("ToString", &AZ::InputEventNotificationId::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("Equal", &AZ::InputEventNotificationId::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("Clone", &AZ::InputEventNotificationId::Clone)
                ->Property("actionName", nullptr, [](AZ::InputEventNotificationId* thisPtr, AZStd::string_view value) { *thisPtr = AZ::InputEventNotificationId(value.data()); })
                ->Method("CreateInputEventNotificationId", [](AZStd::string_view value) -> AZ::InputEventNotificationId { return AZ::InputEventNotificationId(value.data()); },
                { { { "actionName", "The name of the Input event action used to create an InputEventNotificationId" } } })
                ;

            behaviorContext->EBus<AZ::GameplayNotificationBus>("GameplayNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<BehaviorGameplayNotificationBusHandler>()
                ->Event("OnEventBegin", &AZ::GameplayNotificationBus::Events::OnEventBegin)
                ->Event("OnEventUpdating", &AZ::GameplayNotificationBus::Events::OnEventUpdating)
                ->Event("OnEventEnd", &AZ::GameplayNotificationBus::Events::OnEventEnd);

            behaviorContext->Class<AxisWrapper>("AxisType")
                ->Constant("XPositive", BehaviorConstant(AzFramework::Axis::XPositive))
                ->Constant("XNegative", BehaviorConstant(AzFramework::Axis::XNegative))
                ->Constant("YPositive", BehaviorConstant(AzFramework::Axis::YPositive))
                ->Constant("YNegative", BehaviorConstant(AzFramework::Axis::YNegative))
                ->Constant("ZPositive", BehaviorConstant(AzFramework::Axis::ZPositive))
                ->Constant("ZNegative", BehaviorConstant(AzFramework::Axis::ZNegative));

            behaviorContext->Class<MathUtils>("MathUtils")
                ->Method("ConvertTransformToEulerDegrees", &AzFramework::ConvertTransformToEulerDegrees)
                ->Method("ConvertTransformToEulerRadians", &AzFramework::ConvertTransformToEulerRadians)
                ->Method("ConvertEulerDegreesToTransform", &AzFramework::ConvertEulerDegreesToTransform)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("ConvertEulerDegreesToTransformPrecise", &AzFramework::ConvertEulerDegreesToTransformPrecise)
                ->Method("ConvertQuaternionToEulerDegrees", &AzFramework::ConvertQuaternionToEulerDegrees)
                ->Method("ConvertQuaternionToEulerRadians", &AzFramework::ConvertQuaternionToEulerRadians)
                ->Method("ConvertEulerRadiansToQuaternion", &AzFramework::ConvertEulerRadiansToQuaternion)
                ->Method("ConvertEulerDegreesToQuaternion", &AzFramework::ConvertEulerDegreesToQuaternion)
                ->Method("CreateLookAt", &AzFramework::CreateLookAt);

            ShapeComponentGeneric::Reflect(behaviorContext);


            behaviorContext->EBus<AZ::InputEventNotificationBus>("InputEventNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<BehaviorInputEventNotificationBusHandler>();

            behaviorContext->EBus<AZ::InputRequestBus>("InputRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("PushContext", &AZ::InputRequestBus::Events::PushContext)
                ->Event("PopContext", &AZ::InputRequestBus::Events::PopContext)
                ->Event("PopAllContexts", &AZ::InputRequestBus::Events::PopAllContexts)
                ->Event("GetCurrentContext", &AZ::InputRequestBus::Events::GetCurrentContext);
        }
    }

}
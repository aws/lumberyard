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

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>

namespace StarterGameGem
{
    /*!
    * StatRequests
    * Messages serviced by the StatComponent.
    */
    class StatRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~StatRequests() {}

        // NOTE:
        // because it is in the design for mutiple instances of stat objets to exist on a singular entity (for different stats)
        // all the methods take the state name that it is interested in so we can use the bus but only have things that have the specified
        // name respond, because otherwise targeting them nicely is impossible.
        // the name will be a perperty that is effictivly const when in game to avoid possible unintentional interactions
        virtual bool SetMax(const AZStd::string& statName, const float max) = 0;
        virtual bool SetMin(const AZStd::string& statName, const float min) = 0;
        virtual bool SetValue(const AZStd::string& statName, const float value) = 0;
        virtual bool SetRegenSpeed(const AZStd::string& statName, const float value) = 0;
        virtual bool SetRegenDelay(const AZStd::string& statName, const float value) = 0;

        // making these const seems to cause problems
        virtual bool GetMax(const AZStd::string& statName, float& max) /*const*/ = 0;
        virtual bool GetMin(const AZStd::string& statName, float& min) /*const*/ = 0;
        virtual bool GetValue(const AZStd::string& statName, float& value) /*const*/ = 0;
        virtual bool GetRegenSpeed(const AZStd::string& statName, float& value) /*const*/ = 0;
        virtual bool GetRegenDelay(const AZStd::string& statName, float& value) /*const*/ = 0;

        virtual bool DeltaValue(const AZStd::string& statName, const float value) = 0;
        virtual bool Reset(const AZStd::string& statName) = 0;
    };
    using StatRequestBus = AZ::EBus<StatRequests>;


    class StatComponent
        : public AZ::Component
        , private StatRequestBus::Handler
        , public AZ::GameplayNotificationBus::MultiHandler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(StatComponent, "{76D127CC-5E52-4DC9-8B36-DEE62A7BAC39}");

        StatComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override        {}
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        // Optional functions for defining provided and dependent services.
        //static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        //{
        //  required.push_back(AZ_CRC("TransformService"));
        //}

        void SetMax(float max);
        void SetMin(float min);
        void SetValue(float value);
        void SetRegenSpeed(float value);
        void SetRegenDelay(float value);

        void SetFullEvent(const char* name);
        void SetEmptyEvent(const char* name);
        void SetRegenStartEvent(const char* name);
        void SetRegenEndEvent(const char* name);
        void SetValueChangedEvent(const char* name);

        float GetMax() const;
        float GetMin() const;
        float GetValue() const;
        float GetRegenSpeed() const;
        float GetRegenDelay() const;

        AZStd::string GetFullEvent() const;
        AZStd::string GetEmptyEvent() const;
        AZStd::string GetRegenStartEvent() const;
        AZStd::string GetRegenEndEvent() const;
        AZStd::string GetValueChangedEvent() const;

        void DeltaValue(float value);
        void Reset();
        //////////////////////////////////////////////////////////////////////////
        // StatComponentRequestBus::Handler
        bool SetMax(const AZStd::string& statName, const float max);
        bool SetMin(const AZStd::string& statName, const float min);
        bool SetValue(const AZStd::string& statName, const float value);
        bool SetRegenSpeed(const AZStd::string& statName, const float value);
        bool SetRegenDelay(const AZStd::string& statName, const float value);

        bool GetMax(const AZStd::string& statName, float& max) /*const*/;
        bool GetMin(const AZStd::string& statName, float& min) /*const*/;
        bool GetValue(const AZStd::string& statName, float& value) /*const*/;
        bool GetRegenSpeed(const AZStd::string& statName, float& value) /*const*/;
        bool GetRegenDelay(const AZStd::string& statName, float& value) /*const*/;

        bool DeltaValue(const AZStd::string& statName, const float value);
        bool Reset(const AZStd::string& statName);
        //////////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // GameplayNotificationBus::MultiHandler
        void OnEventBegin(const AZStd::any& value) override;
        void OnEventUpdating(const AZStd::any&) override;
        void OnEventEnd(const AZStd::any&) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        AZStd::string m_name;
        float m_max;
        float m_min;
        float m_value;
        float m_regenSpeed;
        float m_regenDelay;

        // storage for the initilised values, so i can be "reset"
        float m_defaultMax;
        float m_defaultMin;
        float m_defaultValue;
        float m_defaultRegenSpeed;
        float m_defaultRegenDelay;

        AZStd::string m_fullEventName;
        AZStd::string m_emptyEventName;
        AZStd::string m_regenStartEventName;
        AZStd::string m_regenEndEventName;
        AZStd::string m_valueChangedEventName;

        AZStd::string m_setMaxEventName;
        AZStd::string m_setMinEventName;
        AZStd::string m_setRegenSpeedEventName;
        AZStd::string m_setRegenDelayEventName;
        AZStd::string m_setValueEventName;
        AZStd::string m_deltaValueEventName;
        AZStd::string m_regenResetEventName;
        AZStd::string m_resetEventName;

        AZStd::string m_GetValueEventName;
        AZStd::string m_SendValueEventName;
        AZStd::string m_GetUsedCapacityEventName;
        AZStd::string m_SendUsedCapacityEventName;
        AZStd::string m_GetUnUsedCapacityEventName;
        AZStd::string m_SendUnUsedCapacityEventName;
        AZStd::string m_GetMinEventName;
        AZStd::string m_SendMinEventName;
        AZStd::string m_GetMaxEventName;
        AZStd::string m_SendMaxEventName;

    private:
        float m_timer;
        bool m_isRegening;
        void ValueChanged(bool resetTimer = true, bool regenStarted = false);

        AZ::GameplayNotificationId m_setMaxEventID;
        AZ::GameplayNotificationId m_setMinEventID;
        AZ::GameplayNotificationId m_setRegenSpeedEventID;
        AZ::GameplayNotificationId m_setRegenDelayEventID;
        AZ::GameplayNotificationId m_setValueEventID;
        AZ::GameplayNotificationId m_deltaValueEventID;
        AZ::GameplayNotificationId m_regenResetEventID;
        AZ::GameplayNotificationId m_resetEventID;
        AZ::GameplayNotificationId m_GetValueEventID;
        AZ::GameplayNotificationId m_GetUsedCapacityEventID;
        AZ::GameplayNotificationId m_GetUnUsedCapacityEventID;
        AZ::GameplayNotificationId m_GetMinEventID;
        AZ::GameplayNotificationId m_GetMaxEventID;
    };
}

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
*/#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>

#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>


namespace StarterGameGem
{
    class ConsoleVarListenerComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides (Configuring this Ebus)
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~ConsoleVarListenerComponentNotifications() {}

        /**
        * Indicates that a console variable is about to be changed.
        * @param name The name of the console variable.
        * @param valueOld The current value of the console variable (before it's changed).
        * @param valueNew The new, proposed value of the console variable.
        * @return boolean If false, the change will be rejected.
        */
        virtual bool OnBeforeConsoleVarChanged(AZStd::string name, AZStd::any valueOld, AZStd::any valueNew)
        {
            return true;
        };

        /**
        * Indicates that a console variable has been changed. The callback doesn't contain the old
        * value as it has already been over-written.
        * @param name The name of the console variable.
        * @param value The new value of the console variable.
        */
        virtual void OnAfterConsoleVarChanged(AZStd::string name, AZStd::any value) {};
    };

    using ConsoleVarListenerComponentNotificationBus = AZ::EBus<ConsoleVarListenerComponentNotifications>;


    class ConsoleVarListenerComponent
        : public AZ::Component
        , public IConsoleVarSink
    {
    public:
        AZ_COMPONENT(ConsoleVarListenerComponent, "{D6584BE3-8822-47F1-901B-596C8C738640}");

        ConsoleVarListenerComponent();

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        virtual AZ::u32 MajorPropertyChanged();

        bool IsListeningForSpecificVariables() const;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // IConsoleVarSink overrides
        // Called by Console before changing console var value, to validate if var can be changed.
        // Return value: true if ok to change value, false if should not change value.
        virtual bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue);
        // Called by Console after variable has changed value.
        virtual void OnAfterVarChange(ICVar* pVar);
        //////////////////////////////////////////////////////////////////////////

    private:
        bool CVarIsRelevant(ICVar* pVar) const;
        AZStd::any ConvertCVarToAny(ICVar* pVar) const;

        bool m_all;
        AZStd::vector<AZStd::string> m_consoleVars;
    };
}

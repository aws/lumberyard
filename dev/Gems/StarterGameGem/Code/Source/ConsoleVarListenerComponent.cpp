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
#include "StarterGameGem_precompiled.h"
#include "ConsoleVarListenerComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>


namespace StarterGameGem
{
    // Behavior Context forwarder for ConsoleVarListenerComponentNotificationBus
    class ConsoleVarListenerComponentNotificationBusHandler
        : public ConsoleVarListenerComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ConsoleVarListenerComponentNotificationBusHandler,"{F9B0CC11-4D2C-451F-BCC3-E8CF7D3B339D}", AZ::SystemAllocator,
            OnBeforeConsoleVarChanged, OnAfterConsoleVarChanged);

        bool OnBeforeConsoleVarChanged(AZStd::string name, AZStd::any valueOld, AZStd::any valueNew) override
        {
            bool allowChange = true;
            CallResult(allowChange, FN_OnBeforeConsoleVarChanged, name, valueOld, valueNew);
            return allowChange;
        }

        void OnAfterConsoleVarChanged(AZStd::string name, AZStd::any value) override
        {
            Call(FN_OnAfterConsoleVarChanged, name, value);
        }
        
    };

    ConsoleVarListenerComponent::ConsoleVarListenerComponent()
        : m_all(false)
        , m_consoleVars("")
    {}

    void ConsoleVarListenerComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ConsoleVarListenerComponent, AZ::Component>()
                ->Version(1)
                ->Field("All", &ConsoleVarListenerComponent::m_all)
                ->Field("ConsoleVars", &ConsoleVarListenerComponent::m_consoleVars)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ConsoleVarListenerComponent>("Console Var Listener", "Listens for changes in a console variable")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Game")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(0, &ConsoleVarListenerComponent::m_all, "All?", "If ticked, will listen for all console variables.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ConsoleVarListenerComponent::MajorPropertyChanged)
                    ->DataElement(0, &ConsoleVarListenerComponent::m_consoleVars, "Console Vars", "The name of the console variables.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ConsoleVarListenerComponent::IsListeningForSpecificVariables)
				;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<ConsoleVarListenerComponentNotificationBus>("ConsoleVarListenerComponentNotificationBus")
                ->Handler<ConsoleVarListenerComponentNotificationBusHandler>()
            ;
        }
    }

    AZ::u32 ConsoleVarListenerComponent::MajorPropertyChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool ConsoleVarListenerComponent::IsListeningForSpecificVariables() const
    {
        return !m_all;
    }

    void ConsoleVarListenerComponent::Activate()
    {
        gEnv->pConsole->AddConsoleVarSink(this);
    }

	void ConsoleVarListenerComponent::Deactivate()
	{
        gEnv->pConsole->RemoveConsoleVarSink(this);
    }

    bool ConsoleVarListenerComponent::OnBeforeVarChange(ICVar* pVar, const char* sNewValue)
    {
        // If it's not a console variable we care about then don't interfere.
        if (!CVarIsRelevant(pVar))
        {
            return true;
        }

        bool allowChange = true;
        AZStd::any valueOld = ConvertCVarToAny(pVar);
        AZStd::string str(sNewValue);   // this NEEDS to be put into an AZStd::string on a SEPARATE line otherwise it'll fail
        AZStd::any valueNew(str);
        EBUS_EVENT_ID_RESULT(allowChange, GetEntityId(), ConsoleVarListenerComponentNotificationBus, OnBeforeConsoleVarChanged, pVar->GetName(), valueOld, valueNew);

        return allowChange;
    }

    void ConsoleVarListenerComponent::OnAfterVarChange(ICVar* pVar)
    {
        // If it's not a console variable we care about then exit now.
        if (!CVarIsRelevant(pVar))
        {
            return;
        }

        AZStd::any value = ConvertCVarToAny(pVar);
        EBUS_EVENT_ID(GetEntityId(), ConsoleVarListenerComponentNotificationBus, OnAfterConsoleVarChanged, pVar->GetName(), value);
    }

    bool ConsoleVarListenerComponent::CVarIsRelevant(ICVar* pVar) const
    {
        if (m_all)
        {
            return true;
        }

        for (size_t i = 0; i < m_consoleVars.size(); ++i)
        {
            if (strcmp(pVar->GetName(), m_consoleVars[i].c_str()) == 0)
            {
                return true;
            }
        }
        return false;
    }

    AZStd::any ConsoleVarListenerComponent::ConvertCVarToAny(ICVar* pVar) const
    {
        AZStd::any value;
        switch(pVar->GetType())
        {
            case CVAR_INT:
                value = AZStd::any(pVar->GetIVal());
            break;

            case CVAR_FLOAT:
                value = AZStd::any(pVar->GetFVal());
            break;

            case CVAR_STRING:
            {
                AZStd::string str(pVar->GetString());   // this NEEDS to be put into an AZStd::string on a SEPARATE line otherwise it'll fail
                value = AZStd::any(str);
            }
            break;

            default:
                AZ_Assert(true, "%s: Don't recognise the console variable [%s] value type.", __FUNCTION__, pVar->GetName());
        }

        return value;
    }
}


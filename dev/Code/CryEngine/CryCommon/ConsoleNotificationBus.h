#pragma once
// Send EBus notifications when console variables change,
// so that we do not need to create Gem-specific EBuses only to redirect CVar's static OnChanged notification
// to the actual listener. Additionally, a console command can be registered with a generic handler
// that redirects the command execution to the EBus handler .

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <IConsole.h>

namespace AZ
{
    // Subscribe to AZ::Crc32() (crc zero) to get notifications about all variables.
    class ConsoleNotifications : public AZ::EBusTraits
    {
    public:
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        using BusIdType = AZ::Crc32;

        virtual bool OnVariableChanging(const ICVar* cvar) { return true; };
        virtual void OnVariableChanged(const ICVar* cvar) {};
        virtual void OnExecuteConsoleCommand(const IConsoleCmdArgs* args) {};
    };
    using ConsoleNotificationBus = AZ::EBus<ConsoleNotifications>;

    // Helper method that forwards console command request to an EBus event. This can be used with REGISTER_COMMAND macros.
    class ConsoleCommand
    {
    public:
        static AZ_INLINE void UseConsoleNotificationBus(IConsoleCmdArgs* args)
        {
            EBUS_EVENT_ID(AZ::Crc32(args->GetArg(0)), AZ::ConsoleNotificationBus, OnExecuteConsoleCommand, args);
        }
    };
}
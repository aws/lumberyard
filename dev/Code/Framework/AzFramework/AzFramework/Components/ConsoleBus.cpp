
#include "ConsoleBus.h"
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework
{
    void ConsoleRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ConsoleRequestBus>("ConsoleRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Utilities")
                ->Event("ExecuteConsoleCommand", &ConsoleRequestBus::Events::ExecuteConsoleCommand)
                ;
        }
    }
}

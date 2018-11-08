#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Network/NetBindingSystemBus.h>

namespace AZ
{
    /**
     * Abstract Component for logic that need to handle Tick update in game session
     * for dedicated server Tick update start only after session is hosted, to prevent CPU load in standby mode
     * for game and editor Tick update start after component activation
     */
    class SessionTickComponent
        : public Component
        , public TickBus::Handler
        , public AzFramework::NetBindingSystemEventsBus::Handler
    {
    public:

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // NetBindingSystemBus
        void OnNetworkSessionCreated(GridMate::GridSession* session) override;
        ////////////////////////////////////////////////////////////////////////

    };
}
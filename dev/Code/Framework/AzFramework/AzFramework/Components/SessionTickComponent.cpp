#include "SessionTickComponent.h"

namespace AZ
{
    void SessionTickComponent::Activate()
    {
#ifdef DEDICATED_SERVER
        AzFramework::NetBindingSystemEventsBus::Handler::BusConnect();
#else
        AZ::TickBus::Handler::BusConnect();
#endif
    }

    void SessionTickComponent::Deactivate()
    {
#ifdef DEDICATED_SERVER
        AzFramework::NetBindingSystemEventsBus::Handler::BusDisconnect();
#endif
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SessionTickComponent::OnNetworkSessionCreated(GridMate::GridSession* session)
    {
        (void)session;
        AZ::TickBus::Handler::BusConnect();
    }

} // namespace AZ

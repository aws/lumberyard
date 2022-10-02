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

#include <Websockets/WebsocketsBus.h>
#include <WebsocketClient.h>
#include <SecureWebsocketClient.h>

namespace Websockets
{
    class WebsocketsSystemComponent
        : public AZ::Component
        , protected WebsocketsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(WebsocketsSystemComponent, "{A5000EED-F872-4C36-8053-A45529010E48}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        void Activate() override;
        void Deactivate() override;

        AZStd::unique_ptr<IWebsocketClient> CreateClientWithAuth(const AZStd::string& websocket,
            const OnMessage& messageFunc, const AZStd::string& authorization) override;

        AZStd::unique_ptr<IWebsocketClient> CreateClient(const AZStd::string& websocket,
            const OnMessage& messageFunc, WebsocketType type = WebsocketType::Secure) override;
    };

} // namespace Websockets

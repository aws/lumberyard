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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Component/Component.h>
#include <CrySystemBus.h>

#include "Cinematics/Movie.h"
#include "Maestro/MaestroBus.h"

namespace Maestro
{
    //////////////////////////////////////////////////////////////////////////
    struct CSystemEventListener_Movie
        : public ISystemEventListener
    {
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    };

    //////////////////////////////////////////////////////////////////////////
    class MaestroSystemComponent
        : public AZ::Component
        , protected MaestroRequestBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(MaestroSystemComponent, "{47991994-4417-4CD7-AE0B-FEF1C8720766}");

        MaestroSystemComponent() = default;
        // The MaestroSystemComponent is a singleton, so should never by copied. An explicit copy constuctor is 
        // required to avoid a compile error with the implicit default copy constructor that is generated, which
        // fails because the AZStd::unique_ptr members cannot be copied.
        MaestroSystemComponent(const MaestroSystemComponent&) 
        { 
            AZ_Assert(false, "MaestroSystemComponent copy constuctor called - this is a singleton, something is wrong."); 
        }

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // MaestroRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        // CrySystemEventBus ///////////////////////////////////////////////////////
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
        virtual void OnCrySystemShutdown(ISystem&) override;
        ////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // singletons representing the movie system
        AZStd::unique_ptr<CMovieSystem> m_movieSystem;
        AZStd::unique_ptr<CSystemEventListener_Movie> m_movieSystemEventListener;
    };
}

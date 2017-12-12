
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemSamples/CloudGemSamplesBus.h>
#include <IEditorGame.h>
#include "System/GameStartup.h"

namespace LYGame
{
    class CloudGemSamplesSystemComponent
        : public AZ::Component
        , protected EditorGameRequestBus::Handler
        , protected CloudGemSamplesRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemSamplesSystemComponent, "{A3ACD99F-8DE4-4C2B-8D1C-2BAA35CB2B88}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemSamplesRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorGameRequestBus interface implementation
        IGameStartup* CreateGameStartup() override;
        IEditorGame* CreateEditorGame() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}

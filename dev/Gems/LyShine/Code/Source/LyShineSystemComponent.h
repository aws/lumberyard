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

#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <LyShine/LyShineBus.h>
#include <LyShine/Bus/UiSystemBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <LyShine/UiComponentTypes.h>
#include "LyShine.h"

namespace LyShine
{
    class LyShineSystemComponent
        : public AZ::Component
        , protected LyShineRequestBus::Handler
        , protected UiSystemBus::Handler
        , protected UiSystemToolsBus::Handler
    {
    public:
        AZ_COMPONENT(LyShineSystemComponent, lyShineSystemComponentUuid);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static void SetLyShineComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors);

        LyShineSystemComponent();

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemBus interface implementation
        void InitializeSystem() override;
        void RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid) override;
        const AZStd::vector<AZ::Uuid>* GetComponentTypesForMenuOrdering() override;
        const AZStd::list<AZ::ComponentDescriptor*>* GetLyShineComponentDescriptors();
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // UiSystemToolsInterface interface implementation
        CanvasAssetHandle* LoadCanvasFromStream(AZ::IO::FileIOStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc) override;
        void SaveCanvasToStream(CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream) override;
        AZ::SliceComponent* GetRootSliceSliceComponent(CanvasAssetHandle* canvas) override;
        AZ::Entity* GetRootSliceEntity(CanvasAssetHandle* canvas) override;
        void ReplaceRootSliceSliceComponent(CanvasAssetHandle* canvas, AZ::SliceComponent* newSliceComponent) override;
        void DestroyCanvas(CanvasAssetHandle* canvas) override;
        ////////////////////////////////////////////////////////////////////////

        void BroadcastCursorImagePathname();

    protected:  // data

        CLyShine* m_pLyShine = nullptr;

        AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_cursorImagePathname;

        // The components types registers in order to cotrol their order in the add component
        // menu and the properties pane - may go away soon
        AZStd::vector<AZ::Uuid> m_componentTypes;

        // We only store this in order to generate metrics on LyShine specific components
        static const AZStd::list<AZ::ComponentDescriptor*>* m_componentDescriptors;
    };
}

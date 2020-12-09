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
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

#include "ClipVolumeComponent.h"

namespace LmbrCentral
{
    /*!
     * In-editor clip volume component.
     */
    class EditorClipVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public PolygonPrismShapeComponentNotificationBus::Handler
        , public ClipVolumeComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EditorClipVolumeComponent, "{BA3890BD-D2E7-4DB6-95CD-7E7D5525562B}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void Init() override;
        
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // PolygonPrismShapeComponentNotificationBus::Handler
        void OnVertexAdded(size_t /*index*/) override { ShapeChanged(); }
        void OnVertexRemoved(size_t /*index*/) override { ShapeChanged(); }
        void OnVertexUpdated(size_t /*index*/) override { ShapeChanged(); }
        void OnVerticesSet(const AZStd::vector<AZ::Vector2>& /*vertices*/) override { ShapeChanged(); }
        void OnVerticesCleared() override { ShapeChanged(); }

        // ClipVolumeComponentRequestBus::Handler
        IClipVolume* GetClipVolume() override { return m_clipVolume.GetClipVolume(); }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ClipVolumeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ClipVolumeService"));
        }

        void BuildGameEntity(AZ::Entity* gameEntity) override;
    private:
        void ShapeChanged();

        ClipVolumeComponentCommon m_clipVolume;
    };
} // namespace LmbrCentral


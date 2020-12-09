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
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Rendering/ClipVolumeComponentBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>

#include <smartptr.h>

struct IClipVolume;
struct IRenderMesh;
struct IBSPTree3D;

namespace LmbrCentral
{
    // Clip volume common holds variables and functionality common to both editor and run-time clip volumes.
    class ClipVolumeComponentCommon
        : public AZ::TransformNotificationBus::Handler
    {
        friend class ClipVolumeComponent;
        friend class EditorClipVolumeComponent;
    public:
        AZ_TYPE_INFO(ClipVolumeComponentCommon, "{46FF2BC4-BEF9-4CC4-9456-36C127C313D7}");

        void Cleanup();

        void CopyPropertiesTo(ClipVolumeComponentCommon& rhs) const;

        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void AttachToEntity(AZ::EntityId entityId);

        void UpdatedVertices(const AZStd::vector<AZ::Vector3>& vertices);

        static void Reflect(AZ::ReflectContext* context);

        IClipVolume* GetClipVolume() { return m_pClipVolume; }

    private:
        void CreateClipVolume();
        void UpdateClipVolume();

        AZ::EntityId m_entityId;
        IClipVolume* m_pClipVolume = nullptr;
        _smart_ptr<IRenderMesh> m_pRenderMesh;
        IBSPTree3D* m_pBspTree = nullptr;

        AZStd::vector<AZ::Vector3> m_vertices;
    };


     // Clip volume component.
     // Handles creating clip volumes ahnd notifying listeners.
    class ClipVolumeComponent
        : public AZ::Component
        , public ClipVolumeComponentRequestBus::Handler
    {
        friend class EditorClipVolumeComponent;
    public:
        AZ_COMPONENT(ClipVolumeComponent, "{1C2CEAA8-786F-4684-8202-CA7D940D629C}");

        ClipVolumeComponent();
        ClipVolumeComponent(AZStd::vector<AZ::Vector3>& vertices);

        // AZ::Component.
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        // ClipVolumeComponentRequestBus
        IClipVolume* GetClipVolume() override { return m_clipVolume.GetClipVolume();  }
        
    protected:
        ClipVolumeComponentCommon m_clipVolume;
        AZStd::vector<AZ::Vector3> m_vertices;
    };
} // namespace LmbrCentral


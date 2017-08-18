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
#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include "LightComponent.h"

#include <IStatObj.h>

namespace LmbrCentral
{
    /**
     * Extends LightConfiguration structure to add editor functionality
     * such as property handlers and visibility filters, as well as
     * reflection for editing.
     */
    class EditorLightConfiguration
        : public LightConfiguration
    {
    public:

        AZ_TYPE_INFO(EditorLightConfiguration, "{1D3B114F-8FB2-47BD-9C21-E089F4F37861}");

        ~EditorLightConfiguration() override {}

        static void Reflect(AZ::ReflectContext* context);

        AZ::Crc32 GetAmbientLightVisibility() const override;
        AZ::Crc32 GetPointLightVisibility() const override;
        AZ::Crc32 GetProjectorLightVisibility() const override;
        AZ::Crc32 GetProbeLightVisibility() const override;
        AZ::Crc32 GetShadowSpecVisibility() const override;
        AZ::Crc32 GetShadowSettingsVisibility() const override;
        AZ::Crc32 GetAreaSettingVisibility() const override;

        AZ::Crc32 MinorPropertyChanged() override;
        AZ::Crc32 MajorPropertyChanged() override;
        AZ::Crc32 OnAnimationSettingChanged() override;
    };

    /*!
     * In-editor light component.
     * Handles previewing and activating lights in the editor.
     */
    class EditorLightComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private LightComponentEditorRequestBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;
    public:
        
        //old guid "{33BB1CD4-6A33-46AA-87ED-8BBB40D94B0D}" before splitting editor light component
        AZ_COMPONENT(EditorLightComponent, "{7C18B273-5BA3-4E0F-857D-1F30BD6B0733}", AzToolsFramework::Components::EditorComponentBase); 
 
        EditorLightComponent();
        ~EditorLightComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorVisibilityNotificationBus interface implementation
        void OnEntityVisibilityChanged(bool visibility) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus interface implementation
        void DisplayEntity(bool& handled) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // LightComponentEditorRequestBus::Handler interface implementation
        void SetCubemap(const char* cubemap) override;
        AZ::u32 GetCubemapResolution() override;
        bool UseCustomizedCubemap() const override;
        void RefreshLight() override;
        const LightConfiguration& GetConfiguration() const override;
        AZ::Crc32 OnCubemapAssetChanged();
        AZ::Crc32 OnCustomizedCubemapChanged();


        ///////////////////////////////////////////
        // LightComponentEditorRequestBus Modifiers
        void SetVisible(bool isVisible) override;
        bool GetVisible() override;

        void SetColor(const AZ::Color& newColor) override;
        const AZ::Color GetColor() override;

        void SetDiffuseMultiplier(float newMultiplier) override;
        float GetDiffuseMultiplier() override;

        void SetSpecularMultiplier(float newMultiplier) override;
        float GetSpecularMultiplier() override;

        void SetAmbient(bool isAmbient) override;
        bool GetAmbient() override;

        void SetPointMaxDistance(float newMaxDistance) override;
        float GetPointMaxDistance() override;

        void SetPointAttenuationBulbSize(float newAttenuationBulbSize) override;
        float GetPointAttenuationBulbSize() override;

        void SetAreaMaxDistance(float newMaxDistance) override;
        float GetAreaMaxDistance() override;

        void SetAreaWidth(float newWidth) override;
        float GetAreaWidth() override;

        void SetAreaHeight(float newHeight) override;
        float GetAreaHeight() override;

        void SetProjectorMaxDistance(float newMaxDistance) override;
        float GetProjectorMaxDistance() override;

        void SetProjectorAttenuationBulbSize(float newAttenuationBulbSize) override;
        float GetProjectorAttenuationBulbSize() override;

        void SetProjectorFOV(float newFOV) override;
        float GetProjectorFOV() override;

        void SetProjectorNearPlane(float newNearPlane) override;
        float GetProjectorNearPlane() override;

        // Environment Probe Settings
        void SetProbeAreaDimensions(const AZ::Vector3& newDimensions) override;
        const AZ::Vector3 GetProbeAreaDimensions() override;

        void SetProbeSortPriority(float newPriority) override;
        float GetProbeSortPriority() override;

        void SetProbeBoxProjected(bool isProbeBoxProjected) override;
        bool GetProbeBoxProjected() override;

        void SetProbeBoxHeight(float newHeight) override;
        float GetProbeBoxHeight() override;

        void SetProbeBoxLength(float newLength) override;
        float GetProbeBoxLength() override;

        void SetProbeBoxWidth(float newWidth) override;
        float GetProbeBoxWidth() override;

        void SetProbeAttenuationFalloff(float newAttenuationFalloff) override;
        float GetProbeAttenuationFalloff() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus::Handler interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus::Handler interface implementation
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LightComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LightComponent::GetRequiredServices(required);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            LightComponent::GetDependentServices(dependent);
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }

        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

    protected:
        void SetLightType(EditorLightConfiguration::LightType lightType)
        {
            m_configuration.m_lightType = lightType;
        }

        virtual const char* GetLightTypeText() const;

    private:

        //! Handles rendering of the preview cubemap by creating a simple cubemapped sphere.
        class CubemapPreview : public IRenderNode
        {
        public:
            CubemapPreview();
            void Setup(const char* textureName);
            void UpdateTexture(const char* textureName);
            void SetTransform(const Matrix34& transform);

        private:
            //////////////////////////////////////////////////////////////////////////
            // IRenderNode interface implementation
            void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
            EERType GetRenderNodeType() override;
            const char* GetName() const override;
            const char* GetEntityClassName() const override;
            Vec3 GetPos(bool bWorldOnly = true) const override;
            const AABB GetBBox() const override;
            void SetBBox(const AABB& WSBBox) override {}
            void OffsetPosition(const Vec3& delta) override {}
            struct IPhysicalEntity* GetPhysics() const override;
            void SetPhysics(IPhysicalEntity* pPhys) override {}
            void SetMaterial(_smart_ptr<IMaterial> pMat) override {}
            _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = nullptr) override;
            _smart_ptr<IMaterial> GetMaterialOverride() override;
            IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = nullptr, bool bReturnOnlyVisible = false);
            float GetMaxViewDist() override;
            void GetMemoryUsage(class ICrySizer* pSizer) const override;
            //////////////////////////////////////////////////////////////////////////
        private:
            Matrix34 m_renderTransform;
            _smart_ptr<IStatObj> m_statObj;
        };

        //if an cubemap asset is added or changed
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        void OnEditorSpecChange() override;

        bool IsProbe() const;
        void GenerateCubemap();
        void OnViewCubemapChanged();

        //return true if it's a probe and not using customized cubemap
        bool CanGenerateCubemap() const;
        
        const char* GetCubemapAssetName() const;

        void DrawProjectionGizmo(AzFramework::EntityDebugDisplayRequests* dc, const float radius) const;
        void DrawPlaneGizmo(AzFramework::EntityDebugDisplayRequests* dc, const float depth) const;

        EditorLightConfiguration m_configuration;
        bool m_viewCubemap;
        bool m_useCustomizedCubemap;
        bool m_cubemapRegen;
        //for cubemap asset selection
        AzFramework::SimpleAssetReference<TextureAsset> m_cubemapAsset;

        CubemapPreview m_cubemapPreview;

        LightInstance m_light;

        //Statics that can be used by every light
        static IEditor*             m_editor;
        static IMaterialManager*    m_materialManager;
    };
} // namespace LmbrCentral


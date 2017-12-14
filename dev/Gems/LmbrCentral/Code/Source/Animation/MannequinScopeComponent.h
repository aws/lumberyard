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
#include <LmbrCentral/Animation/MannequinAsset.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

class IActionController;

namespace LmbrCentral
{
    /* The mannequin scope component allows for associating a runtime character instance with a given scope
    * context, along with an adb file. This component complements the Mannequin component but it cannot
    * function without the same. The mannequin component on the other hand, could use other means to set scope
    * contexts thereby eliminating its dependence on the scope context component.
    */
    class MannequinScopeComponent
        : public AZ::Component
        , private MeshComponentNotificationBus::Handler
    {
    public:

        friend class EditorMannequinScopeComponent;

        AZ_COMPONENT(MannequinScopeComponent, "{AB4FDB4A-D742-4EF8-B36E-9A1775FA6FA5}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        //////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MannequinScopeService", 0x7adf3115));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AnimationService", 0x553f5760));
            required.push_back(AZ_CRC("MannequinService", 0x424b0eea));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        /**
        * \brief Resets / Sets up this component to the indicated mannequin scope
        */
        void ResetMannequinScopeContext();

        /**
        * \brief Clears any mannequin scope context settings for this component
        */
        void ClearMannequinScopeContext();

        //////////////////////////////////////////////////////////////////////////
        // Serialized Data
        // Name of the scope context that this anim database is to be attached to
        AZStd::string m_scopeContextName;

        // The animation database file that is to be attached to this scope context
        AzFramework::SimpleAssetReference<MannequinAnimationDatabaseAsset> m_animationDatabase;

        // Entity Id for the target of this scope context setting
        AZ::EntityId m_targetEntityId;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral

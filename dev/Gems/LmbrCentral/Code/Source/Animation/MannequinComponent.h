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

#include <AzCore/Component/TickBus.h>
#include <LmbrCentral/Animation/MannequinComponentBus.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <LmbrCentral/Animation/MannequinAsset.h>

namespace LmbrCentral
{
    /**
     * Bus for component's internal action event handling.
     */
    class MannequinActionNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual void OnActionStart(FragmentRequestId /*requestId*/) {};
        virtual void OnActionComplete(FragmentRequestId /*requestId*/) {};
        virtual void OnActionExit(FragmentRequestId /*requestId*/) {};
        virtual void OnActionDelete(FragmentRequestId /*requestId*/) {};
    };

    using MannequinActionNotificationBus = AZ::EBus<MannequinActionNotifications>;

    /*
    * This component allows for a component entity to be animated using the Mannequin system. This component
    * works in conjunction with the Mannequin Scope Context component which is responsible for setting scope
    * context; it is important to note that users don't 'have' to use the scope context component, as long
    * as the scope context is set, the Mannequin component works.
    * This component acts as the programmer and designer facing interface for component entities with
    * respect to mannequin.
    */
    class MannequinComponent
        : public AZ::Component
        , private MannequinRequestsBus::Handler
        , private MannequinActionNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(MannequinComponent, "{83E4AC4C-2184-49D1-AAD0-A0687EEE1405}");

        friend class EditorMannequinComponent;
        MannequinComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MannequinRequestsBus::Handler implementation
        FragmentRequestId QueueFragment(int priority, const char* fragmentName, const char* fragTags, bool isPersistent) override;
        FragmentRequestId QueueFragmentById(int priority, FragmentID fragmentId, const char* fragTags, bool isPersistent) override;
        void QueueAction(_smart_ptr<IAction> action) override;
        void PauseAll() override;
        void ResumeAll(IActionController::EResumeFlags resumeFlag) override;
        void SetTag(const char* tagName) override;
        void SetTagById(TagID tagId) override;
        void ClearTag(const char* tagName) override;
        void ClearTagById(TagID tagId) override;
        void SetGroupTag(const char* groupName, const char* tagName) override;
        void SetGroupTagById(TagGroupID groupId, TagID tagId) override;
        void ClearGroup(const char* groupName) override;
        void ClearGroupById(TagGroupID groupId) override;
        void SetScopeContext(const char* scopeContextName, const AZ::EntityId& entityId, const char* animationDatabaseName) override;
        void SetScopeContextById(TagID scopeContextId, const AZ::EntityId& entityId, const char* animationDatabaseName) override;
        void ClearScopeContext(const char* scopeContextName) override;
        void ClearScopeContextById(const TagID scopeContextId) override;
        IActionController* GetActionController() override;
        IActionPtr GetActionForRequestId(FragmentRequestId requestId) override;
        void StopRequest(FragmentRequestId requestId) override;
        IAction::EStatus GetRequestStatus(FragmentRequestId requestId) override;
        void ForceFinishRequest(FragmentRequestId requestId) override;
        void SetRequestSpeedBias(FragmentRequestId requestId, float speedBias) override;
        float GetRequestSpeedBias(FragmentRequestId requestId) override;
        void SetRequestAnimWeight(FragmentRequestId requestId, float animWeight) override;
        float GetRequestAnimWeight(FragmentRequestId requestId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MannequinActionNotifications::Handler implementation
        void OnActionStart(FragmentRequestId requestId) override;
        void OnActionComplete(FragmentRequestId requestId) override;
        void OnActionExit(FragmentRequestId requestId) override;
        void OnActionDelete(FragmentRequestId requestId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Helper
        /**
        * \brief Loads an indicated controller definition file from disk and creates an IActionController
        * \param Full path to the file on disk
        * \return The newly created SControllerDef or nullptr if it could not be created
        */
        static const SControllerDef* LoadControllerDefinition(const char* filename);
        //////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AnimationService", 0x553f5760));
            provided.push_back(AZ_CRC("MannequinService", 0x424b0eea));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AnimationService", 0x553f5760));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // Helpers
        bool IsFragmentIdValid(FragmentID id) const;
        bool IsTagIdValid(TagID id) const;
        bool IsGroupIdValid(TagGroupID id) const;
        bool IsScopeContextIdValid(TagID id) const;
        TagID GetTagIdFromName(const char* name) const;
        TagGroupID GetTagGroupId(const char* name) const;
        void SetTagState(TagID tagId, bool newState);
        IActionController* CreateAnimationController(const AzFramework::SimpleAssetReference<MannequinControllerDefinitionAsset>& mannequinControllerDef);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        // Reference to the animation controller definition file
        AzFramework::SimpleAssetReference<LmbrCentral::MannequinControllerDefinitionAsset> m_controllerDefinition;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Runtime data
        // Mannequin Action controller
        IActionController* m_actionController;

        // Mannequin animation context
        SAnimationContext* m_animationContext;

        //! The next available request id, these id's are used to uniquely identify Queuing requests that were made to the Mannequin component
        static FragmentRequestId s_nextRequestId;

        //! Action pointers are stored raw, since we queue them immediately. This ensures we
        //! can accurately monitor their usage and clear the entries in an event-driven fashion
        //! when they're freed by Mannequin.
        AZStd::unordered_map<FragmentRequestId, IAction*> m_currentlyActiveRequests;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral

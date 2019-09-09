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

#include <IFlowSystem.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Scripting/FlowGraphSerialization.h>

#include <FlowGraphInformation.h>

#include <HyperGraph/IHyperGraph.h>

#include <Objects/IEntityObjectListener.h>

#include <Objects/EntityObject.h>

namespace LmbrCentral
{
    /*
    * Wrapper to allow registration and unregistration of created game flowgraphs with the editor hypergraph system
    */
    class FlowGraphWrapper
        : public IEntityObjectListener
    {
    public:

        AZ_TYPE_INFO(FlowGraphWrapper, "{C0943E3C-0009-4BF0-9A92-821E471CFB0C}");
        AZ_CLASS_ALLOCATOR(FlowGraphWrapper, AZ::SystemAllocator, 0);

        FlowGraphWrapper();
        FlowGraphWrapper(const AZ::EntityId& id, const AZStd::string& flowGraphName);
        FlowGraphWrapper(FlowGraphWrapper&& rhs);
    private:
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        FlowGraphWrapper(const FlowGraphWrapper&) = delete;
    public:
        virtual ~FlowGraphWrapper();

        //////////////////////////////////////////////////////////////////////////
        // IEntityObjectListener
        void OnNameChanged(const char* pName) override;
        void OnSelectionChanged(const bool bSelected) override {}
        void OnDone() override {}
        //////////////////////////////////////////////////////////////////////////

        //! Sets the appropriate entity id on the editor flowgraph (hypergraph) so that graph entity is known
        void SetEditorFlowGraphEntity(const AZ::EntityId& id);

        static void Reflect(AZ::ReflectContext* context);

        //! Serialization event handlers.
        void OnBeforeSave();
        void OnAfterLoad();

        inline bool operator == (const FlowGraphWrapper& rhs) const
        {
            return m_flowGraph.get() == rhs.m_flowGraph.get();
        }

        void OnGraphNameChanged();

        void OpenFlowGraphEditor();
        bool m_openEditorButton;
        AZ::EntityId m_entityId;

        //! The actual flowgraph
        IFlowGraphPtr m_flowGraph = nullptr;

        AZStd::string m_flowGraphName;

        //! This flag is used to ensure that we do not overwrite the existing flowgraph data during
        //! serialization.
        bool m_isReady = false;

        AZStd::string m_flowGraphXML;

        SerializedFlowGraph m_serializedFlowGraph;
    };

    class EditorFlowGraphComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private FlowGraphEditorRequestsBus::Handler
    {
    public:

        AZ_COMPONENT(EditorFlowGraphComponent,
            "{400972DE-DD1F-4407-8F53-7E514C5767CA}",
            AzToolsFramework::Components::EditorComponentBase);

        ~EditorFlowGraphComponent() override;
        EditorFlowGraphComponent() = default;

        friend class EditorFlowGraphComponentSerializationEvents;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // FlowGraphInfoRequestBus
        void GetFlowGraphs(AZStd::vector<AZStd::pair<AZStd::string, IFlowGraph*> >& flowGraphs) override;
        void OpenFlowGraphView(IFlowGraph* flowGraph) override;
        IFlowGraph* AddFlowGraph(const AZStd::string& flowGraphName) override;
        void RemoveFlowGraph(IFlowGraph* flowGraph, bool shouldPromptUser) override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void OnGraphAdded();
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("FlowGraphService", 0xd7933ebe));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        EditorFlowGraphComponent(const EditorFlowGraphComponent&) = delete;

        //! The FlowGraphs that are managed by this component
        AZStd::list<FlowGraphWrapper> m_flowGraphs;
    };
} // namespace LmbrCentral

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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeCategorizer.h>

#include <ScriptEvents/ScriptEventsAsset.h>

#include <Editor/View/Widgets/NodePalette/NodePaletteModelBus.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    // Move these down into GraphCanvas for more general re-use
    struct NodePaletteModelInformation
    {
        AZ_RTTI(NodeModelInformation, "{CC031806-7610-4C29-909D-9527F265E014}");
        AZ_CLASS_ALLOCATOR(NodePaletteModelInformation, AZ::SystemAllocator, 0);

        virtual ~NodePaletteModelInformation() = default;

        void PopulateTreeItem(GraphCanvas::NodePaletteTreeItem& treeItem) const;

        ScriptCanvas::NodeTypeIdentifier m_nodeIdentifier;

        AZStd::string                    m_displayName;
        AZStd::string                    m_toolTip;

        AZStd::string                    m_categoryPath;
        AZStd::string                    m_styleOverride;
        AZStd::string                    m_titlePaletteOverride;
    };

    struct CategoryInformation
    {
        AZStd::string m_styleOverride;
        AZStd::string m_paletteOverride = GraphCanvas::NodePaletteTreeItem::DefaultNodeTitlePalette;

        AZStd::string m_tooltip;
    };

    class NodePaletteModel
        : public GraphCanvas::CategorizerInterface
    {
    public:
        typedef AZStd::unordered_map< ScriptCanvas::NodeTypeIdentifier, NodePaletteModelInformation* > NodePaletteRegistry;

        AZ_CLASS_ALLOCATOR(NodePaletteModel, AZ::SystemAllocator, 0);

        NodePaletteModel();
        ~NodePaletteModel();

        NodePaletteId GetNotificationId() const;

        void AssignAssetModel(AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);

        void RepopulateModel();

        void RegisterCustomNode(AZStd::string_view categoryPath, const AZ::Uuid& uuid, AZStd::string_view name, const AZ::SerializeContext::ClassData* classData);
        void RegisterClassNode(const AZStd::string& categoryPath, const AZStd::string& methodClass, const AZStd::string& methodName, AZ::BehaviorMethod* behaviorMethod, AZ::BehaviorContext* behaviorContext);

        void RegisterEBusHandlerNodeModelInformation(AZStd::string_view categoryPath, AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusBusId& busId, const AZ::BehaviorEBusHandler::BusForwarderEvent& forwardEvent);
        void RegisterEBusSenderNodeModelInformation(AZStd::string_view categoryPath, AZStd::string_view busName, AZStd::string_view eventName, const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId, const AZ::BehaviorEBusEventSender& eventSender);

        // Asset Based Registrations
        AZStd::vector<ScriptCanvas::NodeTypeIdentifier> RegisterScriptEvent(ScriptEvents::ScriptEventsAsset* scriptEventAsset);

        void RegisterCategoryInformation(const AZStd::string& category, const CategoryInformation& categoryInformation);
        const CategoryInformation* FindCategoryInformation(const AZStd::string& categoryStyle) const;
        const CategoryInformation* FindBestCategoryInformation(AZStd::string_view categoryView) const;

        const NodePaletteModelInformation* FindNodePaletteInformation(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier) const;

        const NodePaletteRegistry& GetNodeRegistry() const;

        // GraphCanvas::CategorizerInterface
        GraphCanvas::GraphCanvasTreeItem* CreateCategoryNode(AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvas::GraphCanvasTreeItem* treeItem) const override;
        ////

        // Asset Node Support
        void OnRowsInserted(const QModelIndex& parentIndex, int first, int last);
        void OnRowsAboutToBeRemoved(const QModelIndex& parentIndex, int first, int last);

        void TraverseTree(QModelIndex index = QModelIndex());
        ////

    private:

        AZStd::vector<ScriptCanvas::NodeTypeIdentifier> ProcessAsset(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void RemoveAsset(const AZ::Data::AssetId& assetId);

        void ClearRegistry();

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_assetModel =  nullptr;
        AZStd::vector< QMetaObject::Connection > m_lambdaConnections;

        AZStd::unordered_map< AZStd::string, CategoryInformation > m_categoryInformation;
        NodePaletteRegistry m_registeredNodes;

        AZStd::unordered_multimap<AZ::Data::AssetId, ScriptCanvas::NodeTypeIdentifier> m_assetMapping;

        NodePaletteId m_paletteId;
    };

    // Concrete Sub Classes with whatever extra data is required [ScriptCanvas Only]
    struct CustomNodeModelInformation
        : public NodePaletteModelInformation
    {
        AZ_RTTI(CustomNodeModelInformation, "{481FB8AE-8683-4E50-95C1-B4B1C1B6806C}", NodePaletteModelInformation);
        AZ_CLASS_ALLOCATOR(CustomNodeModelInformation, AZ::SystemAllocator, 0);

        AZ::Uuid m_typeId;
    };

    struct MethodNodeModelInformation
        : public NodePaletteModelInformation
    {
        AZ_RTTI(CustomNodeModelInformation, "{9B6337F9-B8D0-4B63-9EE7-91079FE386B9}", NodePaletteModelInformation);
        AZ_CLASS_ALLOCATOR(CustomNodeModelInformation, AZ::SystemAllocator, 0);

        AZStd::string m_classMethod;
        AZStd::string m_metehodName;
    };

    struct EBusHandlerNodeModelInformation
        : public NodePaletteModelInformation
    {
        AZ_RTTI(EBusNodeModelInformation, "{D1438D14-0CE9-4202-A1C5-9F5F13DFC0C4}", NodePaletteModelInformation);
        AZ_CLASS_ALLOCATOR(EBusHandlerNodeModelInformation, AZ::SystemAllocator, 0);

        AZStd::string m_busName;
        AZStd::string m_eventName;

        ScriptCanvas::EBusBusId m_busId;
        ScriptCanvas::EBusEventId m_eventId;
    };

    struct EBusSenderNodeModelInformation
        : public NodePaletteModelInformation
    {
        AZ_RTTI(EBusSenderNodeModelInformation, "{EE0F0385-3596-4D4E-9DC7-BE147EBB3C15}", NodePaletteModelInformation);
        AZ_CLASS_ALLOCATOR(EBusHandlerNodeModelInformation, AZ::SystemAllocator, 0);

        AZStd::string m_busName;
        AZStd::string m_eventName;

        ScriptCanvas::EBusBusId m_busId;
        ScriptCanvas::EBusEventId m_eventId;
    };

    struct ScriptEventHandlerNodeModelInformation
        : public EBusHandlerNodeModelInformation
    {
        AZ_RTTI(ScriptEventHandlerNodeModelInformation, "{BCA92869-63F4-4A1F-B751-F3F28443BBFC}", EBusHandlerNodeModelInformation);
        AZ_CLASS_ALLOCATOR(ScriptEventHandlerNodeModelInformation, AZ::SystemAllocator, 0);
    };

    struct ScriptEventSenderNodeModelInformation
        : public EBusSenderNodeModelInformation
    {
        AZ_RTTI(ScriptEventSenderNodeModelInformation, "{99046345-080C-42A6-BE76-D09583055EED}", EBusSenderNodeModelInformation);
        AZ_CLASS_ALLOCATOR(ScriptEventSenderNodeModelInformation, AZ::SystemAllocator, 0);
    };
}
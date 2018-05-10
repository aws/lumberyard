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
#include "precompiled.h"

#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QScrollBar>
#include <qboxlayout.h>
#include <qpainter.h>
#include <qevent.h>
#include <QCoreApplication>
#include <QCompleter>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <Editor/Settings.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <Editor/Components/IconComponent.h>
#include <Editor/Translation/TranslationHelper.h>
#include <ScriptCanvas/Data/DataRegistry.h>

#include <Core/Attributes.h>
#include <Libraries/Entity/EntityRef.h>
#include <Libraries/Libraries.h>

#include <Libraries/Core/GetVariable.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/SetVariable.h>


namespace
{
    bool ShouldExcludeFromNodeList(const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>* excludeAttributeData, const AZ::Uuid& typeId, bool showExcludedPreviewNodes)
    {
        if (excludeAttributeData)
        {
            AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List;
            if (!showExcludedPreviewNodes)
            {
                exclusionFlags |= AZ::Script::Attributes::ExcludeFlags::Preview;
            }

            if (typeId == AzToolsFramework::Components::EditorComponentBase::TYPEINFO_Uuid())
            {
                return true;
            }

            return (static_cast<AZ::u64>(excludeAttributeData->Get(nullptr)) & exclusionFlags);
        }
        return false;
    }

    // Used for changing the node's style to identify as a preview node.
    bool IsPreviewNode(const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>* excludeAttributeData)
    {
        if (excludeAttributeData)
        {
            return static_cast<AZ::u64>(excludeAttributeData->Get(nullptr)) & AZ::Script::Attributes::ExcludeFlags::Preview;
        }

        return false;
    }

    bool HasExcludeFromNodeListAttribute(AZ::SerializeContext* serializeContext, const AZ::Uuid& typeId, bool showExcludedPreviewNodes)
    {
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);
        if (classData && classData->m_editData)
        {            
            if (auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto excludeAttribute = editorElementData->FindAttribute(AZ::Script::Attributes::ExcludeFrom))
                {
                    auto excludeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(excludeAttribute);
                    return excludeAttributeData && ShouldExcludeFromNodeList(excludeAttributeData, typeId, showExcludedPreviewNodes);
                }
            }
        }
        
        return false;
    }

    bool HasAttribute(AZ::BehaviorClass* behaviorClass, AZ::Crc32 attribute)
    {
        auto foundItem = AZStd::find_if(behaviorClass->m_methods.begin(), behaviorClass->m_methods.end(),
            [attribute](const AZStd::pair<AZStd::string, AZ::BehaviorMethod*>& method)
        {
            if (AZ::FindAttribute(attribute, method.second->m_attributes))
            {
                return true;
            }

            return false;
        });

        return foundItem != behaviorClass->m_methods.end();
    }

    // Checks for and returns the Category attribute from an AZ::AttributeArray
    const char* GetCategoryPath(const AZ::AttributeArray& attributes)
    {
        AZ::Attribute* categoryAttribute = AZ::FindAttribute(AZ::Script::Attributes::Category, attributes);

        if (!categoryAttribute)
        {
            // The element doesn't have a Category attribute
            return nullptr;
        }

        auto categoryData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute);
        const char* categoryPath = categoryData->Get(nullptr);

        return categoryPath;
    }

    typedef AZStd::pair<GraphCanvas::NodePaletteTreeItem*, AZStd::string> CategoryKey;
    typedef AZStd::unordered_map<CategoryKey, GraphCanvas::NodePaletteTreeItem*> CategoryRootsMap;

    // Given a category path (e.g. "My/Category") and a parent node, creates the necessary intermediate
    // nodes under the given parent and returns the leaf tree item under the given category path.
    template<class NodeType = GraphCanvas::NodePaletteTreeItem>
    GraphCanvas::NodePaletteTreeItem* GetCategoryNode(
        const char* categoryPath,
        GraphCanvas::NodePaletteTreeItem* parentRoot,
        CategoryRootsMap& categoryRoots
    )
    {
        if (!categoryPath)
        {
            return parentRoot;
        }

        AZStd::vector<AZStd::string> categories;
        AzFramework::StringFunc::Tokenize(categoryPath, categories, "/", true, true);

        GraphCanvas::NodePaletteTreeItem* intermediateRoot = parentRoot;
        for (auto it = categories.begin(); it < categories.end(); it++)
        {
            AZStd::string intermediatePath;
            AzFramework::StringFunc::Join(intermediatePath, categories.begin(), it+1, "/");

            CategoryKey key(parentRoot, intermediatePath);

            if (categoryRoots.find(key) == categoryRoots.end())
            {
                categoryRoots[key] = intermediateRoot->CreateChildNode<NodeType>(it->c_str(), ScriptCanvasEditor::AssetEditorId);
            }

            intermediateRoot = categoryRoots[key];
        }

        return intermediateRoot;
    }

    struct CategoryInformation
    {
        AZ::Uuid m_uuid;
        AZStd::string m_styleOverride;
        AZStd::string m_paletteOverride = "DefaultNodeTitlePalette";
        AZStd::string m_path;
    };

    // Keeping this here for the time being to more easily faciliate some known merges.
    // Once merges are complete this can be just moved into the other method.
    GraphCanvas::NodePaletteTreeItem* ExternalCreatePaletteRoot()
    {
        auto dataRegistry = ScriptCanvas::GetDataRegistry();

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        bool showExcludedPreviewNodes = false;
        AZStd::intrusive_ptr<ScriptCanvasEditor::EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<ScriptCanvasEditor::EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            showExcludedPreviewNodes = settings->m_showExcludedNodes;
        }

        AZ_Error("NodePalette", serializeContext, "Could not find SerializeContext. Aborting Palette Creation.");
        AZ_Error("NodePalette", behaviorContext, "Could not find BehaviorContext. Aborting Palette Creation.");

        if (serializeContext == nullptr || behaviorContext == nullptr)
        {
            return nullptr;
        }

        // Have a set of pairs to simulate as an unsorted map.
        AZStd::vector< AZStd::pair<QString, CategoryInformation> > categoryData;
        {
            // Get all the types.
            serializeContext->EnumerateDerived<ScriptCanvas::Library::LibraryDefinition>(
                [&categoryData]
            (const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& classUuid) -> bool
            {
                CategoryInformation categoryInfo;
                categoryInfo.m_uuid = classData->m_typeId;

                if (classData->m_editData)
                {
                    auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    if (editorElementData)
                    {
                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Category))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                categoryInfo.m_path = categoryAttributeData->Get(nullptr);
                            }
                        }

                        if (auto categoryStyleAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                            {
                                categoryInfo.m_styleOverride = categoryAttributeData->Get(nullptr);
                            }
                        }

                        if (auto titlePaletteAttribute = editorElementData->FindAttribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(titlePaletteAttribute))
                            {
                                categoryInfo.m_paletteOverride = categoryAttributeData->Get(nullptr);
                            }
                        }
                    }
                }

                categoryData.emplace_back(classData->m_editData ? classData->m_editData->m_name : classData->m_name, categoryInfo);
                return true;
            });
        }

        QString defaultIconPath("Editor/Icons/ScriptCanvas/Libraries/All.png");

        // The root.
        GraphCanvas::NodePaletteTreeItem* root = aznew GraphCanvas::NodePaletteTreeItem("root", ScriptCanvasEditor::AssetEditorId);

        // Map to keep track of the category nodes under which to put any categorized ebuses/objects
        CategoryRootsMap categoryRoots;

        {
            GraphCanvas::NodePaletteTreeItem* utilitiesRoot = GetCategoryNode("Utilities", root, categoryRoots);

            utilitiesRoot->CreateChildNode<ScriptCanvasEditor::CommentNodePaletteTreeItem>("Comment", QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(AZ::Uuid()).c_str()));
            utilitiesRoot->CreateChildNode<ScriptCanvasEditor::BlockCommentNodePaletteTreeItem>("Block Comment", QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(AZ::Uuid()).c_str()));

            root->CreateChildNode<ScriptCanvasEditor::LocalVariablesListNodePaletteTreeItem>("Variables");
        }

        // When the NodePalette is used elsewhere than a context menu,
        // sourceSlotId is invalid, and sourceSlotType remains "None".
        // This has the side effect of disabling the filtering of nodes.

        // Add all the types in alphabetical order.
        for (auto& type : categoryData)
        {
            // Parent
            // Passing in the root will hook it up to the root.
            AZStd::string categoryPath = type.second.m_path.empty() ? type.first.toUtf8().data() : type.second.m_path.c_str();
            GraphCanvas::NodePaletteTreeItem* parentItem = GetCategoryNode(categoryPath.c_str(), root, categoryRoots);
            parentItem->SetStyleOverride(type.second.m_styleOverride);
            parentItem->SetTitlePalette(type.second.m_paletteOverride);

            // Children
            for (auto& node : ScriptCanvas::Library::LibraryDefinition::GetNodes(type.second.m_uuid))
            {
                GraphCanvas::NodePaletteTreeItem* nodeParent = nullptr;

                if (HasExcludeFromNodeListAttribute(serializeContext, node.first, showExcludedPreviewNodes))
                {
                    continue;
                }

                // Pass in the associated class data so we can do more intensive lookups?
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(node.first);

                AZStd::string name = node.second;
                QString toolTip;

                bool isToolTipSet(false);

                bool isMissingEntry(false);
                bool isMissingTooltip(false);

                if (classData && classData->m_editData && classData->m_editData->m_name)
                {
                    auto nodeContextName = ScriptCanvasEditor::Nodes::GetContextName(*classData);
                    auto contextName = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, nodeContextName);

                    GraphCanvas::TranslationKeyedString nodeKeyedString({}, contextName);
                    nodeKeyedString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, nodeContextName, classData->m_editData->m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Name);
                    name = nodeKeyedString.GetDisplayString();

                    isMissingEntry = name.empty();

                    GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
                    tooltipKeyedString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, nodeContextName, classData->m_editData->m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Tooltip);

                    auto toolTipTranslated = tooltipKeyedString.GetDisplayString();
                    isMissingTooltip = toolTipTranslated.empty();
                    if (!isMissingTooltip)
                    {
                        toolTip = QString(toolTipTranslated.c_str());
                    }

                    if (name.empty())
                    {
                        name = classData->m_editData->m_name;
                    }

                    GraphCanvas::TranslationKeyedString categoryKeyedString(ScriptCanvasEditor::Nodes::GetCategoryName(*classData), nodeKeyedString.m_context);
                    categoryKeyedString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, nodeContextName, classData->m_editData->m_name, ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);
                    auto categoryName = categoryKeyedString.GetDisplayString();

                    if (categoryName.empty())
                    {
                        categoryName = contextName;
                    }

                    if (!categoryName.empty())
                    {
                        // a better category name may have been found
                        nodeParent = GetCategoryNode(categoryName.c_str(), root, categoryRoots);
                    }
                    else
                    {
                        nodeParent = parentItem;
                    }

                    // Need to detect primitive types.
                    if (classData->m_azRtti && classData->m_azRtti->IsTypeOf<ScriptCanvas::PureData>())
                    {
                        continue;
                    }
                    else if (classData->m_azRtti && classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::GetVariableNode>())
                    {
                        continue;
                    }
                    else if (classData->m_azRtti && classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::SetVariableNode>())
                    {
                        continue;
                    }
                    else
                    {
                        GraphCanvas::NodePaletteTreeItem* customItem{};
                        auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (editorDataElement)
                        {
                            if (auto treeItemOverrideFunc = azrtti_cast<AZ::AttributeFunction<GraphCanvas::NodePaletteTreeItem*(GraphCanvas::NodePaletteTreeItem*)>*>(editorDataElement->FindAttribute(ScriptCanvas::Attributes::NodePalette::TreeItemOverride)))
                            {
                                customItem = treeItemOverrideFunc->m_function(nodeParent);
                            }
                        }

                        if (!customItem)
                        {
                            customItem = nodeParent->CreateChildNode<ScriptCanvasEditor::CustomNodePaletteTreeItem>(node.first, QString(name.c_str()), QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(node.first).c_str()));
                        }

                        if (editorDataElement)
                        {
                            if (auto categoryStyleAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                            {
                                if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                                {
                                    if (categoryAttributeData->Get(nullptr))
                                    {
                                        customItem->SetStyleOverride(categoryAttributeData->Get(nullptr));
                                    }
                                }
                            }

                            if (auto titlePaletteAttribute = editorDataElement->FindAttribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride))
                            {
                                if (auto titlePaletteAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(titlePaletteAttribute))
                                {
                                    if (titlePaletteAttributeData->Get(nullptr))
                                    {
                                        customItem->SetTitlePalette(titlePaletteAttributeData->Get(nullptr));
                                    }
                                }
                            }

                            if (toolTip.isEmpty() && classData->m_editData->m_description)
                            {
                                toolTip = classData->m_editData->m_description;
                            }
                        }

                        customItem->SetToolTip(toolTip);
                    }
                }
            }
        }

        // Merged all of these into a single pass(to avoid going over the giant lists multiple times), 
        // so now this looks absolutely horrifying in what it's doing. 
        // Everything is still mostly independent, and independently scoped to try to make the division of labor clear.
        GraphCanvas::NodePaletteTreeItem* otherRoot = GetCategoryNode("Other", root, categoryRoots);

        {
            // We will skip buses that are ONLY registered on classes that derive from EditorComponentBase,
            // because they don't have a runtime implementation. Buses such as the TransformComponent which
            // is implemented by both an EditorComponentBase derived class and a Component derived class
            // will still appear
            AZStd::unordered_set<AZ::Crc32> skipBuses;
            AZStd::unordered_set<AZ::Crc32> potentialSkipBuses;
            AZStd::unordered_set<AZ::Crc32> nonSkipBuses;

            for (const auto& classIter : behaviorContext->m_classes)
            {
                const AZ::BehaviorClass* behaviorClass = classIter.second;

                // Check for "ExcludeFrom" attribute for ScriptCanvas
                auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));

                // We don't want to show any components, since there isn't anything we can do with them
                // from ScriptCanvas since we use buses to communicate to everything.
                if (ShouldExcludeFromNodeList(excludeClassAttributeData, behaviorClass->m_azRtti ? behaviorClass->m_azRtti->GetTypeId() : behaviorClass->m_typeId, showExcludedPreviewNodes))
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        skipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }

                    continue;
                }
                
                // Objects and Object methods
                {
                    bool canCreate = serializeContext->FindClassData(behaviorClass->m_typeId) != nullptr;
                
                    // createable variables must have full memory support
                    canCreate = canCreate && 
                        ( behaviorClass->m_allocate
                        && behaviorClass->m_cloner
                        && behaviorClass->m_mover
                        && behaviorClass->m_destructor
                        && behaviorClass->m_deallocate);

                    if (canCreate)
                    {
                        // Do not allow variable creation for data that derives from AZ::Component
                        for (auto base : behaviorClass->m_baseClasses)
                        {
                            if (AZ::Component::TYPEINFO_Uuid() == base)
                            {
                                canCreate = false;
                                break;
                            }
                        }
                    }
                    
                    AZStd::string categoryPath;
                    
                    AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, behaviorClass->m_name);
                    AZStd::string translationKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, behaviorClass->m_name, ScriptCanvasEditor::TranslationKeyId::Category);
                    AZStd::string translatedCategory = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

                    if (translatedCategory != translationKey)
                    {
                        categoryPath = translatedCategory;
                    }
                    else
                    {
                        const char* behaviourContextCategory = GetCategoryPath(behaviorClass->m_attributes);
                        if (behaviourContextCategory != nullptr)
                        {
                            categoryPath = AZStd::string(behaviourContextCategory);
                        }
                    }

                    if (canCreate)
                    {
                        ScriptCanvas::Data::Type type = dataRegistry->m_typeIdTraitMap[ScriptCanvas::Data::eType::BehaviorContextObject].m_dataTraits.GetSCType(behaviorClass->m_typeId);

                        if (type.IsValid())
                        {
                            ScriptCanvasEditor::VariablePaletteRequestBus::Broadcast(&ScriptCanvasEditor::VariablePaletteRequests::RegisterVariableType, type);
                        }
                    }
                    
                    GraphCanvas::NodePaletteTreeItem* categorizedRoot = nullptr;
                    if (categoryPath.empty())
                    {
                        categoryPath = "Other";
                    }

                    categoryPath.append("/");

                    AZStd::string displayName = ScriptCanvasEditor::TranslationHelper::GetClassKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, classIter.first, ScriptCanvasEditor::TranslationKeyId::Name);

                    if (displayName.empty())
                    {
                        categoryPath.append(classIter.first.c_str());
                    }
                    else
                    {
                        categoryPath.append(displayName.c_str());
                    }

                    GraphCanvas::NodePaletteTreeItem* objectItem = GetCategoryNode(categoryPath.c_str(), root, categoryRoots);

                    for (auto method : behaviorClass->m_methods)
                    {
                        // Check for "ExcludeFrom" attribute for ScriptCanvas
                        auto excludeMethodAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, method.second->m_attributes));
                        if (ShouldExcludeFromNodeList(excludeMethodAttributeData, behaviorClass->m_azRtti ? behaviorClass->m_azRtti->GetTypeId() : behaviorClass->m_typeId, showExcludedPreviewNodes))
                        {
                            continue; // skip this method
                        }

                        const auto isExposableOutcome = ScriptCanvas::IsExposable(*method.second);
                        if (!isExposableOutcome.IsSuccess())
                        {
                            AZ_Warning("ScriptCanvas", false, "Unable to expose method: %s to ScriptCanvas because: %s", method.second->m_name.data(), isExposableOutcome.GetError().data());
                            continue;
                        }
                        
                        GraphCanvas::TranslationKeyedString methodCategoryString;
                        methodCategoryString.m_context = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, classIter.first.c_str());
                        methodCategoryString.m_key = ScriptCanvasEditor::TranslationHelper::GetKey(ScriptCanvasEditor::TranslationContextGroup::ClassMethod, classIter.first.c_str(), method.first.c_str(), ScriptCanvasEditor::TranslationItemType::Node, ScriptCanvasEditor::TranslationKeyId::Category);
                        
                        AZStd::string methodCategoryPath = methodCategoryString.GetDisplayString();
                        
                        if (methodCategoryPath.empty())
                        {
                            objectItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(QString(classIter.first.c_str()), QString(method.first.c_str()));
                        }
                        else
                        {
                            GraphCanvas::NodePaletteTreeItem* methodParentItem = GetCategoryNode(methodCategoryPath.c_str(), root, categoryRoots);
                            methodParentItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(QString(classIter.first.c_str()), QString(method.first.c_str()));
                        }
                    }
                }

                auto baseClass = AZStd::find(behaviorClass->m_baseClasses.begin(),
                    behaviorClass->m_baseClasses.end(),
                    AzToolsFramework::Components::EditorComponentBase::TYPEINFO_Uuid());

                if (baseClass != behaviorClass->m_baseClasses.end())
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        potentialSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }
                }
                // If the Ebus does not inherit from EditorComponentBase then do not skip it
                else
                {
                    for (const auto& requestBus : behaviorClass->m_requestBuses)
                    {
                        nonSkipBuses.insert(AZ::Crc32(requestBus.c_str()));
                    }
                }
            }

            // Add buses which are not on the non-skip list to the skipBuses set
            for (auto potentialSkipBus : potentialSkipBuses)
            {
                if (nonSkipBuses.find(potentialSkipBus) == nonSkipBuses.end())
                {
                    skipBuses.insert(potentialSkipBus);
                }
            }

            for (const auto& ebusIter : behaviorContext->m_ebuses)
            {
                AZ::BehaviorEBus* ebus = ebusIter.second;

                if (ebus == nullptr)
                {
                    continue;
                }

                auto skipBusIterator = skipBuses.find(AZ::Crc32(ebusIter.first.c_str()));
                if (skipBusIterator != skipBuses.end())
                {
                    continue;
                }

                // Skip buses mapped by their deprecated name (usually duplicates)
                if (ebusIter.first == ebus->m_deprecatedName)
                {
                    continue;
                }

                // EBus Handler
                {
                    if (ebus->m_createHandler)
                    {
                        AZ::BehaviorEBusHandler* handler(nullptr);
                        if (ebus->m_createHandler->InvokeResult(handler) && handler)
                        {
                            const AZ::BehaviorEBusHandler::EventArray& events(handler->GetEvents());
                            if (!events.empty())
                            {
                                auto excludeEventAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebus->m_attributes));
                                if (ShouldExcludeFromNodeList(excludeEventAttributeData, handler->RTTI_GetType(), showExcludedPreviewNodes))
                                {
                                    continue;
                                }

                                AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name);
                                AZStd::string categoryPath;

                                {
                                    AZStd::string translationKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Category);
                                    AZStd::string translatedCategory = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

                                    if (translatedCategory != translationKey)
                                    {
                                        categoryPath = translatedCategory;
                                    }
                                    else
                                    {
                                        const char* behaviourContextCategory = GetCategoryPath(ebus->m_attributes);
                                        if (behaviourContextCategory != nullptr)
                                        {
                                            categoryPath = AZStd::string(behaviourContextCategory);
                                        }
                                    }
                                }

                                bool useGenericEbusRoot = true;

                                // Treat the EBusHandler name as a Category key in order to allow multiple busses to be merged into a single Category.
                                {
                                    AZStd::string translationKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusHandler, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Name);
                                    AZStd::string translatedName = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

                                    if (!categoryPath.empty())
                                    {
                                        useGenericEbusRoot = false;
                                        categoryPath.append("/");
                                    }

                                    if (translatedName != translationKey)
                                    {
                                        categoryPath.append(translatedName.c_str());
                                    }
                                    else
                                    {
                                        categoryPath.append(ebus->m_name.c_str());
                                    }
                                }

                                GraphCanvas::NodePaletteTreeItem* busItem = GetCategoryNode(categoryPath.c_str(), useGenericEbusRoot ? otherRoot : root, categoryRoots);

                                for (const auto& event : events)
                                {
                                    busItem->CreateChildNode<ScriptCanvasEditor::EBusHandleEventPaletteTreeItem>(QString(ebusIter.first.c_str()), QString(event.m_name));
                                }
                            }

                            if (ebus->m_destroyHandler)
                            {
                                ebus->m_destroyHandler->Invoke(handler);
                            }
                        }
                    }
                }

                // Ebus Sender
                {
                    if (!ebus->m_events.empty())
                    {
                        auto excludeEBusAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, ebus->m_attributes));
                        if (ShouldExcludeFromNodeList(excludeEBusAttributeData, AZ::Uuid::CreateNull(), showExcludedPreviewNodes))
                        {
                            continue;
                        }

                        AZStd::string categoryPath;

                        AZStd::string translationContext = ScriptCanvasEditor::TranslationHelper::GetContextName(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebus->m_name);
                        AZStd::string translationKey = ScriptCanvasEditor::TranslationHelper::GetClassKey(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebus->m_name, ScriptCanvasEditor::TranslationKeyId::Category);
                        AZStd::string translatedCategory = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

                        if (translatedCategory != translationKey)
                        {
                            categoryPath = translatedCategory;
                        }
                        else
                        {
                            const char* behaviourContextCategory = GetCategoryPath(ebus->m_attributes);
                            if (behaviourContextCategory != nullptr)
                            {
                                categoryPath = AZStd::string(behaviourContextCategory);
                            }
                        }

                        bool useGenericSenderRoot = true;

                        // Parent
                        AZStd::string displayName = ScriptCanvasEditor::TranslationHelper::GetClassKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebusIter.first, ScriptCanvasEditor::TranslationKeyId::Name);

                        // Treat the EBus name as a Category key in order to allow multiple busses to be merged into a single Category.
                        if (!categoryPath.empty())
                        {
                            useGenericSenderRoot = false;
                            categoryPath.append("/");
                        }

                        if (displayName.empty())
                        {
                            categoryPath.append(ebusIter.first.c_str());
                        }
                        else
                        {
                            categoryPath.append(displayName.c_str());
                        }

                        GraphCanvas::NodePaletteTreeItem* busItem = GetCategoryNode(categoryPath.c_str(), useGenericSenderRoot ? otherRoot : root, categoryRoots);

                        AZStd::string displayTooltip = ScriptCanvasEditor::TranslationHelper::GetClassKeyTranslation(ScriptCanvasEditor::TranslationContextGroup::EbusSender, ebusIter.first, ScriptCanvasEditor::TranslationKeyId::Tooltip);

                        if (!displayTooltip.empty())
                        {
                            busItem->SetToolTip(displayTooltip.c_str());
                        }

                        for (auto event : ebus->m_events)
                        {
                            auto excludeEventAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, event.second.m_attributes));
                            if (ShouldExcludeFromNodeList(excludeEventAttributeData, AZ::Uuid::CreateNull(), showExcludedPreviewNodes))
                            {
                                continue; // skip this event
                            }

                            busItem->CreateChildNode<ScriptCanvasEditor::EBusSendEventPaletteTreeItem>(QString(ebusIter.first.c_str()), QString(event.first.c_str()));
                        }
                    }
                }
            }

            AZStd::unordered_set< GraphCanvas::GraphCanvasTreeItem* > recursiveSet;

            for (auto& mapPair : categoryRoots)
            {
                if (mapPair.second->GetChildCount() == 0)
                {
                    GraphCanvas::GraphCanvasTreeItem* parentItem = mapPair.second->GetParent();
                    mapPair.second->DetachItem();

                    if (parentItem->GetChildCount() == 0)
                    {
                        recursiveSet.insert(parentItem);
                    }
                    
                    recursiveSet.erase(mapPair.second);
                    delete mapPair.second;
                }
            }

            while (!recursiveSet.empty())
            {
                GraphCanvas::GraphCanvasTreeItem* treeItem = (*recursiveSet.begin());
                recursiveSet.erase(recursiveSet.begin());

                GraphCanvas::GraphCanvasTreeItem* parentItem = treeItem->GetParent();

                treeItem->DetachItem();

                if (parentItem->GetChildCount() == 0)
                {
                    recursiveSet.insert(parentItem);
                }

                delete treeItem;
            }
        }

        return root;
    }

} // anonymous namespace.

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        //////////////////////////
        // NodePaletteDockWidget
        //////////////////////////

        NodePaletteDockWidget::NodePaletteDockWidget(const QString& windowLabel, QWidget* parent, bool inContextMenu)
            : GraphCanvas::NodePaletteDockWidget(ExternalCreatePaletteRoot(), ScriptCanvasEditor::AssetEditorId, windowLabel, parent, inContextMenu)
        {
        }

        GraphCanvas::GraphCanvasTreeItem* NodePaletteDockWidget::CreatePaletteRoot() const
        {
            return ExternalCreatePaletteRoot();
        }
    }
}

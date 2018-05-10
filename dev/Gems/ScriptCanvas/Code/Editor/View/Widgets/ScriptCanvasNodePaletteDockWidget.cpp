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
#include "NodePalette.h"

#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QBoxLayout>
#include <QPainter>
#include <QEvent>
#include <QCoreApplication>
#include <QCompleter>
#include <QMouseEvent>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/map.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <Editor/Model/NodePalette/NodePaletteSortFilterProxyModel.h>

#include <Editor/Nodes/NodeUtils.h>

#include <Editor/View/Widgets/ui_NodePalette.h>
#include <Editor/View/Widgets/NodeTreeView.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

#include <Editor/Settings.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <Editor/Components/IconComponent.h>
#include <Editor/Translation/TranslationHelper.h>

#include <Core/Attributes.h>
#include <Libraries/Entity/EntityRef.h>
#include <Libraries/Libraries.h>

// Primitive data types we are hard coding for now
#include <Libraries/Core/String.h>
#include <Libraries/Logic/Boolean.h>
#include <Libraries/Math/AABBNode.h>
#include <Libraries/Math/CRCNode.h>
#include <Libraries/Math/ColorNode.h>
#include <Libraries/Math/Matrix3x3Node.h>
#include <Libraries/Math/Matrix4x4Node.h>
#include <Libraries/Math/OBBNode.h>
#include <Libraries/Math/PlaneNode.h>
#include <Libraries/Math/Rotation.h>
#include <Libraries/Math/Transform.h>
#include <Libraries/Math/Number.h>
#include <Libraries/Math/Vector.h>

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

    typedef AZStd::pair<ScriptCanvasEditor::NodePaletteTreeItem*, AZStd::string> CategoryKey;
    typedef AZStd::unordered_map<CategoryKey, ScriptCanvasEditor::NodePaletteTreeItem*> CategoryRootsMap;

    // Given a category path (e.g. "My/Category") and a parent node, creates the necessary intermediate
    // nodes under the given parent and returns the leaf tree item under the given category path.
    template<class NodeType = ScriptCanvasEditor::NodePaletteTreeItem>
    ScriptCanvasEditor::NodePaletteTreeItem* GetCategoryNode(
        const char* categoryPath,
        ScriptCanvasEditor::NodePaletteTreeItem* parentRoot,
        CategoryRootsMap& categoryRoots
    )
    {
        if (!categoryPath)
        {
            return parentRoot;
        }

        AZStd::vector<AZStd::string> categories;
        AzFramework::StringFunc::Tokenize(categoryPath, categories, "/", true, true);

        ScriptCanvasEditor::NodePaletteTreeItem* intermediateRoot = parentRoot;
        for (auto it = categories.begin(); it < categories.end(); it++)
        {
            AZStd::string intermediatePath;
            AzFramework::StringFunc::Join(intermediatePath, categories.begin(), it+1, "/");

            CategoryKey key(parentRoot, intermediatePath);

            if (categoryRoots.find(key) == categoryRoots.end())
            {
                categoryRoots[key] = intermediateRoot->CreateChildNode<NodeType>(it->c_str());
            }

            intermediateRoot = categoryRoots[key];
        }

        return intermediateRoot;
    }

    struct CategoryInformation
    {
        AZ::Uuid m_uuid;
        AZStd::string m_styleOverride;
        AZStd::string m_path;
    };

    ScriptCanvasEditor::NodePaletteTreeItem* CreatePaletteRoot()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

        bool showExcludedPreviewNodes = false;
        AZStd::intrusive_ptr<ScriptCanvasEditor::EditorSettings::PreviewSettings> settings = AZ::UserSettings::CreateFind<ScriptCanvasEditor::EditorSettings::PreviewSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
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
                        else
                        if (auto categoryStyleAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                            {
                                categoryInfo.m_styleOverride = categoryAttributeData->Get(nullptr);
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
        ScriptCanvasEditor::NodePaletteTreeItem* root = aznew ScriptCanvasEditor::NodePaletteTreeItem("root");

        // Map to keep track of the category nodes under which to put any categorized ebuses/objects
        CategoryRootsMap categoryRoots;

        ScriptCanvasEditor::NodePaletteTreeItem* variableTypeRoot = nullptr;

        {
            ScriptCanvasEditor::NodePaletteTreeItem* utilitiesRoot = GetCategoryNode("Utilities", root, categoryRoots);

            utilitiesRoot->CreateChildNode<ScriptCanvasEditor::CommentNodePaletteTreeItem>("Comment", QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(AZ::Uuid()).c_str()));
            utilitiesRoot->CreateChildNode<ScriptCanvasEditor::BlockCommentNodePaletteTreeItem>("Block Comment", QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(AZ::Uuid()).c_str()));

            ScriptCanvasEditor::NodePaletteTreeItem* variableRoot = GetCategoryNode<ScriptCanvasEditor::VariableCategoryNodePaletteTreeItem>("Variables", root, categoryRoots);

            variableRoot->CreateChildNode<ScriptCanvasEditor::GetVariableNodePaletteTreeItem>("Get Variable");
            variableRoot->CreateChildNode<ScriptCanvasEditor::SetVariableNodePaletteTreeItem>("Set Variable");

            //Need to create Variable types here in order to get it to show up in the correct spot under the
            // VariableCategoryNodePaletteTreeItem
            variableTypeRoot = GetCategoryNode("Variables/Create Variable", root, categoryRoots);

            variableRoot->CreateChildNode<ScriptCanvasEditor::LocalVariablesListNodePaletteTreeItem>("Local Variables");
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
            ScriptCanvasEditor::NodePaletteTreeItem* parentItem = GetCategoryNode(categoryPath.c_str(), root, categoryRoots);
            parentItem->SetStyleOverride(type.second.m_styleOverride);

            // Children
            for (auto& node : ScriptCanvas::Library::LibraryDefinition::GetNodes(type.second.m_uuid))
            {
                ScriptCanvasEditor::NodePaletteTreeItem* nodeParent = nullptr;
                
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
                    
                    if (auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        if (auto categoryStyleAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                            {
                                parentItem->SetStyleOverride(categoryAttributeData->Get(nullptr));
                            }
                        }
                    }
                }
                
                // Need to detect primitive types.
                if (classData->m_azRtti && classData->m_azRtti->IsTypeOf<ScriptCanvas::PureData>())
                {
                    ScriptCanvasEditor::VariablePrimitiveNodePaletteTreeItem* varItem = variableTypeRoot->CreateChildNode<ScriptCanvasEditor::VariablePrimitiveNodePaletteTreeItem>(node.first, QString(name.c_str()), QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(node.first).c_str()));

                    if (toolTip.isEmpty())
                    {
                        AZStd::string displayTooltip;

                        // TODO: move to and consolidate these in the translation file
                        if (classData->m_editData && classData->m_editData->m_description)
                        {
                            toolTip = classData->m_editData->m_description;
                        }
                    }

                    varItem->SetToolTip(toolTip);
                }
                else
                {
                    ScriptCanvasEditor::CustomNodePaletteTreeItem* customItem = nodeParent->CreateChildNode<ScriptCanvasEditor::CustomNodePaletteTreeItem>(node.first, QString(name.c_str()), QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(node.first).c_str()));

                    if (toolTip.isEmpty() && classData->m_editData->m_description)
                    {
                        toolTip = classData->m_editData->m_description;
                    }

                    customItem->SetToolTip(toolTip);
                }
            }
        }

        // Merged all of these into a single pass(to avoid going over the giant lists multiple times), 
        // so now this looks absolutely horrifying in what it's doing. 
        // Everything is still mostly independent, and independently scoped to try to make the division of labor clear.
        ScriptCanvasEditor::NodePaletteTreeItem* otherRoot = GetCategoryNode("Other", root, categoryRoots);

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
                        variableTypeRoot->CreateChildNode<ScriptCanvasEditor::VariableObjectNodePaletteTreeItem>(QString(classIter.first.c_str()));
                    }
                    
                    ScriptCanvasEditor::NodePaletteTreeItem* categorizedRoot = nullptr;
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

                    ScriptCanvasEditor::NodePaletteTreeItem* objectItem = objectItem = GetCategoryNode(categoryPath.c_str(), root, categoryRoots);

                    for (auto method : behaviorClass->m_methods)
                    {
                        // Check for "ExcludeFrom" attribute for ScriptCanvas
                        auto excludeMethodAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, method.second->m_attributes));
                        if (ShouldExcludeFromNodeList(excludeMethodAttributeData, behaviorClass->m_azRtti ? behaviorClass->m_azRtti->GetTypeId() : behaviorClass->m_typeId, showExcludedPreviewNodes))
                        {
                            continue; // skip this method
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
                            ScriptCanvasEditor::NodePaletteTreeItem* methodParentItem = GetCategoryNode(methodCategoryPath.c_str(), root, categoryRoots);
                            methodParentItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(QString(classIter.first.c_str()), QString(method.first.c_str()));
                        }
                    }

                    if (objectItem->GetNumChildren() == 0)
                    {
                        objectItem->DetachItem();
                        delete objectItem;
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

                                ScriptCanvasEditor::NodePaletteTreeItem* busItem = GetCategoryNode(categoryPath.c_str(), useGenericEbusRoot ? otherRoot : root, categoryRoots);

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

                        ScriptCanvasEditor::NodePaletteTreeItem* busItem = GetCategoryNode(categoryPath.c_str(), useGenericSenderRoot ? otherRoot : root, categoryRoots);

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
                if (mapPair.second->GetNumChildren() == 0)
                {
                    GraphCanvas::GraphCanvasTreeItem* parentItem = mapPair.second->GetParent();
                    mapPair.second->DetachItem();

                    if (parentItem->GetNumChildren() == 0)
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

                if (parentItem->GetNumChildren() == 0)
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
        ////////////////////////////
        // NodePaletteTreeDelegate
        ////////////////////////////
        NodePaletteTreeDelegate::NodePaletteTreeDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
        {
        }

        void NodePaletteTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            painter->save();

            QStyleOptionViewItemV4 options = option;
            initStyleOption(&options, index);

            // paint the original node item
            QStyledItemDelegate::paint(painter, option, index);

            const int textMargin = options.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, options.widget) + 1;
            QRect textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
            textRect = textRect.adjusted(textMargin, 0, -textMargin, 0);

            QModelIndex sourceIndex = static_cast<const Model::NodePaletteSortFilterProxyModel*>(index.model())->mapToSource(index);
            NodePaletteTreeItem* treeItem = static_cast<NodePaletteTreeItem*>(sourceIndex.internalPointer());
            if (treeItem && treeItem->HasHighlight())
            {
                // pos, len
                const AZStd::pair<int, int>& highlight = treeItem->GetHighlight();
                QString preSelectedText = options.text.left(highlight.first);
                int preSelectedTextLength = options.fontMetrics.width(preSelectedText);
                QString selectedText = options.text.mid(highlight.first, highlight.second);
                int selectedTextLength = options.fontMetrics.width(selectedText);

                QRect highlightRect(textRect.left() + preSelectedTextLength, textRect.top(), selectedTextLength, textRect.height());

                // paint the highlight rect
                painter->fillRect(highlightRect, options.palette.highlight());
            }

            painter->restore();
        }

        ////////////////
        // NodePalette
        ////////////////
        NodePalette::NodePalette(const QString& windowLabel, QWidget* parent, bool inContextMenu)
            : AzQtComponents::StyledDockWidget(parent)
            , ui(new Ui::NodePalette())
            , m_contextMenuCreateEvent(nullptr)
            , m_view(nullptr)
            , m_model(nullptr)
        {
            m_view = aznew NodeTreeView(this);
            m_model = aznew Model::NodePaletteSortFilterProxyModel(this);

            m_filterTimer.setInterval(250);
            m_filterTimer.setSingleShot(true);
            m_filterTimer.stop();

            QObject::connect(&m_filterTimer, &QTimer::timeout, this, &NodePalette::UpdateFilter);

            setWindowTitle(windowLabel);
            ui->setupUi(this);

            QObject::connect(ui->m_quickFilter, &QLineEdit::textChanged, this, &NodePalette::OnQuickFilterChanged);

            QAction* clearAction = ui->m_quickFilter->addAction(QIcon(":/ScriptCanvasEditorResources/Resources/lineedit_clear.png"), QLineEdit::TrailingPosition);
            QObject::connect(clearAction, &QAction::triggered, this, &NodePalette::ClearFilter);

            GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(CreatePaletteRoot(), this);
            sourceModel->setMimeType(GetMimeType());

            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

            m_model->setSourceModel(sourceModel);
            m_model->PopulateUnfilteredModel();

            m_view->setModel(m_model);
            m_view->setItemDelegate(aznew NodePaletteTreeDelegate(this));

            if (!inContextMenu)
            {
                QObject::connect(ui->m_quickFilter, &QLineEdit::returnPressed, this, &NodePalette::UpdateFilter);
            }
            else
            {
                QObject::connect(ui->m_quickFilter, &QLineEdit::returnPressed, this, &NodePalette::TrySpawnItem);
            }

            if (inContextMenu)
            {
                setTitleBarWidget(new QWidget());
                setFeatures(NoDockWidgetFeatures);
                setContentsMargins(15, 0, 0, 0);
                ui->dockWidgetContents->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

                QObject::connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePalette::OnSelectionChanged);
            }

            QObject::connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged, this, &NodePalette::OnScrollChanged);

            ui->contentLayout->addWidget(m_view);

            if (!inContextMenu)
            {
                MainWindowNotificationBus::Handler::BusConnect();
                m_view->InitializeTreeViewSaving(AZ_CRC("NodePalette_ContextView", 0x3cfece67));
                m_view->ApplyTreeViewSnapshot();
            }
            else
            {
                m_view->InitializeTreeViewSaving(AZ_CRC("NodePalette_TreeView", 0x9d6844cd));
            }

            m_view->PauseTreeViewSaving();

            ui->m_categoryLabel->SetElideMode(Qt::ElideLeft);
        }

        NodePalette::~NodePalette()
        {
            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusDisconnect();
            delete m_contextMenuCreateEvent;
        }

        void NodePalette::FocusOnSearchFilter()
        {
            ui->m_quickFilter->setFocus(Qt::FocusReason::MouseFocusReason);
        }

        void NodePalette::ResetDisplay()
        {
            delete m_contextMenuCreateEvent;
            m_contextMenuCreateEvent = nullptr;

            {
                QSignalBlocker blocker(ui->m_quickFilter);
                ui->m_quickFilter->clear();

                m_model->ClearFilter();
                m_model->invalidate();
            }

            {
                QSignalBlocker blocker(m_view->selectionModel());
                m_view->clearSelection();
            }

            m_view->collapseAll();
            ui->m_categoryLabel->setFullText("");

            setVisible(true);
        }

        GraphCanvas::GraphCanvasMimeEvent* NodePalette::GetContextMenuEvent() const
        {
            return m_contextMenuCreateEvent;
        }

        void NodePalette::ResetSourceSlotFilter()
        {
            m_model->ResetSourceSlotFilter();
            ui->m_quickFilter->setCompleter(m_model->GetCompleter());
        }

        void NodePalette::FilterForSourceSlot(const AZ::EntityId& sceneId, const AZ::EntityId& sourceSlotId)
        {
            m_model->FilterForSourceSlot(sceneId, sourceSlotId);
            ui->m_quickFilter->setCompleter(m_model->GetCompleter());
        }

        void NodePalette::PreOnActiveSceneChanged()
        {
            ClearSelection();
            m_view->UnpauseTreeViewSaving();
            m_view->CaptureTreeViewSnapshot();
            m_view->PauseTreeViewSaving();

            m_view->model()->layoutAboutToBeChanged();
        }

        void NodePalette::PostOnActiveSceneChanged()
        {
            m_view->model()->layoutChanged();

            m_view->UnpauseTreeViewSaving();
            m_view->ApplyTreeViewSnapshot();
            m_view->PauseTreeViewSaving();

            if (m_model->HasFilter())
            {
                UpdateFilter();
            }
        }

        void NodePalette::ClearSelection()
        {
            m_view->selectionModel()->clearSelection();
        }

        void NodePalette::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            if (selected.indexes().empty())
            {
                // Nothing to do.
                return;
            }

            auto index = selected.indexes().first();

            if (!index.isValid())
            {
                // Nothing to do.
                return;
            }

            // IMPORTANT: mapToSource() is NECESSARY. Otherwise the internalPointer
            // in the index is relative to the proxy model, NOT the source model.
            QModelIndex sourceModelIndex = m_model->mapToSource(index);

            NodePaletteTreeItem* nodePaletteItem = static_cast<NodePaletteTreeItem*>(sourceModelIndex.internalPointer());
            HandleSelectedItem(nodePaletteItem);
        }

        void NodePalette::OnScrollChanged(int scrollPosition)
        {
            RefreshFloatingHeader();
        }

        void NodePalette::RefreshFloatingHeader()
        {
            // TODO: Fix slight visual hiccup with the sizing of the header when labels vanish.
            // Seems to remain at size for one frame.
            QModelIndex proxyIndex = m_view->indexAt(QPoint(0, 0));
            QModelIndex modelIndex = m_model->mapToSource(proxyIndex);
            NodePaletteTreeItem* currentItem = static_cast<NodePaletteTreeItem*>(modelIndex.internalPointer());

            QString fullPathString;

            bool needsSeparator = false;

            while (currentItem && currentItem->GetParent() != nullptr)
            {
                currentItem = static_cast<NodePaletteTreeItem*>(currentItem->GetParent());

                // This is the root element which is invisible. We don't want to display that.
                if (currentItem->GetParent() == nullptr)
                {
                    break;
                }

                if (needsSeparator)
                {
                    fullPathString.prepend("/");
                }

                fullPathString.prepend(currentItem->GetName());
                needsSeparator = true;
            }

            ui->m_categoryLabel->setFullText(fullPathString);
        }

        void NodePalette::OnQuickFilterChanged()
        {
            m_filterTimer.stop();
            m_filterTimer.start();
        }

        void NodePalette::UpdateFilter()
        {
            if (!m_model->HasFilter())
            {
                m_view->UnpauseTreeViewSaving();
                m_view->CaptureTreeViewSnapshot();
                m_view->PauseTreeViewSaving();
            }

            QString text = ui->m_quickFilter->text();
            
            // The QCompleter doesn't seem to update the completion prefix when you delete anything, only when thigns are added.
            // To get it to update correctly when the user deletes something, I'm using the combination of things:
            //
            // 1) If we have a completion, that text will be auto filled into the quick filter because of the completion model.
            // So, we will compare those two values, and if they match, we know we want to search using the completion prefix.
            //
            // 2) If they don't match, it means that user deleted something, and the Completer didn't update it's internal state, so we'll just
            // use whatever is in the text box.
            //
            // 3) When the text field is set to empty, the current completion gets invalidated, but the prefix doesn't, so that gets special cased out.
            //
            // Extra fun: If you type in something, "Like" then delete a middle character, "Lie", and then put the k back in. It will auto complete the E
            // visually but the completion prefix will be the entire word.
            if (ui->m_quickFilter->completer() 
                && ui->m_quickFilter->completer()->currentCompletion().compare(text, Qt::CaseInsensitive) == 0
                && !text.isEmpty())
            {
                text = ui->m_quickFilter->completer()->completionPrefix();
            }

            m_model->SetFilter(text);
            m_model->invalidate();

            if (!m_model->HasFilter())
            {
                m_view->UnpauseTreeViewSaving();
                m_view->ApplyTreeViewSnapshot();
                m_view->PauseTreeViewSaving();

                m_view->clearSelection();
            }
            else
            {
                m_view->expandAll();
            }
        }

        void NodePalette::ClearFilter()
        {
            {
                QSignalBlocker blocker(ui->m_quickFilter);
                ui->m_quickFilter->setText("");
            }

            UpdateFilter();
        }

        void NodePalette::TrySpawnItem()
        {
            QCompleter* completer = ui->m_quickFilter->completer();
            QModelIndex modelIndex = completer->currentIndex();

            if (modelIndex.isValid())
            {
                // The docs say this is fine. So here's hoping.
                QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(completer->completionModel());

                if (proxyModel)
                {
                    QModelIndex sourceIndex = proxyModel->mapToSource(modelIndex);

                    if (sourceIndex.isValid())
                    {
                        Model::NodePaletteAutoCompleteModel* autoCompleteModel = qobject_cast<Model::NodePaletteAutoCompleteModel*>(proxyModel->sourceModel());

                        const GraphCanvas::GraphCanvasTreeItem* treeItem = autoCompleteModel->FindItemForIndex(sourceIndex);

                        if (treeItem)
                        {
                            HandleSelectedItem(treeItem);
                        }
                    }
                }
            }
            else
            {
                UpdateFilter();
            }
        }

        void NodePalette::HandleSelectedItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
        {
            m_contextMenuCreateEvent = treeItem->CreateMimeEvent();

            if (m_contextMenuCreateEvent)
            {
                emit OnContextMenuSelection();
            }
        }

        void NodePalette::ResetModel()
        {
            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusDisconnect(); 

            GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(CreatePaletteRoot(), this);
            sourceModel->setMimeType(GetMimeType());
            
            delete m_model;
            m_model = aznew Model::NodePaletteSortFilterProxyModel(this);
            m_model->setSourceModel(sourceModel);
            m_model->PopulateUnfilteredModel();
            
            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

            m_view->setModel(m_model);

            ResetDisplay();
        }

        #include <Editor/View/Widgets/NodePalette.moc>
    }
}

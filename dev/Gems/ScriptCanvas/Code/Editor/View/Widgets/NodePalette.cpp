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
#include <qboxlayout.h>
#include <qpainter.h>
#include <qevent.h>
#include <QCoreApplication>

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
#include <Libraries/Logic/Boolean.h>
#include <Libraries/Math/Number.h>
#include <Libraries/Core/String.h>

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
            auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
          
            if (auto excludeAttribute = editorElementData->FindAttribute(AZ::Script::Attributes::ExcludeFrom))
            {
                auto excludeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(excludeAttribute);
                return excludeAttributeData && ShouldExcludeFromNodeList(excludeAttributeData, typeId, showExcludedPreviewNodes);
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
                if (HasExcludeFromNodeListAttribute(serializeContext, node.first, showExcludedPreviewNodes))
                {
                    continue;
                }

                // Pass in the associated class data so we can do more intensive lookups?
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(node.first);

                QString name = QString(node.second.c_str());

                if (classData && classData->m_editData && classData->m_editData->m_name)
                {
                    name = classData->m_editData->m_name;
                    
                    auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    if (editorElementData)
                    {
                        if (auto categoryAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Category))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                const char* category = categoryAttributeData->Get(nullptr);
                                parentItem = GetCategoryNode(category, root, categoryRoots);
                            }
                        }

                        if (auto categoryStyleAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::CategoryStyle))
                        {
                            if (auto categoryAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryStyleAttribute))
                            {
                                parentItem->SetStyleOverride(categoryAttributeData->Get(nullptr));
                            }
                        }
                    }
                }

                // Need to detect primitive types.
                if (classData->m_azRtti && 
                    (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Logic::Boolean>()
                     || classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Math::Number>()
                     || classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::String>()))
                {
                    ScriptCanvasEditor::VariablePrimitiveNodePaletteTreeItem* varItem = variableTypeRoot->CreateChildNode<ScriptCanvasEditor::VariablePrimitiveNodePaletteTreeItem>(node.first, name, QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(node.first).c_str()));

                    const char* displayTooltip;

                    // TODO: move to and consolidate these in the translation file
                    if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Logic::Boolean>())
                    {
                        displayTooltip = "A variable that stores one of two values: True or False";
                    }
                    else if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Math::Number>())
                    {
                        displayTooltip = "A variable that stores a number";
                    }
                    else if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::String>())
                    {
                        displayTooltip = "A variable that stores a text string";
                    }

                    varItem->SetToolTip(displayTooltip);
                }
                else
                {
                    ScriptCanvasEditor::CustomNodePaletteTreeItem* customItem = parentItem->CreateChildNode<ScriptCanvasEditor::CustomNodePaletteTreeItem>(node.first, name, QString(ScriptCanvasEditor::IconComponent::LookupClassIcon(node.first).c_str()));

                    if (classData->m_editData->m_description)
                    {
                        customItem->SetToolTip(classData->m_editData->m_description);
                    }
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
                    continue; // skip this class
                }

                // Objects and Object methods
                {
                    bool canCreate = serializeContext->FindClassData(behaviorClass->m_typeId) != nullptr;
                    
                    // Do not allow variable creation for data that derives from AZ::Component
                    for (auto base : behaviorClass->m_baseClasses)
                    {
                        if (AZ::Component::TYPEINFO_Uuid() == base)
                        {
                            canCreate = false;
                            break;
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

                        objectItem->CreateChildNode<ScriptCanvasEditor::ClassMethodEventPaletteTreeItem>(QString(classIter.first.c_str()), QString(method.first.c_str()));
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
        ///////////////////
        // AutoElidedText
        ///////////////////

        AutoElidedText::AutoElidedText(QWidget* parent, Qt::WindowFlags flags)
            : QLabel(parent, flags)
        {
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        }

        AutoElidedText::~AutoElidedText()
        {
        }

        QString AutoElidedText::fullText() const
        {
            return m_fullText;
        }

        void AutoElidedText::setFullText(const QString& text)
        {
            m_fullText = text;
            RefreshLabel();
        }

        void AutoElidedText::resizeEvent(QResizeEvent* resizeEvent)
        {
            RefreshLabel();
            QLabel::resizeEvent(resizeEvent);
        }

        QSize AutoElidedText::minimumSizeHint() const
        {
            QSize retVal = QLabel::minimumSizeHint();
            retVal.setWidth(0);

            return retVal;
        }

        QSize AutoElidedText::sizeHint() const
        {
            QSize retVal = QLabel::sizeHint();
            retVal.setWidth(0);

            return retVal;
        }

        void AutoElidedText::RefreshLabel()
        {
            QFontMetrics metrics(font());

            int left = 0;
            int top = 0;
            int right = 0;
            int bottom = 0;

            getContentsMargins(&left, &top, &right, &bottom);

            int labelWidth = width();

            QString elidedText = metrics.elidedText(m_fullText, Qt::ElideRight, (width() - (left + right)));

            setText(elidedText);
        }

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
            , m_sliding(false)
            , m_scrollRange(0)
            , m_scrollbarHeight(1)
            , m_offsetCounter(0)
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
            QObject::connect(ui->m_quickFilter, &QLineEdit::returnPressed, this, &NodePalette::UpdateFilter);

            QAction* clearAction = ui->m_quickFilter->addAction(QIcon(":/ScriptCanvasEditorResources/Resources/lineedit_clear.png"), QLineEdit::TrailingPosition);
            QObject::connect(clearAction, &QAction::triggered, this, &NodePalette::ClearFilter);

            GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(CreatePaletteRoot(), this);
            sourceModel->setMimeType(GetMimeType());

            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

            m_model->setSourceModel(sourceModel);

            m_view->setModel(m_model);
            m_view->setItemDelegate(aznew NodePaletteTreeDelegate(this));

            if (inContextMenu)
            {
                setTitleBarWidget(new QWidget());
                setFeatures(NoDockWidgetFeatures);
                setContentsMargins(15, 0, 0, 0);
                ui->dockWidgetContents->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

                QObject::connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodePalette::OnSelectionChanged);
            }

            m_view->verticalScrollBar()->installEventFilter(this);
            QObject::connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged, this, &NodePalette::OnScrollChanged);
            QObject::connect(m_view->verticalScrollBar(), &QScrollBar::sliderPressed, [this]() { this->m_sliding = true; });
            QObject::connect(m_view->verticalScrollBar(), &QScrollBar::sliderReleased, [this]() { this->m_sliding = false; });

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

                m_model->m_filter.clear();
                m_model->invalidate();
            }

            {
                QSignalBlocker blocker(m_view->selectionModel());
                m_view->clearSelection();
            }

            m_view->collapseAll();
            for (QLabel* label : m_floatingLabels)
            {
                label->setVisible(false);
            }

            setVisible(true);
        }

        GraphCanvas::GraphCanvasMimeEvent* NodePalette::GetContextMenuEvent() const
        {
            return m_contextMenuCreateEvent;
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

            if (!m_model->m_filter.isEmpty())
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
            
            m_contextMenuCreateEvent = nodePaletteItem->CreateMimeEvent();

            if (m_contextMenuCreateEvent)
            {
                emit OnContextMenuSelection();
            }
        }

        void NodePalette::OnScrollChanged(int scrollPosition)
        {
            RefreshFloatingHeader();
        }

        bool NodePalette::eventFilter(QObject* object, QEvent* event)
        {
            bool consumeEvent = false;

            // The floating headers make the scrollbar a bit of a nightmare to use
            // since they resize the scroll bar from underneath of you.
            //
            // We can't really pre-allocate a lot of space. So to get around that.
            // We're going to fake being the scroll bar, and manage it independently from the widget
            // to make it a consistent feel.
            //
            // Basic idea: We calculate our scroll by using deltas in global space(not local space since that gets modified).
            // Then we translate our screen deltas into scroll bar deltas(using the original sizing) which we then apply.
            if (event->type() == QEvent::MouseButtonPress)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                m_lastPosition = mouseEvent->globalPos();

                m_topRight = m_view->mapToGlobal(m_view->contentsRect().topRight());
                m_bottomRight = m_view->mapToGlobal(m_view->contentsRect().bottomRight());

                m_offsetCounter = 0;
                m_scrollbarHeight = AZ::GetMax(1, m_view->verticalScrollBar()->size().height());
                m_scrollRange = (m_view->verticalScrollBar()->maximum() - m_view->verticalScrollBar()->minimum()) + m_view->verticalScrollBar()->pageStep();
            }
            else if (event->type() == QEvent::MouseMove && m_sliding)
            {
                consumeEvent = true;

                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                QPointF currentPos = mouseEvent->globalPos();

                if (currentPos.y() < m_topRight.y())
                {
                    m_lastPosition = currentPos;
                    m_offsetCounter = 0;
                    m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->minimum());
                }
                else if (currentPos.y() > m_bottomRight.y())
                {
                    m_lastPosition = currentPos;
                    m_offsetCounter = 0;
                    m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
                }
                else
                {
                    qreal delta = m_lastPosition.y() - currentPos.y();
                    qreal percent = delta / m_scrollbarHeight;

                    m_offsetCounter += (m_scrollRange * percent);

                    int valueOffset = 0;

                    if (std::fabs(m_offsetCounter) > 1.0)
                    {
                        valueOffset = static_cast<int>(m_offsetCounter);
                        m_offsetCounter -= valueOffset;
                        m_lastPosition = currentPos;
                    }

                    if (valueOffset != 0)
                    {
                        m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->value() - valueOffset);

                        if (m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->maximum()
                            || m_view->verticalScrollBar()->value() == m_view->verticalScrollBar()->minimum())
                        {
                            m_offsetCounter = 0;
                        }
                    }
                }
            }

            return consumeEvent;
        }

        void NodePalette::RefreshFloatingHeader()
        {
            // TODO: Fix slight visual hiccup with the sizing of the header when labels vanish.
            // Seems to remain at size for one frame.
            QModelIndex proxyIndex = m_view->indexAt(QPoint(0, 0));
            QModelIndex modelIndex = m_model->mapToSource(proxyIndex);
            NodePaletteTreeItem* currentItem = static_cast<NodePaletteTreeItem*>(modelIndex.internalPointer());

            // Assume everything is in order.
            // Once we hit an invisible item. No need to reset it, or anything after it since they weren't used.
            for (QLabel* label : m_floatingLabels)
            {
                if (label->isVisible())
                {
                    label->setContentsMargins(0, 0, 0, 0);
                }
                else
                {
                    break;
                }
            }

            int counter = 0;

            while (currentItem && currentItem->GetParent() != nullptr)
            {
                currentItem = static_cast<NodePaletteTreeItem*>(currentItem->GetParent());

                // This is the root element which is invisible. We don't want to display that.
                if (currentItem->GetParent() == nullptr)
                {
                    break;
                }

                if (counter >= m_floatingLabels.size())
                {
                    AutoElidedText* label = aznew AutoElidedText(ui->floatingHeaderFrame);
                    label->setVisible(false);

                    m_floatingLabels.push_back(label);
                    static_cast<QVBoxLayout*>(ui->floatingHeaderFrame->layout())->insertWidget(0, label);
                }

                AutoElidedText* displayLabel = m_floatingLabels[counter];
                displayLabel->setFullText(currentItem->GetName());
                displayLabel->setVisible(true);
                ++counter;
            }

            for (int i = 0; i < m_floatingLabels.size(); ++i)
            {
                if (i < counter)
                {
                    int margin = 16 * (counter - (i + 1));
                    QLabel* tabLabel = m_floatingLabels[i];
                    tabLabel->setContentsMargins(margin, 0, 0, 0);
                }
                else
                {
                    m_floatingLabels[i]->setVisible(false);
                }
            }
        }

        void NodePalette::OnQuickFilterChanged()
        {
            m_filterTimer.stop();
            m_filterTimer.start();
        }

        void NodePalette::UpdateFilter()
        {
            if (m_model->m_filter.isEmpty())
            {
                m_view->UnpauseTreeViewSaving();
                m_view->CaptureTreeViewSnapshot();
                m_view->PauseTreeViewSaving();
            }

            m_model->m_filter = ui->m_quickFilter->text();
            m_model->invalidate();

            if (m_model->m_filter.isEmpty())
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
                m_model->m_filter.clear();
            }

            UpdateFilter();
        }

        void NodePalette::ResetModel()
        {
            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusDisconnect(); 

            GraphCanvas::GraphCanvasTreeModel* sourceModel = aznew GraphCanvas::GraphCanvasTreeModel(CreatePaletteRoot(), this);
            sourceModel->setMimeType(GetMimeType());
            
            delete m_model;
            m_model = aznew Model::NodePaletteSortFilterProxyModel(this);
            m_model->setSourceModel(sourceModel);
            
            GraphCanvas::GraphCanvasTreeModelRequestBus::Handler::BusConnect(sourceModel);

            m_view->setModel(m_model);

            ResetDisplay();
        }

        #include <Editor/View/Widgets/NodePalette.moc>
    }
}

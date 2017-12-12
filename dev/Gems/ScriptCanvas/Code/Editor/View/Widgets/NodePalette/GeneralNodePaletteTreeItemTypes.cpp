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

#include <AzCore/Serialization/SerializeContext.h>

#include <QCoreApplication>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>

#include "GeneralNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include <Core/Attributes.h>
#include <Editor/Metrics.h>
#include <Libraries/Core/Method.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // CreateClassMethodMimeEvent
    ///////////////////////////////

    void CreateClassMethodMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateClassMethodMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("ClassName", &CreateClassMethodMimeEvent::m_className)
                ->Field("MethodName", &CreateClassMethodMimeEvent::m_methodName)
                ;
        }
    }

    CreateClassMethodMimeEvent::CreateClassMethodMimeEvent(const QString& className, const QString& methodName)
        : m_className(className.toUtf8().data())
        , m_methodName(methodName.toUtf8().data())
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateClassMethodMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::Method>::Uuid(), graphId);
        return Nodes::CreateObjectOrObjectMethodNode(m_className, m_methodName, graphId);
    }

    ////////////////////////////////////
    // ClassMethodEventPaletteTreeItem
    ////////////////////////////////////

    const QString& ClassMethodEventPaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::Method::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    ClassMethodEventPaletteTreeItem::ClassMethodEventPaletteTreeItem(const QString& className, const QString& methodName)
        : DraggableNodePaletteTreeItem(methodName, GetDefaultIcon())
        , m_className(className)
        , m_methodName(methodName)
    {
        AZStd::string displayMethodName = TranslationHelper::GetKeyTranslation(TranslationContextGroup::ClassMethod, m_className.toUtf8().data(), m_methodName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Name);

        if (displayMethodName.empty())
        {
            SetName(m_methodName);
        }
        else
        {
            SetName(displayMethodName.c_str());
        }

        AZStd::string displayEventTooltip = TranslationHelper::GetKeyTranslation(TranslationContextGroup::ClassMethod, m_className.toUtf8().data(), m_methodName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Tooltip);

        if (!displayEventTooltip.empty())
        {
            SetToolTip(displayEventTooltip.c_str());
        }
    }

    GraphCanvas::GraphCanvasMimeEvent* ClassMethodEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateClassMethodMimeEvent(m_className, m_methodName);
    }

    //////////////////////////////
    // CreateCustomNodeMimeEvent
    //////////////////////////////

    void CreateCustomNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCustomNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(1)
                ->Field("TypeId", &CreateCustomNodeMimeEvent::m_typeId)
                ->Field("StyleOverride", &CreateCustomNodeMimeEvent::m_styleOverride)
                ;
        }
    }

    CreateCustomNodeMimeEvent::CreateCustomNodeMimeEvent(const AZ::Uuid& typeId)
        : m_typeId(typeId)
    {
    }

    CreateCustomNodeMimeEvent::CreateCustomNodeMimeEvent(const AZ::Uuid& typeId, const AZStd::string& styleOverride)
        : m_typeId(typeId)
        , m_styleOverride(styleOverride)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateCustomNodeMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropNode, m_typeId, graphId);
        return Nodes::CreateNode(m_typeId, graphId, m_styleOverride);
    }

    //////////////////////////////
    // CustomNodePaletteTreeItem
    //////////////////////////////

    CustomNodePaletteTreeItem::CustomNodePaletteTreeItem(const AZ::Uuid& typeId, const QString& nodeName, const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, iconPath)
        , m_typeId(typeId)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* CustomNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateCustomNodeMimeEvent(m_typeId, GetStyleOverride());
    }
}
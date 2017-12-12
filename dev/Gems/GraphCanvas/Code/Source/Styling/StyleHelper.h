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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/std/string/string_view.h>

#include <QVariant>
#include <QString>
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QPen>

#include <Components/StyleBus.h>
#include <Components/SceneBus.h>
#include <Styling/definitions.h>
#include <Styling/Style.h>
#include <Styling/PseudoElement.h>

#include <QDebug>

Q_DECLARE_METATYPE(QFont::Style);
Q_DECLARE_METATYPE(QFont::Capitalization);
Q_DECLARE_METATYPE(Qt::AlignmentFlag);
Q_DECLARE_METATYPE(Qt::PenCapStyle);
Q_DECLARE_METATYPE(Qt::PenStyle);
Q_DECLARE_METATYPE(GraphCanvas::Styling::Curves);

namespace GraphCanvas
{
    namespace Styling
    {

        //! Convenience wrapper for a styled entity that resolves its style and then provides easy ways to get common
        //! Qt values out of the style for it.
        class StyleHelper
        {
        public:
            AZ_CLASS_ALLOCATOR(StyleHelper, AZ::SystemAllocator, 0);

            StyleHelper() = default;

            StyleHelper(const AZ::EntityId& styledEntity)
            {
                SetStyle(styledEntity);
            }

            StyleHelper(const AZ::EntityId& realStyledEntity, const AZStd::string& virtualChildElement)
            {
                SetStyle(realStyledEntity, virtualChildElement);
            }

            virtual ~StyleHelper()
            {
                ReleaseStyle();
            }

            void SetScene(const AZ::EntityId& sceneId)
            {
                if (m_scene != sceneId)
                {
                    ReleaseStyle();
                    m_scene = sceneId;
                }
            }

            void SetStyle(const AZ::EntityId& styledEntity)
            {
                ReleaseStyle();

                m_styledEntity = styledEntity;

                SceneMemberRequestBus::EventResult(m_scene, m_styledEntity, &SceneMemberRequests::GetScene);
                if (!m_scene.IsValid())
                {
                    return;
                }

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                UpdateStyle();
                
#if 0
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
#endif
            }

            void SetStyle(const AZStd::string& style)
            {
                ReleaseStyle();

                m_deleteStyledEntity = true;

                PseudoElementFactoryRequestBus::BroadcastResult(m_styledEntity, &PseudoElementFactoryRequests::CreateStyleEntity, style);

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                SceneMemberNotificationBus::Event(m_styledEntity, &SceneMemberNotifications::OnSceneSet, m_scene);

                UpdateStyle();
            }

            void SetStyle(const AZ::EntityId& parentStyledEntity, const AZStd::string& virtualChildElement)
            {
                ReleaseStyle();

                m_deleteStyledEntity = true;
                
                SceneMemberRequestBus::EventResult(m_scene, parentStyledEntity, &SceneMemberRequests::GetScene);

                PseudoElementFactoryRequestBus::BroadcastResult(m_styledEntity, &PseudoElementFactoryRequests::CreateVirtualChild, parentStyledEntity, virtualChildElement);

                for (const auto& selector : m_styleSelectors)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.c_str());
                }

                UpdateStyle();

#if 0
                AZStd::string description;
                StyleRequestBus::EventResult(description, m_style, &StyleRequests::GetDescription);
                qDebug() << description.c_str();
#endif
            }

            bool HasAttribute(Styling::Attribute attribute)
            {
                bool hasAttribute = false;
                StyleRequestBus::EventResult(hasAttribute, m_style, &StyleRequests::HasAttribute, static_cast<AZ::u32>(attribute));
                return hasAttribute;
            }

            template<typename Value>
            Value GetAttribute(Styling::Attribute attribute, const Value& defaultValue = Value()) const
            {
                auto rawAttribute = static_cast<AZ::u32>(attribute);

                bool hasAttribute = false;
                StyleRequestBus::EventResult(hasAttribute, m_style, &StyleRequests::HasAttribute, rawAttribute);

                if (!hasAttribute)
                {
                    return defaultValue;
                }

                QVariant result;
                StyleRequestBus::EventResult(result, m_style, &StyleRequests::GetAttribute, rawAttribute);

                return result.value<Value>();
            }

            QColor GetColor(Styling::Attribute color, QColor defaultValue = QColor()) const
            {
                return GetAttribute(color, defaultValue);
            }

            QFont GetFont() const
            {
                QFont font;
                QFontInfo info(font);
                info.pixelSize();

                font.setFamily(GetAttribute(Attribute::FontFamily, font.family()));
                font.setPixelSize(GetAttribute(Attribute::FontSize, info.pixelSize()));
                font.setWeight(GetAttribute(Attribute::FontWeight, font.weight()));
                font.setStyle(GetAttribute(Attribute::FontStyle, font.style()));
                font.setCapitalization(GetAttribute(Attribute::FontVariant, font.capitalization()));

                return font;
            }

            //! Helper method which constructs a stylesheet based on the calculated font style.
            //! We need this too pass along to certain Qt widgets because we use our own custom style parsing system.
            QString GetFontStyleSheet() const
            {
                QFont font = GetFont();
                QColor color = GetColor(Styling::Attribute::Color);

                QStringList fields;

                fields.push_back(QString("color: rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha()));

                fields.push_back(QString("font-family: %1").arg(font.family()));
                fields.push_back(QString("font-size: %1px").arg(font.pixelSize()));

                if (font.bold())
                {
                    fields.push_back("font-weight: bold");
                }

                switch (font.style())
                {
                case QFont::StyleNormal:
                    break;
                case QFont::StyleItalic:
                    fields.push_back("font-style: italic");
                    break;
                case QFont::StyleOblique:
                    fields.push_back("font-style: italic");
                    break;
                }

                const bool underline = font.underline();
                const bool strikeOut = font.strikeOut();

                if (underline && strikeOut)
                {
                    fields.push_back("text-decoration: underline line-through");
                }
                else if (underline)
                {
                    fields.push_back("text-decoration: underline");
                }
                else if (strikeOut)
                {
                    fields.push_back("text-decoration: line-through");
                }

                return fields.join("; ");
            }

            QPen GetPen(Styling::Attribute width, Styling::Attribute style, Styling::Attribute color, Styling::Attribute cap, bool cosmetic = false) const
            {
                QPen pen;
                pen.setColor(GetAttribute(color, QColor(Qt::black)));
                pen.setWidth(GetAttribute(width, 1));
                pen.setStyle(GetAttribute(style, Qt::SolidLine));
                pen.setCapStyle(GetAttribute(cap, Qt::SquareCap));
                pen.setCosmetic(cosmetic);

                return pen;
            }

            QPen GetBorder() const
            {
                return GetPen(Styling::Attribute::BorderWidth, Styling::Attribute::BorderStyle, Styling::Attribute::BorderColor, Styling::Attribute::CapStyle);
            }

            QBrush GetBrush(Styling::Attribute color, QBrush defaultValue = QBrush()) const
            {
                return GetAttribute(color, defaultValue);
            }

            QSizeF GetSize(QSizeF defaultSize) const
            {
                return{
                    GetAttribute(Styling::Attribute::Width, defaultSize.width()),
                    GetAttribute(Styling::Attribute::Height, defaultSize.height())
                };
            }

            QSizeF GetMinimumSize(QSizeF defaultSize = QSizeF(0,0)) const
            {
                return QSizeF(GetAttribute(Styling::Attribute::MinWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MinHeight, defaultSize.height()));
            }

            QSizeF GetMaximumSize(QSizeF defaultSize = QSizeF(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max())) const
            {
                return QSizeF(GetAttribute(Styling::Attribute::MaxWidth, defaultSize.width()), GetAttribute(Styling::Attribute::MaxHeight, defaultSize.height()));
            }

            QMarginsF GetMargins(QMarginsF defaultMargins = QMarginsF()) const
            {
                bool hasMargin = false;
                StyleRequestBus::EventResult(hasMargin, m_style, &StyleRequests::HasAttribute, static_cast<AZ::u32>(Styling::Attribute::Margin));

                if (hasMargin)
                {
                    qreal defaultMargin = GetAttribute(Styling::Attribute::Margin, 0);
                    defaultMargins = QMarginsF(defaultMargin, defaultMargin, defaultMargin, defaultMargin);
                }

                return QMarginsF(
                    GetAttribute(Styling::Attribute::Margin/*TODO Left*/, defaultMargins.left()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Top*/, defaultMargins.top()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Right*/, defaultMargins.right()),
                    GetAttribute(Styling::Attribute::Margin/*TODO Bottom*/, defaultMargins.bottom())
                );
            }

            bool HasTextAlignment()
            {
                return HasAttribute(Styling::Attribute::TextAlignment) || HasAttribute(Styling::Attribute::TextVerticalAlignment);
            }

            Qt::Alignment GetTextAlignment(Qt::Alignment alignment)
            {
                bool horizontalAlignment = HasAttribute(Styling::Attribute::TextAlignment);
                bool verticalAlignment = HasAttribute(Styling::Attribute::TextVerticalAlignment);

                if (horizontalAlignment || verticalAlignment)
                {
                    Qt::Alignment alignment;

                    if (horizontalAlignment)
                    {
                        alignment = alignment | GetAttribute(Styling::Attribute::TextAlignment, Qt::AlignmentFlag::AlignLeft);
                    }

                    if (verticalAlignment)
                    {
                        alignment = alignment | GetAttribute(Styling::Attribute::TextVerticalAlignment, Qt::AlignmentFlag::AlignTop);
                    }

                    return alignment;
                }

                return alignment;
            }

            void AddSelector(const AZStd::string_view& selector)
            {
                auto insertResult = m_styleSelectors.insert(AZStd::string(selector));

                if (insertResult.second && m_styledEntity.IsValid())
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::AddSelectorState, selector.to_string().c_str());

                    UpdateStyle();
                }
            }

            void RemoveSelector(const AZStd::string_view& selector)
            {
                AZStd::size_t elements = m_styleSelectors.erase(selector);

                if (elements > 0)
                {
                    StyledEntityRequestBus::Event(m_styledEntity, &StyledEntityRequests::RemoveSelectorState, selector.to_string().c_str());

                    UpdateStyle();
                }
            }

        private:

            void UpdateStyle()
            {
                ReleaseStyle(false);
                StyleSheetRequestBus::EventResult(m_style, m_scene, &StyleSheetRequests::ResolveStyles, m_styledEntity);
            }

            void ReleaseStyle(bool destroyChildElement = true)
            {
                if (m_style.IsValid())
                {
                    if (m_deleteStyledEntity && destroyChildElement)
                    {
                        m_deleteStyledEntity = false;
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_styledEntity);
                    }
                    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_style);

                    m_style.SetInvalid();
                }
            }
            
            AZ::EntityId m_scene;
            AZ::EntityId m_styledEntity;
            AZ::EntityId m_style;

            bool m_deleteStyledEntity = false;

            AZStd::unordered_set< AZStd::string > m_styleSelectors;
        };

    }
}
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

#include <precompiled.h>
#include <Styling/Style.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>

#include <algorithm>

#pragma warning(push)
#pragma warning(disable : 4244 4251 4996) // _CRT_SECURE_NO_WARNINGS equivalent

#include <QFont>
#include <QFontInfo>
#include <QPen>

#pragma warning(pop)

#include <Components/StyleBus.h>
#include <Styling/definitions.h>
#include <Styling/StyleHelper.h>

#include <QDebug>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QVariant, "{EFF68052-50FF-4072-8047-31243EF95FEE}");
}

namespace
{
    static bool StyleVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 3)
        {
            auto selectorArrayElementIndex = classElement.FindElement(AZ_CRC("Selectors", 0x0e4270a0));
            if (selectorArrayElementIndex != -1)
            {
                classElement.RemoveElement(selectorArrayElementIndex);
            }
        }
        return true;
    }

    namespace Styling = GraphCanvas::Styling;

    struct StyleMatch
    {
        Styling::Style* style;
        int complexity;

        bool operator<(const StyleMatch& o) const
        {
            if (complexity > 0 && o.complexity > 0)
            {
                return complexity > o.complexity;
            }
            else if (complexity < 0 && o.complexity < 0)
            {
                return complexity < o.complexity;
            }
            else if (complexity < 0)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    };

    QString AttributeName(Styling::Attribute attribute)
    {
        switch (attribute)
        {
        case Styling::Attribute::BackgroundColor:
            return Styling::Attributes::BackgroundColor;
        case Styling::Attribute::BackgroundImage:
            return Styling::Attributes::BackgroundImage;
        case Styling::Attribute::CapStyle:
            return Styling::Attributes::CapStyle;
        case Styling::Attribute::GridMajorWidth:
            return Styling::Attributes::GridMajorWidth;
        case Styling::Attribute::GridMajorStyle:
            return Styling::Attributes::GridMajorStyle;
        case Styling::Attribute::GridMajorColor:
            return Styling::Attributes::GridMajorColor;
        case Styling::Attribute::GridMinorWidth:
            return Styling::Attributes::GridMinorWidth;
        case Styling::Attribute::GridMinorStyle:
            return Styling::Attributes::GridMinorStyle;
        case Styling::Attribute::GridMinorColor:
            return Styling::Attributes::GridMinorColor;
        case Styling::Attribute::FontFamily:
            return Styling::Attributes::FontFamily;
        case Styling::Attribute::FontSize:
            return Styling::Attributes::FontSize;
        case Styling::Attribute::FontWeight:
            return Styling::Attributes::FontWeight;
        case Styling::Attribute::FontStyle:
            return Styling::Attributes::FontStyle;
        case Styling::Attribute::FontVariant:
            return Styling::Attributes::FontVariant;
        case Styling::Attribute::Color:
            return Styling::Attributes::Color;
        case Styling::Attribute::BorderWidth:
            return Styling::Attributes::BorderWidth;
        case Styling::Attribute::BorderStyle:
            return Styling::Attributes::BorderStyle;
        case Styling::Attribute::BorderColor:
            return Styling::Attributes::BorderColor;
        case Styling::Attribute::BorderRadius:
            return Styling::Attributes::BorderRadius;
        case Styling::Attribute::LineWidth:
            return Styling::Attributes::LineWidth;
        case Styling::Attribute::LineStyle:
            return Styling::Attributes::LineStyle;
        case Styling::Attribute::LineColor:
            return Styling::Attributes::LineColor;
        case Styling::Attribute::LineCurve:
            return Styling::Attributes::LineCurve;
        case Styling::Attribute::LineSelectionPadding:
            return Styling::Attributes::LineSelectionPadding;
        case Styling::Attribute::Margin:
            return Styling::Attributes::Margin;
        case Styling::Attribute::Padding:
            return Styling::Attributes::Padding;
        case Styling::Attribute::Width:
            return Styling::Attributes::Width;
        case Styling::Attribute::Height:
            return Styling::Attributes::Height;
        case Styling::Attribute::MinWidth:
            return Styling::Attributes::MinWidth;
        case Styling::Attribute::MaxWidth:
            return Styling::Attributes::MaxWidth;
        case Styling::Attribute::MinHeight:
            return Styling::Attributes::MinHeight;
        case Styling::Attribute::MaxHeight:
            return Styling::Attributes::MaxHeight;
        case Styling::Attribute::Selectors:
            return Styling::Attributes::Selectors;
        case Styling::Attribute::TextAlignment:
            return Styling::Attributes::TextAlignment;
        case Styling::Attribute::Invalid:
        default:
            return "Invalid Attribute";
        }
    }

    class QVariantSerializer
        : public AZ::SerializeContext::IDataSerializer
    {
        /// Store the class data into a binary buffer
        virtual size_t Save(const void* classPtr, AZ::IO::GenericStream& stream, bool isDataBigEndian /*= false*/)
        {
            auto variant = reinterpret_cast<const QVariant*>(classPtr);

            QByteArray buffer;
            QDataStream qtStream(&buffer, QIODevice::WriteOnly);
            qtStream.setByteOrder(isDataBigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
            qtStream << *variant;

            return static_cast<size_t>(stream.Write(buffer.length(), reinterpret_cast<const void*>(buffer.data())));
        }

        /// Convert binary data to text
        virtual size_t DataToText(AZ::IO::GenericStream& in, AZ::IO::GenericStream& out, bool isDataBigEndian /*= false*/)
        {
            (void)isDataBigEndian;

            QByteArray buffer = ReadAll(in);
            QByteArray&& base64 = buffer.toBase64();
            return static_cast<size_t>(out.Write(base64.size(), base64.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        virtual size_t TextToData(const char* text, unsigned int textVersion, AZ::IO::GenericStream& stream, bool isDataBigEndian = false)
        {
            (void)textVersion;
            (void)isDataBigEndian;
            AZ_Assert(textVersion == 0, "Unknown char, short, int version!");

            QByteArray decoder = QByteArray::fromBase64(text);
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            return static_cast<size_t>(stream.Write(decoder.size(), decoder.constData()));
        }

        /// Load the class data from a stream.
        virtual bool Load(void* classPtr, AZ::IO::GenericStream& in, unsigned int, bool isDataBigEndian = false)
        {
            QByteArray buffer = ReadAll(in);
            QDataStream qtStream(&buffer, QIODevice::ReadOnly);
            qtStream.setByteOrder(isDataBigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);

            QVariant* variant = reinterpret_cast<QVariant*>(classPtr);
            qtStream >> *variant;

            return true;
        }

        bool CompareValueData(const void* left, const void* right) override
        {
            auto leftVariant = reinterpret_cast<const QVariant*>(left);
            auto rightVariant = reinterpret_cast<const QVariant*>(right);

            return *leftVariant == *rightVariant;
        }

    private:
        QByteArray ReadAll(AZ::IO::GenericStream& in)
        {
            const size_t length = in.GetLength();

            QByteArray buffer;
            buffer.reserve(static_cast<int>(length));

            size_t processed = 0;
            size_t read = 0;
            char* data = buffer.data();
            while (processed < length && (read = in.Read(length - processed, data + processed)))
            {
                processed += read;
                if (read == 0)
                {
                    break;
                }
            }

            AZ_Assert(processed == length, "Incorrect amount of data read from stream");

            buffer.resize(static_cast<int>(processed));

            return buffer;
        }
    };
} // namespace

QDataStream& operator<<(QDataStream& out, const Qt::PenStyle& pen)
{
    out << static_cast<int>(pen);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::PenStyle& pen)
{
    int holder;
    in >> holder;
    pen = static_cast<Qt::PenStyle>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const Qt::PenCapStyle& cap)
{
    out << static_cast<int>(cap);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::PenCapStyle& cap)
{
    int holder;
    in >> holder;
    cap = static_cast<Qt::PenCapStyle>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const Qt::AlignmentFlag& alignment)
{
    out << static_cast<int>(alignment);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::AlignmentFlag& alignment)
{
    int holder;
    in >> holder;
    alignment = static_cast<Qt::AlignmentFlag>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const GraphCanvas::Styling::Curves& curve)
{
    out << static_cast<int>(curve);
    return out;
}

QDataStream& operator>>(QDataStream& in, GraphCanvas::Styling::Curves& curve)
{
    int holder;
    in >> holder;
    curve = static_cast<GraphCanvas::Styling::Curves>(holder);

    return in;
}

namespace GraphCanvas
{
    namespace Styling
    {
        //////////
        // Style
        //////////
        void Style::Reflect(AZ::ReflectContext* context)
        {
            static bool reflected = false;

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext || (reflected && !serializeContext->IsRemovingReflection()))
            {
                return;
            }

            qRegisterMetaTypeStreamOperators<Qt::PenStyle>();
            qRegisterMetaTypeStreamOperators<Qt::PenCapStyle>();
            qRegisterMetaTypeStreamOperators<Qt::AlignmentFlag>();
            qRegisterMetaTypeStreamOperators<Styling::Curves>();

            // Allow QVectors to be serialized
            serializeContext->Class<QVariant>()
                ->Serializer<QVariantSerializer>();

            //TODO port collection class away from QHash so it can be serialized
            serializeContext->Class<Style>()
                ->Version(4, &StyleVersionConverter)
                ->Field("Selectors", &Style::m_selectors)
                ->Field("SelectorsAsString", &Style::m_selectorsAsString)
                ->Field("Attributes", &Style::m_values)
                ;
    
            reflected = true;
        }

        Style::Style(const SelectorVector& selectors)
            : m_selectors(selectors)
            , m_selectorsAsString(SelectorsToString(selectors))
        {
        }

        Style::Style(std::initializer_list<Selector> selectors)
            : m_selectors(selectors)
            , m_selectorsAsString(SelectorsToString(m_selectors))
        {
        }

        int Style::Matches(const AZ::EntityId& object) const
        {
            for (const Selector& selector : m_selectors)
            {
                if (selector.Matches(object))
                {
                    return selector.GetComplexity();
                }
            }
            return 0;
        }

        bool Style::HasAttribute(Attribute attribute) const
        {
            return m_values.find(attribute) != m_values.end();
        }

        inline QVariant Style::GetAttribute(Attribute attribute) const
        {
            return HasAttribute(attribute) ? m_values.at(attribute) : QVariant();
        }

        void Style::SetAttribute(Attribute attribute, const QVariant& value)
        {
            m_values[attribute] = value;
        }

        void Style::MakeSelectorsDefault()
        {
            for (auto& selector : m_selectors)
            {
                selector.MakeDefault();
            }
        }

        void Style::Dump() const
        {
            qDebug() << SelectorsToString(m_selectors).c_str();

            for (auto& entry : m_values)
            {
                qDebug() << AttributeName(entry.first) << entry.second;
            }

            qDebug() << "";
        }

        //////////////////
        // ComputedStyle
        //////////////////

        void ComputedStyle::Reflect(AZ::ReflectContext * context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            Style::Reflect(context);

            serializeContext->Class<ComputedStyle>()
                ->Version(2)
                ->Field("ObjectSelectors", &ComputedStyle::m_objectSelectors)
                ->Field("ObjectSelectorsAsString", &ComputedStyle::m_objectSelectorsAsString)
                ->Field("Styles", &ComputedStyle::m_styles)
                ;
        }

        ComputedStyle::ComputedStyle(const SelectorVector& objectSelectors, StyleVector&& styles)
            : m_objectSelectors(objectSelectors)
            , m_objectSelectorsAsString(SelectorsToString(m_objectSelectors))
            , m_styles(std::move(styles))
        {
        }

        void ComputedStyle::SetStyleSheetId(const AZ::EntityId& sheetId)
        {
            StyleSheetNotificationBus::Handler::BusDisconnect();
            StyleSheetNotificationBus::Handler::BusConnect(sheetId);
        }

        void ComputedStyle::Activate()
        {
            StyleRequestBus::Handler::BusConnect(GetEntityId());
        }

        void ComputedStyle::Deactivate()
        {
            StyleRequestBus::Handler::BusDisconnect();
        }

        AZStd::string ComputedStyle::GetDescription() const
        {
            AZStd::string result("Computed:\n");
            result += "\tObject selectors: " + m_objectSelectorsAsString + "\n";
            result += "\tStyles:\n";
            for (const Style* style : m_styles)
            {
                result += "\t\t" + style->GetSelectorsAsString() + "\n";
            }

            result += "\n";
            return result;
        }

        bool ComputedStyle::HasAttribute(AZ::u32 attribute) const
        {
            Attribute typedAttribute = static_cast<Attribute>(attribute);
            return std::any_of(m_styles.cbegin(), m_styles.cend(), [=](const Style* s) {
                return s->HasAttribute(typedAttribute);
            });
        }

        QVariant ComputedStyle::GetAttribute(AZ::u32 attribute) const
        {
            Attribute typedAttribute = static_cast<Attribute>(attribute);
            for (const Style* style : m_styles)
            {
                if (style->HasAttribute(typedAttribute))
                {
                    return style->GetAttribute(typedAttribute);
                }
            }

            return QVariant();
        }

        void ComputedStyle::OnStyleSheetUnloaded()
        {
            m_styles.clear();
        }

#if 0
        void ComputedStyle::Dump() const
        {
            for (const auto& style : m_styles)
            {
                style->Dump();
            }
        }
#endif

        ///////////////
        // StyleSheet
        ///////////////

        void StyleSheet::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            Style::Reflect(context);

            serializeContext->Class<StyleSheet>()
                ->Version(2)
                ->Field("Styles", &StyleSheet::m_styles)
                ;
        }

        StyleSheet::StyleSheet()
        {
        }

        StyleSheet::~StyleSheet()
        {
            ClearStyles();
        }

        void StyleSheet::Activate()
        {
            StyleSheetRequestBus::Handler::BusConnect(GetEntityId());
        }

        void StyleSheet::Deactivate()
        {
            StyleSheetRequestBus::Handler::BusDisconnect();
        }

        void StyleSheet::ClearStyles()
        {
            StyleSheetNotificationBus::Event(GetEntityId(), &StyleSheetNotifications::OnStyleSheetUnloaded);
            for (auto style : m_styles)
            {
                delete style;
            }
            m_styles.clear();
        }
        
        StyleSheet& GraphCanvas::Styling::StyleSheet::operator=(const StyleSheet& other)
        {
            ClearStyles();

            m_styles.reserve(other.m_styles.size());
            for (Style* style : other.m_styles)
            {
                m_styles.push_back(aznew Style(*style));
            }
            StyleSheetNotificationBus::Event(GetEntityId(), &StyleSheetNotifications::OnStyleSheetLoaded);

            return *this;
        }

        StyleSheet& GraphCanvas::Styling::StyleSheet::operator=(StyleSheet&& other)
        {
            ClearStyles();

            m_styles = std::move(other.m_styles);
            return *this;
        }

        void StyleSheet::MakeStylesDefault()
        {
            for (auto& style : m_styles)
            {
                style->MakeSelectorsDefault();
                SelectorVector updated = style->GetSelectors();
            }
        }

        AZ::EntityId StyleSheet::ResolveStyles(const AZ::EntityId& object) const
        {
            SelectorVector selectors;
            StyledEntityRequestBus::EventResult(selectors, object, &StyledEntityRequests::GetStyleSelectors);

            QVector<StyleMatch> matches;
            for (const auto& style : m_styles)
            {
                int complexity = style->Matches(object);
                if (complexity != 0)
                {
                    matches.push_back({ style, complexity });
                }
            }

            std::stable_sort(matches.begin(), matches.end());
            StyleVector result;
            result.reserve(matches.size());
            const auto& constMatches = matches;
            for (auto& match : constMatches)
            {
                result.push_back(match.style);
            }

            auto computed = new ComputedStyle(selectors, std::move(result));

            computed->SetStyleSheetId(GetEntityId());

            AZ::Entity* entity = new AZ::Entity;
            entity->AddComponent(computed);
            entity->Init();
            entity->Activate();

            return entity->GetId();
        }
    }
}
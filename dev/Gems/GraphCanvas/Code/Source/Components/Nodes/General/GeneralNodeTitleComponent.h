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

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/TranslationTypes.h>
#include <Widgets/GraphCanvasLabel.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    class GeneralNodeTitleGraphicsWidget;

    //! The Title component gives a Node the ability to display a title.
    class GeneralNodeTitleComponent
        : public AZ::Component
        , public NodeTitleRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:

        class GeneralNodeTitleComponentSaveData
            : public SceneMemberComponentSaveData<GeneralNodeTitleComponentSaveData>
        {
        public:
            AZ_RTTI(GeneralNodeTitleComponentSaveData, "{328FF15C-C302-458F-A43D-E1794DE0904E}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(GeneralNodeTitleComponentSaveData, AZ::SystemAllocator, 0);

            GeneralNodeTitleComponentSaveData() = default;
            ~GeneralNodeTitleComponentSaveData() = default;

            bool RequiresSave() const override
            {
                return !m_paletteOverride.empty();
            }

            AZStd::string m_paletteOverride;
        };

        AZ_COMPONENT(GeneralNodeTitleComponent, "{67D54B26-A924-4028-8544-5684B16BF04A}");
        static void Reflect(AZ::ReflectContext*);

        GeneralNodeTitleComponent();
        ~GeneralNodeTitleComponent() = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(NodeTitleServiceCrc);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(NodeTitleServiceCrc);
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
            required.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }
        
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeTitleRequestBus
        void SetTitle(const AZStd::string& title) override;
        void SetTranslationKeyedTitle(const TranslationKeyedString& title) override;
        AZStd::string GetTitle() const override;

        void SetSubTitle(const AZStd::string& subtitle) override;
        void SetTranslationKeyedSubTitle(const TranslationKeyedString& subtitle) override;
        AZStd::string GetSubTitle() const override;

        QGraphicsWidget* GetGraphicsWidget() override;

        void SetDefaultPalette(const AZStd::string& palette) override;

        void SetPaletteOverride(const AZStd::string& paletteOverride) override;
        void SetDataPaletteOverride(const AZ::Uuid& uuid) override;

        void ClearPaletteOverride() override;
        /////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& graphId) override;
        ////

    private:
        GeneralNodeTitleComponent(const GeneralNodeTitleComponent&) = delete;

        TranslationKeyedString m_title;
        TranslationKeyedString m_subTitle;

        AZStd::string          m_basePalette;

        GeneralNodeTitleComponentSaveData m_saveData;
        
        GeneralNodeTitleGraphicsWidget* m_generalNodeTitleWidget = nullptr;
    };    

    //! The Title QGraphicsWidget for displaying a title
    class GeneralNodeTitleGraphicsWidget
        : public QGraphicsWidget
        , public SceneNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public NodeNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(GeneralNodeTitleGraphicsWidget, "{9DE7D3C0-D88C-47D8-85D4-5E0F619E60CB}");
        AZ_CLASS_ALLOCATOR(GeneralNodeTitleGraphicsWidget, AZ::SystemAllocator, 0);        

        GeneralNodeTitleGraphicsWidget(const AZ::EntityId& entityId);
        ~GeneralNodeTitleGraphicsWidget() override = default;

        void Activate();
        void Deactivate();
        
        void SetTitle(const TranslationKeyedString& title);
        void SetSubTitle(const TranslationKeyedString& subtitle);

        void SetPaletteOverride(const AZStd::string& paletteOverride);
        void SetPaletteOverride(const AZ::Uuid& uuid);
        void ClearPaletteOverride();

        void UpdateLayout();

        void UpdateStyles();
        void RefreshDisplay();
    
        // SceneNotificationBus
        void OnStylesChanged() override;
        ////

        // SceneMemberNotificationBus
        void OnAddedToScene(const AZ::EntityId& scene) override;
        void OnRemovedFromScene(const AZ::EntityId& scene) override;
        ////

        // NodeNotificationBus
        void OnTooltipChanged(const AZStd::string& tooltip) override;
        ////

    protected:

        // QGraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

        const AZ::EntityId& GetEntityId() const { return m_entityId; }

    private:
        GeneralNodeTitleGraphicsWidget(const GeneralNodeTitleGraphicsWidget&) = delete;

        QGraphicsLinearLayout* m_linearLayout;
        GraphCanvasLabel* m_titleWidget;
        GraphCanvasLabel* m_subTitleWidget;

        AZ::EntityId m_entityId;

        const Styling::StyleHelper* m_paletteOverride;
        Styling::StyleHelper m_styleHelper;
    };
}
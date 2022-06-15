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

#include <EMotionFX/Source/AnimGraph.h>
#include <Integration/Assets/AnimGraphAsset.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>


namespace EMotionFX
{
    class ObjectAffectedByParameterChanges;

    // Picker that allows to select one or more parameters (depending on mask mode) and affect the ports of the node
    // This Picker and its handlers are used by the BlendTreeParameterNode and the AnimGraphReferenceNode.
    //
    class AnimGraphParameterPicker
        : public QWidget
        , private AZ::Data::AssetBus::Handler
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL
        
        AnimGraphParameterPicker(QWidget* parent,  bool singleSelection = false, bool parameterMaskMode = false);
        ~AnimGraphParameterPicker();

        void SetFilterTypes(const AZStd::vector<AZ::TypeId>& filterTypes) { m_filterTypes = filterTypes; }
        void SetAnimGraph(AnimGraph* animGraph);
        void SetAnimGraphAsset(const AZ::Data::Asset<Integration::AnimGraphAsset>& animGraphAsset);
        void SetObjectAffectedByParameterChanges(ObjectAffectedByParameterChanges* affectedObject);

        // Called to initialize the parameter names in the UI from values in the object
        void InitializeParameterNames(const AZStd::vector<AZStd::string>& parameterNames);

        // Called when the UI wants to update the parameter names
        void UpdateParameterNames(const AZStd::vector<AZStd::string>& parameterNames);

        const AZStd::vector<AZStd::string>& GetParameterNames() const;

        void SetSingleParameterName(const AZStd::string& parameterName);
        const AZStd::string GetSingleParameterName() const;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    signals:
        void ParametersChanged(const AZStd::vector<AZStd::string>& newParameters);

    private slots:
        void OnPickClicked();
        void OnResetClicked();
        void OnShrinkClicked();

    private:
        void UpdateInterface();

        AZ::Data::Asset<Integration::AnimGraphAsset> m_animGraphAsset;
        AnimGraph* m_animGraph;
        ObjectAffectedByParameterChanges* m_affectedByParameterChanges;
        AZStd::vector<AZStd::string> m_parameterNames;
        AZStd::vector<AZ::TypeId> m_filterTypes;
        QPushButton* m_pickButton;
        QPushButton* m_resetButton;
        QPushButton* m_shrinkButton;
        bool m_singleSelection;
        bool m_parameterMaskMode;
    };


    class AnimGraphSingleParameterHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphParameterPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphSingleParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphParameterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    class AnimGraphSingleNumberParameterHandler
        : public AnimGraphSingleParameterHandler
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AnimGraphSingleNumberParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
    };

    class AnimGraphSingleVector2ParameterHandler
        : public AnimGraphSingleParameterHandler
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL
        AnimGraphSingleVector2ParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
    };

    class AnimGraphMultipleParameterHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphMultipleParameterHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(AnimGraphParameterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphParameterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraph* m_animGraph;
    };
} // namespace EMotionFX

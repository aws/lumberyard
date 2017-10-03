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

#ifndef __EMSTUDIO_ATTRIBUTEWIDGET_H
#define __EMSTUDIO_ATTRIBUTEWIDGET_H

#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <AzQtComponents/Components/TagSelector.h>

#include <MysticQt/Source/DoubleSpinbox.h>
#include <MysticQt/Source/IntSpinbox.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <MysticQt/Source/LinkWidget.h>
#include <MysticQt/Source/AttributeWidgets.h>
#include "ParameterSelectionWindow.h"
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendSpaceNode.h>
#include <EMotionFX/Rendering/Common/TransformationManipulator.h>
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include <EMotionFX/Rendering/Common/RotateManipulator.h>


namespace EMStudio
{
    // forward declaration
    class AnimGraphPlugin;

    // file browser attribute widget
    class FileBrowserAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        FileBrowserAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        void OnFileBrowseButton();

    private:
        QLineEdit*          mLineEdit;
        AnimGraphPlugin*   mPlugin;
    };


    // vector3 attribute widget
    class Vector3AttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        class Callback
            : public MCommon::ManipulatorCallback
        {
        public:
            Callback(const MCore::Vector3& oldValue, Vector3AttributeWidget* parent = nullptr)
                : MCommon::ManipulatorCallback(nullptr, oldValue)
            {
                mParentWidget = parent;
            }

            void Update(const MCore::Vector3& value) override
            {
                // call the base class update function
                MCommon::ManipulatorCallback::Update(value);

                // update the value of the attribute
                mParentWidget->OnUpdateAttribute(value);
            }

        private:
            Vector3AttributeWidget* mParentWidget;
        };

        Vector3AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~Vector3AttributeWidget();

        void SetValue(MCore::Attribute* attribute);
        void SetReadOnly(bool readOnly);

        QWidget* CreateGizmoWidget() override;

    protected slots:
        void OnDoubleSpinnerX(double value);
        void OnDoubleSpinnerY(double value);
        void OnDoubleSpinnerZ(double value);
        void OnUpdateAttribute(const MCore::Vector3& value);
        void ToggleTranslationGizmo();

    private:
        MCommon::TranslateManipulator*  mTransformationGizmo;
        MysticQt::DoubleSpinBox*        mSpinBoxX;
        MysticQt::DoubleSpinBox*        mSpinBoxY;
        MysticQt::DoubleSpinBox*        mSpinBoxZ;
    };


    // rotation attribute widget
    class RotationAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        class Callback
            : public MCommon::ManipulatorCallback
        {
        public:
            Callback(const MCore::Quaternion& oldRotation, RotationAttributeWidget* parent = nullptr)
                : MCommon::ManipulatorCallback(nullptr, oldRotation)
            {
                mParentWidget = parent;
            }

            void Update(const MCore::Quaternion& value) override
            {
                // call the base class update function
                MCommon::ManipulatorCallback::Update(value);

                // update the value of the attribute
                mParentWidget->OnUpdateAttribute(value);
            }

        private:
            RotationAttributeWidget* mParentWidget;
        };

        RotationAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~RotationAttributeWidget();

        void SetValue(MCore::Attribute* attribute);
        void SetReadOnly(bool readOnly);

        QWidget* CreateGizmoWidget() override;

    protected slots:
        void OnDoubleSpinner(double value);
        void OnOrderComboBox(int index);
        void OnUpdateAttribute(const MCore::Quaternion& value);
        void ToggleRotateGizmo();

    private:
        MCommon::RotateManipulator* mTransformationGizmo;
        MysticQt::DoubleSpinBox*    mSpinX;
        MysticQt::DoubleSpinBox*    mSpinY;
        MysticQt::DoubleSpinBox*    mSpinZ;
        MysticQt::ComboBox*         mOrderComboBox;
    };


    // node names attribute widget
    class NodeNamesAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        NodeNamesAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~NodeNamesAttributeWidget();
        MysticQt::LinkWidget* CreateNodeMaskLink(EMotionFX::AttributeNodeMask* nodeMask);
        uint32 FindMaskEntryIndexByNodeName(EMotionFX::AttributeNodeMask* nodeMask, const char* nodeName);
        //void EnableWidgets(bool enabled);
    protected slots:
        void OnNodeNames();
        void OnNodeSelected(MCore::Array<SelectionItem> selection);
        void OnNodeSelectionCancelled();
        void OnResetSelection();

    private:
        AnimGraphPlugin*                       mPlugin;
        NodeSelectionWindow*                    mNodeSelectionWindow;
        CommandSystem::SelectionList*               mSelectionList;
        MCore::Array<MCore::String>             mNodeSelection;
        MysticQt::LinkWidget*                   mNodeLink;
        QPushButton*                            mResetSelectionButton;
    };


    // node selection attribute widget
    class NodeSelectionAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        NodeSelectionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~NodeSelectionAttributeWidget();
        MysticQt::LinkWidget* CreateNodeNameLink(const char* nodeName);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnNodeNames();
        void OnNodeSelected(MCore::Array<SelectionItem> selection);
        void OnNodeSelectionCancelled();
        void OnResetSelection();

    private:
        AnimGraphPlugin*                       mPlugin;
        NodeSelectionWindow*                    mNodeSelectionWindow;
        CommandSystem::SelectionList*               mSelectionList;
        MCore::String                           mSelectedNodeName;
        MysticQt::LinkWidget*                   mNodeLink;
        QPushButton*                            mResetSelectionButton;
    };


    // combined node and morph target selection
    // attributeArray[ATTRIBUTEARRAY_NODES] shall be an AttributeArray of AttributeString objects representing the node names
    // attributeArray[ATTRIBUTEARRAY_MORPHS] shall be an AttributeArray of AttributeString objects representing the morph target names
    class NodeAndMorphAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        // the index into the AttributeArray
        enum
        {
            ATTRIBUTEARRAY_NODES        = 0,
            ATTRIBUTEARRAY_MORPHS       = 1
        };

        NodeAndMorphAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~NodeAndMorphAttributeWidget();
        MysticQt::LinkWidget* CreateMaskLink(MCore::AttributeArray* mask);
        uint32 FindMaskEntryIndexByNodeName(MCore::AttributeArray* mask, const char* nodeName);
        uint32 FindMaskEntryIndexByMorphName(MCore::AttributeArray* mask, const char* morphName);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnLinkClicked();
        void OnNodeSelected(MCore::Array<SelectionItem> selection);
        void OnNodeSelectionCancelled();
        void OnResetSelection();

    private:
        AnimGraphPlugin*                       mPlugin;
        NodeSelectionWindow*                    mNodeSelectionWindow;
        CommandSystem::SelectionList*               mSelectionList;
        MCore::Array<MCore::String>             mNodeSelection;
        MysticQt::LinkWidget*                   mLink;
        QPushButton*                            mResetSelectionButton;
    };


    // goal node selection attribute widget
    class GoalNodeSelectionAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        GoalNodeSelectionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~GoalNodeSelectionAttributeWidget();
        MysticQt::LinkWidget* CreateNodeNameLink(const char* nodeName);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnNodeNames();
        void OnNodeSelected(MCore::Array<SelectionItem> selection);
        void OnNodeSelectionCancelled();
        void OnResetSelection();

    private:
        AnimGraphPlugin*                       mPlugin;
        NodeSelectionWindow*                    mNodeSelectionWindow;
        CommandSystem::SelectionList*               mSelectionList;
        MCore::String                           mSelectedNodeName;
        uint32                                  mSelectedNodeActorInstanceID;
        MysticQt::LinkWidget*                   mGoalLink;
        QPushButton*                            mResetSelectionButton;
    };


    // motion picker attribute widget
    class MotionPickerAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        MotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnMotionPicker();
        void OnTextChanged(const QString& text);
        void OnTextEdited(const QString& text);

    private:
        AnimGraphPlugin*   mPlugin;
        QLineEdit*          mLineEdit;
    };



    // motion picker attribute widget
    class MultipleMotionPickerAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        MultipleMotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        void OnMotionPicker();
        //void OnTextChanged(const QString& text);
        //void OnTextEdited(const QString& text);

    private:
        AnimGraphPlugin*       mPlugin;
        MysticQt::LinkWidget*   mLink;

        void UpdateLinkWidget();
    };


    //! Container for one blend space motion for the blend space attribute widget.
    class BlendSpaceMotionWidgetContainer
    {
        public:
            BlendSpaceMotionWidgetContainer(EMotionFX::AttributeBlendSpaceMotion* attribute, QGridLayout* layout, int row);

            void UpdateInterface(EMotionFX::BlendSpaceNode* blendSpaceNode, EMotionFX::AnimGraphInstance* animGraphInstance);

            EMotionFX::AttributeBlendSpaceMotion*   m_attribute;
            MysticQt::DoubleSpinBox*                m_spinboxX;
            MysticQt::DoubleSpinBox*                m_spinboxY;
            QPushButton*                            m_restoreButton;
            QPushButton*                            m_removeButton;
    };


    // Blend space motion picker attribute widget.
    class BlendSpaceMotionsAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        BlendSpaceMotionsAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        BlendSpaceMotionWidgetContainer* FindWidgetContainerByMotionId(const AZStd::string& motionId) const;
        BlendSpaceMotionWidgetContainer* FindWidgetContainer(QObject* object);

        void OnAddMotion();
        void OnRemoveMotion();

        void OnRestorePosition();
        void OnPositionXChanged(double value);
        void OnPositionYChanged(double value);

    private:
        void UpdateMotionPosition(QObject* object, float value, bool updateX, bool updateY);
        void PopulateWidget();
        void UpdateInterface();

    private:
        AZStd::vector<BlendSpaceMotionWidgetContainer*> m_motions;
        MCore::AttributeSettings*                       mAttributeSettings;
        AnimGraphPlugin*                                mPlugin;
        QWidget*                                        mContainerWidget;
        QVBoxLayout*                                    mContainerLayout;
        QLabel*                                         m_addMotionsLabel;
        QWidget*                                        mWidget;
    };

    // a blend space motion picker
    class BlendSpaceMotionPickerAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        BlendSpaceMotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        void OnComboBox(int value);

    protected:
        AnimGraphPlugin* mPlugin;
    };

    // node name attribute widget
    class NodeNameAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        NodeNameAttributeWidget(EMotionFX::AnimGraphNode* node, MCore::AttributeSettings* attributeSettings, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnNodeNameChange();
        void OnNodeNameEdited(const QString& text);

    protected:
        bool ValidateNodeName(const char* newName) const;

        AnimGraphPlugin*           mPlugin;
        EMotionFX::AnimGraphNode*  mNode;
    };


    // a parameter picker checkbox
    class ParameterPickerAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        ParameterPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnComboBox(int value);

    protected:
        AnimGraphPlugin*   mPlugin;
    };


    // single anim graph node attribute widget
    class AnimGraphNodeAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        AnimGraphNodeAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        void OnNodePicker();
        void OnTextChanged(const QString& text);
        void OnTextEdited(const QString& text);

    private:
        AnimGraphPlugin*   mPlugin;
        QLineEdit*          mLineEdit;
    };


    // motion blend node attribute widget
    class BlendTreeMotionAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        BlendTreeMotionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        //void EnableWidgets(bool enabled);
    protected slots:
        void OnMotionNodePicker();
        void OnTextChanged(const QString& text);
        void OnTextEdited(const QString& text);
    private:
        AnimGraphPlugin*   mPlugin;
        QLineEdit*          mLineEdit;
    };


    // anim graph state selection attribute widget
    class AnimGraphStateAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        AnimGraphStateAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnStatePicker();
        void OnTextChanged(const QString& text);
        void OnTextEdited(const QString& text);

    private:
        AnimGraphPlugin*   mPlugin;
        QLineEdit*          mLineEdit;
    };


    // motion event track attribute widget
    class MotionEventTrackAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        MotionEventTrackAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        void ReInit();
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnComboBox(int value);

    private:
        AnimGraphPlugin*                   mPlugin;
        MysticQt::ComboBox*                 mComboBox;
        EMotionFX::AnimGraphMotionNode*    mMotionNode;
    };


    /*
    // motion extraction components attribute widget
    class MotionExtractionComponentWidget : public MysticQt::AttributeWidget
    {
        Q_OBJECT
        public:
            MotionExtractionComponentWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode=false);
        protected slots:
            void UpdateMotionExtractionMask();
        private:
            ButtonGroup* mButtonGroup;
            QPushButton* mPosX;
            QPushButton* mPosY;
            QPushButton* mPosZ;
            QPushButton* mRotX;
            QPushButton* mRotY;
            QPushButton* mRotZ;
    };
    */


    // parameter names attribute widget
    class ParameterNamesAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        ParameterNamesAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~ParameterNamesAttributeWidget();

        uint32 FindMaskEntryIndexByParameterName(EMotionFX::AttributeParameterMask* mask, const char* parameterName);
        void UpdateMask(bool modifyMaskAttribute = true);
        //void EnableWidgets(bool enabled);

    protected slots:
        void OnParameterNames();
        void OnParametersSelected(const AZStd::vector<AZStd::string>& selectedItems);
        void OnSelectionCancelled();
        void OnResetSelection();
        void OnShinkButton();
    private:
        AnimGraphPlugin*                mPlugin;
        ParameterSelectionWindow*       mParameterSelectionWindow;
        QLabel*                         mParameterLink;
        QPushButton*                    mResetSelectionButton;
        QPushButton*                    mShrinkButton;

        AZStd::vector<AZStd::string>    mSelectedParameters;
    };


    // forward declaration
    class StateFilterSelectionWindow;

    // state filter local attribute widget
    class StateFilterLocalAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        StateFilterLocalAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~StateFilterLocalAttributeWidget();
        void UpdateLink();

    protected slots:
        void OnSelectNodeGroups();
        void OnResetGroupSelection();
    private:
        AnimGraphPlugin*                       mPlugin;
        StateFilterSelectionWindow*             mSelectionWindow;
        MCore::Array<MCore::String>             mSelectedGroupNames;
        MCore::Array<MCore::String>             mSelectedNodeNames;
        MysticQt::LinkWidget*                   mNodeLink;
        QPushButton*                            mResetSelectionButton;
    };


    // button attribute widget
    class ButtonAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        ButtonAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);
        ~ButtonAttributeWidget();
        QPushButton* GetButton() const                          { return mButton; }
    protected slots:
    private:
        QPushButton*        mButton;
        AnimGraphPlugin*   mPlugin;
    };


    // Tag picker attribute widget.
    class TagPickerAttributeWidget
        : public MysticQt::AttributeWidget
    {
        Q_OBJECT
    public:
        TagPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func);

    protected slots:
        void OnTagsChanged();

    private:
        void GetAvailableTags(QVector<QString>& outTags) const;
        void GetSelectedTags(QVector<QString>& outTags) const;

        AnimGraphPlugin*                mPlugin;
        AzQtComponents::TagSelector*    m_tagSelector;
    };
} // namespace EMStudio

#endif
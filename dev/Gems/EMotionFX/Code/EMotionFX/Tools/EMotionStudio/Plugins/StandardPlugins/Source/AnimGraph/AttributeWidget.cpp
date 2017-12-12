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

#include "AttributeWidget.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include "AttributesWindow.h"
#include "AnimGraphPlugin.h"
#include "BlendResourceWidget.h"
#include "BlendGraphWidget.h"
#include "BlendNodeSelectionWindow.h"
#include "StateFilterSelectionWindow.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QMessageBox>
#include <MysticQt/Source/IntSpinbox.h>
#include <MysticQt/Source/DoubleSpinbox.h>
#include "../../../../EMStudioSDK/Source/MotionSetSelectionWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <MCore/Source/Vector.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/ActorManager.h>


#define SPINNER_WIDTH 58

namespace EMStudio
{
    // Find the currently active anim graph plugin.
    AnimGraphPlugin* AttributeWidgetFindAnimGraphPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            MCORE_ASSERT(false);
            return nullptr;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        return animGraphPlugin;
    }


    // Get the active anim graph.
    EMotionFX::AnimGraph* GetActiveAnimGraph()
    {
        AnimGraphPlugin* plugin = AttributeWidgetFindAnimGraphPlugin();
        return plugin->GetActiveAnimGraph();
    }


    // Get the currently active anim graph instance in case only exactly one actor instance is selected.
    EMotionFX::AnimGraphInstance* GetSingleSelectedAnimGraphInstance()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (!actorInstance)
        {
            return nullptr;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
 
        AZ_Assert(!animGraphInstance || (animGraphInstance->GetAnimGraph() == GetActiveAnimGraph()), "The currently activated anim graph in the plugin differs from the one the current actor instance uses.");

        return animGraphInstance;
    }


    /*
    // constructor
    AttributeWidget::AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode) : QWidget()
    {
        mReadOnly       = readOnly;
        mAttributeInfo  = attributeInfo;
        mAttributes     = attributes;
        mFirstAttribute = nullptr;
        mPlugin         = plugin;
        mCustomData     = customData;

        mAttributes.SetMemoryCategory( MEMCATEGORY_ANIMGRAPHPLUGIN_PLUGIN );
        if (attributes.GetLength() > 0)
            mFirstAttribute = attributes[0];

    //  if (attributeInfo)
    //      mNameString.Format("%s:", attributeInfo->mName.AsChar());
    }


    // destructor
    AttributeWidget::~AttributeWidget()
    {
    }


    // trigger a graph redraw
    void AttributeWidget::UpdateGraph()
    {
        mPlugin->GetGraphWidget()->update();
    }


    // update the interfaces
    void AttributeWidget::UpdateInterface(bool sync)
    {
        // get the object of which we just modified an attribute
        EMotionFX::AnimGraphObject* object = mPlugin->GetAttributesWindow()->GetObject();
        if (object)
        {
            if (sync)
            {
                switch (object->GetBaseType())
                {
                    // in case of a node
                    case EMotionFX::AnimGraphNode::BASETYPE_ID:
                    {
                        EMotionFX::AnimGraphNode* animGraphNode = static_cast<EMotionFX::AnimGraphNode*>(object);

                        // update all anim graph instances
                        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
                        for (uint32 i=0; i<numActorInstances; ++i)
                        {
                            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance( i )->GetAnimGraphInstance();
                            if (animGraphInstance == nullptr)
                                continue;

                            animGraphNode->Prepare(animGraphInstance);
                        }

                        mPlugin->SyncVisualNode( static_cast<EMotionFX::AnimGraphNode*>(object) );
                    }
                    break;

                    // in case of a state transition
                    case EMotionFX::AnimGraphStateTransition::BASETYPE_ID:
                        mPlugin->SyncTransition( static_cast<EMotionFX::AnimGraphStateTransition*>(object) );
                    break;

                    // in any other case
                    default:
                        object->OnUpdateAttributes();
                };

                // notify for all nodes, that an attribute has changed
                mPlugin->OnUpdateUniqueData();

            // redraw the graph
            //mPlugin->GetGraphWidget()->update();
            }
            else
                object->OnUpdateAttributes();
        }
    }


    void AttributeWidget::CreateStandardLayout(QWidget* widget, MCore::AttributeSettings* attributeSettings)
    {
        assert(mAttributeInfo);

        //QLabel*           label       = new QLabel( mNameString.AsChar() );
        QHBoxLayout*        layout      = new QHBoxLayout();

        //layout->addWidget(label);
        layout->addWidget(widget);
        layout->setAlignment(Qt::AlignLeft);

        setLayout(layout);

        //label->setMinimumWidth(ATTRIBUTEWIDGET_LABEL_WIDTH);
        //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        //layout->setAlignment(Qt::AlignTop);

        layout->setMargin(0);
        //label->setToolTip( mAttributeInfo->mDescription.AsChar() );
        widget->setToolTip( mAttributeInfo->mDescription.AsChar() );
    }


    void AttributeWidget::CreateStandardLayout(QWidget* widgetA, QWidget* widgetB, MCore::AttributeSettings* attributeSettings)
    {
        assert(mAttributeInfo);

        QWidget*        rightWidget = new QWidget();
        QHBoxLayout*    rightLayout = new QHBoxLayout();

        //rightLayout->setAlignment(Qt::AlignTop);

        rightLayout->addWidget(widgetA);
        rightLayout->addWidget(widgetB);
        rightLayout->setMargin(0);
        rightLayout->setAlignment(Qt::AlignLeft);

        rightWidget->setToolTip( mAttributeInfo->mDescription.AsChar() );
        rightWidget->setLayout(rightLayout);
        rightWidget->setObjectName("TransparentWidget");

        CreateStandardLayout(rightWidget, attributeInfo);
    }*/

    //-----------------------------------------------------------------------------------------------------------------

    FileBrowserAttributeWidget::FileBrowserAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mLineEdit           = new QLineEdit();
        QPushButton* button = new QPushButton("...");
        CreateStandardLayout(mLineEdit, button, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        mLineEdit->setText(value);
        mLineEdit->setReadOnly(readOnly);

        connect(button, SIGNAL(clicked(bool)), this, SLOT(OnFileBrowseButton()));
    }


    // file browse button pressed
    void FileBrowserAttributeWidget::OnFileBrowseButton()
    {
        // choose the filename
        GetMainWindow()->StopRendering();
        const QString fileName = QFileDialog::getOpenFileName(this, tr("Select a file"), QString(), tr("EMotion FX Motion Files (*.motion);;EMotion FX Skeletal Motion Files (*.motion);;All Files (*)"));
        GetMainWindow()->StartRendering();

        // stop here if the user cancel the dialog
        if (fileName.length() == 0)
        {
            UpdateInterface();
            return;
        }

        mLineEdit->setText(fileName);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(fileName));
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    Vector3AttributeWidget::Vector3AttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mTransformationGizmo        = nullptr;
        QWidget*        widget      = new QWidget();
        QHBoxLayout*    hLayout     = new QHBoxLayout();
        mSpinBoxX                   = new MysticQt::DoubleSpinBox();
        mSpinBoxY                   = new MysticQt::DoubleSpinBox();
        mSpinBoxZ                   = new MysticQt::DoubleSpinBox();

        mSpinBoxX->setMinimumWidth(SPINNER_WIDTH);
        mSpinBoxX->setMaximumWidth(SPINNER_WIDTH);

        mSpinBoxY->setMinimumWidth(SPINNER_WIDTH);
        mSpinBoxY->setMaximumWidth(SPINNER_WIDTH);

        mSpinBoxZ->setMinimumWidth(SPINNER_WIDTH);
        mSpinBoxZ->setMaximumWidth(SPINNER_WIDTH);

        hLayout->addWidget(mSpinBoxX);
        hLayout->addWidget(mSpinBoxY);
        hLayout->addWidget(mSpinBoxZ);
        hLayout->setAlignment(Qt::AlignLeft);
        hLayout->setMargin(0);
        hLayout->setSpacing(1);
        CreateStandardLayout(widget, attributeSettings);
        widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        widget->setLayout(hLayout);

        MCore::Vector3  value       = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);
        MCore::Vector3  minValue    = static_cast<MCore::AttributeVector3*>(attributeSettings->GetMinValue())->GetValue();
        MCore::Vector3  maxValue    = static_cast<MCore::AttributeVector3*>(attributeSettings->GetMaxValue())->GetValue();
        float           stepSize    = 0.1f;

        mSpinBoxX->setRange(minValue.x, maxValue.x);
        mSpinBoxX->setValue(value.x);
        mSpinBoxX->setSingleStep(stepSize);
        //  mSpinBoxX->setPrefix("x: ");
        mSpinBoxX->setEnabled(!readOnly);
        mSpinBoxX->setDecimals(4);

        mSpinBoxY->setRange(minValue.y, maxValue.y);
        mSpinBoxY->setValue(value.y);
        mSpinBoxY->setSingleStep(stepSize);
        //  mSpinBoxY->setPrefix("y: ");
        mSpinBoxY->setEnabled(!readOnly);
        mSpinBoxY->setDecimals(4);

        mSpinBoxZ->setRange(minValue.z, maxValue.z);
        mSpinBoxZ->setValue(value.z);
        mSpinBoxZ->setSingleStep(stepSize);
        //  mSpinBoxZ->setPrefix("z: ");
        mSpinBoxZ->setEnabled(!readOnly);
        mSpinBoxZ->setDecimals(4);

        widget->setTabOrder(mSpinBoxX->GetLineEdit(), mSpinBoxY->GetLineEdit());
        widget->setTabOrder(mSpinBoxY->GetLineEdit(), mSpinBoxZ->GetLineEdit());

        connect(mSpinBoxX, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerX(double)));
        connect(mSpinBoxY, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerY(double)));
        connect(mSpinBoxZ, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinnerZ(double)));
    }


    Vector3AttributeWidget::~Vector3AttributeWidget()
    {
        // remove translation gizmo if it has been created
        GetManager()->RemoveTransformationManipulator(mTransformationGizmo);
        delete mTransformationGizmo;
    }


    void Vector3AttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        const MCore::Vector3 value = (attribute) ? static_cast<MCore::AttributeVector3*>(attribute)->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);

        if (MCore::Compare<float>::CheckIfIsClose(value.x, mSpinBoxX->value(), 0.000001f) == false)
        {
            mSpinBoxX->setValue(value.x);
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.y, mSpinBoxY->value(), 0.000001f) == false)
        {
            mSpinBoxY->setValue(value.y);
        }

        if (MCore::Compare<float>::CheckIfIsClose(value.z, mSpinBoxZ->value(), 0.000001f) == false)
        {
            mSpinBoxZ->setValue(value.z);
        }

        if (mTransformationGizmo)
        {
            mTransformationGizmo->SetRenderOffset(value);
        }
    }


    void Vector3AttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinBoxX->setDisabled(readOnly);
        mSpinBoxY->setDisabled(readOnly);
        mSpinBoxZ->setDisabled(readOnly);
    }


    QWidget* Vector3AttributeWidget::CreateGizmoWidget()
    {
        QPushButton* gizmoButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        connect(gizmoButton, SIGNAL(clicked()), this, SLOT(ToggleTranslationGizmo()));
        gizmoButton->setCheckable(true);
        return gizmoButton;
    }


    void Vector3AttributeWidget::OnDoubleSpinnerX(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.x = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // update the spinbox text
        mSpinBoxX->blockSignals(true);
        mSpinBoxX->setValue(value);
        mSpinBoxX->blockSignals(false);

        // update the gizmo position
        if (mTransformationGizmo)
        {
            OnUpdateAttribute(curValue);
        }

        // update the interface
        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector3AttributeWidget::OnDoubleSpinnerY(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.y = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // update the spinbox text
        mSpinBoxY->blockSignals(true);
        mSpinBoxY->setValue(value);
        mSpinBoxY->blockSignals(false);

        // update the gizmo position
        if (mTransformationGizmo)
        {
            OnUpdateAttribute(curValue);
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void Vector3AttributeWidget::OnDoubleSpinnerZ(double value)
    {
        MCore::Vector3 curValue = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(mSpinBoxX->value(), mSpinBoxY->value(), mSpinBoxZ->value());
        curValue.z = value;

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(curValue);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // update the spinbox text
        mSpinBoxZ->blockSignals(true);
        mSpinBoxZ->setValue(value);
        mSpinBoxZ->blockSignals(false);

        // update the gizmo position
        if (mTransformationGizmo)
        {
            OnUpdateAttribute(curValue);
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // function to update the attribute
    void Vector3AttributeWidget::OnUpdateAttribute(const MCore::Vector3& value)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeVector3*>(mAttributes[i])->SetValue(value);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // update the spinbox text
        mSpinBoxX->blockSignals(true);
        mSpinBoxY->blockSignals(true);
        mSpinBoxZ->blockSignals(true);
        mSpinBoxX->setValue(value.x);
        mSpinBoxY->setValue(value.y);
        mSpinBoxZ->setValue(value.z);
        mSpinBoxY->blockSignals(false);
        mSpinBoxX->blockSignals(false);
        mSpinBoxZ->blockSignals(false);

        // update the gizmo if it exists
        if (mTransformationGizmo)
        {
            mTransformationGizmo->Init(value);
        }

        FireValueChangedSignal();
        //UpdateInterface();
    }


    // enables or disables the gizmo for setting the value
    void Vector3AttributeWidget::ToggleTranslationGizmo()
    {
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (button->isChecked())
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Vector3Gizmo.png", "Show/Hide translation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        }

        if (mTransformationGizmo == nullptr)
        {
            mTransformationGizmo = (MCommon::TranslateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::TranslateManipulator(70.0f, true));
            const MCore::Vector3 pos = (mFirstAttribute) ? static_cast<MCore::AttributeVector3*>(mFirstAttribute)->GetValue() : MCore::Vector3(0.0f, 0.0f, 0.0f);
            mTransformationGizmo->Init(pos);
            mTransformationGizmo->SetCallback(new Callback(pos, this));
            mTransformationGizmo->SetName(mAttributeSettings->GetName());
        }
        else
        {
            GetManager()->RemoveTransformationManipulator(mTransformationGizmo);
            delete mTransformationGizmo;
            mTransformationGizmo = nullptr;
        }
    }

    //-----------------------------------------------------------------------------------------------------------------

    RotationAttributeWidget::RotationAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mTransformationGizmo        = nullptr;
        QWidget*        widget      = new QWidget();
        QHBoxLayout*    hLayout     = new QHBoxLayout();

        mSpinX  = new MysticQt::DoubleSpinBox();
        mSpinY  = new MysticQt::DoubleSpinBox();
        mSpinZ  = new MysticQt::DoubleSpinBox();

        mSpinX->setMinimumWidth(SPINNER_WIDTH);
        mSpinX->setMaximumWidth(SPINNER_WIDTH);

        mSpinY->setMinimumWidth(SPINNER_WIDTH);
        mSpinY->setMaximumWidth(SPINNER_WIDTH);

        mSpinZ->setMinimumWidth(SPINNER_WIDTH);
        mSpinZ->setMaximumWidth(SPINNER_WIDTH);

        mOrderComboBox = new MysticQt::ComboBox();
        mOrderComboBox->setMaximumWidth(50);
        mOrderComboBox->setEditable(false);
        mOrderComboBox->setEnabled(!readOnly);

        mOrderComboBox->addItem("ZYX");
        mOrderComboBox->addItem("ZXY");
        mOrderComboBox->addItem("YZX");
        mOrderComboBox->addItem("YXZ");
        mOrderComboBox->addItem("XYZ");
        mOrderComboBox->addItem("XZY");

        hLayout->addWidget(mSpinX);
        hLayout->addWidget(mSpinY);
        hLayout->addWidget(mSpinZ);
        hLayout->addWidget(mOrderComboBox);
        hLayout->setMargin(0);
        hLayout->setSpacing(1);
        hLayout->setAlignment(Qt::AlignLeft);
        widget->setLayout(hLayout);
        CreateStandardLayout(widget, attributeSettings);

        EMotionFX::AttributeRotation* value = (mFirstAttribute) ? static_cast<EMotionFX::AttributeRotation*>(mFirstAttribute) : nullptr;

        const MCore::Vector3&   angles      = (value) ? value->GetRotationAngles() : MCore::Vector3(0.0f, 0.0f, 0.0f);
        const float             stepSize    = 0.25f;
        const int               order       = (value) ? (int)value->GetRotationOrder() : 0;

        mSpinX->setRange(-360.0f, 360.0f);
        mSpinX->setValue(angles.x);
        mSpinX->setSingleStep(stepSize);
        mSpinX->setEnabled(!readOnly);
        mSpinX->setDecimals(2);

        mSpinY->setRange(-360.0f, 360.0f);
        mSpinY->setValue(angles.y);
        mSpinY->setSingleStep(stepSize);
        mSpinY->setEnabled(!readOnly);
        mSpinY->setDecimals(2);

        mSpinZ->setRange(-360.0f, 360.0f);
        mSpinZ->setValue(angles.z);
        mSpinZ->setSingleStep(stepSize);
        mSpinZ->setEnabled(!readOnly);
        mSpinZ->setDecimals(2);

        mOrderComboBox->setCurrentIndex(order);

        widget->setTabOrder(mSpinX->GetLineEdit(), mSpinY->GetLineEdit());
        widget->setTabOrder(mSpinY->GetLineEdit(), mSpinZ->GetLineEdit());
        widget->setTabOrder(mSpinZ->GetLineEdit(), mOrderComboBox);

        connect(mSpinX, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinner(double)));
        connect(mSpinY, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinner(double)));
        connect(mSpinZ, SIGNAL(valueChanged(double)), this, SLOT(OnDoubleSpinner(double)));
        connect(mOrderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnOrderComboBox(int)));
    }


    RotationAttributeWidget::~RotationAttributeWidget()
    {
        // remove translation gizmo if it has been created
        GetManager()->RemoveTransformationManipulator(mTransformationGizmo);
        delete mTransformationGizmo;
    }


    void RotationAttributeWidget::OnOrderComboBox(int index)
    {
        MCORE_ASSERT(index >= 0 && index <= 5);
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::AttributeRotation* rotation = static_cast<EMotionFX::AttributeRotation*>(mAttributes[i]);
            rotation->SetRotationOrder((EMotionFX::AttributeRotation::ERotationOrder)index);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }
    }


    void RotationAttributeWidget::OnDoubleSpinner(double value)
    {
        MCORE_UNUSED(value);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::AttributeRotation* rotation = static_cast<EMotionFX::AttributeRotation*>(mAttributes[i]);
            rotation->SetRotationAngles(MCore::Vector3(mSpinX->value(), mSpinY->value(), mSpinZ->value()));
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    void RotationAttributeWidget::SetValue(MCore::Attribute* attribute)
    {
        const MCore::Vector3 rotation = (attribute) ? static_cast<EMotionFX::AttributeRotation*>(attribute)->GetRotationAngles() : MCore::Vector3(0.0f, 0.0f, 0.0f);

        if (MCore::Compare<float>::CheckIfIsClose(rotation.x, mSpinX->value(), 0.000001f) == false)
        {
            mSpinX->setValue(rotation.x);
        }

        if (MCore::Compare<float>::CheckIfIsClose(rotation.y, mSpinY->value(), 0.000001f) == false)
        {
            mSpinY->setValue(rotation.y);
        }

        if (MCore::Compare<float>::CheckIfIsClose(rotation.z, mSpinZ->value(), 0.000001f) == false)
        {
            mSpinZ->setValue(rotation.z);
        }
    }


    void RotationAttributeWidget::SetReadOnly(bool readOnly)
    {
        mSpinX->setDisabled(readOnly);
        mSpinY->setDisabled(readOnly);
        mSpinZ->setDisabled(readOnly);
        mOrderComboBox->setDisabled(readOnly);
    }


    QWidget* RotationAttributeWidget::CreateGizmoWidget()
    {
        QPushButton* gizmoButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide rotation gizmo for visual manipulation");
        connect(gizmoButton, SIGNAL(clicked()), this, SLOT(ToggleRotateGizmo()));
        gizmoButton->setCheckable(true);
        return gizmoButton;
    }


    void RotationAttributeWidget::OnUpdateAttribute(const MCore::Quaternion& value)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::AttributeRotation* rotAttrib = static_cast<EMotionFX::AttributeRotation*>(mAttributes[i]);
            rotAttrib->SetDirect(value * rotAttrib->GetRotationQuaternion(), true);
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // convert to degrees
        EMotionFX::AttributeRotation* firstRotAttrib = static_cast<EMotionFX::AttributeRotation*>(mFirstAttribute);
        MCore::Vector3 degrees;
        if (mFirstAttribute)
        {
            MCore::Quaternion newValue = value * firstRotAttrib->GetRotationQuaternion();
            MCore::Vector3 euler = newValue.ToEuler();
            degrees = MCore::Vector3(MCore::Math::RadiansToDegrees(euler.x), MCore::Math::RadiansToDegrees(euler.y), MCore::Math::RadiansToDegrees(euler.z));
        }
        else
        {
            degrees.Zero();
        }

        // update the spinbox text
        mSpinX->blockSignals(true);
        mSpinY->blockSignals(true);
        mSpinZ->blockSignals(true);
        mSpinX->setValue(degrees.x);
        mSpinY->setValue(degrees.y);
        mSpinZ->setValue(degrees.z);
        mSpinY->blockSignals(false);
        mSpinX->blockSignals(false);
        mSpinZ->blockSignals(false);

        // fire the event
        FireValueChangedSignal();
    }


    void RotationAttributeWidget::ToggleRotateGizmo()
    {
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (button->isChecked())
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Vector3Gizmo.png", "Show/Hide rotation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide rotation gizmo for visual manipulation");
        }

        if (mTransformationGizmo == nullptr)
        {
            mTransformationGizmo = (MCommon::RotateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::RotateManipulator(70.0f, true));
            const MCore::Quaternion rot = (mFirstAttribute) ? static_cast<EMotionFX::AttributeRotation*>(mFirstAttribute)->GetRotationQuaternion() : MCore::Quaternion();
            mTransformationGizmo->Init(MCore::Vector3(0.0f, 0.0f, 0.0f));
            mTransformationGizmo->SetCallback(new Callback(rot, this));
            mTransformationGizmo->SetName(mAttributeSettings->GetName());
        }
        else
        {
            GetManager()->RemoveTransformationManipulator(mTransformationGizmo);
            delete mTransformationGizmo;
            mTransformationGizmo = nullptr;
        }
    }

    //-----------------------------------------------------------------------------------------------------------------

    NodeNamesAttributeWidget::NodeNamesAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mNodeSelectionWindow    = new NodeSelectionWindow(this, false);
        mSelectionList          = new CommandSystem::SelectionList();

        connect(mNodeSelectionWindow,                           SIGNAL(rejected()),                                    this, SLOT(OnNodeSelectionCancelled()));
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),  this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)));

        if (mFirstAttribute)
        {
            EMotionFX::AttributeNodeMask* nodeMask = static_cast<EMotionFX::AttributeNodeMask*>(mFirstAttribute);
            mNodeLink = CreateNodeMaskLink(nodeMask);
            if (mNodeLink->toolTip().length() == 0)
            {
                mNodeLink->setToolTip(attributeSettings->GetDescription());
            }

            mNodeLink->setEnabled(!readOnly);

            connect(mNodeLink, SIGNAL(clicked()), this, SLOT(OnNodeNames()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);
            mNodeLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mNodeLink, 0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);

            CreateStandardLayout(helperWidget, attributeSettings);
        }
    }


    // create a node mask link
    MysticQt::LinkWidget* NodeNamesAttributeWidget::CreateNodeMaskLink(EMotionFX::AttributeNodeMask* nodeMask)
    {
        MysticQt::LinkWidget* nodeLink = nullptr;

        // if there is no mask setup yet
        if (nodeMask == nullptr)
        {
            nodeLink = new MysticQt::LinkWidget("select nodes");
        }
        else
        {
            // if no nodes are setup in the mask
            if (nodeMask->GetNumNodes() == 0)
            {
                nodeLink = new MysticQt::LinkWidget("select nodes");
            }
            else
            {
                // show how many nodes there are and setup the tooltip
                MCore::String labelText;
                labelText.Reserve(1024 * 100);
                labelText.Format("%d nodes inside mask", nodeMask->GetNumNodes());
                nodeLink = new MysticQt::LinkWidget(labelText.AsChar());

                labelText.Clear();
                const uint32 numNodes = nodeMask->GetNumNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    if (i < numNodes - 1)
                    {
                        labelText += nodeMask->GetNode(i).mName;
                        labelText += "\n";
                    }
                    else
                    {
                        labelText += nodeMask->GetNode(i).mName;
                    }
                }

                // update the tooltip text
                nodeLink->setToolTip(labelText.AsChar());
            }
        }

        // setup the size policy
        nodeLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return nodeLink;
    }


    NodeNamesAttributeWidget::~NodeNamesAttributeWidget()
    {
        delete mSelectionList;
    }



    void NodeNamesAttributeWidget::OnResetSelection()
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::AttributeNodeMask* nodeMask = static_cast<EMotionFX::AttributeNodeMask*>(mAttributes[i]);
            nodeMask->InitFromString("");
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        mNodeLink->setText("select nodes");
        FireValueChangedSignal();
        UpdateInterface();
    }



    uint32 NodeNamesAttributeWidget::FindMaskEntryIndexByNodeName(EMotionFX::AttributeNodeMask* nodeMask, const char* nodeName)
    {
        if (nodeMask == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        const uint32 numMaskNodes = nodeMask->GetNumNodes();
        for (uint32 i = 0; i < numMaskNodes; ++i)
        {
            if (nodeMask->GetNode(i).mName.CheckIfIsEqual(nodeName))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // when we want to select a bunch of nodes
    void NodeNamesAttributeWidget::OnNodeNames()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            QMessageBox::warning(this, "Selection Invalid", "Cannot open node selection window. Please make sure exactly one actor instance is selected.");
            return;
        }

        // get the old and current node mask and set the selection of the selection window to it
        EMotionFX::AttributeNodeMask*   oldNodeMask     = static_cast<EMotionFX::AttributeNodeMask*>(mFirstAttribute);
        EMotionFX::Actor*               actor           = actorInstance->GetActor();

        mSelectionList->Clear();
        mSelectionList->AddActorInstance(actorInstance);

        if (oldNodeMask)
        {
            // iterate over the node mask and add all nodes to the selection list
            const uint32 numOldMaskNodes = oldNodeMask->GetNumNodes();
            for (uint32 j = 0; j < numOldMaskNodes; ++j)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(oldNodeMask->GetNode(j).mName.AsChar());
                if (node)
                {
                    mSelectionList->AddNode(node);
                }
            }
        }

        // show the node selection window
        mNodeSelectionWindow->Update(actorInstance->GetID(), mSelectionList);
        mNodeSelectionWindow->setModal(true);
        if (mNodeSelectionWindow->exec() != QDialog::Rejected)  // we pressed cancel or the close cross
        {
            uint32 i;
            uint32 numSelectedNodes = mNodeSelection.GetLength();

            /*
                    // store the mask names in the node
                    EMotionFX::AnimGraphNodeMask* nodeMask;
                    if (mFirstAttribute->GetDataPointer() == nullptr)
                    {
                        nodeMask = new EMotionFX::AnimGraphNodeMask();
                        mFirstAttribute->SetDataObjectPointer( nodeMask );
                        mFirstAttribute->SetAutoFreeMem( false );
                        mFirstAttribute->SetDataTypeID( EMotionFX::ANIMGRAPH_ATTRIB_NODEMASK );
                    }
                    else
            */
            EMotionFX::AttributeNodeMask* nodeMask = static_cast<EMotionFX::AttributeNodeMask*>(mFirstAttribute);

            // update the node mask

            // remove the ones that got unselected
            for (i = 0; i < nodeMask->GetNumNodes(); )
            {
                const MCore::String& nodeName = nodeMask->GetNode(i).mName;

                // skip node names that are not inside the current actor
                if (actor->GetSkeleton()->FindNodeByName(nodeName.AsChar()) == nullptr)
                {
                    i++;
                    continue;
                }

                // check if the given node name of the mask is selected
                bool isSelected = false;
                if (mNodeSelection.Find(nodeName) != MCORE_INVALIDINDEX32)
                {
                    isSelected = true;
                }

                if (isSelected)
                {
                    i++;
                    continue;
                }

                // if the node is not selected remove it
                const uint32 maskEntryIndex = FindMaskEntryIndexByNodeName(nodeMask, nodeName.AsChar());
                if  (maskEntryIndex != MCORE_INVALIDINDEX32)
                {
                    nodeMask->GetNodes().Remove(maskEntryIndex);
                }
                else
                {
                    i++;
                }
            }

            // add the newly selected nodes
            for (i = 0; i < numSelectedNodes; ++i)
            {
                const uint32 maskEntryIndex = FindMaskEntryIndexByNodeName(nodeMask, mNodeSelection[i].AsChar());
                if (maskEntryIndex == MCORE_INVALIDINDEX32)
                {
                    nodeMask->GetNodes().AddEmpty();
                    nodeMask->GetNodes().GetLast().mName = mNodeSelection[i];
                    nodeMask->GetNodes().GetLast().mWeight = 0.0f;
                }
            }

            const uint32 numMaskNodes = nodeMask->GetNumNodes();

            // update the link name
            MCore::String htmlLink;
            if (numMaskNodes > 0)
            {
                htmlLink.Format("%d nodes inside mask", numMaskNodes);
            }
            else
            {
                htmlLink.Format("select nodes");
            }

            mNodeLink->setText(htmlLink.AsChar());

            // build the tooltip text
            htmlLink.Clear();
            htmlLink.Reserve(16384);
            for (i = 0; i < numMaskNodes; ++i)
            {
                if (i < numMaskNodes - 1)
                {
                    htmlLink += nodeMask->GetNode(i).mName;
                    htmlLink += "\n";
                }
                else
                {
                    htmlLink += nodeMask->GetNode(i).mName;
                }
            }

            // update the tooltip text
            mNodeLink->setToolTip(htmlLink.AsChar());

            const uint32 numAttributes = mAttributes.GetLength();
            for (i = 0; i < numAttributes; ++i)
            {
                if (mAttribChangedFunc)
                {
                    mAttribChangedFunc(mAttributes[i], mAttributeSettings);
                }
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // when the selection succeeded
    void NodeNamesAttributeWidget::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        mNodeSelection.Clear();

        // check if selection is valid
        if (selection.GetLength() == 0)
        {
            return;
        }

        const uint32 actorInstanceID = selection[0].mActorInstanceID;
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        const uint32 numSelected = selection.GetLength();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            if (selection[i].GetNodeNameString().GetIsEmpty() == false)
            {
                mNodeSelection.Add(selection[i].GetNodeName());
            }
        }
    }


    // when the node selection got cancelled
    void NodeNamesAttributeWidget::OnNodeSelectionCancelled()
    {
        mNodeSelection.Clear();
    }

    //-----------------------------------------------------------------------------------------------------------------

    MotionPickerAttributeWidget::MotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mLineEdit           = new QLineEdit();
        QPushButton* button = new QPushButton("...");
        button->setMaximumHeight(18);
        CreateStandardLayout(mLineEdit, button, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        mLineEdit->setText(value);
        mLineEdit->setReadOnly(readOnly);

        connect(mLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnTextChanged(const QString&)));
        connect(mLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
        connect(button, SIGNAL(clicked(bool)), this, SLOT(OnMotionPicker()));
    }


    // called when manually entering a motion id
    void MotionPickerAttributeWidget::OnTextChanged(const QString& text)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(text).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // called when manually changing a motion id, this is just a verification for the motion id
    void MotionPickerAttributeWidget::OnTextEdited(const QString& text)
    {
        // get the current motion set
        EMotionFX::MotionSet* currentMotionSet;
        EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (animGraphPlugin)
        {
            AnimGraphPlugin* pluginCast = static_cast<AnimGraphPlugin*>(animGraphPlugin);
            currentMotionSet = pluginCast->GetResourceWidget()->GetSelectedMotionSet();
        }
        else
        {
            currentMotionSet = nullptr;
        }

        // Set the line edit invalid if no current motion set.
        if (!currentMotionSet)
        {
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
            return;
        }

        const AZStd::string motionId = text.toUtf8().data();

        // Is the given motion id present in the motion set?
        EMotionFX::MotionSet::MotionEntry* motionEntry = currentMotionSet->FindMotionEntryByStringID(motionId.c_str());
        if (motionEntry)
        {
            mLineEdit->setStyleSheet("");
            return;
        }

        // Is the given motion id present in any of the parent motion sets?
        EMotionFX::MotionSet* parentMotionSet = currentMotionSet->GetParentSet();
        while (parentMotionSet)
        {
            EMotionFX::MotionSet::MotionEntry* motionEntry = parentMotionSet->FindMotionEntryByStringID(motionId.c_str());
            if (motionEntry)
            {
                mLineEdit->setStyleSheet("");
                return;
            }

            parentMotionSet = parentMotionSet->GetParentSet();
        }

        // Motion id not found in the motion set or any of its parents, set the line edit invalid.
        GetManager()->SetWidgetAsInvalidInput(mLineEdit);
    }


    // file browse button pressed
    void MotionPickerAttributeWidget::OnMotionPicker()
    {
        // get the current motion set
        EMotionFX::MotionSet* currentMotionSet;
        EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (animGraphPlugin)
        {
            AnimGraphPlugin* pluginCast = static_cast<AnimGraphPlugin*>(animGraphPlugin);
            currentMotionSet = pluginCast->GetResourceWidget()->GetSelectedMotionSet();
        }
        else
        {
            currentMotionSet = nullptr;
        }

        // motion set must be valid
        if (currentMotionSet == nullptr)
        {
            QMessageBox::warning(this, "Motion Set Invalid", "Cannot open motion selection window. Please make sure exactly one motion set is selected.");
            return;
        }

        // get the number of attributes
        const uint32 numAttributes = mAttributes.GetLength();

        // create and show the motion picker window
        MotionSetSelectionWindow motionPickWindow(this);
        motionPickWindow.Update(currentMotionSet);
        motionPickWindow.setModal(true);
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCORE_ASSERT(mAttributes[i]->GetType() == MCore::AttributeString::TYPE_ID);
            const MCore::String& value = static_cast<MCore::AttributeString*>(mAttributes[i])->GetValue();
            QTreeWidgetItemIterator itemIterator(motionPickWindow.GetHierarchyWidget()->GetTreeWidget());
            QTreeWidgetItem* lastItemFound = nullptr;
            while (*itemIterator)
            {
                QTreeWidgetItem* item = *itemIterator;
                if (item->text(0) == value.AsChar())
                {
                    lastItemFound = item;
                }
                ++itemIterator;
            }
            if (lastItemFound)
            {
                lastItemFound->setSelected(true);
            }
        }
        if (motionPickWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected motion entries.
        const AZStd::vector<MotionSetSelectionItem>& selectedMotionEntries = motionPickWindow.GetHierarchyWidget()->GetSelectedItems();
        if (selectedMotionEntries.empty())
        {
            return;
        }

        mLineEdit->setText(selectedMotionEntries[0].mMotionId.c_str());

        // get the number of attributes and iterate through them
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(selectedMotionEntries[0].mMotionId.c_str());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();

        // request reinit of the whole attribute window
        //if (mAttributeInfo->GetReinitOnValueChange())
        emit RequestParentReInit();
    }



    //-----------------------------------------------------------------------------------------------------------------

    MultipleMotionPickerAttributeWidget::MultipleMotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();
        mNeedsHeavyUpdate = true;

        // if there is no mask setup yet
        mLink = new MysticQt::LinkWidget("select motions");
        mLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mLink->setEnabled(!readOnly);
        UpdateLinkWidget();
        connect(mLink, SIGNAL(clicked()), this, SLOT(OnMotionPicker()));

        // the reset selection button
        //mResetSelectionButton = new QPushButton();
        //EMStudioManager::MakeTransparentButton( mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection" );
        //connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

        // create the widget and the layout for the link and the reset button
        QWidget*        helperWidget = new QWidget();
        QHBoxLayout*    helperLayout = new QHBoxLayout();

        // adjust some layout settings
        helperLayout->setSpacing(0);
        helperLayout->setMargin(0);
        mLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // add the node link and the reset button to the layout and set it as the helper widget's layout
        helperLayout->addWidget(mLink, 0, Qt::AlignLeft);
        //helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        helperLayout->addWidget(spacerWidget);
        helperWidget->setLayout(helperLayout);

        CreateStandardLayout(helperWidget, attributeSettings);
    }


    // update the link widget text and tooltip
    void MultipleMotionPickerAttributeWidget::UpdateLinkWidget()
    {
        if (mAttributes[0] == nullptr)
        {
            return;
        }

        MCORE_ASSERT(mAttributes[0]->GetType() == MCore::AttributeArray::TYPE_ID);
        MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[0]);
        const uint32 numArrayItems = arrayAttrib->GetNumAttributes();

        // update the link widget text
        QString text;
        if (numArrayItems == 0)
        {
            text = "select motions";
        }
        else
        if (numArrayItems == 1)
        {
            MCORE_ASSERT(arrayAttrib->GetAttribute(0)->GetType() == MCore::AttributeString::TYPE_ID);
            text = static_cast<MCore::AttributeString*>(arrayAttrib->GetAttribute(0))->GetValue().AsChar();
        }
        else
        {
            text.sprintf("%d motions", numArrayItems);
        }

        mLink->setText(text);

        // build the tooltip string
        text.clear();
        for (uint32 i = 0; i < numArrayItems; ++i)
        {
            MCORE_ASSERT(arrayAttrib->GetAttribute(i)->GetType() == MCore::AttributeString::TYPE_ID);
            text += static_cast<MCore::AttributeString*>(arrayAttrib->GetAttribute(i))->GetValue().AsChar();
            if (i != numArrayItems - 1)
            {
                text += "\n";
            }
        }
        mLink->setToolTip(text);
    }


    // file browse button pressed
    void MultipleMotionPickerAttributeWidget::OnMotionPicker()
    {
        // get the current motion set
        EMotionFX::MotionSet* currentMotionSet;
        EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (animGraphPlugin)
        {
            AnimGraphPlugin* pluginCast = static_cast<AnimGraphPlugin*>(animGraphPlugin);
            currentMotionSet = pluginCast->GetResourceWidget()->GetSelectedMotionSet();
        }
        else
        {
            currentMotionSet = nullptr;
        }

        // motion set must be valid
        if (currentMotionSet == nullptr)
        {
            QMessageBox::warning(this, "Motion Set Invalid", "Cannot open motion selection window. Please make sure exactly one motion set is selected.");
            return;
        }

        // get the number of attributes
        const uint32 numAttributes = mAttributes.GetLength();

        // create and show the motion picker window
        MotionSetSelectionWindow motionPickWindow(this);
        motionPickWindow.GetHierarchyWidget()->SetSelectionMode(false);
        motionPickWindow.Update(currentMotionSet);
        motionPickWindow.setModal(true);
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCORE_ASSERT(mAttributes[i]->GetType() == MCore::AttributeArray::TYPE_ID);
            MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[i]);
            const uint32 numArrayItems = arrayAttrib->GetNumAttributes();
            for (uint32 j = 0; j < numArrayItems; ++j)
            {
                MCORE_ASSERT(arrayAttrib->GetAttribute(j)->GetType() == MCore::AttributeString::TYPE_ID);
                const MCore::String& value = static_cast<MCore::AttributeString*>(arrayAttrib->GetAttribute(j))->GetValue();
                QTreeWidgetItemIterator itemIterator(motionPickWindow.GetHierarchyWidget()->GetTreeWidget());
                QTreeWidgetItem* lastItemFound = nullptr;
                while (*itemIterator)
                {
                    QTreeWidgetItem* item = *itemIterator;
                    if (item->text(0) == value.AsChar())
                    {
                        lastItemFound = item;
                    }
                    ++itemIterator;
                }
                if (lastItemFound)
                {
                    lastItemFound->setSelected(true);
                }
            }
        }
        if (motionPickWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected motion entries.
        const AZStd::vector<MotionSetSelectionItem>& selectedMotionEntries = motionPickWindow.GetHierarchyWidget()->GetSelectedItems();
        if (selectedMotionEntries.empty())
        {
            return;
        }

        // Generate the unique id strings. This is needed because parent motion sets can have the same id.
        const size_t numSelected = selectedMotionEntries.size();
        AZStd::vector<AZStd::string> uniqueIdStrings;
        uniqueIdStrings.reserve(numSelected);
        for (size_t s = 0; s < numSelected; ++s)
        {
            if (AZStd::find(uniqueIdStrings.begin(), uniqueIdStrings.end(), selectedMotionEntries[s].mMotionId) == uniqueIdStrings.end())
            {
                uniqueIdStrings.push_back(selectedMotionEntries[s].mMotionId);
            }
        }

        // Put all selected items in the attribute.
        const size_t numUniqueIdStrings = uniqueIdStrings.size();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[i]);
            arrayAttrib->Clear();

            for (size_t s = 0; s < numUniqueIdStrings; ++s)
            {
                MCore::AttributeString* newStringAttrib = MCore::AttributeString::Create(uniqueIdStrings[s].c_str());
                arrayAttrib->AddAttribute(newStringAttrib, false);
            }

            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        UpdateLinkWidget();

        FireValueChangedSignal();
        UpdateInterface();

        // request reinit of the whole attribute window
        //if (mAttributeInfo->GetReinitOnValueChange())
        emit RequestParentReInit();
    }

    //-----------------------------------------------------------------------------------------------------------------

    BlendSpaceMotionWidgetContainer::BlendSpaceMotionWidgetContainer(EMotionFX::AttributeBlendSpaceMotion* attribute, QGridLayout* layout, int row)
        : m_attribute(attribute)
    {
        const AZStd::string& motionId = attribute->GetMotionId();
        const bool showYFields = attribute->GetDimension() == 2;

        int column = 0;

        // Motion name
        QLabel* labelMotionName = new QLabel(motionId.c_str());
        labelMotionName->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addWidget(labelMotionName, row, column);
        column++;

        // Motion position x
        QHBoxLayout* layoutX = new QHBoxLayout();
        layoutX->setAlignment(Qt::AlignRight);

        QLabel* labelX = new QLabel("X");
        labelX->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        labelX->setStyleSheet("QLabel { font-weight: bold; color : red; }");
        layoutX->addWidget(labelX);

        m_spinboxX = new MysticQt::DoubleSpinBox();
        m_spinboxX->setSingleStep(0.1);
        m_spinboxX->setDecimals(4);
        m_spinboxX->setRange(-FLT_MAX, FLT_MAX);
        m_spinboxX->setProperty("motionId", motionId.c_str());
        layoutX->addWidget(m_spinboxX);

        layout->addLayout(layoutX, row, column);
        column++;

        // Motion position y
        if (showYFields)
        {
            QHBoxLayout* layoutY = new QHBoxLayout();
            layoutY->setAlignment(Qt::AlignRight);

            QLabel* labelY = new QLabel("Y");
            labelY->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            labelY->setStyleSheet("QLabel { font-weight: bold; color : green; }");
            layoutY->addWidget(labelY);

            m_spinboxY = new MysticQt::DoubleSpinBox();
            m_spinboxY->setSingleStep(0.1);
            m_spinboxY->setDecimals(4);
            m_spinboxY->setRange(-FLT_MAX, FLT_MAX);
            m_spinboxY->setProperty("motionId", motionId.c_str());
            layoutY->addWidget(m_spinboxY);

            layout->addLayout(layoutY, row, column);
            column++;
        }
        else
        {
            m_spinboxY = nullptr;
        }

        // Restore button.
        const int iconSize = 20;
        m_restoreButton = new QPushButton();
        m_restoreButton->setToolTip("Restore value to automatically computed one");
        m_restoreButton->setMinimumSize(iconSize, iconSize);
        m_restoreButton->setMaximumSize(iconSize, iconSize);
        m_restoreButton->setIcon(QIcon(":/EMotionFX/Restore.png"));
        m_restoreButton->setProperty("motionId", motionId.c_str());
        layout->addWidget(m_restoreButton, row, column);
        column++;

        // Remove motion from blend space button.
        m_removeButton = new QPushButton();
        m_removeButton->setToolTip("Remove motion from blend space");
        m_removeButton->setMinimumSize(iconSize, iconSize);
        m_removeButton->setMaximumSize(iconSize, iconSize);
        m_removeButton->setIcon(QIcon(":/EMotionFX/Trash.png"));
        m_removeButton->setProperty("motionId", motionId.c_str());
        layout->addWidget(m_removeButton, row, column);
    }


    void BlendSpaceMotionWidgetContainer::UpdateInterface(EMotionFX::BlendSpaceNode* blendSpaceNode, EMotionFX::AnimGraphInstance* animGraphInstance)
    {
        bool positionsComputed = false;
        AZ::Vector2 computedPosition = AZ::Vector2::CreateZero();
        if (blendSpaceNode && animGraphInstance)
        {
            blendSpaceNode->ComputeMotionPosition(m_attribute->GetMotionId(), animGraphInstance, computedPosition);
            positionsComputed = true;
        }

        // Spinbox X
        m_spinboxX->blockSignals(true);

        if (m_attribute->IsXCoordinateSetByUser())
        {
            m_spinboxX->setValue( m_attribute->GetXCoordinate() );
        }
        else
        {
            m_spinboxX->setValue( computedPosition.GetX() );
        }

        m_spinboxX->blockSignals(false);
        m_spinboxX->setEnabled( m_attribute->IsXCoordinateSetByUser() || positionsComputed );


        // Spinbox Y
        if (m_spinboxY)
        {
            m_spinboxY->blockSignals(true);

            if (m_attribute->IsYCoordinateSetByUser())
            {
                m_spinboxY->setValue( m_attribute->GetYCoordinate() );
            }
            else
            {
                m_spinboxY->setValue( computedPosition.GetY() );
            }

            m_spinboxY->blockSignals(false);
            m_spinboxY->setEnabled( m_attribute->IsYCoordinateSetByUser() || positionsComputed );
        }


        // Enable the restore button in case the user manually set any of the.
        const bool enableRestoreButton = m_attribute->IsXCoordinateSetByUser() || m_attribute->IsYCoordinateSetByUser();
        m_restoreButton->setEnabled(enableRestoreButton);    
    }

    //-----------------------------------------------------------------------------------------------------------------

    BlendSpaceMotionsAttributeWidget::BlendSpaceMotionsAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings,
            void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
        , mAttributeSettings(attributeSettings)
        , mWidget(nullptr)
    {
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mContainerWidget = new QWidget();
        mContainerLayout = new QVBoxLayout();

        mContainerLayout->setSpacing(0);
        mContainerLayout->setMargin(0);
        mContainerWidget->setLayout(mContainerLayout);

        PopulateWidget();
        UpdateInterface();

        CreateStandardLayout(mContainerWidget, mAttributeSettings);
    }

    BlendSpaceMotionWidgetContainer* BlendSpaceMotionsAttributeWidget::FindWidgetContainerByMotionId(const AZStd::string& motionId) const
    {
        for (BlendSpaceMotionWidgetContainer* container : m_motions)
        {
            const AZStd::string& attributeMotionId = container->m_attribute->GetMotionId();
            if (attributeMotionId == motionId)
            {
                return container;
            }
        }

        return nullptr;
    }

    BlendSpaceMotionWidgetContainer* BlendSpaceMotionsAttributeWidget::FindWidgetContainer(QObject* object)
    {
        const AZStd::string motionId = object->property("motionId").toString().toUtf8().data();

        BlendSpaceMotionWidgetContainer* widgetContainer = FindWidgetContainerByMotionId(motionId);
        AZ_Assert(widgetContainer, "Can't find widget container for motion with id '%s'.", motionId.c_str());

        return widgetContainer;
    }

    void BlendSpaceMotionsAttributeWidget::OnAddMotion()
    {
        EMotionFX::MotionSet* currentMotionSet = nullptr;
        EMStudioPlugin* animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (animGraphPlugin)
        {
            AnimGraphPlugin* pluginCast = static_cast<AnimGraphPlugin*>(animGraphPlugin);
            currentMotionSet = pluginCast->GetResourceWidget()->GetSelectedMotionSet();
        }
        if (!currentMotionSet)
        {
            QMessageBox::warning(this, "Motion Set Invalid", "Cannot open motion selection window. Please make sure exactly one motion set is selected.");
            return;
        }

        const uint32 numAttributes = mAttributes.GetLength();

        // Create and show the motion picker window.
        MotionSetSelectionWindow motionPickWindow(this);
        motionPickWindow.GetHierarchyWidget()->SetSelectionMode(false);
        motionPickWindow.Update(currentMotionSet);
        motionPickWindow.setModal(true);
        if (motionPickWindow.exec() == QDialog::Rejected)
        {
            // User pressed cancel or close button.
            return;
        }

        const AZStd::vector<MotionSetSelectionItem>& selectedItems = motionPickWindow.GetHierarchyWidget()->GetSelectedItems();
        const size_t numSelected = selectedItems.size();
        if (numSelected == 0)
        {
            return;
        }

        if (!mAttributes[0])
        {
            return;
        }
        MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[0]);
 
        for (size_t s=0; s < numSelected; ++s)
        {
            const AZStd::string& motionId = selectedItems[s].mMotionId;

            bool alreadyPresent(false);
            const uint32 numArrayItems = arrayAttrib->GetNumAttributes();
            for (uint32 i = 0; i < numArrayItems; ++i)
            {
                MCORE_ASSERT(arrayAttrib->GetAttribute(i)->GetType() == EMotionFX::AttributeBlendSpaceMotion::TYPE_ID);
                const EMotionFX::AttributeBlendSpaceMotion* attrib = static_cast<EMotionFX::AttributeBlendSpaceMotion*>(arrayAttrib->GetAttribute(i));
                if (attrib->GetMotionId() == motionId)
                {
                    alreadyPresent = true;
                    break;
                }
            }
            if (!alreadyPresent)
            {
                EMotionFX::AttributeBlendSpaceMotion* newAttrib = EMotionFX::AttributeBlendSpaceMotion::Create(motionId.c_str());
                arrayAttrib->AddAttribute(newAttrib, false);
            }
        }

        if (mAttribChangedFunc)
        {
            mAttribChangedFunc(arrayAttrib, mAttributeSettings);
        }

        FireValueChangedSignal();
        UpdateInterface();

        // Request reinit of the whole attribute window.
        emit RequestParentReInit();
    }

    void BlendSpaceMotionsAttributeWidget::OnRemoveMotion()
    {
        QPushButton* removeButton = qobject_cast<QPushButton*>(sender());
        if (!removeButton)
        {
            AZ_Assert(false, "No corresponding sender QPushButton.");
            return;
        }

        // Get the motion id for the remove button that got clicked.
        const AZStd::string motionId = removeButton->property("motionId").toString().toUtf8().data();
        if (motionId.empty())
        {
            AZ_Assert(false, "There should not be an element for a motion with an empty id.");
            return;
        }

        MCore::AttributeArray* arrayAttribute = static_cast<MCore::AttributeArray*>(mAttributes[0]);
        if (!arrayAttribute)
        {
            AZ_Assert(false, "There should at least be one array attribute.");
            return;
        }

        // Iterate through the arributes back to front and delete the ones with the motion id from the delete button.
        // Note: Normally there should only be once instance as motion ids should be unique within this array.
        const int32 motionCount = arrayAttribute->GetNumAttributes();
        for (int32 i = motionCount-1; i >= 0; i--)
        {
            AZ_Assert(arrayAttribute->GetAttribute(i)->GetType() == EMotionFX::AttributeBlendSpaceMotion::TYPE_ID, "AttributeBlendSpaceMotion attributes expected.");
            const EMotionFX::AttributeBlendSpaceMotion* attribute = static_cast<EMotionFX::AttributeBlendSpaceMotion*>(arrayAttribute->GetAttribute(i));

            if (motionId == attribute->GetMotionId())
            {
                arrayAttribute->RemoveAttribute(i);
            }
        }

        // Request reinit of the whole attribute window.
        emit RequestParentReInit();
    }

    void BlendSpaceMotionsAttributeWidget::OnPositionXChanged(double value)
    {
        UpdateMotionPosition(sender(), static_cast<float>(value), true, false);
    }

    void BlendSpaceMotionsAttributeWidget::OnPositionYChanged(double value)
    {
        UpdateMotionPosition(sender(), static_cast<float>(value), false, true);
    }

    void BlendSpaceMotionsAttributeWidget::UpdateMotionPosition(QObject* object, float value, bool updateX, bool updateY)
    {
        BlendSpaceMotionWidgetContainer* widgetContainer = FindWidgetContainer(object);
        if (!widgetContainer)
        {
            return;
        }

        EMotionFX::AttributeBlendSpaceMotion* attribute = widgetContainer->m_attribute;

        // Get access to the blend space node of the anim graph to be able to calculate the blend space position.
        EMotionFX::AnimGraphObject* animGraphObject = mPlugin->GetAttributesWindow()->GetObject();
        EMotionFX::BlendSpaceNode* blendSpaceNode = azdynamic_cast<EMotionFX::BlendSpaceNode*>(animGraphObject);
        if (blendSpaceNode)
        {
            // Get the anim graph instance in case only exactly one actor instance is selected.
            EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();
            if (animGraphInstance)
            {
                // Compute the position of the motion using the set evaluators.
                AZ::Vector2 computedPosition;
                blendSpaceNode->ComputeMotionPosition(widgetContainer->m_attribute->GetMotionId(), animGraphInstance, computedPosition);

                const float epsilon = 1.0f / (powf(10, static_cast<float>(widgetContainer->m_spinboxX->decimals())));
                if (updateX)
                {
                    if (attribute->IsXCoordinateSetByUser())
                    {
                        // If we already manually set the motion position, just update the x coordinate.
                        attribute->SetXCoordinate(value);
                    }
                    else
                    {
                        // Check if the user just clicked the interface and triggered a value change or if he actually changed the value.
                        if (!AZ::IsClose(computedPosition.GetX(), value, epsilon))
                        {
                            // Mark the position as manually set in case the user entered a new position that differs from the automatically computed one.
                            attribute->MarkXCoordinateSetByUser(true);
                            attribute->SetXCoordinate(value);
                        }
                    }
                }

                if (updateY)
                {
                    if (attribute->IsYCoordinateSetByUser())
                    {
                        attribute->SetYCoordinate(value);
                    }
                    else
                    {
                        if (!AZ::IsClose(computedPosition.GetY(), value, epsilon))
                        {
                            attribute->MarkYCoordinateSetByUser(true);
                            attribute->SetYCoordinate(value);
                        }
                    }
                }
            }
            else
            {
                // In case there is no character, only the motion positions that are already in manual mode are enabled.
                // Thus, we can just forward the position shown in the interface to the attribute.

                if (updateX)
                {
                    attribute->MarkXCoordinateSetByUser(true);
                    attribute->SetXCoordinate(value);

                }
                if (updateY)
                {
                    attribute->MarkYCoordinateSetByUser(true);
                    attribute->SetYCoordinate(value);
                }
            }
        }

        blendSpaceNode->OnUpdateAttributes();
        UpdateInterface();
    }

    void BlendSpaceMotionsAttributeWidget::OnRestorePosition()
    {
        BlendSpaceMotionWidgetContainer* widgetContainer = FindWidgetContainer(sender());
        if (!widgetContainer)
        {
            return;
        }

        EMotionFX::AttributeBlendSpaceMotion* attribute = widgetContainer->m_attribute;

        // Get the anim graph instance in case only exactly one actor instance is selected.
        EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();

        // Get access to the blend space node of the anim graph to be able to calculate the blend space position.
        EMotionFX::AnimGraphObject* object = mPlugin->GetAttributesWindow()->GetObject();
        EMotionFX::BlendSpaceNode* blendSpaceNode = azdynamic_cast<EMotionFX::BlendSpaceNode*>(object);
        if (blendSpaceNode && animGraphInstance)
        {
            blendSpaceNode->RestoreMotionCoords(widgetContainer->m_attribute->GetMotionId(), animGraphInstance); 
            blendSpaceNode->OnUpdateAttributes();

            UpdateInterface();
        }
    }

    void BlendSpaceMotionsAttributeWidget::UpdateInterface()
    {
        // Get access to the blend space node.
        EMotionFX::BlendSpaceNode* blendSpaceNode = azdynamic_cast<EMotionFX::BlendSpaceNode*>(mPlugin->GetAttributesWindow()->GetObject());
        
        // Get the anim graph instance in case only exactly one actor instance is selected.
        EMotionFX::AnimGraphInstance* animGraphInstance = GetSingleSelectedAnimGraphInstance();

        for (BlendSpaceMotionWidgetContainer* container : m_motions)
        {
            container->UpdateInterface(blendSpaceNode, animGraphInstance);
        }

        MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[0]);
        const uint32 numMotions = arrayAttrib->GetNumAttributes();
        if (numMotions == 0)
        {
            m_addMotionsLabel->setText("Add motions and set coordinates.");
        }
        else
        {
            m_addMotionsLabel->setText("");
        }
    }


    void BlendSpaceMotionsAttributeWidget::PopulateWidget()
    {
        if (!mAttributes[0] || !mContainerWidget)
        {
            return;
        }

        if (mWidget)
        {
            // Hide the old widget and request deletion.
            mWidget->hide();
            mWidget->deleteLater();
            m_addMotionsLabel = nullptr;

            // Get rid of all widget containers;
            for (BlendSpaceMotionWidgetContainer* container : m_motions)
            {
                delete container;
            }
            m_motions.clear();
        }

        MCORE_ASSERT(mAttributes[0]->GetType() == MCore::AttributeArray::TYPE_ID);
        MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[0]);
        const uint32 numMotions = arrayAttrib->GetNumAttributes();
        AZ_Assert(m_motions.empty(), "Motion container array must be empty.");

        mWidget = new QWidget(mContainerWidget);
        mWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        QVBoxLayout* widgetLayout = new QVBoxLayout();

        QHBoxLayout* topRowLayout = new QHBoxLayout();

        // Add helper label left of the add button.
        m_addMotionsLabel = new QLabel();
        topRowLayout->addWidget(m_addMotionsLabel, 0, Qt::AlignLeft);

        // Add motions button.
        QPushButton* addMotionsButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(addMotionsButton, "/Images/Icons/Plus.png", "Add motions to blend space");
        connect(addMotionsButton, &QPushButton::clicked, this, &BlendSpaceMotionsAttributeWidget::OnAddMotion);
        topRowLayout->addWidget(addMotionsButton, 0, Qt::AlignRight);

        widgetLayout->addLayout(topRowLayout);

        if (numMotions > 0)
        {
            QWidget* motionsWidget = new QWidget(mWidget);
            motionsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            QGridLayout* motionsLayout = new QGridLayout();
            motionsLayout->setMargin(0);

            for (uint32 i = 0; i < numMotions; ++i)
            {
                AZ_Assert(arrayAttrib->GetAttribute(i)->GetType() == EMotionFX::AttributeBlendSpaceMotion::TYPE_ID, "AttributeBlendSpaceMotion attributes expected.");
                EMotionFX::AttributeBlendSpaceMotion* attribute = static_cast<EMotionFX::AttributeBlendSpaceMotion*>(arrayAttrib->GetAttribute(i));

                BlendSpaceMotionWidgetContainer* widgetContainer = new BlendSpaceMotionWidgetContainer(attribute, motionsLayout, i);

                connect(widgetContainer->m_spinboxX, SIGNAL(valueChanged(double)), this, SLOT(OnPositionXChanged(double)));
                if (widgetContainer->m_spinboxY)
                {
                    connect(widgetContainer->m_spinboxY, SIGNAL(valueChanged(double)), this, SLOT(OnPositionYChanged(double)));
                }
                connect(widgetContainer->m_restoreButton, &QPushButton::clicked, this, &BlendSpaceMotionsAttributeWidget::OnRestorePosition);
                connect(widgetContainer->m_removeButton, &QPushButton::clicked, this, &BlendSpaceMotionsAttributeWidget::OnRemoveMotion);

                m_motions.push_back(widgetContainer);
            }

            motionsWidget->setLayout(motionsLayout);
            widgetLayout->addWidget(motionsWidget);
        }
        mWidget->setLayout(widgetLayout);
        mContainerLayout->addWidget(mWidget);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // BlendSpaceMotionPickerAttributeWidget
    //-----------------------------------------------------------------------------------------------------------------
    BlendSpaceMotionPickerAttributeWidget::BlendSpaceMotionPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        MysticQt::ComboBox* comboBox = new MysticQt::ComboBox();
        CreateStandardLayout(comboBox, attributeSettings);

        // Get the initial combobox string.
        int32 initialIndex = -1;
        const AZStd::string initialValue = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";

        // Get access to the blend space node of the anim graph.
        EMotionFX::AnimGraphObject* animGraphObject = mPlugin->GetAttributesWindow()->GetObject();
        EMotionFX::BlendSpaceNode* blendSpaceNode = azdynamic_cast<EMotionFX::BlendSpaceNode*>(animGraphObject);
        if (blendSpaceNode)
        {
            MCore::AttributeArray* motionAttributeArray = blendSpaceNode->GetMotionAttributeArray();

            const uint32 numMotions = motionAttributeArray->GetNumAttributes();
            for (uint32 i = 0; i < numMotions; ++i)
            {
                AZ_Assert(motionAttributeArray->GetAttribute(i)->GetType() == EMotionFX::AttributeBlendSpaceMotion::TYPE_ID, "AttributeBlendSpaceMotion attributes expected.");
                EMotionFX::AttributeBlendSpaceMotion* attribute = static_cast<EMotionFX::AttributeBlendSpaceMotion*>(motionAttributeArray->GetAttribute(i));

                if (attribute->GetMotionId() == initialValue)
                {
                    initialIndex = i;
                }

                comboBox->addItem(attribute->GetMotionId().c_str());
            }
        }

        comboBox->setEditable(false);
        comboBox->setEnabled(!readOnly);
        connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnComboBox(int)));

        // Select the previously selected motion.
        comboBox->setCurrentIndex(initialIndex);
    }

    void BlendSpaceMotionPickerAttributeWidget::OnComboBox(int value)
    {
        MCORE_UNUSED(value);

        MysticQt::ComboBox* combo = qobject_cast<MysticQt::ComboBox*>(sender());
        QString stringValue = combo->currentText();

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(stringValue).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    NodeNameAttributeWidget::NodeNameAttributeWidget(EMotionFX::AnimGraphNode* node, MCore::AttributeSettings* attributeSettings, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : AttributeWidget(MCore::Array<MCore::Attribute*>(), attributeSettings, nullptr, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mNode                       = node;
        QLineEdit*      lineEdit    = new QLineEdit();
        QHBoxLayout*    layout      = new QHBoxLayout();

        layout->addWidget(lineEdit);
        setLayout(layout);
        layout->setMargin(0);

        const char* description = "The unique name of the anim graph node.";

        lineEdit->setText(mNode->GetName());
        lineEdit->setToolTip(description);
        lineEdit->setReadOnly(readOnly);

        connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(OnNodeNameChange()));
        connect(lineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnNodeNameEdited(const QString&)));
    }


    // validate a given name
    bool NodeNameAttributeWidget::ValidateNodeName(const char* newName) const
    {
        // get the active animgraph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        assert(animGraph);

        // check if the node already exists in the active anim graph
        EMotionFX::AnimGraphNode* blendNode = animGraph->RecursiveFindNode(newName);
        if (blendNode && blendNode != mNode) // it already exists
        {
            return false;
        }

        // $ROOT$ as node name is not allowed!
        if (strcmp(newName, "$ROOT$") == 0)
        {
            return false;
        }

        // empty node name is not allowed!
        if (strcmp(newName, "") == 0)
        {
            return false;
        }

        return true;
    }


    void NodeNameAttributeWidget::OnNodeNameEdited(const QString& text)
    {
        assert(sender()->inherits("QLineEdit"));
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());

        if (ValidateNodeName(FromQtString(text).AsChar()) == false)
        {
            GetManager()->SetWidgetAsInvalidInput(widget);
        }
        else
        {
            widget->setStyleSheet("");
        }
    }


    // try to apply the new node name
    void NodeNameAttributeWidget::OnNodeNameChange()
    {
        assert(sender()->inherits("QLineEdit"));
        QLineEdit* widget = qobject_cast<QLineEdit*>(sender());
        QString newName = widget->text();
        MCore::String coreString;
        FromQtString(newName, &coreString);

        // if the name didn't change do nothing
        if (mNode->GetNameString().CheckIfIsEqual(coreString.AsChar()))
        {
            return;
        }

        // validate the name
        if (ValidateNodeName(coreString.AsChar()) == false)
        {
            MCore::LogWarning("The name '%s' is either invalid or already in use by another node, please select another name.", coreString.AsChar());
            widget->setText(mNode->GetName());
        }
        else // trigger the rename
        {
            // get the active anim graph
            EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
            if (animGraph == nullptr)
            {
                return;
            }

            // build the command string
            MCore::String commandString;
            commandString.Format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -newName \"%s\"", animGraph->GetID(), mNode->GetName(), coreString.AsChar());

            // execute the command
            MCore::String commandResult;
            if (GetCommandManager()->ExecuteCommand(commandString.AsChar(), commandResult) == false)
            {
                MCore::LogError(commandResult.AsChar());
                return;
            }
        }
    }




    //-----------------------------------------------------------------------------------------------------------------
    // ParameterPickerAttributeWidget
    //-----------------------------------------------------------------------------------------------------------------
    ParameterPickerAttributeWidget::ParameterPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        MysticQt::ComboBox* comboBox = new MysticQt::ComboBox();
        CreateStandardLayout(comboBox, attributeSettings);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

        // get the initial value
        const MCore::String initialValue = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";

        // add all anim graph parameters
        int32 initialIndex = -1;
        const uint32 numParams = animGraph->GetNumParameters();
        for (uint32 i = 0; i < numParams; ++i)
        {
            if (animGraph->GetParameter(i)->GetNameString() == initialValue)
            {
                initialIndex = i;
            }

            comboBox->addItem(animGraph->GetParameter(i)->GetName());
        }

        comboBox->setEditable(false);
        comboBox->setEnabled(!readOnly);
        connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnComboBox(int)));

        // get the initial value
        comboBox->setCurrentIndex(initialIndex);
    }


    // adjust a combobox value
    void ParameterPickerAttributeWidget::OnComboBox(int value)
    {
        MCORE_UNUSED(value);

        MysticQt::ComboBox* combo = qobject_cast<MysticQt::ComboBox*>(sender());
        QString stringValue = combo->currentText();

        // update the string value in all attributes
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(stringValue).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    AnimGraphNodeAttributeWidget::AnimGraphNodeAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mLineEdit           = new QLineEdit();
        QPushButton* button = new QPushButton("...");
        button->setMaximumHeight(18);
        CreateStandardLayout(mLineEdit, button, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        mLineEdit->setReadOnly(readOnly);

        connect(mLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnTextChanged(const QString&)));
        connect(mLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
        connect(button, SIGNAL(clicked(bool)), this, SLOT(OnNodePicker()));

        QString lineEditText = value;
        mLineEdit->setText(lineEditText);
        OnTextEdited(lineEditText);
    }


    // called when manually entering a node name
    void AnimGraphNodeAttributeWidget::OnTextChanged(const QString& text)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(text).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // called when manually changing the node name, this is just a verification for it
    void AnimGraphNodeAttributeWidget::OnTextEdited(const QString& text)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // mark edit field in red in case the currently entered text is no valid node name
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(FromQtString(text).AsChar());
        if (node == nullptr)
        {
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else
        {
            mLineEdit->setStyleSheet("");
        }
    }


    // browse button pressed
    void AnimGraphNodeAttributeWidget::OnNodePicker()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // create and show the node picker window
        BlendNodeSelectionWindow nodePickWindow(this, true, nullptr);
        nodePickWindow.Update(animGraph->GetID(), nullptr);
        nodePickWindow.setModal(true);
        if (nodePickWindow.exec() == QDialog::Rejected) // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected nodes.
        const AZStd::vector<AnimGraphSelectionItem>& selectedNodes = nodePickWindow.GetAnimGraphHierarchyWidget()->GetSelectedItems();
        if (selectedNodes.empty())
        {
            return;
        }

        QString selectedNodeName = selectedNodes[0].mNodeName.c_str();
        mLineEdit->setText(selectedNodeName);
        OnTextEdited(selectedNodeName);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(selectedNodes[0].mNodeName.c_str());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    BlendTreeMotionAttributeWidget::BlendTreeMotionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mLineEdit           = new QLineEdit();
        QPushButton* button = new QPushButton("...");
        button->setMaximumHeight(18);
        CreateStandardLayout(mLineEdit, button, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        mLineEdit->setReadOnly(readOnly);
        connect(mLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnTextChanged(const QString&)));
        connect(mLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
        connect(button, SIGNAL(clicked(bool)), this, SLOT(OnMotionNodePicker()));

        QString lineEditText = value;
        mLineEdit->setText(lineEditText);
        OnTextEdited(lineEditText);
    }


    // called when manually entering a node name
    void BlendTreeMotionAttributeWidget::OnTextChanged(const QString& text)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(text).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // called when manually changing the node name, this is just a verification for it
    void BlendTreeMotionAttributeWidget::OnTextEdited(const QString& text)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // mark edit field in red in case the currently entered text is no valid node name
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(FromQtString(text).AsChar());
        if (node == nullptr)
        {
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else
        {
            mLineEdit->setStyleSheet("");
        }
    }


    // browse button pressed
    void BlendTreeMotionAttributeWidget::OnMotionNodePicker()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // create and show the motion picker window
        BlendNodeSelectionWindow motionNodePickWindow(this, true, nullptr, EMotionFX::AnimGraphMotionNode::TYPE_ID);
        motionNodePickWindow.Update(animGraph->GetID(), nullptr);
        motionNodePickWindow.setModal(true);
        if (motionNodePickWindow.exec() == QDialog::Rejected)   // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected nodes.
        const AZStd::vector<AnimGraphSelectionItem>& selectedNodes = motionNodePickWindow.GetAnimGraphHierarchyWidget()->GetSelectedItems();
        if (selectedNodes.empty())
        {
            return;
        }

        QString selectedNodeName = selectedNodes[0].mNodeName.c_str();
        mLineEdit->setText(selectedNodeName);
        OnTextEdited(selectedNodeName);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(selectedNodes[0].mNodeName.c_str());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    AnimGraphStateAttributeWidget::AnimGraphStateAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mLineEdit           = new QLineEdit();
        QPushButton* button = new QPushButton("...");
        CreateStandardLayout(mLineEdit, button, attributeSettings);

        const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        mLineEdit->setReadOnly(readOnly);
        connect(mLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnTextChanged(const QString&)));
        connect(mLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
        connect(button, SIGNAL(clicked(bool)), this, SLOT(OnStatePicker()));

        QString lineEditText = value;
        mLineEdit->setText(lineEditText);
        OnTextEdited(lineEditText);
    }


    // called when manually entering a node name
    void AnimGraphStateAttributeWidget::OnTextChanged(const QString& text)
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(FromQtString(text).AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // called when manually changing the node name, this is just a verification for it
    void AnimGraphStateAttributeWidget::OnTextEdited(const QString& text)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // mark edit field in red in case the currently entered text is no valid node name
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNode(FromQtString(text).AsChar());
        if (node == nullptr)
        {
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else
        {
            mLineEdit->setStyleSheet("");
        }
    }


    // browse button pressed
    void AnimGraphStateAttributeWidget::OnStatePicker()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // create and show the motion picker window
        BlendNodeSelectionWindow statePickWindow(this, true, nullptr, MCORE_INVALIDINDEX32, true);
        statePickWindow.Update(animGraph->GetID(), nullptr);
        statePickWindow.setModal(true);
        if (statePickWindow.exec() == QDialog::Rejected)    // we pressed cancel or the close cross
        {
            return;
        }

        // Get the selected states.
        const AZStd::vector<AnimGraphSelectionItem>& selectedStates = statePickWindow.GetAnimGraphHierarchyWidget()->GetSelectedItems();
        if (selectedStates.empty())
        {
            return;
        }

        QString selectedNodeName = selectedStates[0].mNodeName.c_str();
        mLineEdit->setText(selectedNodeName);
        OnTextEdited(selectedNodeName);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(selectedStates[0].mNodeName.c_str());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    MotionEventTrackAttributeWidget::MotionEventTrackAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mMotionNode = (EMotionFX::AnimGraphMotionNode*)customData;

        mComboBox = new MysticQt::ComboBox();
        CreateStandardLayout(mComboBox, attributeSettings);

        connect(mComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnComboBox(int)));
        ReInit();
    }


    void MotionEventTrackAttributeWidget::ReInit()
    {
        mComboBox->clear();
        //  mComboBox->addItem("None");
        mComboBox->setEnabled(!mReadOnly);

        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            //      MCORE_ASSERT(animGraphInstance);
            if (animGraphInstance == nullptr)
            {
                return;
            }

            // get the motion set
            EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
            if (motionSet)
            {
                // get the motion from the motion set, and load it on demand
                EMotionFX::Motion* motion = motionSet->RecursiveFindMotionByStringID(static_cast<MCore::AttributeString*>(mMotionNode->GetAttribute(EMotionFX::AnimGraphMotionNode::ATTRIB_MOTION))->AsChar());

                // make sure the motion loaded successfully
                if (motion)
                {
                    EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
                    const uint32 numMotionTracks = eventTable->GetNumTracks();
                    for (uint32 i = 0; i < numMotionTracks; ++i)
                    {
                        if (eventTable->GetTrack(i)->GetNameString().GetIsEmpty() == false)
                        {
                            mComboBox->addItem(eventTable->GetTrack(i)->GetName());
                        }
                    }
                }
                else
                {
                    //MCore::LogWarning("Cannot initialize sync event track combo box. The motion '%s' is not loaded and valid yet.", mMotionNode->GetAttributeValue(EMotionFX::BlendTreeMotionNode::ATTRIB_MOTION).AsCharPtr());
                    mComboBox->setEnabled(false);
                }
            }
            else
            {
                //MCore::LogWarning("Cannot initialize sync event track combo box. Please select a motion set.");
                mComboBox->setEnabled(false);
            }
        }
        else
        {
            //MCore::LogWarning("Cannot initialize sync event track combo box. Please select an actor instance.");
            mComboBox->setEnabled(false);
        }

        MCore::String text = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        int index = mComboBox->findText(text.AsChar());

        if (text.GetIsEmpty())
        {
            index = 0;
        }

        mComboBox->setCurrentIndex(index);
        mComboBox->setEditable(false);
    }


    // adjust a combobox value
    void MotionEventTrackAttributeWidget::OnComboBox(int value)
    {
        int index = value;

        MCore::String itemText;
        FromQtString(mComboBox->itemText(index), &itemText);

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(itemText.AsChar());
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------


    NodeSelectionAttributeWidget::NodeSelectionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mNodeSelectionWindow    = nullptr;//new NodeSelectionWindow(this, true);
        mSelectionList          = new CommandSystem::SelectionList();

        //connect( mNodeSelectionWindow,                             SIGNAL(rejected()),                                    this, SLOT(OnNodeSelectionCancelled()) );
        //connect( mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),    this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)) );

        if (mFirstAttribute)
        {
            const char* value = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
            mNodeLink = CreateNodeNameLink(value);

            mNodeLink->setEnabled(!readOnly);
            connect(mNodeLink, SIGNAL(clicked()), this, SLOT(OnNodeNames()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mNodeLink, 0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);

            CreateStandardLayout(helperWidget, attributeSettings);
        }
    }


    NodeSelectionAttributeWidget::~NodeSelectionAttributeWidget()
    {
        delete mSelectionList;
    }



    void NodeSelectionAttributeWidget::OnResetSelection()
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeString* nodeNameAttribute = static_cast<MCore::AttributeString*>(mAttributes[i]);
            nodeNameAttribute->SetValue("");
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        mNodeLink->setText("select node");
        FireValueChangedSignal();
        UpdateInterface();
    }


    // create a node name link
    MysticQt::LinkWidget* NodeSelectionAttributeWidget::CreateNodeNameLink(const char* nodeName)
    {
        MysticQt::LinkWidget* nodeLink = nullptr;

        // if there is no mask setup yet
        if (nodeName == nullptr || MCore::String(nodeName).GetIsEmpty())
        {
            nodeLink = new MysticQt::LinkWidget("select node");
        }
        else
        {
            nodeLink = new MysticQt::LinkWidget(nodeName);
        }

        // setup the size policy
        nodeLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return nodeLink;
    }


    // when we want to select a bunch of nodes
    void NodeSelectionAttributeWidget::OnNodeNames()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            QMessageBox::warning(this, "Selection Invalid", "Cannot open node selection window. Please make sure exactly one actor instance is selected.");
            return;
        }

        if (mNodeSelectionWindow == nullptr)
        {
            mNodeSelectionWindow    = new NodeSelectionWindow(this, true);
            connect(mNodeSelectionWindow,                           SIGNAL(rejected()),                                    this, SLOT(OnNodeSelectionCancelled()));
            connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),  this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)));
        }

        // get the old and current node name and set the selection of the selection window to it
        MCore::String       oldNodeName = (mFirstAttribute) ? static_cast<MCore::AttributeString*>(mFirstAttribute)->AsChar() : "";
        EMotionFX::Actor*   actor       = actorInstance->GetActor();

        mSelectionList->Clear();

        // add the old selected node to the selection list
        if (oldNodeName.GetIsEmpty() == false)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(oldNodeName.AsChar());
            if (node)
            {
                mSelectionList->AddNode(node);
            }
        }

        // show the node selection window
        mNodeSelectionWindow->Update(actorInstance->GetID(), mSelectionList);
        mNodeSelectionWindow->setModal(true);
        if (mNodeSelectionWindow->exec() != QDialog::Rejected)  // we pressed cancel or the close cross
        {
            if (mSelectedNodeName.GetIsEmpty() == false)
            {
                mNodeLink->setText(mSelectedNodeName.AsChar());
            }
            else
            {
                mNodeLink->setText("select node");
            }

            // get the number of attributes and iterate through them
            const uint32 numAttributes = mAttributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                static_cast<MCore::AttributeString*>(mAttributes[i])->SetValue(mSelectedNodeName.AsChar());
                if (mAttribChangedFunc)
                {
                    mAttribChangedFunc(mAttributes[i], mAttributeSettings);
                }
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // when the selection succeeded
    void NodeSelectionAttributeWidget::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        mSelectedNodeName.Clear();

        // check if selection is valid
        if (selection.GetLength() == 0)
        {
            return;
        }

        const uint32 actorInstanceID = selection[0].mActorInstanceID;
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        const uint32 numSelected = selection.GetLength();
        if (numSelected == 1)
        {
            mSelectedNodeName = selection[0].GetNodeName();
        }
    }


    // when the node selection got cancelled
    void NodeSelectionAttributeWidget::OnNodeSelectionCancelled()
    {
        mSelectedNodeName.Clear();
    }

    //-----------------------------------------------------------------------------------------------------------------

    NodeAndMorphAttributeWidget::NodeAndMorphAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mNodeSelectionWindow    = new NodeSelectionWindow(this, false);
        mSelectionList          = new CommandSystem::SelectionList();

        connect(mNodeSelectionWindow,                           SIGNAL(rejected()),                                    this, SLOT(OnNodeSelectionCancelled()));
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),  this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)));

        if (mFirstAttribute)
        {
            MCore::AttributeArray* mask = static_cast<MCore::AttributeArray*>(mFirstAttribute);
            mLink = CreateMaskLink(mask);
            if (mLink->toolTip().length() == 0)
            {
                mLink->setToolTip(attributeSettings->GetDescription());
            }

            mLink->setEnabled(!readOnly);

            connect(mLink, SIGNAL(clicked()), this, SLOT(OnLinkClicked()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);
            mLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mLink, 0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);

            CreateStandardLayout(helperWidget, attributeSettings);
        }
    }


    // create the mask link
    MysticQt::LinkWidget* NodeAndMorphAttributeWidget::CreateMaskLink(MCore::AttributeArray* mask)
    {
        MysticQt::LinkWidget* nodeLink = nullptr;

        // if there is no mask setup yet
        if (mask == nullptr)
        {
            nodeLink = new MysticQt::LinkWidget("setup mask");
        }
        else
        {
            /*      // if no nodes are setup in the mask
                    if (nodeMask->GetNumNodes() == 0)
                        nodeLink = new MysticQt::LinkWidget("select nodes");
                    else
                    {
                        // show how many nodes there are and setup the tooltip
                        MCore::String labelText;
                        labelText.Reserve( 1024 * 100 );
                        labelText.Format("%d nodes inside mask", nodeMask->GetNumNodes());
                        nodeLink = new MysticQt::LinkWidget( labelText.AsChar() );

                        labelText.Clear();
                        const uint32 numNodes = nodeMask->GetNumNodes();
                        for (uint32 i=0; i<numNodes; ++i)
                        {
                            if (i < numNodes-1)
                            {
                                labelText += nodeMask->GetNode(i).mName;
                                labelText += "\n";
                            }
                            else
                                labelText += nodeMask->GetNode(i).mName;
                        }

                        // update the tooltip text
                        nodeLink->setToolTip( labelText.AsChar() );
                    }*/
        }

        // setup the size policy
        nodeLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return nodeLink;
    }


    NodeAndMorphAttributeWidget::~NodeAndMorphAttributeWidget()
    {
        delete mSelectionList;
    }



    void NodeAndMorphAttributeWidget::OnResetSelection()
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeArray* nodeMask = static_cast<MCore::AttributeArray*>(mAttributes[i]);
            nodeMask->InitFromString("");
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        mLink->setText("setup mask");
        FireValueChangedSignal();
        UpdateInterface();
    }


    // find an entry by name
    uint32 NodeAndMorphAttributeWidget::FindMaskEntryIndexByNodeName(MCore::AttributeArray* mask, const char* nodeName)
    {
        if (mask == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        MCORE_ASSERT(mask->GetNumAttributes() == 2);
        MCORE_ASSERT(mask->GetAttribute(ATTRIBUTEARRAY_NODES)->GetType() == MCore::AttributeArray::TYPE_ID);
        MCore::AttributeArray* stringArray = static_cast<MCore::AttributeArray*>(mask->GetAttribute(ATTRIBUTEARRAY_NODES));
        const uint32 numItems = stringArray->GetNumAttributes();
        for (uint32 i = 0; i < numItems; ++i)
        {
            MCore::AttributeString* name = static_cast<MCore::AttributeString*>(stringArray->GetAttribute(i));
            if (name->GetValue().CheckIfIsEqual(nodeName))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find an entry by name
    uint32 NodeAndMorphAttributeWidget::FindMaskEntryIndexByMorphName(MCore::AttributeArray* mask, const char* morphName)
    {
        if (mask == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        MCORE_ASSERT(mask->GetNumAttributes() == 2);
        MCORE_ASSERT(mask->GetAttribute(ATTRIBUTEARRAY_MORPHS)->GetType() == MCore::AttributeArray::TYPE_ID);
        MCore::AttributeArray* stringArray = static_cast<MCore::AttributeArray*>(mask->GetAttribute(ATTRIBUTEARRAY_MORPHS));
        const uint32 numItems = stringArray->GetNumAttributes();
        for (uint32 i = 0; i < numItems; ++i)
        {
            MCore::AttributeString* name = static_cast<MCore::AttributeString*>(stringArray->GetAttribute(i));
            if (name->GetValue().CheckIfIsEqual(morphName))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // when we want to select a bunch of nodes
    void NodeAndMorphAttributeWidget::OnLinkClicked()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            QMessageBox::warning(this, "Selection Invalid", "Cannot open node selection window. Please make sure exactly one actor instance is selected.");
            return;
        }
        /*
            // get the old and current node mask and set the selection of the selection window to it
            EMotionFX::AttributeNodeMask*   oldNodeMask     = static_cast<EMotionFX::AttributeNodeMask*>( mFirstAttribute );
            EMotionFX::Actor*               actor           = actorInstance->GetActor();

            mSelectionList->Clear();
            mSelectionList->AddActorInstance(actorInstance);

            if (oldNodeMask)
            {
                // iterate over the node mask and add all nodes to the selection list
                const uint32 numOldMaskNodes = oldNodeMask->GetNumNodes();
                for (uint32 j=0; j<numOldMaskNodes; ++j)
                {
                    EMotionFX::Node* node = actor->FindNodeByName( oldNodeMask->GetNode(j).mName.AsChar() );
                    if (node)
                        mSelectionList->AddNode(node);
                }
            }

            // show the node selection window
            mNodeSelectionWindow->Update( actorInstance->GetID(), mSelectionList );
            mNodeSelectionWindow->setModal( true );
            if (mNodeSelectionWindow->exec() != QDialog::Rejected)      // we pressed cancel or the close cross
            {
                uint32 i;
                uint32 numSelectedNodes = mNodeSelection.GetLength();

                EMotionFX::AttributeNodeMask* nodeMask = static_cast<EMotionFX::AttributeNodeMask*>( mFirstAttribute );

                // update the node mask
                // remove the ones that got unselected
                for (i=0; i<nodeMask->GetNumNodes();)
                {
                    const MCore::String& nodeName = nodeMask->GetNode(i).mName;

                    // skip node names that are not inside the current actor
                    if (actor->FindNodeByName( nodeName.AsChar() ) == nullptr)
                    {
                        i++;
                        continue;
                    }

                    // check if the given node name of the mask is selected
                    bool isSelected = false;
                    if (mNodeSelection.Find(nodeName) != MCORE_INVALIDINDEX32)
                        isSelected = true;

                    if (isSelected)
                    {
                        i++;
                        continue;
                    }

                    // if the node is not selected remove it
                    const uint32 maskEntryIndex = FindMaskEntryIndexByNodeName( nodeMask, nodeName.AsChar() );
                    if  (maskEntryIndex != MCORE_INVALIDINDEX32)
                        nodeMask->GetNodes().Remove(maskEntryIndex);
                    else
                        i++;
                }

                // add the newly selected nodes
                for (i=0; i<numSelectedNodes; ++i)
                {
                    const uint32 maskEntryIndex = FindMaskEntryIndexByNodeName( nodeMask, mNodeSelection[i].AsChar() );
                    if (maskEntryIndex == MCORE_INVALIDINDEX32)
                    {
                        nodeMask->GetNodes().AddEmpty();
                        nodeMask->GetNodes().GetLast().mName = mNodeSelection[i];
                        nodeMask->GetNodes().GetLast().mWeight = 0.0f;
                    }
                }

                const uint32 numMaskNodes = nodeMask->GetNumNodes();

                // update the link name
                MCore::String htmlLink;
                if (numMaskNodes > 0)
                    htmlLink.Format("%d nodes inside mask", numMaskNodes);
                else
                    htmlLink.Format("select nodes");

                mNodeLink->setText( htmlLink.AsChar() );

                // build the tooltip text
                htmlLink.Clear();
                htmlLink.Reserve(16384);
                for (i=0; i<numMaskNodes; ++i)
                {
                    if (i < numMaskNodes-1)
                    {
                        htmlLink += nodeMask->GetNode(i).mName;
                        htmlLink += "\n";
                    }
                    else
                        htmlLink += nodeMask->GetNode(i).mName;
                }

                // update the tooltip text
                mNodeLink->setToolTip( htmlLink.AsChar() );
            }

            FireValueChangedSignal();
            UpdateInterface();
        */
    }


    // when the selection succeeded
    void NodeAndMorphAttributeWidget::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        mNodeSelection.Clear();

        // check if selection is valid
        if (selection.GetLength() == 0)
        {
            return;
        }

        const uint32 actorInstanceID = selection[0].mActorInstanceID;
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        const uint32 numSelected = selection.GetLength();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            if (selection[i].GetNodeNameString().GetIsEmpty() == false)
            {
                mNodeSelection.Add(selection[i].GetNodeName());
            }
        }
    }


    // when the node selection got cancelled
    void NodeAndMorphAttributeWidget::OnNodeSelectionCancelled()
    {
        mNodeSelection.Clear();
    }

    //-----------------------------------------------------------------------------------------------------------------


    GoalNodeSelectionAttributeWidget::GoalNodeSelectionAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mNodeSelectionWindow        = new NodeSelectionWindow(this, true);
        mSelectionList              = new CommandSystem::SelectionList();
        mSelectedNodeActorInstanceID = MCORE_INVALIDINDEX32;

        connect(mNodeSelectionWindow,                           SIGNAL(rejected()),                                    this, SLOT(OnNodeSelectionCancelled()));
        connect(mNodeSelectionWindow->GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),  this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)));


        if (mFirstAttribute)
        {
            const char* value = (mFirstAttribute) ? static_cast<EMotionFX::AttributeGoalNode*>(mFirstAttribute)->GetNodeName() : "";
            mGoalLink = CreateNodeNameLink(value);

            mGoalLink->setEnabled(!readOnly);
            connect(mGoalLink, SIGNAL(clicked()), this, SLOT(OnNodeNames()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mGoalLink, 0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);

            CreateStandardLayout(helperWidget, attributeSettings);
        }
    }


    void GoalNodeSelectionAttributeWidget::OnResetSelection()
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeString* nodeNameAttribute = static_cast<MCore::AttributeString*>(mAttributes[i]);
            nodeNameAttribute->SetValue("");
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        mGoalLink->setText("select node");
        FireValueChangedSignal();
        UpdateInterface();
    }


    GoalNodeSelectionAttributeWidget::~GoalNodeSelectionAttributeWidget()
    {
        delete mSelectionList;
    }

    /*
    void GoalNodeSelectionAttributeWidget::OnResetSelection()
    {
        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i=0; i<numAttributes; ++i)
        {
            static_cast<EMotionFX::AttributeGoalNode*>(mAttributes[i])->SetNodeName( "" );
            static_cast<EMotionFX::AttributeGoalNode*>(mAttributes[i])->SetParentDepth( 0 );
        }

        mLabel->setText("select node");
    }*/


    // create a node name link
    MysticQt::LinkWidget* GoalNodeSelectionAttributeWidget::CreateNodeNameLink(const char* nodeName)
    {
        MysticQt::LinkWidget* nodeLink = nullptr;

        // if there is no mask setup yet
        if (nodeName == nullptr || MCore::String(nodeName).GetIsEmpty())
        {
            nodeLink = new MysticQt::LinkWidget("select node");
        }
        else
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance && actorInstance->GetAnimGraphInstance())
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

                EMotionFX::AttributeGoalNode* goalNodeAttrib = static_cast<EMotionFX::AttributeGoalNode*>(mFirstAttribute);
                if (goalNodeAttrib)
                {
                    MCore::String linkText;
                    EMotionFX::ActorInstance* parentInstance = animGraphInstance->FindActorInstanceFromParentDepth(goalNodeAttrib->GetParentDepth());
                    if (parentInstance)
                    {
                        const MCore::String& fileName = parentInstance->GetActor()->GetFileName();
                        linkText.Format("%s (%s)", nodeName, fileName.ExtractFileName().AsChar());
                        nodeLink = new MysticQt::LinkWidget(linkText.AsChar());
                    }
                    else
                    {
                        nodeLink = new MysticQt::LinkWidget(nodeName);
                    }
                }
                else
                {
                    nodeLink = new MysticQt::LinkWidget(nodeName);
                }
            }
            else
            {
                nodeLink = new MysticQt::LinkWidget(nodeName);
            }
        }

        // setup the size policy
        nodeLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return nodeLink;
    }


    // when we want to select a bunch of nodes
    void GoalNodeSelectionAttributeWidget::OnNodeNames()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            QMessageBox::warning(this, "Selection Invalid", "Cannot open node selection window. Please make sure exactly one actor instance is selected.");
            return;
        }

        // get the old and current node name and set the selection of the selection window to it
        MCore::String       oldNodeName = (mFirstAttribute) ? static_cast<EMotionFX::AttributeGoalNode*>(mFirstAttribute)->GetNodeName() : "";
        EMotionFX::Actor*   actor       = actorInstance->GetActor();

        mSelectionList->Clear();

        // add the old selected node to the selection list
        if (oldNodeName.GetIsEmpty() == false)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByName(oldNodeName.AsChar());
            if (node)
            {
                mSelectionList->AddNode(node);
            }
        }

        // add the actor instance to the selection window
        MCore::Array<uint32> actorInstanceIDs;

        // add the current actor instance and all the ones it is attached to
        EMotionFX::ActorInstance* curInstance = actorInstance;
        //EMotionFX::Attachment* attachment = actorInstance->GetSelfAttachment();
        while (curInstance)
        {
            actorInstanceIDs.Add(curInstance->GetID());
            EMotionFX::Attachment* attachment = curInstance->GetSelfAttachment();
            //      EMotionFX::ActorInstance* attachTo = nullptr;
            if (attachment)
            {
                curInstance = attachment->GetAttachToActorInstance();
            }
            else
            {
                curInstance = nullptr;
            }
        }

        // show the node selection window
        mNodeSelectionWindow->Update(actorInstanceIDs, mSelectionList);
        mNodeSelectionWindow->setModal(true);
        if (mNodeSelectionWindow->exec() != QDialog::Rejected)  // we pressed cancel or the close cross
        {
            uint32 parentDepth = actorInstanceIDs.Find(mSelectedNodeActorInstanceID);
            MCORE_ASSERT(parentDepth != MCORE_INVALIDINDEX32);  // the selected actor instance must be one of the ones we showed in the list

            if (mSelectedNodeName.GetIsEmpty() == false)
            {
                MCore::String linkText;
                const MCore::String& actorFile = EMotionFX::GetActorManager().FindActorInstanceByID(mSelectedNodeActorInstanceID)->GetActor()->GetFileName();
                linkText.Format("%s (%s)", mSelectedNodeName.AsChar(), actorFile.ExtractFileName().AsChar());
                mGoalLink->setText(linkText.AsChar());
            }
            else
            {
                mGoalLink->setText("select node");
            }

            // get the number of attributes and iterate through them
            const uint32 numAttributes = mAttributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                static_cast<EMotionFX::AttributeGoalNode*>(mAttributes[i])->SetNodeName(mSelectedNodeName.AsChar());
                static_cast<EMotionFX::AttributeGoalNode*>(mAttributes[i])->SetParentDepth(parentDepth);

                if (mAttribChangedFunc)
                {
                    mAttribChangedFunc(mAttributes[i], mAttributeSettings);
                }
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }


    // when the selection succeeded
    void GoalNodeSelectionAttributeWidget::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        mSelectedNodeName.Clear();
        mSelectedNodeActorInstanceID = MCORE_INVALIDINDEX32;

        // check if selection is valid
        if (selection.GetLength() == 0)
        {
            return;
        }

        const uint32 actorInstanceID = selection[0].mActorInstanceID;
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        const uint32 numSelected = selection.GetLength();
        if (numSelected == 1)
        {
            mSelectedNodeName               = selection[0].GetNodeName();
            mSelectedNodeActorInstanceID    = selection[0].mActorInstanceID;
        }
    }


    // when the node selection got cancelled
    void GoalNodeSelectionAttributeWidget::OnNodeSelectionCancelled()
    {
        mSelectedNodeName.Clear();
    }


    //-----------------------------------------------------------------------------------------------------------------
    /*
    MotionExtractionComponentWidget::MotionExtractionComponentWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode) : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode)
    {
        mButtonGroup = new ButtonGroup(this, 2, 3);
        CreateStandardLayout(mButtonGroup, attributeInfo);

        MCore::AttributeInt32 defaultValue;
        MCore::AttributeInt32* value = (mFirstAttribute) ? static_cast<MCore::AttributeInt32*>(mFirstAttribute) : &defaultValue;

        mPosX = mButtonGroup->GetButton(0, 0);
        mPosY = mButtonGroup->GetButton(0, 1);
        mPosZ = mButtonGroup->GetButton(0, 2);

        mRotX = mButtonGroup->GetButton(1, 0);
        mRotY = mButtonGroup->GetButton(1, 1);
        mRotZ = mButtonGroup->GetButton(1, 2);

        mPosX->setText("Pos X");
        mPosY->setText("Pos Y");
        mPosZ->setText("Pos Z");

        mRotX->setText("Rot X");
        mRotY->setText("Rot Y");
        mRotZ->setText("Rot Z");

        const int32 mask = value->GetValue();

        mPosX->setChecked( mask & EMotionFX::MOTIONEXTRACT_POSITION_X );
        mPosY->setChecked( mask & EMotionFX::MOTIONEXTRACT_POSITION_Y );
        mPosZ->setChecked( mask & EMotionFX::MOTIONEXTRACT_POSITION_Z );

        mRotX->setChecked( mask & EMotionFX::MOTIONEXTRACT_ROTATION_X );
        mRotY->setChecked( mask & EMotionFX::MOTIONEXTRACT_ROTATION_Y );
        mRotZ->setChecked( mask & EMotionFX::MOTIONEXTRACT_ROTATION_Z );

        connect(mPosX, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
        connect(mPosY, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
        connect(mPosZ, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
        connect(mRotX, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
        connect(mRotY, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
        connect(mRotZ, SIGNAL(clicked()), this, SLOT(UpdateMotionExtractionMask()));
    }


    void MotionExtractionComponentWidget::UpdateMotionExtractionMask()
    {
        EMotionFX::EMotionExtractionMask mask = EMotionFX::MotionInstance::ConstructMotionExtractionMask(   mPosX->isChecked(),
                                                                                                            mPosY->isChecked(),
                                                                                                            mPosZ->isChecked(),
                                                                                                            mRotX->isChecked(),
                                                                                                            mRotY->isChecked(),
                                                                                                            mRotZ->isChecked() );

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i=0; i<numAttributes; ++i)
        {
            MCore::AttributeInt32* value = static_cast<MCore::AttributeInt32*>( mAttributes[i] );
            value->SetValue( (int32)mask );
        }

        UpdateInterface();
    }
    */

    //-----------------------------------------------------------------------------------------------------------------

    ParameterNamesAttributeWidget::ParameterNamesAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mParameterSelectionWindow = new ParameterSelectionWindow(this, false);

        connect(mParameterSelectionWindow,                         SIGNAL(rejected()),                   this, SLOT(OnSelectionCancelled()));
        connect(mParameterSelectionWindow->GetParameterWidget(),   &ParameterWidget::OnSelectionDone,    this, &ParameterNamesAttributeWidget::OnParametersSelected);

        if (mFirstAttribute)
        {
            //EMotionFX::AttributeParameterMask* parameterMask = static_cast<EMotionFX::AttributeParameterMask*>( mFirstAttribute );
            mParameterLink = new QLabel();
            mParameterLink->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            UpdateMask(false);

            mParameterLink->setEnabled(!readOnly);

            connect(mParameterLink, SIGNAL(linkActivated(QString)), this, SLOT(OnParameterNames()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset selection");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetSelection()));

            // the shrink button
            mShrinkButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mShrinkButton, "/Images/Icons/Cut.png", "Shrink the parameter mask to the ports that are actually connected.");
            connect(mShrinkButton, SIGNAL(clicked()), this, SLOT(OnShinkButton()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);
            mParameterLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mParameterLink,         0, Qt::AlignLeft);
            helperLayout->addWidget(mShrinkButton,          0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton,  0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);

            CreateStandardLayout(helperWidget, attributeSettings);
        }
    }


    ParameterNamesAttributeWidget::~ParameterNamesAttributeWidget()
    {
    }



    void ParameterNamesAttributeWidget::OnResetSelection()
    {
        mSelectedParameters.clear();
        UpdateMask();

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // TODO: why false here?
        //UpdateInterface(false);
        UpdateInterface();
    }


    void ParameterNamesAttributeWidget::OnShinkButton()
    {
        EMotionFX::AnimGraphObject* object = mPlugin->GetAttributesWindow()->GetObject();
        if (object->GetType() != EMotionFX::BlendTreeParameterNode::TYPE_ID)
        {
            return;
        }

        // Get the names of the parameters that are connected to other nodes. These are the ones that will survive the shrinking process.
        EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(object);
        parameterNode->CalcConnectedParameterNames(mSelectedParameters);

        UpdateMask();

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // TODO: why false here?
        //UpdateInterface(false);
        UpdateInterface();
    }


    uint32 ParameterNamesAttributeWidget::FindMaskEntryIndexByParameterName(EMotionFX::AttributeParameterMask* mask, const char* parameterName)
    {
        if (mask == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        const uint32 numParameters = mask->GetNumParameterNames();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            if (mask->GetParameterNameString(i).CheckIfIsEqual(parameterName))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // when we want to select a bunch of nodes
    void ParameterNamesAttributeWidget::OnParameterNames()
    {
        // get the active animgraph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            QMessageBox::warning(this, "Selection Invalid", "Cannot open parameter selection window. Please make sure a anim graph is activated.");
            return;
        }

        // get the old and current mask and set the selection of the selection window to it
        EMotionFX::AttributeParameterMask* oldMask = static_cast<EMotionFX::AttributeParameterMask*>(mFirstAttribute);

        mSelectedParameters.clear();

        if (oldMask)
        {
            // iterate over the mask and add all parameters to the the current selection
            const uint32 numOldParameters = oldMask->GetNumParameterNames();
            for (uint32 j = 0; j < numOldParameters; ++j)
            {
                mSelectedParameters.push_back(oldMask->GetParameterName(j));
            }
        }

        // show the selection window
        mParameterSelectionWindow->Update(animGraph, mSelectedParameters);
        mParameterSelectionWindow->setModal(true);
        if (mParameterSelectionWindow->exec() != QDialog::Rejected) // we pressed cancel or the close cross
        {
            UpdateMask();
        }

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        // TODO: why false here?
        //UpdateInterface(false);
        UpdateInterface();
    }


    void ParameterNamesAttributeWidget::UpdateMask(bool modifyMaskAttribute)
    {
        // get the active animgraph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            mParameterLink->setText(GetManager()->ConstructHTMLLink("select parameters"));
            mParameterLink->setToolTip("");
            mSelectedParameters.clear();
            return;
        }

        uint32 i;
        EMotionFX::AttributeParameterMask* mask = static_cast<EMotionFX::AttributeParameterMask*>(mFirstAttribute);

        if (modifyMaskAttribute)
        {
            const uint32 numSelectedParameters = static_cast<uint32>(mSelectedParameters.size());

            // update the mask

            // remove the ones that got unselected
            AZStd::string parameterName;
            for (i = 0; i < mask->GetNumParameterNames(); )
            {
                parameterName = mask->GetParameterName(i);

                // Skip parameter names that are not inside the current anim graph.
                if (!animGraph->FindParameter(parameterName.c_str()))
                {
                    i++;
                    continue;
                }

                // check if the given parameter name of the mask is selected
                bool isSelected = false;
                if (AZStd::find(mSelectedParameters.begin(), mSelectedParameters.end(), parameterName) != mSelectedParameters.end())
                {
                    isSelected = true;
                }

                if (isSelected)
                {
                    i++;
                    continue;
                }

                // if the parameter is not selected remove it
                const uint32 maskEntryIndex = FindMaskEntryIndexByParameterName(mask, parameterName.c_str());
                if  (maskEntryIndex != MCORE_INVALIDINDEX32)
                {
                    mask->RemoveParameterName(maskEntryIndex);
                }
                else
                {
                    i++;
                }
            }

            // add the newly selected parameters
            for (i = 0; i < numSelectedParameters; ++i)
            {
                const uint32 maskEntryIndex = FindMaskEntryIndexByParameterName(mask, mSelectedParameters[i].c_str());
                if (maskEntryIndex == MCORE_INVALIDINDEX32)
                {
                    mask->AddParameterName(mSelectedParameters[i].c_str());
                }
            }
        }

        const uint32 numMaskParameters = mask->GetNumParameterNames();

        // update the link name
        MCore::String labelText;
        labelText.Reserve(1024 * 100);
        if (numMaskParameters > 0)
        {
            labelText.Format("%d parameters inside mask", numMaskParameters);
        }
        else
        {
            labelText.Format("select parameters");
        }

        mParameterLink->setText(GetManager()->ConstructHTMLLink(labelText.AsChar()));

        // build the tooltip text
        labelText.Clear();
        for (i = 0; i < numMaskParameters; ++i)
        {
            if (i < numMaskParameters - 1)
            {
                labelText += mask->GetParameterName(i);
                labelText += "\n";
            }
            else
            {
                labelText += mask->GetParameterName(i);
            }
        }

        // update the tooltip text
        mParameterLink->setToolTip(labelText.AsChar());
    }


    // when the selection succeeded
    void ParameterNamesAttributeWidget::OnParametersSelected(const AZStd::vector<AZStd::string>& selectedItems)
    {
        mSelectedParameters = selectedItems;
    }


    // when the node selection got cancelled
    void ParameterNamesAttributeWidget::OnSelectionCancelled()
    {
        mSelectedParameters.clear();
    }


    //-----------------------------------------------------------------------------------------------------------------

    StateFilterLocalAttributeWidget::StateFilterLocalAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mSelectionWindow = new StateFilterSelectionWindow(mPlugin, this);

        //MCore::LogError( "%s", mFirstAttribute->GetTypeString() );

        if (mFirstAttribute)
        {
            //      EMotionFX::AttributeStateFilterLocal* attributeStateFilter = static_cast<EMotionFX::AttributeStateFilterLocal*>( mFirstAttribute );
            mNodeLink = new MysticQt::LinkWidget("select states");
            mNodeLink->setEnabled(!readOnly);

            connect(mNodeLink, SIGNAL(clicked()), this, SLOT(OnSelectNodeGroups()));

            // the reset selection button
            mResetSelectionButton = new QPushButton();
            EMStudioManager::MakeTransparentButton(mResetSelectionButton, "/Images/Icons/SearchClearButton.png", "Reset");
            connect(mResetSelectionButton, SIGNAL(clicked()), this, SLOT(OnResetGroupSelection()));

            // create the widget and the layout for the link and the reset button
            QWidget*        helperWidget = new QWidget();
            QHBoxLayout*    helperLayout = new QHBoxLayout();

            // adjust some layout settings
            helperLayout->setSpacing(0);
            helperLayout->setMargin(0);
            mNodeLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            // add the node link and the reset button to the layout and set it as the helper widget's layout
            helperLayout->addWidget(mNodeLink, 0, Qt::AlignLeft);
            helperLayout->addWidget(mResetSelectionButton, 0, Qt::AlignLeft);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            helperLayout->addWidget(spacerWidget);
            helperWidget->setLayout(helperLayout);
            spacerWidget->setToolTip(attributeSettings->GetDescription());

            CreateStandardLayout(helperWidget, attributeSettings);
            UpdateLink();
        }
    }


    // update the node link
    void StateFilterLocalAttributeWidget::UpdateLink()
    {
        // get the currently active anim graph and currently shown graph
        EMotionFX::AnimGraph*      animGraph  = mPlugin->GetActiveAnimGraph();
        EMotionFX::AnimGraphNode*  parentNode  = mPlugin->GetGraphWidget()->GetCurrentNode();
        if (animGraph == nullptr || parentNode == nullptr)
        {
            QMessageBox::warning(this, "No Valid Selection", "Cannot open state filter selection window. Please make sure a state machine is shown in the graph window.");
            return;
        }

        EMotionFX::AttributeStateFilterLocal* attributeStateFilter = static_cast<EMotionFX::AttributeStateFilterLocal*>(mFirstAttribute);

        // calculate the number of unique states selected within the node groups and individually selected nodes
        const uint32 numStates = attributeStateFilter->CalcNumTotalStates(animGraph, parentNode);

        // update the link name
        MCore::String htmlLink;
        if (attributeStateFilter->GetIsEmpty() == false)
        {
            if (numStates == 1)
            {
                htmlLink.Format("1 state", numStates);
            }
            else
            {
                htmlLink.Format("%d states", numStates);
            }

            mNodeLink->setText(htmlLink.AsChar());
        }
        else
        {
            mNodeLink->setText("select states");
            mNodeLink->setToolTip("Transitioning allowed from all states");
        }

        // build the tooltip text
        htmlLink.Clear();
        htmlLink.Reserve(16384);

        // add the node names
        const uint32 numNodes = attributeStateFilter->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            htmlLink += attributeStateFilter->GetNodeName(i);
            htmlLink += "\n";
        }

        // add the node groups names
        const uint32 numGroups = attributeStateFilter->GetNumNodeGroups();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (i < numGroups - 1)
            {
                htmlLink += "[";
                htmlLink += attributeStateFilter->GetNodeGroupName(i);
                htmlLink += "]\n";
            }
            else
            {
                htmlLink += "[";
                htmlLink += attributeStateFilter->GetNodeGroupName(i);
                htmlLink += "]";
            }
        }

        // update the tooltip text
        mNodeLink->setToolTip(htmlLink.AsChar());
    }


    // destructor
    StateFilterLocalAttributeWidget::~StateFilterLocalAttributeWidget()
    {
    }


    // unselect all node groups
    void StateFilterLocalAttributeWidget::OnResetGroupSelection()
    {
        mSelectedGroupNames.Clear();
        mSelectedNodeNames.Clear();

        // get the number of attributes and iterate through them
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::AttributeStateFilterLocal* attributeStateFilter = static_cast<EMotionFX::AttributeStateFilterLocal*>(mAttributes[i]);
            attributeStateFilter->InitFromString("");
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        UpdateLink();
        FireValueChangedSignal();
        UpdateInterface();
    }


    // when we want to select a bunch of nodes
    void StateFilterLocalAttributeWidget::OnSelectNodeGroups()
    {
        // get the currently active anim graph and currently shown graph
        EMotionFX::AnimGraph*      animGraph  = mPlugin->GetActiveAnimGraph();
        EMotionFX::AnimGraphNode*  parentNode  = mPlugin->GetGraphWidget()->GetCurrentNode();
        if (animGraph == nullptr || parentNode == nullptr)
        {
            QMessageBox::warning(this, "No Valid Selection", "Cannot open state filter selection window. Please make sure a state machine is shown in the graph window.");
            return;
        }

        // get the node group mask attribute
        EMotionFX::AttributeStateFilterLocal* oldMask = static_cast<EMotionFX::AttributeStateFilterLocal*>(mFirstAttribute);

        // get the number of nodes and node groups inside the mask, iterate through them and add them to the arrays
        mSelectedGroupNames.Clear();
        mSelectedNodeNames.Clear();

        const uint32 numNodeGroups = oldMask->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            mSelectedGroupNames.Add(oldMask->GetNodeGroupName(i));
        }

        const uint32 numNodes = oldMask->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mSelectedNodeNames.Add(oldMask->GetNodeName(i));
        }


        // create the dialog
        StateFilterSelectionWindow dialog(mPlugin, this);
        dialog.ReInit(animGraph, mSelectedNodeNames, mSelectedGroupNames);
        if (dialog.exec() != QDialog::Rejected)
        {
            // set the new selection back to the attribute
            EMotionFX::AttributeStateFilterLocal* attributeStateFilter = static_cast<EMotionFX::AttributeStateFilterLocal*>(mFirstAttribute);
            attributeStateFilter->SetNodeGroupNames(dialog.GetSelectedGroupNames());
            attributeStateFilter->SetNodeNames(dialog.GetSelectedNodeNames());

            // update the link accordingly
            UpdateLink();
        }

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();
    }

    //-----------------------------------------------------------------------------------------------------------------

    ButtonAttributeWidget::ButtonAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();

        mButton = new QPushButton();
        CreateStandardLayout(mButton, "Remove condition");
    }

    ButtonAttributeWidget::~ButtonAttributeWidget()
    {
    }

    //-----------------------------------------------------------------------------------------------------------------

    TagPickerAttributeWidget::TagPickerAttributeWidget(const MCore::Array<MCore::Attribute*> attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func)
        : MysticQt::AttributeWidget(attributes, attributeSettings, customData, readOnly, creationMode, func)
    {
        // search for the anim graph plugin and store a link to it
        mPlugin = AttributeWidgetFindAnimGraphPlugin();
        mNeedsHeavyUpdate = true;

        // Get the list of available tags.
        QVector<QString> availableTags;
        GetAvailableTags(availableTags);

        // Create the tag selector widget.
        m_tagSelector = new AzQtComponents::TagSelector(availableTags, this);

        // Get the tag strings from the array of string attributes for selection.
        QVector<QString> tagStrings;
        GetSelectedTags(tagStrings);

        // Pre-select tags.
        m_tagSelector->SelectTags(tagStrings);

        m_tagSelector->setEnabled(!readOnly);
        connect(m_tagSelector, &AzQtComponents::TagSelector::TagsChanged, [this]{ OnTagsChanged(); });

        CreateStandardLayout(m_tagSelector, attributeSettings);
    }


    void TagPickerAttributeWidget::GetAvailableTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        const AZ::u32 numParameters = animGraph->GetNumParameters();
        for (AZ::u32 i = 0; i < numParameters; ++i)
        {
            MCore::AttributeSettings* parameter = animGraph->GetParameter(i);

            if (parameter->GetInterfaceType() == EMotionFX::ATTRIBUTE_INTERFACETYPE_TAG)
            {
                outTags.push_back(parameter->GetName());
            }
        }
    }


    void TagPickerAttributeWidget::GetSelectedTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[i]);

            const AZ::u32 numArrayAttributes = arrayAttrib->GetNumAttributes();
            for (AZ::u32 j = 0; j < numArrayAttributes; ++j)
            {
                MCore::AttributeString* stringAttribute = static_cast<MCore::AttributeString*>(arrayAttrib->GetAttribute(j));
                outTags.push_back(stringAttribute->GetValue().AsChar());
            }
        }
    }


    void TagPickerAttributeWidget::OnTagsChanged()
    {
        // Get the currently selected tag strings from the widget.
        QVector<QString> tagStrings;
        m_tagSelector->GetSelectedTagStrings(tagStrings);
        const int numTags = tagStrings.count();

        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            MCore::AttributeArray* arrayAttrib = static_cast<MCore::AttributeArray*>(mAttributes[i]);
            arrayAttrib->Clear();

            for (int t = 0; t < numTags; ++t)
            {
                MCore::AttributeString* newStringAttrib = MCore::AttributeString::Create(tagStrings[t].toUtf8().data());
                arrayAttrib->AddAttribute(newStringAttrib, false);
            }

            if (mAttribChangedFunc)
            {
                mAttribChangedFunc(mAttributes[i], mAttributeSettings);
            }
        }

        FireValueChangedSignal();
        UpdateInterface();

        // request reinit of the whole attribute window
        emit RequestParentReInit();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributeWidget.moc>
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

#ifndef __DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR_H
#define __DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include "AttributeWidget.h"
#include <MysticQt/Source/AttributeWidgetCreators.h>
#include <MCore/Source/AttributeArray.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>


namespace EMStudio
{
    // easy creator class definition
#define DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(CLASSNAME, ATTRIBUTEWIDGETCLASSNAME, TYPEID, DEFAULTYPEID, MINMAXTYPEID, DATATYPEID, HASMINMAXVALUES, CANBEPARAMETER, TYPESTRING)                                                      \
    class CLASSNAME                                                                                                                                                                                                                   \
        : public MysticQt::AttributeWidgetCreator                                                                                                                                                                                     \
    {                                                                                                                                                                                                                                 \
    public:                                                                                                                                                                                                                           \
        enum { TYPE_ID = TYPEID };                                                                                                                                                                                                    \
        CLASSNAME()                                                                                                                                                                                                                   \
            : MysticQt::AttributeWidgetCreator()        { mInitialMinValue = nullptr; mInitialMaxValue = nullptr; }                                                                                                                   \
        ~CLASSNAME() {}                                                                                                                                                                                                               \
        uint32 GetType() const                      { return TYPEID; };                                                                                                                                                               \
        uint32 GetDefaultType() const               { return DEFAULTYPEID; };                                                                                                                                                         \
        uint32 GetMinMaxType() const                { return MINMAXTYPEID; };                                                                                                                                                         \
        uint32 GetDataType() const                  { return DATATYPEID; };                                                                                                                                                           \
        bool CanBeParameter() const                 { return CANBEPARAMETER; }                                                                                                                                                        \
        const char* GetTypeString() const           { return TYPESTRING; };                                                                                                                                                           \
        bool GetHasMinMaxValues() const             { return HASMINMAXVALUES; }                                                                                                                                                       \
        void InitAttributes(MCore::Array<MCore::Attribute*>&attributes, MCore::AttributeSettings* attributeSettings, bool forceInitMinMaxAttributes = false, bool resetMinMaxAttributes = true);                                      \
        MysticQt::AttributeWidget* Clone(MCore::Array<MCore::Attribute*>&attributes, MCore::AttributeSettings* attributeSettings, void* customData, bool readOnly, bool creationMode, const MysticQt::AttributeChangedFunction& func) \
        {                                                                                                                                                                                                                             \
            return new ATTRIBUTEWIDGETCLASSNAME(attributes, attributeSettings, customData, readOnly, creationMode, func);                                                                                                             \
        }                                                                                                                                                                                                                             \
    };

    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(FileBrowserAttributeWidgetCreator,               FileBrowserAttributeWidget,             MCore::ATTRIBUTE_INTERFACETYPE_FILEBROWSE,                      MCore::ATTRIBUTE_INTERFACETYPE_FILEBROWSE,                      MCore::ATTRIBUTE_INTERFACETYPE_FILEBROWSE,                      MCore::AttributeString::TYPE_ID,              false,  false,  "FileBrowser");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(Vector3AttributeWidgetCreator,                   Vector3AttributeWidget,                 MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO,                    MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO,                    MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO,                    MCore::AttributeVector3::TYPE_ID,             true,   true,   "Vector3");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(NodeNamesAttributeWidgetCreator,                 NodeNamesAttributeWidget,               EMotionFX::ATTRIBUTE_INTERFACETYPE_NODENAMES,                   EMotionFX::ATTRIBUTE_INTERFACETYPE_NODENAMES,                   EMotionFX::ATTRIBUTE_INTERFACETYPE_NODENAMES,                   EMotionFX::AttributeNodeMask::TYPE_ID,        false,  false,  "NodeNames");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(NodeSelectionAttributeWidgetCreator,             NodeSelectionAttributeWidget,           EMotionFX::ATTRIBUTE_INTERFACETYPE_NODESELECTION,               EMotionFX::ATTRIBUTE_INTERFACETYPE_NODESELECTION,               EMotionFX::ATTRIBUTE_INTERFACETYPE_NODESELECTION,               MCore::AttributeString::TYPE_ID,              false,  false,  "NodeSelection");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(GoalNodeSelectionAttributeWidgetCreator,         GoalNodeSelectionAttributeWidget,       EMotionFX::ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION,           EMotionFX::ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION,           EMotionFX::ATTRIBUTE_INTERFACETYPE_GOALNODESELECTION,           EMotionFX::AttributeGoalNode::TYPE_ID,        false,  false,  "GoalNodeSelection");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(MotionPickerAttributeWidgetCreator,              MotionPickerAttributeWidget,            EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONPICKER,                EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONPICKER,                EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONPICKER,                MCore::AttributeString::TYPE_ID,              false,  false,  "MotionPicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(MultipleMotionPickerAttributeWidgetCreator,      MultipleMotionPickerAttributeWidget,    EMotionFX::ATTRIBUTE_INTERFACETYPE_MULTIPLEMOTIONPICKER,        EMotionFX::ATTRIBUTE_INTERFACETYPE_MULTIPLEMOTIONPICKER,        EMotionFX::ATTRIBUTE_INTERFACETYPE_MULTIPLEMOTIONPICKER,        MCore::AttributeArray::TYPE_ID,               false,  false,  "MultipleMotionPicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(BlendSpaceMotionsAttributeWidgetCreator,         BlendSpaceMotionsAttributeWidget,       EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS,           EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS,           EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS,           MCore::AttributeArray::TYPE_ID,               false,  false,  "BlendSpaceMotions");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(BlendSpaceMotionPickerAttributeWidgetCreator,    BlendSpaceMotionPickerAttributeWidget,  EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONPICKER,      EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONPICKER,      EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONPICKER,      MCore::AttributeString::TYPE_ID,              false,  false,  "BlendSpaceMotionPicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(ParameterPickerAttributeWidgetCreator,           ParameterPickerAttributeWidget,         EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER,             EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER,             EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERPICKER,             MCore::AttributeString::TYPE_ID,              false,  false,  "ParameterPicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(BlendTreeMotionAttributeWidgetCreator,           BlendTreeMotionAttributeWidget,         EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER,   EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER,   EMotionFX::ATTRIBUTE_INTERFACETYPE_BLENDTREEMOTIONNODEPICKER,   MCore::AttributeString::TYPE_ID,              false,  false,  "BlendTreeMotionNodePicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(MotionEventTrackAttributeWidgetCreator,          MotionEventTrackAttributeWidget,        EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER,      EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER,      EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEVENTTRACKPICKER,      MCore::AttributeString::TYPE_ID,              false,  false,  "MotionEventTrackPicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(RotationAttributeWidgetCreator,                  RotationAttributeWidget,                EMotionFX::ATTRIBUTE_INTERFACETYPE_ROTATION,                    EMotionFX::ATTRIBUTE_INTERFACETYPE_ROTATION,                    EMotionFX::ATTRIBUTE_INTERFACETYPE_ROTATION,                    EMotionFX::AttributeRotation::TYPE_ID,        false,  true,   "Rotation");
    //DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR( MotionExtractionComponentWidgetCreator,       MotionExtractionComponentWidget,        EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEXTRACTIONCOMPONENTS,  EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEXTRACTIONCOMPONENTS,  EMotionFX::ATTRIBUTE_INTERFACETYPE_MOTIONEXTRACTIONCOMPONENTS,  MCore::AttributeInt32::TYPE_ID,               false,  true,   "MotionExtractionComponents" );
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(ParameterNamesAttributeWidgetCreator,            ParameterNamesAttributeWidget,          EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES,              EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES,              EMotionFX::ATTRIBUTE_INTERFACETYPE_PARAMETERNAMES,              EMotionFX::AttributeParameterMask::TYPE_ID,   false,  false,  "ParameterNames");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(AnimGraphStateAttributeWidgetCreator,            AnimGraphStateAttributeWidget,          EMotionFX::ATTRIBUTE_INTERFACETYPE_STATESELECTION,              EMotionFX::ATTRIBUTE_INTERFACETYPE_STATESELECTION,              EMotionFX::ATTRIBUTE_INTERFACETYPE_STATESELECTION,              MCore::AttributeString::TYPE_ID,              false,  false,  "StateSelection");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(StateFilterLocalAttributeWidgetCreator,          StateFilterLocalAttributeWidget,        EMotionFX::ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL,            EMotionFX::ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL,            EMotionFX::ATTRIBUTE_INTERFACETYPE_STATEFILTERLOCAL,            EMotionFX::AttributeStateFilterLocal::TYPE_ID, false,  false,  "StateFilterLocal");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(AnimGraphNodeAttributeWidgetCreator,             AnimGraphNodeAttributeWidget,           EMotionFX::ATTRIBUTE_INTERFACETYPE_ANIMGRAPHNODEPICKER,         EMotionFX::ATTRIBUTE_INTERFACETYPE_ANIMGRAPHNODEPICKER,         EMotionFX::ATTRIBUTE_INTERFACETYPE_ANIMGRAPHNODEPICKER,         MCore::AttributeString::TYPE_ID,              false,  false,  "AnimGraphNodePicker");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(TagAttributeWidgetCreator,                       MysticQt::CheckBoxAttributeWidget,      EMotionFX::ATTRIBUTE_INTERFACETYPE_TAG,                         EMotionFX::ATTRIBUTE_INTERFACETYPE_TAG,                         EMotionFX::ATTRIBUTE_INTERFACETYPE_TAG,                         MCore::AttributeFloat::TYPE_ID,               false,  true,   "Tag");
    DEFINE_ANIMGRAPHATTRIBUTEWIDGETCREATOR(TagPickerAttributeWidgetCreator,                 TagPickerAttributeWidget,               EMotionFX::ATTRIBUTE_INTERFACETYPE_TAGPICKER,                   EMotionFX::ATTRIBUTE_INTERFACETYPE_TAGPICKER,                   EMotionFX::ATTRIBUTE_INTERFACETYPE_TAGPICKER,                   MCore::AttributeArray::TYPE_ID,               false,  false,  "TagPicker");
}   // namespace EMStudio

#endif
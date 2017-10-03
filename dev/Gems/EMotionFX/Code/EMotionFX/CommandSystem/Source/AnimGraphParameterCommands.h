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

#include "CommandSystemConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/StringIDGenerator.h>
#include "CommandManager.h"


EMFX_FORWARD_DECLARE(AttributeParameterMask);
MCORE_FORWARD_DECLARE(Attribute);

namespace CommandSystem
{
    // Create a new anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateParameter, "Create a anim graph parameter", true)
        int32           mIndex;
        bool            mOldDirtyFlag;

    public:
        int32 GetIndex()        { return mIndex; }
    MCORE_DEFINECOMMAND_END


    // Remove a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveParameter, "Remove a anim graph parameter", true)
        AZStd::string   mName;
        uint32          mInterfaceType;
        AZStd::string   mMinValue;
        AZStd::string   mMaxValue;
        AZStd::string   mDefaultValue;
        AZStd::string   mDescription;
        AZStd::string   mOldParameterGroupName;
        int32           mIndex;
        bool            mOldDirtyFlag;
        bool            mIsScalable;

    public:
        int32 GetIndex()        { return mIndex; }
    MCORE_DEFINECOMMAND_END


    // Adjust a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustParameter, "Adjust a anim graph parameter", true)
        AZStd::string   mOldName;
        uint32          mOldInterfaceType;
        AZStd::string   mOldMinValue;
        AZStd::string   mOldMaxValue;
        AZStd::string   mOldDefaultValue;
        AZStd::string   mOldDescription;
        bool            mOldDirtyFlag;
        bool            mOldIsScalable;

    public:
        void RenameParameterNodePorts(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* startNode, const char* oldName, const char* newName);
        void UpdateTransitionConditions(const char* oldParameterName, const char* newParameterName);
    MCORE_DEFINECOMMAND_END


    // Switch a given anim graph parameter with another one.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphSwapParameters, "Swap anim graph parameter", true)
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct COMMANDSYSTEM_API ParameterConnectionItem
    {
        uint32  mTargetNodePort;

        void SetParameterNodeName(const char* name)         { mParameterNodeNameID  = MCore::GetStringIDGenerator().GenerateIDForString(name); }
        void SetTargetNodeName(const char* name)            { mTargetNodeNameID     = MCore::GetStringIDGenerator().GenerateIDForString(name); }
        void SetParameterName(const char* name)             { mParameterNameID      = MCore::GetStringIDGenerator().GenerateIDForString(name); }

        const char* GetParameterNodeName() const            { return MCore::GetStringIDGenerator().GetName(mParameterNodeNameID).AsChar(); }
        const char* GetTargetNodeName() const               { return MCore::GetStringIDGenerator().GetName(mTargetNodeNameID).AsChar(); }
        const char* GetParameterName() const                { return MCore::GetStringIDGenerator().GetName(mParameterNameID).AsChar(); }

    private:
        uint32  mParameterNodeNameID;
        uint32  mTargetNodeNameID;
        uint32  mParameterNameID;
    };

    COMMANDSYSTEM_API bool RemoveParametersCommand(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNames, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void RecreateOldConnections(EMotionFX::AnimGraph* animGraph, const AZStd::vector<ParameterConnectionItem>& oldParameterConnections, MCore::CommandGroup* commandGroup, const AZStd::vector<AZStd::string>& parametersToBeRemoved);
    COMMANDSYSTEM_API void ConstructParameterMaskString(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, EMotionFX::AttributeParameterMask* parameterMaskAttribute, AZStd::vector<AZStd::string> excludeParameterNames = AZStd::vector<AZStd::string>());

    // Construct the create parameter command string using the the given information.
    COMMANDSYSTEM_API void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, uint32 insertAtIndex = MCORE_INVALIDINDEX32, bool isScalable = false);
    COMMANDSYSTEM_API void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const char* parameterName, uint32 interfaceType, const MCore::Attribute* minAttribute, const MCore::Attribute* maxAttribute, const MCore::Attribute* defaultAttribute, const AZStd::string& description, uint32 insertAtIndex = MCORE_INVALIDINDEX32, bool isScalable = false);
} // namespace CommandSystem
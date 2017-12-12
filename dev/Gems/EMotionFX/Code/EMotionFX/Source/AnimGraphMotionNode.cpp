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

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphMotionNode.h"
#include "MotionInstance.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "MotionSet.h"
#include "SkeletalSubMotion.h"
#include "SkeletalMotion.h"
#include "AnimGraphManager.h"
#include "MotionManager.h"
#include "MotionInstancePool.h"
#include "EMotionFXManager.h"
#include "MotionEventTable.h"
#include "AnimGraph.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // only motion and playback info are known
    AnimGraphMotionNode::AnimGraphMotionNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        mLastReverse                = false;
        mLastLoop                   = true;
        mLastEvents                 = true;
        mLastRetarget               = true;
        mLastMirror                 = false;
        mLastMotionExtraction       = true;
    }


    // destructor
    AnimGraphMotionNode::~AnimGraphMotionNode()
    {
    }


    // create
    AnimGraphMotionNode* AnimGraphMotionNode::Create(AnimGraph* animGraph)
    {
        return new AnimGraphMotionNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphMotionNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr, MCORE_INVALIDINDEX32, nullptr);
    }


    // init
    void AnimGraphMotionNode::Init(AnimGraphInstance* animGraphInstance)
    {
        // force it to use some sync track
        //  if (GetAttributeString(ATTRIB_EVENTTRACK)->GetValue().GetLength() == 0)
        //  GetAttributeString(ATTRIB_EVENTTRACK)->SetValue("Sync");

        // pre-create the motion instance
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        PickNewActiveMotion(uniqueData);
        if (uniqueData->mMotionInstance == nullptr)
        {
            CreateMotionInstance(animGraphInstance->GetActorInstance(), animGraphInstance);
        }
    }


    // register the ports
    void AnimGraphMotionNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("Play Speed", INPUTPORT_PLAYSPEED, PORTID_INPUT_PLAYSPEED);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
        SetupOutputPortAsMotionInstance("Motion", OUTPUTPORT_MOTION, PORTID_OUTPUT_MOTION);
    }


    // register the attributes
    void AnimGraphMotionNode::RegisterAttributes()
    {
        // register the source motion file name
        MCore::AttributeSettings* param = RegisterAttribute("Motion", "motionID", "The source input motion to use.", ATTRIBUTE_INTERFACETYPE_MULTIPLEMOTIONPICKER);
        param->SetReinitGuiOnValueChange(true);
        param->SetDefaultValue(MCore::AttributeArray::Create(MCore::AttributeString::TYPE_ID));

        param = RegisterAttribute("Loop", "loop", "Loop the motion?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));

        param = RegisterAttribute("Retarget", "retargetEnabled", "Is this motion allowed to be retargeted?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));

        param = RegisterAttribute("Reverse", "reverse", "Playback reversed?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));

        param = RegisterAttribute("Emit Events", "events", "Are motion events enabled?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));

        param = RegisterAttribute("Mirror Motion", "mirror", "Mirror the motion?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));

        param = RegisterAttribute("Motion Extraction", "motionExtraction", "Enable motion extraction?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));

        param = RegisterAttribute("Play Speed", "playSpeed", "The playback speed factor.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        param->SetDefaultValue(MCore::AttributeFloat::Create(1.0f));
        param->SetMinValue(MCore::AttributeFloat::Create(0.0f));
        param->SetMaxValue(MCore::AttributeFloat::Create(100.0f));

        param = RegisterAttribute("Indexing Mode", "indexMode", "The indexing mode to use when using multiple motions inside this motion node.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        param->ResizeComboValues((uint32)INDEXMODE_NUMMETHODS);
        param->SetComboValue(INDEXMODE_RANDOMIZE,           "Randomize");
        param->SetComboValue(INDEXMODE_RANDOMIZE_NOREPEAT,  "Random No Repeat");
        param->SetComboValue(INDEXMODE_SEQUENTIAL,          "Sequential");
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));

        param = RegisterAttribute("Next Motion After Loop", "nextMotionAfterEnd", "Switch to the next motion after this motion has ended/looped?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));

        param = RegisterAttribute("Rewind On Zero Weight", "rewindOnZeroWeight", "Rewind the motion when its local weight is near zero. Useful to restart non-looping motions. Looping needs to be disabled for this to work.", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        param->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));

        /*  param = RegisterAttribute("Clip Start", "clipStart", "The start time of the motion, used when you want to play or loop a given range of the motion.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
            param->SetDefaultValue( MCore::AttributeFloat::Create(0.0f) );
            param->SetMinValue( MCore::AttributeFloat::Create(0.0f) );
            param->SetMaxValue( MCore::AttributeFloat::Create(FLT_MAX) );

            param = RegisterAttribute("Clip End", "clipEnd", "The end time of the motion, used when you want to play or loop a given range of the motion, set to zero to use the real motion maximum time.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
            param->SetDefaultValue( MCore::AttributeFloat::Create(0.0f) );
            param->SetMinValue( MCore::AttributeFloat::Create(0.0f) );
            param->SetMaxValue( MCore::AttributeFloat::Create(FLT_MAX) );*/
    }


    // get the palette name
    const char* AnimGraphMotionNode::GetPaletteName() const
    {
        return "Motion";
    }


    // get the palette category
    AnimGraphObject::ECategory AnimGraphMotionNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // convert attributes for backward compatibility
    // this handles attributes that got renamed or who's types have changed during the development progress
    bool AnimGraphMotionNode::ConvertAttribute(uint32 attributeIndex, const MCore::Attribute* attributeToConvert, const MCore::String& attributeName)
    {
        // convert things by the base class
        const bool result = AnimGraphObject::ConvertAttribute(attributeIndex, attributeToConvert, attributeName);

        // if we try to convert the old motionID string attribute into an array attribute
        if (attributeName == "motionID" && attributeIndex != MCORE_INVALIDINDEX32) // if we deal with the motionID and it is an attribute in the current attribute set
        {
            // if its a string, convert into an array
            if (attributeToConvert->GetType() == MCore::AttributeString::TYPE_ID)
            {
                MCore::AttributeString* motionAttrib    = MCore::AttributeString::Create(static_cast<const MCore::AttributeString*>(attributeToConvert)->GetValue());
                MCore::AttributeArray*  arrayAttrib     = GetAttributeArray(ATTRIB_MOTION);
                arrayAttrib->Clear();
                arrayAttrib->AddAttribute(motionAttrib, false);
                return true;
            }
        }

        return result;
    }


    // create a clone of this node
    AnimGraphNode* AnimGraphMotionNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        AnimGraphMotionNode* clone = AnimGraphMotionNode::Create(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // copy settings specific to this node type
        clone->mPlayInfo = mPlayInfo;

        // return a pointer to the clone
        return clone;
    }


    // post sync update
    void AnimGraphMotionNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the input nodes
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection && mDisabled == false)
        {
            playSpeedConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // clear the event buffer
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        // trigger the motion update
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance)
        {
            // update the time values and extract events into the event buffer
            motionInstance->SetWeight(uniqueData->GetLocalWeight());
            motionInstance->UpdateByTimeValues(uniqueData->GetPreSyncTime(), uniqueData->GetCurrentPlayTime(), &data->GetEventBuffer());

            // mark all events to be emitted from this node
            data->GetEventBuffer().UpdateEmitters(this);
        }
        else
        {
            return;
        }

        if (animGraphInstance->GetIsResynced(mObjectIndex))
        {
            return;
        }

        // make sure the motion instance is ready for sampling
        if (motionInstance->GetIsReadyForSampling() == false)
        {
            motionInstance->InitForSampling();
        }

        // extract current delta
        Transform trajectoryDelta;
        const bool isMirrored = motionInstance->GetMirrorMotion();
        motionInstance->ExtractMotion(trajectoryDelta);
        data->SetTrajectoryDelta(trajectoryDelta);

        // extract mirrored version of the current delta
        motionInstance->SetMirrorMotion(!isMirrored);
        motionInstance->ExtractMotion(trajectoryDelta);
        data->SetTrajectoryDeltaMirrored(trajectoryDelta);

        // restore current mirrored flag
        motionInstance->SetMirrorMotion(isMirrored);
    }


    // top down update
    void AnimGraphMotionNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        // check if we have multiple motions in this node
        const uint32 numMotions = GetAttributeArray(ATTRIB_MOTION)->GetNumAttributes();
        if (numMotions > 1)
        {
            // check if we reached the end of the motion, if so, pick a new one
            if (uniqueData->mMotionInstance)
            {
                if (uniqueData->mMotionInstance->GetHasLooped() && GetAttributeFloatAsBool(ATTRIB_NEXTMOTIONAFTEREND))
                {
                    PickNewActiveMotion(uniqueData);
                }
            }
        }

        // sync all input nodes
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        const uint32 numConnections = mConnections.GetLength();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            mConnections[i]->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // update the motion instance
    void AnimGraphMotionNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the input nodes
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection && mDisabled == false)
        {
            UpdateIncomingNode(animGraphInstance, playSpeedConnection->GetSourceNode(), timePassedInSeconds);
        }

        // update the motion instance (current time etc)
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance == nullptr || mDisabled)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            if (mDisabled == false)
            {
                if (motionInstance == nullptr)
                {
                    SetHasError(animGraphInstance, true);
                }
            }
        #endif

            uniqueData->Clear();
            return;
        }

    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // enable freeze at last frame for motions that are not looping
        motionInstance->SetFreezeAtLastFrame(!motionInstance->GetIsPlayingForever());

        // if there is a node connected to the speed input port, read that value and use it as internal speed if not get the playspeed attribute
        float customSpeed = ExtractCustomPlaySpeed(animGraphInstance);

        // rewind when the weight reaches 0 when we want to
        if (GetAttributeFloatAsBool(ATTRIB_LOOP) == false)
        {
            if (uniqueData->GetLocalWeight() < 0.0001f && GetAttributeFloatAsBool(ATTRIB_REWINDONZEROWEIGHT))
            {
                motionInstance->SetCurrentTime(0.0f);
            }
        }

        // set the internal speed and play speeds etc
        motionInstance->SetPlaySpeed(uniqueData->GetPlaySpeed());
        uniqueData->SetPlaySpeed(customSpeed);
        uniqueData->SetPreSyncTime(motionInstance->GetCurrentTime());

        bool hasLooped = false;
        if (animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false || animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_IS_SYNCMASTER))
        {
            // calculate the new internal values when we would update with a given time delta
            float newTime;
            float passedTime;
            float totalPlayTime;
            float timeDifToEnd;
            uint32 numLoops;
            bool frozenInLastFrame;
            motionInstance->CalcNewTimeAfterUpdate(timePassedInSeconds, &newTime, &passedTime, &totalPlayTime, &timeDifToEnd, &numLoops, &hasLooped, &frozenInLastFrame);

            // set the current time to the new calculated time
            uniqueData->ClearInheritFlags();
            uniqueData->SetCurrentPlayTime(newTime);
        }

        uniqueData->SetDuration(motionInstance->GetDuration());

        // make sure the motion is not paused
        motionInstance->SetPause(false);

        // init the sync track
        //uniqueData->GetSyncTrack().InitFromEventTrack( motionInstance->GetMotion()->GetEventTable().GetSyncTrack() );
        if (uniqueData->mEventTrackIndex < motionInstance->GetMotion()->GetEventTable()->GetNumTracks())
        {
            if (motionInstance->GetMirrorMotion() == false)
            {
                uniqueData->GetSyncTrack().InitFromEventTrack(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
            }
            else
            {
                uniqueData->GetSyncTrack().InitFromEventTrackMirrored(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
            }
        }
        else
        {
            uniqueData->GetSyncTrack().Clear();
        }

        // update some flags
        if (motionInstance->GetPlayMode() == PLAYMODE_BACKWARD)
        {
            uniqueData->SetBackwardFlag();
        }

        if (hasLooped)
        {
            uniqueData->SetLoopedFlag();
        }
    }


    void AnimGraphMotionNode::UpdatePlayBackInfo(AnimGraphInstance* animGraphInstance)
    {
        // add the unique data of this node to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // update the last motion ID
        mLastLoop               = GetAttributeFloatAsBool(ATTRIB_LOOP);
        mLastRetarget           = GetAttributeFloatAsBool(ATTRIB_RETARGET);
        mLastReverse            = GetAttributeFloatAsBool(ATTRIB_REVERSE);
        mLastEvents             = GetAttributeFloatAsBool(ATTRIB_EVENTS);
        mLastMirror             = GetAttributeFloatAsBool(ATTRIB_MIRROR);
        mLastMotionExtraction   = GetAttributeFloatAsBool(ATTRIB_MOTIONEXTRACTION);
        /////mLastClipStart         = GetAttributeFloat(ATTRIB_CLIPSTART)->GetValue();
        /////mLastClipEnd           = GetAttributeFloat(ATTRIB_CLIPEND)->GetValue();

        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance)
        {
            uniqueData->mEventTrackIndex = motionInstance->GetMotion()->GetEventTable()->FindTrackIndexByName("Sync");
        }
        else
        {
            uniqueData->mEventTrackIndex = MCORE_INVALIDINDEX32;
        }

        // check if we need to play backwards
        mPlayInfo.mPlayMode             = (mLastReverse) ? PLAYMODE_BACKWARD : PLAYMODE_FORWARD;
        mPlayInfo.mNumLoops             = (mLastLoop) ? EMFX_LOOPFOREVER : 1;
        mPlayInfo.mFreezeAtLastFrame    = true;
        mPlayInfo.mEnableMotionEvents   = mLastEvents;
        mPlayInfo.mMirrorMotion         = mLastMirror;
        mPlayInfo.mPlaySpeed            = ExtractCustomPlaySpeed(animGraphInstance);
        mPlayInfo.mMotionExtractionEnabled = mLastMotionExtraction;

        /////mPlayInfo.mClipStartTime       = mLastClipStart;
        /////mPlayInfo.mClipEndTime         = mLastClipEnd;
        mPlayInfo.mRetarget             = mLastRetarget;
    }


    // create the motion instance
    MotionInstance* AnimGraphMotionNode::CreateMotionInstance(ActorInstance* actorInstance, AnimGraphInstance* animGraphInstance)
    {
        // add the unique data of this node to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // update the last motion ID
        UpdatePlayBackInfo(animGraphInstance);

        // try to find the motion to use for this actor instance in this blend node
        Motion*         motion      = nullptr;
        PlayBackInfo    playInfo    = mPlayInfo;

        // reset playback properties
        const float curPlayTime = uniqueData->GetCurrentPlayTime();
        uniqueData->Clear();

        // remove the motion instance if it already exists
        if (uniqueData->mMotionInstance && uniqueData->mReload)
        {
            //delete uniqueData->mMotionInstance;
            GetMotionInstancePool().Free(uniqueData->mMotionInstance);
            uniqueData->mMotionInstance = nullptr;
            uniqueData->mMotionSetID    = MCORE_INVALIDINDEX32;
            uniqueData->mReload         = false;
        }

        // get the motion set
        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        if (motionSet == nullptr)
        {
            //#ifdef EMFX_EMSTUDIOBUILD
            //SetHasError(animGraphInstance, true);
            //#endif
            return nullptr;
        }

        // get the motion from the motion set, load it on demand and make sure the motion loaded successfully
        if (uniqueData->mActiveMotionIndex != MCORE_INVALIDINDEX32)
        {
            motion = motionSet->RecursiveFindMotionByStringID(GetMotionID(uniqueData->mActiveMotionIndex));
        }

        if (motion == nullptr)
        {
            //#ifdef EMFX_EMSTUDIOBUILD
            //SetHasError(animGraphInstance, true);
            //#endif
            return nullptr;
        }

        uniqueData->mMotionSetID = motionSet->GetID();

        // update the event track index and find the sync motion event track index by the event track name attribute string
        //mLastEventTrackIndex = motion->GetEventTable().FindTrackIndexByName( mLastEventTrackName.AsChar() );
        // TODO: actually use the mLastEventTrackIndex, maybe we add it to the motion instance and playback info, does that make sense?

        // create the motion instance
        MotionInstance* motionInstance = GetMotionInstancePool().RequestNew(motion, actorInstance, playInfo.mStartNodeIndex);
        motionInstance->InitFromPlayBackInfo(playInfo, true);
        motionInstance->SetRetargetingEnabled(animGraphInstance->GetRetargetingEnabled() && playInfo.mRetarget);

        // find the sync track
        uniqueData->mEventTrackIndex = motionInstance->GetMotion()->GetEventTable()->FindTrackIndexByName("Sync");

        if (uniqueData->mEventTrackIndex < motionInstance->GetMotion()->GetEventTable()->GetNumTracks())
        {
            if (motionInstance->GetMirrorMotion() == false)
            {
                uniqueData->GetSyncTrack().InitFromEventTrack(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
            }
            else
            {
                uniqueData->GetSyncTrack().InitFromEventTrackMirrored(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
            }
        }
        else
        {
            uniqueData->GetSyncTrack().Clear();
        }

        // create the motion links
        //motion->CreateMotionLinks( actorInstance->GetActor(), motionInstance );
        if (motionInstance->GetIsReadyForSampling() == false && animGraphInstance->GetInitSettings().mPreInitMotionInstances)
        {
            motionInstance->InitForSampling();
        }

        // make sure it is not in pause mode
        motionInstance->UnPause();
        motionInstance->SetIsActive(true);
        motionInstance->SetWeight(1.0f, 0.0f);

        // update play info
        uniqueData->mMotionInstance = motionInstance;
        uniqueData->SetDuration(motionInstance->GetDuration());
        uniqueData->SetCurrentPlayTime(curPlayTime);
        motionInstance->SetCurrentTime(curPlayTime);

        // trigger an event
        GetEventManager().OnStartMotionInstance(motionInstance, &playInfo);
        return motionInstance;
    }


    void AnimGraphMotionNode::Prepare(AnimGraphInstance* animGraphInstance)
    {
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // get the motion set
        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        if (motionSet == nullptr)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, true);
        #endif
            return;
        }

        // get the motion from the motion set, load it on demand and make sure the motion loaded successfully
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        Motion* motion = motionSet->RecursiveFindMotionByStringID(GetMotionID(uniqueData->mActiveMotionIndex));
        if (motion == nullptr)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            SetHasError(animGraphInstance, true);
        #endif
            return;
        }
    }



    // the main process method of the final node
    void AnimGraphMotionNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // if this motion is disabled, output the bind pose
        if (mDisabled)
        {
            // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // output the playspeed node
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;
        if (playSpeedConnection)
        {
            OutputIncomingNode(animGraphInstance, playSpeedConnection->GetSourceNode());
        }

        // create and register the motion instance when this is the first time its being when it hasn't been registered yet
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        MotionInstance* motionInstance = nullptr;
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mReload)
        {
            motionInstance = CreateMotionInstance(actorInstance, animGraphInstance);
            uniqueData->mReload = false;
        }
        else
        {
            motionInstance = uniqueData->mMotionInstance;
        }

        // update the motion instance output port
        GetOutputMotionInstance(animGraphInstance, OUTPUTPORT_MOTION)->SetValue(motionInstance);

        if (motionInstance == nullptr)
        {
            // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            //#ifdef EMFX_EMSTUDIOBUILD
            //SetHasError(animGraphInstance, true);
            //#endif

            return;
        }
        //#ifdef EMFX_EMSTUDIOBUILD
        //else
        //SetHasError(animGraphInstance, false);
        //#endif

        // make sure the motion instance is ready for sampling
        if (motionInstance->GetIsReadyForSampling() == false)
        {
            motionInstance->InitForSampling();
        }

        // sync the motion instance with the motion node attributes
        motionInstance->SetPlayMode(mPlayInfo.mPlayMode);
        motionInstance->SetRetargetingEnabled(mPlayInfo.mRetarget && animGraphInstance->GetRetargetingEnabled());
        motionInstance->SetFreezeAtLastFrame(mPlayInfo.mFreezeAtLastFrame);
        motionInstance->SetMotionEventsEnabled(mPlayInfo.mEnableMotionEvents);
        motionInstance->SetMirrorMotion(mPlayInfo.mMirrorMotion);
        motionInstance->SetEventWeightThreshold(mPlayInfo.mEventWeightThreshold);
        motionInstance->SetMaxLoops(mPlayInfo.mNumLoops);
        motionInstance->SetMotionExtractionEnabled(mPlayInfo.mMotionExtractionEnabled);

        // request poses to use from the pool, so that all output pose ports have a valid pose to output to we reuse them using a pool system to save memory
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        Pose& outputTransformPose = outputPose->GetPose();

        // fill the output with the bind pose
        outputPose->InitFromBindPose(actorInstance); // TODO: is this really needed?

        // we use as input pose the same as the output, as this blend tree node takes no input
        motionInstance->GetMotion()->Update(&outputTransformPose, &outputTransformPose, motionInstance);

        // compensate for motion extraction
        // we already moved our actor instance's position and rotation at this point
        // so we have to cancel/compensate this delta offset from the motion extraction node, so that we don't double-transform
        // basically this will keep the motion in-place rather than moving it away from the origin
        if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled())
        {
            outputTransformPose.CompensateForMotionExtractionDirect();
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
    }


    // get the type string
    const char* AnimGraphMotionNode::GetTypeString() const
    {
        return "AnimGraphMotionNode";
    }


    // when parameter values have changed through the interface
    void AnimGraphMotionNode::OnUpdateAttributes()
    {
        // check if the motion string attribute changed
        const bool convertSuccess = GetAttributeArray(ATTRIB_MOTION)->ConvertToString(mCurMotionArrayString);
        MCORE_ASSERT(convertSuccess);

        // if the motion changed, update all corresponding anim graph instances
        const uint32 numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            if (uniqueData)
            {
                PickNewActiveMotion(uniqueData);

                if (convertSuccess && mLastMotionArrayString.CheckIfIsEqual(mCurMotionArrayString) == false)
                {
                    uniqueData->mReload = true;
                }
            }

            OnUpdateUniqueData(animGraphInstance);
        }

        if (convertSuccess && mLastMotionArrayString.CheckIfIsEqual(mCurMotionArrayString) == false)
        {
            // update the node info
            const uint32 numMotions = GetAttributeArray(ATTRIB_MOTION)->GetNumAttributes();
            if (numMotions > 0)
            {
                if (numMotions > 1)
                {
                    SetNodeInfo("<Multiple>");
                }
                else
                {
                    SetNodeInfo(GetMotionID(0));
                }
            }
            else
            {
                SetNodeInfo("<None>");
            }

            mLastMotionArrayString = mCurMotionArrayString;
        }

        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // if events processing is disabled also disable the events threshold
        if (GetAttributeArray(ATTRIB_MOTION)->GetNumAttributes() <= 1)
        {
            SetAttributeDisabled(ATTRIB_INDEXMODE);
            SetAttributeDisabled(ATTRIB_NEXTMOTIONAFTEREND);
        }

        if (GetAttributeFloatAsBool(ATTRIB_LOOP))
        {
            SetAttributeDisabled(ATTRIB_REWINDONZEROWEIGHT);
        }
    #endif
    }


    // get the motion instance for a given anim graph instance
    MotionInstance* AnimGraphMotionNode::FindMotionInstance(AnimGraphInstance* animGraphInstance) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        return uniqueData->mMotionInstance;
    }


    // update the parameter contents, such as combobox values
    void AnimGraphMotionNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);

            // pre-create the motion instance
            CreateMotionInstance(animGraphInstance->GetActorInstance(), animGraphInstance);
        }

        // get the id of the currently used the motion set
        MotionSet*  motionSet   = animGraphInstance->GetMotionSet();
        uint32      motionSetID = MCORE_INVALIDINDEX32;
        if (motionSet)
        {
            motionSetID = motionSet->GetID();
        }

        // check if some settings have changed that will need a reload of the motion
        if ((mLastMotionArrayString.CheckIfIsEqual(mCurMotionArrayString) == false) ||
            (uniqueData->mMotionSetID   != motionSetID) ||
            //(MCore::Math::IsFloatEqual(mLastClipStart, GetAttributeFloat(ATTRIB_CLIPSTART)->GetValue()) == false) ||
            //(MCore::Math::IsFloatEqual(mLastClipEnd, GetAttributeFloat(ATTRIB_CLIPEND)->GetValue()) == false)  ||
            (mLastMirror != GetAttributeFloatAsBool(ATTRIB_MIRROR)))
        {
            uniqueData->mReload = true;
        }

        // check if some settings have changed that will will require to update the motion instance without recreating it
        if ((mLastLoop              != GetAttributeFloatAsBool(ATTRIB_LOOP)) ||
            (mLastRetarget          != GetAttributeFloatAsBool(ATTRIB_RETARGET)) ||
            (mLastReverse           != GetAttributeFloatAsBool(ATTRIB_REVERSE)) ||
            (mLastEvents            != GetAttributeFloatAsBool(ATTRIB_EVENTS)) ||
            (mLastMirror            != GetAttributeFloatAsBool(ATTRIB_MIRROR)) ||
            (mLastMotionExtraction  != GetAttributeFloatAsBool(ATTRIB_MOTIONEXTRACTION)))
        {
            // update the internally stored playback info
            UpdatePlayBackInfo(animGraphInstance);
        }

        // update play info
        if (uniqueData->mMotionInstance)
        {
            MotionInstance* motionInstance = uniqueData->mMotionInstance;
            uniqueData->SetDuration(motionInstance->GetDuration());
            uniqueData->SetCurrentPlayTime(motionInstance->GetCurrentTime());

            uniqueData->mEventTrackIndex = motionInstance->GetMotion()->GetEventTable()->FindTrackIndexByName("Sync");
            if (uniqueData->mEventTrackIndex < motionInstance->GetMotion()->GetEventTable()->GetNumTracks())
            {
                if (motionInstance->GetMirrorMotion() == false)
                {
                    uniqueData->GetSyncTrack().InitFromEventTrack(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
                }
                else
                {
                    uniqueData->GetSyncTrack().InitFromEventTrackMirrored(motionInstance->GetMotion()->GetEventTable()->GetTrack(uniqueData->mEventTrackIndex));
                }
            }
            else
            {
                uniqueData->GetSyncTrack().Clear();
            }
        }
    }


    // set the current play time
    void AnimGraphMotionNode::SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        uniqueData->SetCurrentPlayTime(timeInSeconds);
        if (uniqueData->mMotionInstance)
        {
            uniqueData->mMotionInstance->SetCurrentTime(timeInSeconds);
        }
    }


    // unique data constructor
    AnimGraphMotionNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, uint32 motionSetID, MotionInstance* instance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
        mMotionInstance     = instance;
        mReload             = false;
        mEventTrackIndex    = MCORE_INVALIDINDEX32;
        mMotionSetID        = motionSetID;
        mActiveMotionIndex  = MCORE_INVALIDINDEX32;
    }


    // unique data destructor
    AnimGraphMotionNode::UniqueData::~UniqueData()
    {
        GetMotionInstancePool().Free(mMotionInstance);
    }


    void AnimGraphMotionNode::UniqueData::Reset()
    {
        // stop and delete the motion instance
        if (mMotionInstance)
        {
            mMotionInstance->Stop(0.0f);
            GetMotionInstancePool().Free(mMotionInstance);
        }

        // reset the unique data
        mMotionSetID    = MCORE_INVALIDINDEX32;
        mMotionInstance = nullptr;
        mReload         = true;
        mPlaySpeed      = 1.0f;
        mCurrentTime    = 0.0f;
        mDuration       = 0.0f;
        mActiveMotionIndex = MCORE_INVALIDINDEX32;
        mSyncTrack.Clear();
    }


    // this function will get called to rewind motion nodes as well as states etc. to reset several settings when a state gets exited
    void AnimGraphMotionNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // find the motion instance for the given anim graph and return directly in case it is invalid
        MotionInstance* motionInstance = uniqueData->mMotionInstance;
        if (motionInstance == nullptr)
        {
            return;
        }

        // reset several settings to rewind the motion instance
        motionInstance->ResetTimes();
        motionInstance->SetIsFrozen(false);
        SetSyncIndex(animGraphInstance, MCORE_INVALIDINDEX32);
        uniqueData->SetCurrentPlayTime(motionInstance->GetCurrentTime());
        uniqueData->SetDuration(motionInstance->GetDuration());
        uniqueData->SetPreSyncTime(uniqueData->GetCurrentPlayTime());
        //uniqueData->SetPlaySpeed( uniqueData->GetPlaySpeed() );

        PickNewActiveMotion(uniqueData);
    }


    // get the speed from the connection if there is one connected, if not get the speed attribute
    float AnimGraphMotionNode::ExtractCustomPlaySpeed(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::BlendTreeConnection* playSpeedConnection = GetInputPort(INPUTPORT_PLAYSPEED).mConnection;

        // if there is a node connected to the speed input port, read that value and use it as internal speed
        float customSpeed;
        if (playSpeedConnection)
        {
            customSpeed = MCore::Max(0.0f, GetInputNumberAsFloat(animGraphInstance, INPUTPORT_PLAYSPEED));
        }
        else
        {
            customSpeed = GetAttributeFloat(ATTRIB_PLAYSPEED)->GetValue(); // otherwise read the node's speed attribute value and use that
        }
        return customSpeed;
    }


    // pick a new motion from the list
    void AnimGraphMotionNode::PickNewActiveMotion(UniqueData* uniqueData)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        const uint32 numMotions = GetAttributeArray(ATTRIB_MOTION)->GetNumAttributes();
        if (numMotions > 0)
        {
            if (numMotions > 1)
            {
                uniqueData->mReload = true;
                const uint32 indexMode = static_cast<uint32>(GetAttributeFloat(ATTRIB_INDEXMODE)->GetValue());
                switch (indexMode)
                {
                // pick a random one, but make sure its not the same as the last one we played
                case INDEXMODE_RANDOMIZE_NOREPEAT:
                {
                    uint32 curIndex = uniqueData->mActiveMotionIndex;
                    while (curIndex == uniqueData->mActiveMotionIndex)  // repeat until we found another value
                    {
                        curIndex = rand() % numMotions;
                    }
                    uniqueData->mActiveMotionIndex = curIndex;
                    ;
                }
                break;

                // pick the next motion from the list
                case INDEXMODE_SEQUENTIAL:
                {
                    uniqueData->mActiveMotionIndex++;
                    if (uniqueData->mActiveMotionIndex >= numMotions)
                    {
                        uniqueData->mActiveMotionIndex = 0;
                    }
                }
                break;

                // just pick a random one, this can result in the same one we already play
                case INDEXMODE_RANDOMIZE:
                default:
                {
                    const uint32 index = rand() % numMotions;
                    uniqueData->mActiveMotionIndex = index;
                }
                }
            }
            else
            {
                uniqueData->mActiveMotionIndex = 0;
            }
        }
        else
        {
            uniqueData->mActiveMotionIndex = MCORE_INVALIDINDEX32;
        }
    }


    AZ::u32 AnimGraphMotionNode::GetNumMotions() const
    {
        return GetAttributeArray(ATTRIB_MOTION)->GetNumAttributes();
    }


    // get the motion ID from the list
    const char* AnimGraphMotionNode::GetMotionID(uint32 index) const
    {
        MCore::AttributeArray* attributeArray = GetAttributeArray(ATTRIB_MOTION);
        if (attributeArray->GetNumAttributes() <= index)
        {
            return nullptr;
        }

        MCore::Attribute* attrib = attributeArray->GetAttribute(index);
        if (attrib == nullptr)
        {
            return nullptr;
        }

        return static_cast<const MCore::AttributeString*>(attrib)->GetValue().AsChar();
    }


    // motion extraction node changed
    void AnimGraphMotionNode::OnActorMotionExtractionNodeChanged()
    {
        const uint32 numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            uniqueData->mReload = true;

            OnUpdateUniqueData(animGraphInstance);
        }
    }
}   // namespace EMotionFX


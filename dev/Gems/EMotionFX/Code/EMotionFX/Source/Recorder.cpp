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
#include "Recorder.h"
#include "RecorderBus.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "AnimGraphManager.h"
#include "AnimGraphInstance.h"
#include "AnimGraph.h"
#include "AnimGraphMotionNode.h"
#include "AnimGraphStateMachine.h"
#include "EventManager.h"
#include "EMotionFXManager.h"
#include "ActorManager.h"
#include "MorphSetupInstance.h"
#include "MotionEvent.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/Matrix4.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Recorder, RecorderAllocator, 0)


    // constructor
    Recorder::Recorder()
        : BaseObject()
    {
        mIsInPlayMode   = false;
        mIsRecording    = false;
        mAutoPlay       = false;
        mRecordTime     = 0.0f;
        mLastRecordTime = 0.0f;
        mCurrentPlayTime = 0.0f;

        mActorInstanceDatas.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
        mObjects.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
        mActiveNodes.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
    }


    // destructor
    Recorder::~Recorder()
    {
        Clear();
    }


    // create a new recorder
    Recorder* Recorder::Create()
    {
        return aznew Recorder();
    }


    // enable or disable auto play mode
    void Recorder::SetAutoPlay(bool enabled)
    {
        mAutoPlay = enabled;
    }


    // set the current play time
    void Recorder::SetCurrentPlayTime(float timeInSeconds)
    {
        mCurrentPlayTime = timeInSeconds;

        if (mCurrentPlayTime < 0.0f)
        {
            mCurrentPlayTime = 0.0f;
        }

        if (mCurrentPlayTime > mRecordTime)
        {
            mCurrentPlayTime = mRecordTime;
        }
    }


    // start playback mode
    void Recorder::StartPlayBack()
    {
        StopRecording();
        mIsInPlayMode = true;
    }


    // stop playback mode
    void Recorder::StopPlayBack()
    {
        mIsInPlayMode = false;
    }


    // rewind the playback
    void Recorder::Rewind()
    {
        mCurrentPlayTime = 0.0f;
    }


    // clear any currently recorded data
    void Recorder::Clear()
    {
        Lock();

        mIsInPlayMode   = false;
        mIsRecording    = false;
        mAutoPlay       = false;
        mRecordTime     = 0.0f;
        mLastRecordTime = 0.0f;
        mCurrentPlayTime = 0.0f;
        mRecordSettings.mActorInstances.Clear();

        // delete all actor instance datas
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            delete mActorInstanceDatas[a];
        }
        mActorInstanceDatas.Clear();

        Unlock();
    }


    // start recording
    void Recorder::StartRecording(const RecordSettings& settings)
    {
        // clear any previous recorded data
        Clear();
        StopRecording();

        Lock();

        // Generate a new random UUID for this recording session.
        m_sessionUuid = AZ::Uuid::Create();

        // we are recording again
        mRecordSettings = settings;
        mIsRecording    = true;

        // add all actor instances if we want to record them all
        if (mRecordSettings.mActorInstances.GetLength() == 0)
        {
            const uint32 numActorInstances = GetActorManager().GetNumActorInstances();
            mRecordSettings.mActorInstances.Resize(numActorInstances);
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
                mRecordSettings.mActorInstances[i] = actorInstance;
            }
        }

        // prepare for recording
        // this resizes arrays and allocates buffers upfront
        PrepareForRecording();

        // record the initial frame
        RecordCurrentFrame();

        Unlock();
    }


    // update (when playback)
    void Recorder::UpdatePlayMode(float timeDelta)
    {
        // increase the playtime if we are in automatic play mode and playback is enabled
        if (mIsInPlayMode && mAutoPlay)
        {
            SetCurrentPlayTime(mCurrentPlayTime + timeDelta);
        }
    }


    // update the recorder
    void Recorder::Update(float timeDelta)
    {
        Lock();

        // if we are not recording there is nothing to do
        if (mIsRecording == false)
        {
            Unlock();
            return;
        }

        // increase the time we record
        mRecordTime += timeDelta;

        // save a sample when more time passed than the desired sample rate
        const float sampleRate = 1.0f / (float)mRecordSettings.mFPS;
        if (mRecordTime - mLastRecordTime >= sampleRate)
        {
            RecordCurrentFrame();
        }

        Unlock();
    }


    // stop recording
    void Recorder::StopRecording(bool lock)
    {
        if (lock)
        {
            Lock();
        }

        mIsRecording = false;
        FinalizeAllNodeHistoryItems();

        if (lock)
        {
            Unlock();
        }
    }


    // prepare for recording by resizing and preallocating space/arrays
    void Recorder::PrepareForRecording()
    {
        const uint32 numActorInstances = mRecordSettings.mActorInstances.GetLength();
        mActorInstanceDatas.Resize(numActorInstances);
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            mActorInstanceDatas[i] = new ActorInstanceData();
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[i];

            // link it to the right actor instance
            ActorInstance* actorInstance = mRecordSettings.mActorInstances[i];
            actorInstanceData.mActorInstance = actorInstance;

            // add the transform tracks
            if (mRecordSettings.mRecordTransforms)
            {
                // for all nodes in the actor instance
                const uint32 numNodes = actorInstance->GetNumNodes();
                actorInstanceData.mTransformTracks.Resize(numNodes);
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    actorInstanceData.mTransformTracks[n].mPositions.Reserve(mRecordSettings.mNumPreAllocTransformKeys);
                    actorInstanceData.mTransformTracks[n].mRotations.Reserve(mRecordSettings.mNumPreAllocTransformKeys);

                    EMFX_SCALECODE
                    (
                        if (mRecordSettings.mRecordScale)
                        {
                            actorInstanceData.mTransformTracks[n].mScales.Reserve(mRecordSettings.mNumPreAllocTransformKeys);

                            //if (mRecordSettings.mRecordNonUniformScale)
                            //actorInstanceData.mTransformTracks[n].mScaleRotations.Reserve( mRecordSettings.mNumPreAllocTransformKeys );
                        }
                    )
                }
            } // if recording transforms

            // if recording morph targets, resize the morphs array
            if (mRecordSettings.mRecordMorphs)
            {
                const uint32 numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
                actorInstanceData.mMorphTracks.Resize(numMorphs);
                for (uint32 m = 0; m < numMorphs; ++m)
                {
                    actorInstanceData.mMorphTracks[m].Reserve(256);
                }
            }

            // add the animgraph data
            if (mRecordSettings.mRecordAnimGraphStates)
            {
                if (actorInstance->GetAnimGraphInstance())
                {
                    actorInstanceData.mAnimGraphData = new AnimGraphInstanceData();
                    actorInstanceData.mAnimGraphData->mAnimGraphInstance = actorInstance->GetAnimGraphInstance();
                    if (mRecordSettings.mInitialAnimGraphAnimBytes > 0)
                    {
                        actorInstanceData.mAnimGraphData->mDataBuffer  = (uint8*)MCore::Allocate(mRecordSettings.mInitialAnimGraphAnimBytes, EMFX_MEMCATEGORY_RECORDER);
                    }

                    actorInstanceData.mAnimGraphData->mDataBufferSize  = mRecordSettings.mInitialAnimGraphAnimBytes;
                }
            } // if recording animgraphs
        } // for all actor instances
    }


    // optimize the current recorded data (remove keys etc)
    void Recorder::OptimizeRecording()
    {
        OptimizeTransforms();
        OptimizeAnimGraphStates();
    }


    // optimize transformations
    void Recorder::OptimizeTransforms()
    {
        //const float maxPosError = 0.0001f;
        //const float maxRotError = 0.0001f;

        // for all actor instances
        const uint32 numActorInstances = mRecordSettings.mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[i];
            const ActorInstance* actorInstance = actorInstanceData.mActorInstance;

            if (actorInstanceData.mTransformTracks.GetLength() == 0)
            {
                continue;
            }

            // for all nodes in the actor instance
            const uint32 numNodes = actorInstance->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                //actorInstanceData.mTransformTracks[n].mPositions.Optimize( maxPosError );
                //actorInstanceData.mTransformTracks[n].mRotations.Optimize( maxRotError );
                actorInstanceData.mTransformTracks[n].mPositions.Shrink();
                actorInstanceData.mTransformTracks[n].mRotations.Shrink();

                EMFX_SCALECODE
                (
                    //actorInstanceData.mTransformTracks[n].mScales.Optimize( maxPosError );
                    //actorInstanceData.mTransformTracks[n].mScaleRotations.Optimize( maxRotError );
                    actorInstanceData.mTransformTracks[n].mScales.Shrink();
                    //actorInstanceData.mTransformTracks[n].mScaleRotations.Shrink();
                )
            }
        }
    }


    // optimize anim graph transforms
    void Recorder::OptimizeAnimGraphStates()
    {
    }


    // save to a file
    bool Recorder::SaveToFile(const char* outFile, const SaveSettings& settings)
    {
        // try to create the file
        MCore::DiskFile file;
        if (file.Open(outFile, MCore::DiskFile::WRITE) == false)
        {
            MCore::LogError("EMotionFX::Recorder::SaveToFile() - Failed to create output file '%s'...", outFile);
            return false;
        }

        // save the file
        return SaveToFile(file, settings);
    }


    // save the data to the given file
    bool Recorder::SaveToFile(MCore::File& file, const SaveSettings& settings)
    {
        MCORE_UNUSED(file);
        MCORE_UNUSED(settings);
        Lock();
        // TODO: implement
        MCore::LogWarning("EMotionFX::Recorder::SaveToFile() - This has not yet been implemented");
        Unlock();
        return true;
    }


    // record the current frame
    void Recorder::RecordCurrentFrame()
    {
        // record the current transforms
        if (mRecordSettings.mRecordTransforms)
        {
            RecordCurrentTransforms();
        }

        // record the current anim graph states
        if (mRecordSettings.mRecordAnimGraphStates)
        {
            if (!RecordCurrentAnimGraphStates())
            {
                StopRecording();
                Clear();
                return;
            }
        }

        // always record the main transforms
        RecordMainLocalTransforms();

        // record morphs
        if (mRecordSettings.mRecordMorphs)
        {
            RecordMorphs();
        }

        // update (while recording) the node history items
        if (mRecordSettings.mRecordNodeHistory)
        {
            UpdateNodeHistoryItems();
        }

        // recordo the events
        if (mRecordSettings.mRecordEvents)
        {
            RecordEvents();
        }

        // update the last record time
        mLastRecordTime = mRecordTime;
    }


    // record the morph weights
    void Recorder::RecordMorphs()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[i];
            ActorInstance* actorInstance = actorInstanceData.mActorInstance;

            const uint32 numMorphs = actorInstance->GetMorphSetupInstance()->GetNumMorphTargets();
            for (uint32 m = 0; m < numMorphs; ++m)
            {
                KeyTrackLinearDynamic<float, float>& morphTrack = actorInstanceData.mMorphTracks[i]; // morph animation data
                morphTrack.AddKey(mRecordTime, actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->GetWeight());
            }
        }
    }


    // record all actor instance main transformations, so not of the nodes, but of the actor instance itself
    void Recorder::RecordMainLocalTransforms()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[i];
            ActorInstance* actorInstance = actorInstanceData.mActorInstance;
            const Transform& transform = actorInstance->GetLocalSpaceTransform();

        #ifndef EMFX_SCALE_DISABLED
            AddTransformKey(actorInstanceData.mActorLocalTransform, transform.mPosition, transform.mRotation, transform.mScale);
        #else
            AddTransformKey(actorInstanceData.mActorLocalTransform, transform.mPosition, transform.mRotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
        #endif
        }
    }


    // record current transforms
    void Recorder::RecordCurrentTransforms()
    {
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[i];
            ActorInstance* actorInstance = actorInstanceData.mActorInstance;

            const TransformData* transformData = actorInstance->GetTransformData();
            {
                const uint32 numNodes = actorInstance->GetNumNodes();
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    const Transform& localTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);

                #ifndef EMFX_SCALE_DISABLED
                    AddTransformKey(actorInstanceData.mTransformTracks[n], localTransform.mPosition, localTransform.mRotation, localTransform.mScale);
                #else
                    AddTransformKey(actorInstanceData.mTransformTracks[n], localTransform.mPosition, localTransform.mRotation, AZ::Vector3(1.0f, 1.0f, 1.0f));
                #endif
                }
            }
        }
    }


    // record current animgraph states
    bool Recorder::RecordCurrentAnimGraphStates()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            if (actorInstanceData.mAnimGraphData == nullptr)
            {
                continue;
            }

            // for all anim graph instances we are recording
            //      const uint32 numAnimGraphInstances = actorInstanceData.mAnimGraphDatas.GetLength();
            //      for (uint32 i=0; i<numAnimGraphInstances; ++i)
            {
                // get some shortcuts
                AnimGraphInstanceData& animGraphInstanceData  = *actorInstanceData.mAnimGraphData;
                const AnimGraphInstance*   animGraphInstance  = animGraphInstanceData.mAnimGraphInstance;
                const AnimGraph*           animGraph          = animGraphInstance->GetAnimGraph();

                // add a new frame
                MCore::Array<AnimGraphAnimFrame>& frames = animGraphInstanceData.mFrames;
                if (frames.GetLength() > 0)
                {
                    const uint32 byteOffset = frames.GetLast().mByteOffset + frames.GetLast().mNumBytes;
                    frames.AddEmpty();
                    frames.GetLast().mByteOffset    = byteOffset;
                    frames.GetLast().mNumBytes      = 0;
                }
                else
                {
                    frames.AddEmpty();
                    frames.GetLast().mByteOffset    = 0;
                    frames.GetLast().mNumBytes      = 0;
                }

                // get the current frame
                AnimGraphAnimFrame& currentFrame = frames.GetLast();
                currentFrame.mTimeValue = mRecordTime;

                // save the parameter values
                const uint32 numParams = static_cast<uint32>(animGraphInstance->GetAnimGraph()->GetNumValueParameters());
                currentFrame.mParameterValues.SetMemoryCategory(EMFX_MEMCATEGORY_RECORDER);
                currentFrame.mParameterValues.Resize(numParams);
                for (uint32 p = 0; p < numParams; ++p)
                {
                    currentFrame.mParameterValues[p] = animGraphInstance->GetParameterValue(p)->Clone();
                }

                // recursively save all unique datas
                if (!SaveUniqueData(animGraphInstance, animGraph->GetRootStateMachine(), animGraphInstanceData))
                {
                    return false;
                }

                // increase the frames counter
                animGraphInstanceData.mNumFrames++;

                //MCore::LogInfo("Frame %d = %d bytes (offset=%d), with %d objects - dataBuffer = %d kb", animGraphInstanceData.mNumFrames, frames.GetLast().mNumBytes, frames.GetLast().mByteOffset, frames.GetLast().mObjectInfos.GetLength(), animGraphInstanceData.mDataBufferSize / 1024);
            }
        } // for all actor instances
        return true;
    }


    // recursively save the node's unique data
    bool Recorder::SaveUniqueData(const AnimGraphInstance* animGraphInstance, AnimGraphObject* object, AnimGraphInstanceData& animGraphInstanceData)
    {
        // get the current frame's data pointer
        AnimGraphAnimFrame& currentFrame = animGraphInstanceData.mFrames.GetLast();
        const uint32 frameOffset = currentFrame.mByteOffset;

        // prepare the objects array
        mObjects.Clear(false);
        mObjects.Reserve(1024);

        // collect the objects we are going to save for this frame
        object->RecursiveCollectObjects(mObjects);

        // resize the object infos array
        const uint32 numObjects = mObjects.GetLength();
        currentFrame.mObjectInfos.Resize(numObjects);

        // calculate how much memory we need for this frame
        uint32 requiredFrameBytes = 0;
        for (uint32 i = 0; i < numObjects; ++i)
        {
            requiredFrameBytes += mObjects[i]->SaveUniqueData(animGraphInstance, nullptr);
        }

        // make sure we have at least the given amount of space in the buffer we are going to write the frame data to
        if (!AssureAnimGraphBufferSize(animGraphInstanceData, requiredFrameBytes + currentFrame.mByteOffset))
        {
            return false;
        }
        uint8* dataPointer = &animGraphInstanceData.mDataBuffer[frameOffset];

        // save all the unique datas for the objects
        for (uint32 i = 0; i < numObjects; ++i)
        {
            // store the object info
            AnimGraphObject* curObject = mObjects[i];
            currentFrame.mObjectInfos[i].mObject = curObject;
            currentFrame.mObjectInfos[i].mFrameByteOffset = currentFrame.mNumBytes;

            // write the unique data
            const uint32 numBytesWritten = curObject->SaveUniqueData(animGraphInstance, dataPointer);

            // increase some offsets/pointers
            currentFrame.mNumBytes += numBytesWritten;
            dataPointer += numBytesWritten;
        }

        // make sure we have a match here, otherwise some of the object->SaveUniqueData(dataPointer) returns different values than object->SaveUniqueData(nullptr)
        MCORE_ASSERT(requiredFrameBytes == currentFrame.mNumBytes);

        return true;
    }


    // make sure our anim graph anim buffer is big enough to hold a specified amount of bytes
    bool Recorder::AssureAnimGraphBufferSize(AnimGraphInstanceData& animGraphInstanceData, uint32 numBytes)
    {
        // if the buffer is big enough, do nothing
        if (animGraphInstanceData.mDataBufferSize >= numBytes)
        {
            return true;
        }

        // we need to reallocate to grow the buffer
        const uint32 newNumBytes = animGraphInstanceData.mDataBufferSize + (numBytes - animGraphInstanceData.mDataBufferSize) * 100; // allocate 100 frames ahead
        void* newBuffer = MCore::Realloc(animGraphInstanceData.mDataBuffer, newNumBytes, EMFX_MEMCATEGORY_RECORDER);
        MCORE_ASSERT(newBuffer);
        if (newBuffer)
        {
            animGraphInstanceData.mDataBuffer = static_cast<uint8*>(newBuffer);
            animGraphInstanceData.mDataBufferSize = newNumBytes;
            return true;
        }
        RecorderNotificationBus::Broadcast(&RecorderNotificationBus::Events::OnRecordingFailed,
            "There is not enough memory to continue the current EMotionFX "
            "recording. It was deleted to free memory in order to keep the "
            "editor stable."
            );
        return false;
    }


    // calculate the memory usage
    uint32 Recorder::CalcMemoryUsage() const
    {
        uint32 result = 0;

        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            // add all animgraph data
            //const uint32 numAnimGraphInstances = actorInstanceData.mAnimGraphDatas.GetLength();
            //result += actorInstanceData.mAnimGraphData.CalcMemoryUsage();
            //for (uint32 i=0; i<numAnimGraphInstances; ++i)
            {
                const AnimGraphInstanceData& animGraphInstanceData = *actorInstanceData.mAnimGraphData;
                result += animGraphInstanceData.mDataBufferSize;
                result += animGraphInstanceData.mFrames.CalcMemoryUsage(true);

                // add all frames
                const uint32 numFrames = animGraphInstanceData.mFrames.GetLength();
                for (uint32 f = 0; f < numFrames; ++f)
                {
                    const AnimGraphAnimFrame& currentFrame = animGraphInstanceData.mFrames[f];
                    result += currentFrame.mObjectInfos.CalcMemoryUsage(true);
                    result += currentFrame.mParameterValues.CalcMemoryUsage(true);

                    // for all parameter values
                    const uint32 numParams = currentFrame.mParameterValues.GetLength();
                    for (uint32 p = 0; p < numParams; ++p)
                    {
                        result += currentFrame.mParameterValues[p]->GetDataSize();
                    }
                }
            }

            // add the transform data
            result += actorInstanceData.mTransformTracks.CalcMemoryUsage(true);
            const uint32 numNodes = actorInstanceData.mTransformTracks.GetLength();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                const TransformTracks& track = actorInstanceData.mTransformTracks[n];
                result += track.mPositions.CalcMemoryUsage(true);
                result += track.mRotations.CalcMemoryUsage(true);

                EMFX_SCALECODE
                (
                    result += track.mScales.CalcMemoryUsage(true);
                    //result += track.mScaleRotations.CalcMemoryUsage(true);
                )
            }

            // add the morphs
            result += actorInstanceData.mMorphTracks.CalcMemoryUsage();
            const uint32 numMorphs = actorInstanceData.mMorphTracks.GetLength();
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                result += actorInstanceData.mMorphTracks[i].CalcMemoryUsage();
            }

            // add the actor instance track
            const TransformTracks& track = actorInstanceData.mActorLocalTransform;
            result += track.mPositions.CalcMemoryUsage(true);
            result += track.mRotations.CalcMemoryUsage(true);
            EMFX_SCALECODE
            (
                result += track.mScales.CalcMemoryUsage(true);
                //result += track.mScaleRotations.CalcMemoryUsage(true);
            )

            // add node history items
            result += actorInstanceData.mNodeHistoryItems.CalcMemoryUsage();
            const uint32 numItems = actorInstanceData.mNodeHistoryItems.GetLength();
            for (uint32 n = 0; n < numItems; ++n)
            {
                NodeHistoryItem* curItem = actorInstanceData.mNodeHistoryItems[n];
                result += sizeof(NodeHistoryItem);
                result += curItem->mGlobalWeights.CalcMemoryUsage(false);
                result += curItem->mLocalWeights.CalcMemoryUsage(false);
                result += curItem->mPlayTimes.CalcMemoryUsage(false);
                result += static_cast<uint32>(curItem->mName.capacity()) + 1;
                result += static_cast<uint32>(curItem->mMotionFileName.capacity()) + 1;
            }
        } // for all actor instances

        result += mActorInstanceDatas.CalcMemoryUsage();
        return result;
    }


    // add a transform key
    void Recorder::AddTransformKey(TransformTracks& track, const AZ::Vector3& pos, const MCore::Quaternion rot, const AZ::Vector3& scale)
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(scale);
    #endif

        // check if we need to add a position key at all
        if (track.mPositions.GetNumKeys() > 0)
        {
            const AZ::Vector3 lastPos = AZ::Vector3(track.mPositions.GetLastKey()->GetValue());
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(pos, lastPos, 0.0001f) == false)
            {
                track.mPositions.AddKey(mRecordTime, AZ::PackedVector3f(pos));
            }
        }
        else
        {
            track.mPositions.AddKey(mRecordTime, AZ::PackedVector3f(pos));
        }


        // check if we need to add a rotation key at all
        if (track.mRotations.GetNumKeys() > 0)
        {
            const MCore::Quaternion lastRot = track.mRotations.GetLastKey()->GetValue();
            if (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(rot, lastRot, 0.0001f) == false)
            {
                track.mRotations.AddKey(mRecordTime, rot);
            }
        }
        else
        {
            track.mRotations.AddKey(mRecordTime, rot);
        }


        EMFX_SCALECODE
        (
            if (mRecordSettings.mRecordScale)
            {
                // check if we need to add a scale key
                if (track.mScales.GetNumKeys() > 0)
                {
                    const AZ::Vector3 lastScale = AZ::Vector3(track.mScales.GetLastKey()->GetValue());
                    if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(scale, lastScale, 0.0001f) == false)
                    {
                        track.mScales.AddKey(mRecordTime, AZ::PackedVector3f(scale));
                    }
                }
                else
                {
                    track.mScales.AddKey(mRecordTime, AZ::PackedVector3f(scale));
                }

                /*
                            // check if we need to add a rotation key at all
                            if (mRecordSettings.mRecordNonUniformScale)
                            {
                                if (track.mScaleRotations.GetNumKeys() > 0)
                                {
                                    const MCore::Quaternion lastRot = track.mScaleRotations.GetLastKey()->GetValue();
                                    if (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(scaleRot, lastRot, 0.0001f) == false)
                                        track.mScaleRotations.AddKey( mRecordTime, scaleRot );
                                }
                                else
                                    track.mScaleRotations.AddKey( mRecordTime, scaleRot );
                            }*/
            }
        )
    }


    // sample and apply
    void Recorder::SampleAndApplyTransforms(float timeInSeconds, ActorInstance* actorInstance) const
    {
        // sample the transforms
        const uint32 index = mRecordSettings.mActorInstances.Find(actorInstance);
        if (index != MCORE_INVALIDINDEX32)
        {
            SampleAndApplyTransforms(timeInSeconds, index);
        }
    }


    // sample and apply all animgraphs
    void Recorder::SampleAndApplyAnimGraphs(float timeInSeconds) const
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            // sample and apply the animgraph changes
            //const uint32 numAnimGraphs = actorInstanceData.mAnimGraphDatas.GetLength();
            //for (uint32 i=0; i<numAnimGraphs; ++i)
            SampleAndApplyAnimGraphStates(timeInSeconds, *actorInstanceData.mAnimGraphData);
        }
    }


    // sample and apply a given main actor instance transform
    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const uint32 index = mRecordSettings.mActorInstances.Find(actorInstance);
        if (index != MCORE_INVALIDINDEX32)
        {
            SampleAndApplyMainTransform(timeInSeconds, index);
        }
    }


    // sample and apply morph targets
    void Recorder::SampleAndApplyMorphs(float timeInSeconds, ActorInstance* actorInstance) const
    {
        const uint32 index = mRecordSettings.mActorInstances.Find(actorInstance);
        if (index != MCORE_INVALIDINDEX32)
        {
            const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[index];
            const uint32 numMorphs = actorInstanceData.mMorphTracks.GetLength();
            if (numMorphs == actorInstance->GetMorphSetupInstance()->GetNumMorphTargets())
            {
                for (uint32 i = 0; i < numMorphs; ++i)
                {
                    actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->SetWeight(actorInstanceData.mMorphTracks[i].GetValueAtTime(timeInSeconds));
                    //  actorInstance->GetMorphSetupInstance()->GetMorphTarget(i)->SetManualMode(true);
                }
            }
        }
    }


    // sample and apply the actor instance transform
    void Recorder::SampleAndApplyMainTransform(float timeInSeconds, uint32 actorInstanceIndex) const
    {
        // get the actor instance
        const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.mActorInstance;

        // sample and apply
        const TransformTracks& track = actorInstanceData.mActorLocalTransform;
        actorInstance->SetLocalSpacePosition(AZ::Vector3(track.mPositions.GetValueAtTime(timeInSeconds)));
        actorInstance->SetLocalSpaceRotation(track.mRotations.GetValueAtTime(timeInSeconds));
        EMFX_SCALECODE
        (
            if (mRecordSettings.mRecordScale)
            {
                actorInstance->SetLocalSpaceScale(AZ::Vector3(track.mScales.GetValueAtTime(timeInSeconds)));
            }
        )
    }


    // sample and apply transforms
    void Recorder::SampleAndApplyTransforms(float timeInSeconds, uint32 actorInstanceIndex) const
    {
        // get the actor instance
        const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[actorInstanceIndex];
        ActorInstance* actorInstance = actorInstanceData.mActorInstance;

        // get the transform data
        TransformData* transformData = actorInstance->GetTransformData();
        //Transform* localTransforms = transformData->GetLocalTransforms();

        // for all nodes in the actor instance
        Transform outTransform;
        const uint32 numNodes = actorInstance->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            outTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(n);
            const TransformTracks& track = actorInstanceData.mTransformTracks[n];

            // build the output transform by sampling the keytracks
            outTransform.mPosition      = AZ::Vector3(track.mPositions.GetValueAtTime(timeInSeconds));
            outTransform.mRotation      = track.mRotations.GetValueAtTime(timeInSeconds);

            EMFX_SCALECODE
            (
                if (mRecordSettings.mRecordScale)
                {
                    outTransform.mScale = AZ::Vector3(track.mScales.GetValueAtTime(timeInSeconds));

                    //if (mRecordSettings.mRecordNonUniformScale)
                    //outTransform.mScaleRotation   = track.mScaleRotations.GetValueAtTime( timeInSeconds );
                }
            )

            // set the transform
            transformData->GetCurrentPose()->SetLocalSpaceTransform(n, outTransform);
        }
    }


    // sample and apply anim graphs
    void Recorder::SampleAndApplyAnimGraphStates(float timeInSeconds, const AnimGraphInstanceData& animGraphInstanceData) const
    {
        // find out the frame number
        const uint32 frameNumber = FindAnimGraphDataFrameNumber(timeInSeconds);
        if (frameNumber == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // for all animgraph instances that we recorded, restore their internal states
        const AnimGraphInstance* animGraphInstance = animGraphInstanceData.mAnimGraphInstance;

        // get the real frame number (clamped)
        const uint32 realFrameNumber = MCore::Min<uint32>(frameNumber, animGraphInstanceData.mFrames.GetLength() - 1);
        const AnimGraphAnimFrame& currentFrame = animGraphInstanceData.mFrames[realFrameNumber];

        // get the data and objects buffers
        const uint32 byteOffset = currentFrame.mByteOffset;
        const uint8* frameDataBuffer = &animGraphInstanceData.mDataBuffer[byteOffset];
        const MCore::Array<AnimGraphAnimObjectInfo>& frameObjects = currentFrame.mObjectInfos;

        // first lets update all parameter values
        MCORE_ASSERT(currentFrame.mParameterValues.GetLength() == animGraphInstance->GetAnimGraph()->GetNumParameters());
        const uint32 numParameters = currentFrame.mParameterValues.GetLength();
        for (uint32 p = 0; p < numParameters; ++p)
        {
            //  make sure the parameters are of the same type
            MCORE_ASSERT(animGraphInstance->GetParameterValue(p)->GetType() == currentFrame.mParameterValues[p]->GetType());
            animGraphInstance->GetParameterValue(p)->InitFrom(currentFrame.mParameterValues[p]);
        }

        // process all objects for this frame
        uint32 totalBytesRead = 0;
        const uint32 numObjects = frameObjects.GetLength();
        for (uint32 a = 0; a < numObjects; ++a)
        {
            const AnimGraphAnimObjectInfo& objectInfo = frameObjects[a];
            const uint32 numBytesRead = objectInfo.mObject->LoadUniqueData(animGraphInstance, &frameDataBuffer[objectInfo.mFrameByteOffset]);
            totalBytesRead += numBytesRead;
        }

        // make sure this matches, otherwise the data read is not the same as we have written
        MCORE_ASSERT(totalBytesRead == currentFrame.mNumBytes);
    }


    // find the actor instance data
    uint32 Recorder::FindActorInstanceDataIndex(ActorInstance* actorInstance) const
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            if (mActorInstanceDatas[a]->mActorInstance == actorInstance)
            {
                return a;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // update the node history items
    void Recorder::UpdateNodeHistoryItems()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData.mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&mActiveNodes);

            // get the history items as shortcut
            MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;

            // finalize items
            const uint32 numActiveNodes = mActiveNodes.GetLength();
            const uint32 numHistoryItems = historyItems.GetLength();
            for (uint32 h = 0; h < numHistoryItems; ++h)
            {
                NodeHistoryItem* curItem = historyItems[h];
                if (curItem->mIsFinalized)
                {
                    continue;
                }

                // check if we have an active node for the given item
                uint32 index = MCORE_INVALIDINDEX32;
                for (uint32 x = 0; x < numActiveNodes; ++x)
                {
                    if (mActiveNodes[x]->GetId() == curItem->mNodeId)
                    {
                        index = x;
                        break;
                    }
                }

                // the node got deactivated, finalize the item
                if (index == MCORE_INVALIDINDEX32)
                {
                    curItem->mGlobalWeights.Optimize(0.0001f);
                    curItem->mLocalWeights.Optimize(0.0001f);
                    curItem->mPlayTimes.Optimize(0.0001f);
                    curItem->mIsFinalized = true;
                    curItem->mEndTime = mRecordTime;
                    continue;
                }
            }

            // iterate over all active nodes
            for (uint32 i = 0; i < numActiveNodes; ++i)
            {
                AnimGraphNode* activeNode = mActiveNodes[i];
                if (activeNode == animGraphInstance->GetRootNode()) // skip the root node
                {
                    continue;
                }

                const AZ::TypeId typeID = azrtti_typeid(activeNode);

                // if the parent isn't a state machine then it isn't a state
                if (mRecordSettings.mHistoryStatesOnly)
                {
                    if (azrtti_typeid(activeNode->GetParentNode()) != azrtti_typeid<AnimGraphStateMachine>())
                    {
                        continue;
                    }
                }

                // make sure this node is on our capture list
                if (!mRecordSettings.mNodeHistoryTypes.empty())
                {
                    if (mRecordSettings.mNodeHistoryTypes.find(typeID) == mRecordSettings.mNodeHistoryTypes.end())
                    {
                        continue;
                    }
                }

                // skip node types we do not want to capture
                if (!mRecordSettings.mNodeHistoryTypesToIgnore.empty())
                {
                    if (mRecordSettings.mNodeHistoryTypesToIgnore.find(typeID) != mRecordSettings.mNodeHistoryTypesToIgnore.end())
                    {
                        continue;
                    }
                }

                // try to locate an existing item
                NodeHistoryItem* item = FindNodeHistoryItem(actorInstanceData, activeNode, mRecordTime);
                if (item == nullptr)
                {
                    item = new NodeHistoryItem();
                    item->mName             = activeNode->GetName();
                    item->mAnimGraphID      = animGraphInstance->GetAnimGraph()->GetID();
                    item->mStartTime        = mRecordTime;
                    item->mIsFinalized      = false;
                    item->mTrackIndex       = FindFreeNodeHistoryItemTrack(actorInstanceData, item);
                    item->mNodeId           = activeNode->GetId();
                    item->mColor            = activeNode->GetVisualizeColor();
                    item->mTypeColor        = activeNode->GetVisualColor();
                    item->mCategoryID       = (uint32)activeNode->GetPaletteCategory();
                    item->mNodeType         = typeID;
                    item->mAnimGraphInstance = animGraphInstance;
                    item->mGlobalWeights.Reserve(1024);
                    item->mLocalWeights.Reserve(1024);
                    item->mPlayTimes.Reserve(1024);

                    // get the motion instance
                    if (typeID == azrtti_typeid<AnimGraphMotionNode>())
                    {
                        AnimGraphMotionNode* motionNode = static_cast<AnimGraphMotionNode*>(activeNode);
                        MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
                        if (motionInstance)
                        {
                            item->mMotionID = motionInstance->GetMotion()->GetID();
                            AzFramework::StringFunc::Path::GetFileName(item->mMotionFileName.c_str(), item->mMotionFileName);
                        }
                    }

                    historyItems.Add(item);
                }

                // add the weight key and update infos
                const AnimGraphNodeData* uniqueData = activeNode->FindUniqueNodeData(animGraphInstance);
                const float keyTime = mRecordTime - item->mStartTime;
                item->mGlobalWeights.AddKey(keyTime, uniqueData->GetGlobalWeight());
                item->mLocalWeights.AddKey(keyTime, uniqueData->GetLocalWeight());

                float normalizedTime = uniqueData->GetCurrentPlayTime();
                const float duration = uniqueData->GetDuration();
                if (duration > MCore::Math::epsilon)
                {
                    normalizedTime /= duration;
                }
                else
                {
                    normalizedTime = 0.0f;
                }

                item->mPlayTimes.AddKey(keyTime, normalizedTime);
                item->mEndTime  = mRecordTime;
            }
        } // for all actor instances
    }


    // try to find a given node history item
    Recorder::NodeHistoryItem* Recorder::FindNodeHistoryItem(const ActorInstanceData& actorInstanceData, AnimGraphNode* node, float recordTime) const
    {
        const MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            NodeHistoryItem* curItem = historyItems[i];
            if (curItem->mNodeId == node->GetId() && curItem->mStartTime <= recordTime && curItem->mIsFinalized == false)
            {
                return curItem;
            }

            if (curItem->mNodeId == node->GetId() && curItem->mStartTime <= recordTime && curItem->mEndTime >= recordTime && curItem->mIsFinalized)
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // find a free track
    uint32 Recorder::FindFreeNodeHistoryItemTrack(const ActorInstanceData& actorInstanceData, NodeHistoryItem* item) const
    {
        const MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        const uint32 numItems = historyItems.GetLength();

        bool found = false;
        uint32 trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (uint32 i = 0; i < numItems; ++i)
            {
                NodeHistoryItem* curItem = historyItems[i];
                if (curItem->mTrackIndex != trackIndex)
                {
                    continue;
                }

                // if the current item is not active anymore
                if (curItem->mIsFinalized)
                {
                    // if the start time of the item we try to insert is within the range of this item
                    if (item->mStartTime > curItem->mStartTime && item->mStartTime < curItem->mEndTime)
                    {
                        hasCollision = true;
                        break;
                    }

                    // if the end time of this item is within the range of this item
                    if (item->mEndTime > curItem->mStartTime && item->mEndTime < curItem->mEndTime)
                    {
                        hasCollision = true;
                        break;
                    }
                    /*
                                    if (MCore::Compare<float>::CheckIfIsClose(item->mStartTime, curItem->mStartTime, 0.001f) || MCore::Compare<float>::CheckIfIsClose(item->mStartTime, curItem->mEndTime, 0.001f))
                                    {
                                        hasCollision = true;
                                        break;
                                    }*/
                }
                else // if the current item is still active and has no real end time yet
                {
                    if (item->mStartTime >= curItem->mStartTime)
                    {
                        hasCollision = true;
                        break;
                    }
                }
            }

            if (hasCollision)
            {
                trackIndex++;
            }
            else
            {
                found = true;
            }
        }

        return trackIndex;
    }


    // find the maximum track index
    uint32 Recorder::CalcMaxNodeHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const
    {
        uint32 result = 0;
        const MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            NodeHistoryItem* curItem = historyItems[i];
            if (curItem->mTrackIndex > result)
            {
                result = curItem->mTrackIndex;
            }
        }

        return result;
    }


    // find the maximum event track index
    uint32 Recorder::CalcMaxEventHistoryTrackIndex(const ActorInstanceData& actorInstanceData) const
    {
        uint32 result = 0;
        const MCore::Array<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EventHistoryItem* curItem = historyItems[i];
            if (curItem->mTrackIndex > result)
            {
                result = curItem->mTrackIndex;
            }
        }

        return result;
    }


    // find the maximum track index
    uint32 Recorder::CalcMaxNodeHistoryTrackIndex() const
    {
        uint32 result = 0;

        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            result = MCore::Max<uint32>(result, CalcMaxNodeHistoryTrackIndex(*mActorInstanceDatas[a]));
        }

        return result;
    }


    // finalize all items
    void Recorder::FinalizeAllNodeHistoryItems()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData.mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // collect all active motion nodes
            animGraphInstance->CollectActiveAnimGraphNodes(&mActiveNodes);

            // get the history items as shortcut
            MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;

            // finalize all items
            const uint32 numItems = historyItems.GetLength();
            for (uint32 i = 0; i < numItems; ++i)
            {
                // remove unneeded key frames
                if (historyItems[i]->mIsFinalized == false)
                {
                    historyItems[i]->mGlobalWeights.Optimize(0.0001f);
                    historyItems[i]->mLocalWeights.Optimize(0.0001f);
                    historyItems[i]->mPlayTimes.Optimize(0.0001f);
                    historyItems[i]->mIsFinalized = true;
                }
            }
        }
    }


    // record events
    void Recorder::RecordEvents()
    {
        // for all actor instances
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 a = 0; a < numActorInstances; ++a)
        {
            ActorInstanceData& actorInstanceData = *mActorInstanceDatas[a];

            // get the animgraph instance
            AnimGraphInstance* animGraphInstance = actorInstanceData.mActorInstance->GetAnimGraphInstance();
            if (animGraphInstance == nullptr)
            {
                continue;
            }

            // get the event buffer
            const AnimGraphEventBuffer& eventBuffer = animGraphInstance->GetEventBuffer();

            // iterate over all events
            MCore::Array<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
            const uint32 numEvents = eventBuffer.GetNumEvents();
            for (uint32 i = 0; i < numEvents; ++i)
            {
                const EventInfo& eventInfo = eventBuffer.GetEvent(i);
                if (eventInfo.m_eventState == EventInfo::EventInfo::ACTIVE)
                {
                    continue;
                }
                EventHistoryItem* item = FindEventHistoryItem(actorInstanceData, eventInfo, mRecordTime);
                if (item == nullptr) // create a new one
                {
                    item = new EventHistoryItem();

                    // TODO
                    //item->mEventIndex   = GetEventManager().FindEventTypeIndex(eventInfo.mTypeID);
                    item->mEventInfo    = eventInfo;
                    item->mIsTickEvent  = eventInfo.mEvent->GetIsTickEvent();
                    item->mStartTime    = mRecordTime;
                    item->mAnimGraphID  = animGraphInstance->GetAnimGraph()->GetID();
                    item->mEmitterNodeId= eventInfo.mEmitter->GetId();
                    item->mColor        = eventInfo.mEmitter->GetVisualizeColor();

                    item->mTrackIndex   = FindFreeEventHistoryItemTrack(actorInstanceData, item);

                    historyItems.Add(item);
                }

                item->mEndTime  = mRecordTime;
            }
        }
    }


    // find a given event
    Recorder::EventHistoryItem* Recorder::FindEventHistoryItem(const ActorInstanceData& actorInstanceData, const EventInfo& eventInfo, float recordTime)
    {
        MCORE_UNUSED(recordTime);
        const MCore::Array<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EventHistoryItem* curItem = historyItems[i];

            if (curItem->mStartTime < eventInfo.mTimeValue)
            {
                continue;
            }
        }

        return nullptr;
    }


    // find a free event track index
    uint32 Recorder::FindFreeEventHistoryItemTrack(const ActorInstanceData& actorInstanceData, EventHistoryItem* item) const
    {
        const MCore::Array<EventHistoryItem*>& historyItems = actorInstanceData.mEventHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        bool found = false;
        uint32 trackIndex = 0;
        while (found == false)
        {
            bool hasCollision = false;

            for (uint32 i = 0; i < numItems; ++i)
            {
                EventHistoryItem* curItem = historyItems[i];
                if (curItem->mTrackIndex != trackIndex)
                {
                    continue;
                }

                if (MCore::Compare<float>::CheckIfIsClose(curItem->mStartTime, item->mStartTime, 0.01f))
                {
                    hasCollision = true;
                    break;
                }
            }

            if (hasCollision)
            {
                trackIndex++;
            }
            else
            {
                found = true;
            }
        }

        return trackIndex;
    }


    // find the frame number for a time value
    uint32 Recorder::FindAnimGraphDataFrameNumber(float timeValue) const
    {
        // check if we recorded any actor instances at all
        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        if (numActorInstances == 0)
        {
            return 0;
        }

        // just search in the first actor instances data
        const ActorInstanceData& actorInstanceData = *mActorInstanceDatas[0];
        const AnimGraphInstanceData* animGraphData = actorInstanceData.mAnimGraphData;
        if (animGraphData == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        const uint32 numFrames = animGraphData->mFrames.GetLength();
        if (numFrames == 0)
        {
            return MCORE_INVALIDINDEX32;
        }
        else
        if (numFrames == 1)
        {
            return 0;
        }

        if (timeValue <= 0.0f)
        {
            return 0;
        }

        if (timeValue > animGraphData->mFrames.GetLast().mTimeValue)
        {
            return animGraphData->mFrames.GetLength() - 1;
        }

        for (uint32 i = 0; i < numFrames - 1; ++i)
        {
            const AnimGraphAnimFrame& curFrame  = animGraphData->mFrames[i];
            const AnimGraphAnimFrame& nextFrame = animGraphData->mFrames[i + 1];
            if (curFrame.mTimeValue <= timeValue && nextFrame.mTimeValue > timeValue)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // remove an actor instance from the recorder
    void Recorder::RemoveActorInstanceFromRecording(ActorInstance* actorInstance)
    {
        Lock();

        // check if we recorded any actor instances at all
        for (uint32 i = 0; i < mActorInstanceDatas.GetLength();)
        {
            if (mActorInstanceDatas[i]->mActorInstance == actorInstance)
            {
                delete mActorInstanceDatas[i];
                mActorInstanceDatas.Remove(i);
            }
            else
            {
                i++;
            }
        }

        Unlock();
    }


    // remove an animgraph from the recording
    void Recorder::RemoveAnimGraphFromRecording(AnimGraph* animGraph)
    {
        Lock();

        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            if (mActorInstanceDatas[i]->mAnimGraphData && mActorInstanceDatas[i]->mAnimGraphData->mAnimGraphInstance)
            {
                AnimGraph* curAnimGraph = mActorInstanceDatas[i]->mAnimGraphData->mAnimGraphInstance->GetAnimGraph();
                if (animGraph != curAnimGraph)
                {
                    continue;
                }

                delete mActorInstanceDatas[i]->mAnimGraphData;
                mActorInstanceDatas[i]->mAnimGraphData = nullptr;
            }
        }

        Unlock();
    }


    // multithread lock the recorder
    void Recorder::Lock()
    {
        mLock.Lock();
    }


    // multithread unlock
    void Recorder::Unlock()
    {
        mLock.Unlock();
    }


    // extract sorted active items
    void Recorder::ExtractNodeHistoryItems(const ActorInstanceData& actorInstanceData, float timeValue, bool sort, EValueType valueType, MCore::Array<ExtractedNodeHistoryItem>* outItems, MCore::Array<uint32>* outMap)
    {
        // clear the map array
        const uint32 maxIndex = CalcMaxNodeHistoryTrackIndex(actorInstanceData);
        outItems->Resize(maxIndex + 1);
        for (uint32 i = 0; i <= maxIndex; ++i)
        {
            ExtractedNodeHistoryItem item;
            item.mTrackIndex        = i;
            item.mValue             = 0.0f;
            item.mKeyTrackSampleTime = 0.0f;
            item.mNodeHistoryItem   = nullptr;
            outItems->SetElem(i, item);
        }

        // find all node history items
        const MCore::Array<NodeHistoryItem*>& historyItems = actorInstanceData.mNodeHistoryItems;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            NodeHistoryItem* curItem = historyItems[i];
            if (curItem->mStartTime <= timeValue && curItem->mEndTime > timeValue)
            {
                ExtractedNodeHistoryItem item;
                item.mTrackIndex        = curItem->mTrackIndex;
                item.mKeyTrackSampleTime = timeValue - curItem->mStartTime;
                item.mNodeHistoryItem   = curItem;

                switch (valueType)
                {
                case VALUETYPE_GLOBALWEIGHT:
                    item.mValue = curItem->mGlobalWeights.GetValueAtTime(item.mKeyTrackSampleTime);
                    break;

                case VALUETYPE_LOCALWEIGHT:
                    item.mValue = curItem->mLocalWeights.GetValueAtTime(item.mKeyTrackSampleTime);
                    break;

                case VALUETYPE_PLAYTIME:
                    item.mValue = curItem->mPlayTimes.GetValueAtTime(item.mKeyTrackSampleTime);
                    break;

                default:
                    MCORE_ASSERT(false);    // unsupported mode
                    item.mValue = curItem->mGlobalWeights.GetValueAtTime(item.mKeyTrackSampleTime);
                }

                outItems->SetElem(curItem->mTrackIndex, item);
            }
        }

        // build the map
        outMap->Resize(maxIndex + 1);
        for (uint32 i = 0; i <= maxIndex; ++i)
        {
            outMap->SetElem(i, i);
        }

        // sort if desired
        if (sort)
        {
            outItems->Sort();

            for (uint32 i = 0; i <= maxIndex; ++i)
            {
                outMap->SetElem(outItems->GetItem(i).mTrackIndex, i);
            }
        }
    }


    AZ::u32 Recorder::CalcMaxNumActiveMotions(const ActorInstanceData& actorInstanceData) const
    {
        AZ::u32 result = 0;

        // Array of flags that each iteration will use to determine if a given track has already been counted in or not.
        AZStd::vector<bool> trackFlags;
        const size_t maxNumTracks = static_cast<size_t>(CalcMaxNodeHistoryTrackIndex()) + 1;
        trackFlags.resize(maxNumTracks);

        const uint32 numNodeHistoryItems = actorInstanceData.mNodeHistoryItems.GetLength();
        for (uint32 i = 0; i < numNodeHistoryItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* item = actorInstanceData.mNodeHistoryItems[i];

            // Only process motion history items.
            if (item->mMotionID == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // Reset the track flags
            for (size_t t = 0; t < maxNumTracks; ++t)
            {
                trackFlags[t] = false;
            }

            // We at least have a single active motion.
            AZ::u32 intermediateResult = 1;
            trackFlags[item->mTrackIndex] = true;

            for (uint32 j = 0; j < numNodeHistoryItems; ++j)
            {
                EMotionFX::Recorder::NodeHistoryItem* innerItem = actorInstanceData.mNodeHistoryItems[j];

                // Did we count this track in already? If yes, skip.
                if (trackFlags[innerItem->mTrackIndex])
                {
                    continue;
                }

                // Skip self comparison and only process motion history items.
                if (i == j || innerItem->mMotionID == MCORE_INVALIDINDEX32)
                {
                    continue;
                }

                // Are the item and innerItem events overlapping?
                if ((item->mStartTime >= innerItem->mStartTime && item->mStartTime <= innerItem->mEndTime) ||
                    (item->mEndTime >= innerItem->mStartTime && item->mEndTime <= innerItem->mEndTime) ||
                    (innerItem->mStartTime >= item->mStartTime && innerItem->mStartTime <= item->mEndTime) ||
                    (innerItem->mEndTime >= item->mStartTime && innerItem->mEndTime <= item->mEndTime))
                {
                    intermediateResult++;
                    trackFlags[innerItem->mTrackIndex] = true;
                }
            }

            result = MCore::Max<AZ::u32>(result, intermediateResult);
        }

        return result;
    }


    AZ::u32 Recorder::CalcMaxNumActiveMotions() const
    {
        AZ::u32 result = 0;

        const uint32 numActorInstances = mActorInstanceDatas.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            const ActorInstanceData* actorInstanceData = mActorInstanceDatas[i];
            result = MCore::Max<AZ::u32>(result, CalcMaxNumActiveMotions(*actorInstanceData));
        }

        return result;
    }
} // namespace EMotionFX

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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "TrackViewNodes.h"
#include "TrackViewAnimNode.h"
#include "TrackViewUndo.h"
#include "TrackViewSequenceManager.h"
#include "AnimationContext.h"

#include <IMovieSystem.h>

#include "Util/BoostPythonHelpers.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"

namespace
{
    CTrackViewSequence* GetSequenceByEntityIdOrName(const CTrackViewSequenceManager* pSequenceManager, const char* entityIdOrName)
    {
        // the "name" string will be an AZ::EntityId in string form if this was called from
        // TrackView code. But for backward compatibility we also support a sequence name.
        bool isNameAValidU64 = false;
        QString entityIdString = entityIdOrName;
        AZ::u64 nameAsU64 = entityIdString.toULongLong(&isNameAValidU64);

        CTrackViewSequence* pSequence = nullptr;
        if (isNameAValidU64)
        {
            // "name" string was a valid u64 represented as a string. Use as an entity Id to search for sequence.
            pSequence = pSequenceManager->GetSequenceByEntityId(AZ::EntityId(nameAsU64));
        }

        if (!pSequence)
        {
            // name passed in could not find a sequence by using it as an EntityId. Use it as a
            // sequence name for backward compatibility
            pSequence = pSequenceManager->GetSequenceByName(entityIdOrName);
        }

        return pSequence;
    }

}

namespace
{
    //////////////////////////////////////////////////////////////////////////
    // Misc
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewSetRecording(bool bRecording)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (pAnimationContext)
        {
            pAnimationContext->SetRecording(bRecording);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Sequences
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewNewSequence(const char* name, int sequenceType)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();

        CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByName(name);
        if (pSequence)
        {
            throw std::runtime_error("A sequence with this name already exists");
        }

        CUndo undo("Create TrackView sequence");
        pSequenceManager->CreateSequence(name, static_cast<SequenceType>(sequenceType));
    }

    void PyTrackViewDeleteSequence(const char* name)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (pSequence)
        {
            pSequenceManager->DeleteSequence(pSequence);
            return;
        }

        throw std::runtime_error("Could not find sequence");
    }

    void PyTrackViewSetCurrentSequence(const char* name)
    {
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = GetSequenceByEntityIdOrName(sequenceManager, name);
        CAnimationContext* animationContext = GetIEditor()->GetAnimation();
        bool force = false;
        bool noNotify = false;
        bool user = true;
        animationContext->SetSequence(sequence, force, noNotify, user);
    }

    int PyTrackViewGetNumSequences()
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        return pSequenceManager->GetCount();
    }

    QString PyTrackViewGetSequenceName(unsigned int index)
    {
        if (index < PyTrackViewGetNumSequences())
        {
            const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
            return pSequenceManager->GetSequenceByIndex(index)->GetName();
        }

        throw std::runtime_error("Could not find sequence");
    }

    boost::python::tuple PyTrackViewGetSequenceTimeRange(const char* name)
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (!pSequence)
        {
            throw std::runtime_error("A sequence with this name doesn't exists");
        }

        const Range timeRange = pSequence->GetTimeRange();
        return boost::python::make_tuple(timeRange.start, timeRange.end);
    }

    void PyTrackViewSetSequenceTimeRange(const char* name, float start, float end)
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, name);
        if (!pSequence)
        {
            throw std::runtime_error("A sequence with this name doesn't exists");
        }

        CUndo undo("Set sequence time range");
        pSequence->SetTimeRange(Range(start, end));
        pSequence->MarkAsModified();
    }

    void PyTrackViewPlaySequence()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (pAnimationContext->IsPlaying())
        {
            throw std::runtime_error("A sequence is already playing");
        }

        pAnimationContext->SetPlaying(true);
    }

    void PyTrackViewStopSequence()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        if (!pAnimationContext->IsPlaying())
        {
            throw std::runtime_error("No sequence is playing");
        }

        pAnimationContext->SetPlaying(false);
    }

    void PyTrackViewSetSequenceTime(float time)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        pAnimationContext->SetTime(time);
    }

    //////////////////////////////////////////////////////////////////////////
    // Nodes
    //////////////////////////////////////////////////////////////////////////
    void PyTrackViewAddNode(const char* nodeTypeString, const char* nodeName)
    {
        CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        const AnimNodeType nodeType = GetIEditor()->GetMovieSystem()->GetNodeTypeFromString(nodeTypeString);
        if (nodeType == AnimNodeType::Invalid)
        {
            throw std::runtime_error("Invalid node type");
        }

        CUndo undo("Create anim node");
        pSequence->CreateSubNode(nodeName, nodeType);
    }

    void PyTrackViewAddSelectedEntities()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CUndo undo("Add entities to TrackView");

        // Create Position, Rotation and Event by default to preserve compatibility with
        // existing scripts.
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
        DynArray<unsigned int> trackCount(pMovieSystem->GetEntityNodeParamCount(), 0);
        for (int i = 0; i < trackCount.size(); ++i)
        {
            CAnimParamType paramType = pMovieSystem->GetEntityNodeParamType(i);
            if (paramType == AnimParamType::Position || paramType == AnimParamType::Rotation || paramType == AnimParamType::Event)
            {
                trackCount[i] = 1;
            }
        }

        pSequence->AddSelectedEntities(trackCount);
        pSequence->BindToEditorObjects();
    }

    void PyTrackViewAddLayerNode()
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CUndo undo("Add current layer to TrackView");
        pSequence->AddCurrentLayer();
    }

    CTrackViewAnimNode* GetNodeFromName(const char* nodeName, const char* parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (strlen(parentDirectorName) > 0)
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName);
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAnimNodesByName(nodeName);
        return (foundNodes.GetCount() > 0) ? foundNodes.GetNode(0) : nullptr;
    }

    void PyTrackViewDeleteNode(const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (pNode == nullptr)
        {
            throw std::runtime_error("Couldn't find node");
        }

        CTrackViewAnimNode* pParentNode = static_cast<CTrackViewAnimNode*>(pNode->GetParentNode());

        CUndo undo("Delete TrackView Node");
        pParentNode->RemoveSubNode(pNode);
    }

    void PyTrackViewAddTrack(const char* paramName, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        // Add tracks to menu, that can be added to animation node.
        const int paramCount = pNode->GetParamCount();
        for (int i = 0; i < paramCount; ++i)
        {
            CAnimParamType paramType = pNode->GetParamType(i);

            if (paramType == AnimParamType::Invalid)
            {
                continue;
            }

            IAnimNode::ESupportedParamFlags paramFlags = pNode->GetParamFlags(paramType);

            CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType);
            if (!pTrack || (paramFlags & IAnimNode::eSupportedParamFlags_MultipleTracks))
            {
                const char* name = pNode->GetParamName(paramType);
                if (_stricmp(name, paramName) == 0)
                {
                    CUndo undo("Create track");
                    if (!pNode->CreateTrack(paramType))
                    {
                        undo.Cancel();
                        throw std::runtime_error("Could not create track");
                    }

                    pNode->SetSelected(true);
                    return;
                }
            }
        }

        throw std::runtime_error("Could not create track");
    }

    void PyTrackViewDeleteTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        const CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetParamTypeFromString(paramName);
        CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType, index);
        if (!pTrack)
        {
            throw std::runtime_error("Could not find track");
        }

        CUndo undo("Delete TrackView track");
        pNode->RemoveTrack(pTrack);
    }

    int PyTrackViewGetNumNodes(const char* parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (strlen(parentDirectorName) > 0)
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName);
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAllAnimNodes();
        return foundNodes.GetCount();
    }

    QString PyTrackViewGetNodeName(int index, const char* parentDirectorName)
    {
        CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
        CTrackViewSequence* pSequence = pAnimationContext->GetSequence();
        if (!pSequence)
        {
            throw std::runtime_error("No sequence is active");
        }

        CTrackViewAnimNode* pParentDirector = pSequence;
        if (strlen(parentDirectorName) > 0)
        {
            CTrackViewAnimNodeBundle foundNodes = pSequence->GetAnimNodesByName(parentDirectorName);
            if (foundNodes.GetCount() == 0 || foundNodes.GetNode(0)->GetType() != AnimNodeType::Director)
            {
                throw std::runtime_error("Director node not found");
            }

            pParentDirector = foundNodes.GetNode(0);
        }

        CTrackViewAnimNodeBundle foundNodes = pParentDirector->GetAllAnimNodes();
        if (index < 0 || index >= foundNodes.GetCount())
        {
            throw std::runtime_error("Invalid node index");
        }

        return foundNodes.GetNode(index)->GetName();
    }

    //////////////////////////////////////////////////////////////////////////
    // Tracks
    //////////////////////////////////////////////////////////////////////////
    CTrackViewTrack* GetTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewAnimNode* pNode = GetNodeFromName(nodeName, parentDirectorName);
        if (!pNode)
        {
            throw std::runtime_error("Couldn't find node");
        }

        const CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetParamTypeFromString(paramName);
        CTrackViewTrack* pTrack = pNode->GetTrackForParameter(paramType, index);
        if (!pTrack)
        {
            throw std::runtime_error("Track doesn't exist");
        }

        return pTrack;
    }

    std::set<float> GetKeyTimeSet(CTrackViewTrack* pTrack)
    {
        std::set<float> keyTimeSet;
        for (uint i = 0; i < pTrack->GetKeyCount(); ++i)
        {
            CTrackViewKeyHandle keyHandle = pTrack->GetKey(i);
            keyTimeSet.insert(keyHandle.GetTime());
        }

        return keyTimeSet;
    }

    int PyTrackViewGetNumTrackKeys(const char* paramName, int trackIndex, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);
        return (int)GetKeyTimeSet(pTrack).size();
    }

    SPyWrappedProperty PyTrackViewGetInterpolatedValue(const char* paramName, int trackIndex, float time, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);

        SPyWrappedProperty prop;

        switch (pTrack->GetValueType())
        {
        case AnimValueType::Float:
        case AnimValueType::DiscreteFloat:
        {
            float value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Float;
            prop.property.floatValue = value;
        }
        break;
        case AnimValueType::Bool:
        {
            bool value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Bool;
            prop.property.boolValue = value;
        }
        break;
        case AnimValueType::Quat:
        {
            Quat value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Vec3;
            Ang3 rotation(value);
            prop.property.vecValue.x = rotation.x;
            prop.property.vecValue.y = rotation.y;
            prop.property.vecValue.z = rotation.z;
        }
        case AnimValueType::Vector:
        {
            Vec3 value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Vec3;
            prop.property.vecValue.x = value.x;
            prop.property.vecValue.y = value.y;
            prop.property.vecValue.z = value.z;
        }
        break;
        case AnimValueType::Vector4:
        {
            Vec4 value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Vec4;
            prop.property.vecValue.x = value.x;
            prop.property.vecValue.y = value.y;
            prop.property.vecValue.z = value.z;
            prop.property.vecValue.w = value.w;
        }
        break;
        case AnimValueType::RGB:
        {
            Vec3 value;
            pTrack->GetValue(time, value);
            prop.type = SPyWrappedProperty::eType_Color;
            prop.property.colorValue.r = value.x;
            prop.property.colorValue.g = value.y;
            prop.property.colorValue.b = value.z;
        }
        break;
        default:
            throw std::runtime_error("Unsupported key type");
        }

        return prop;
    }

    SPyWrappedProperty PyTrackViewGetKeyValue(const char* paramName, int trackIndex, int keyIndex, const char* nodeName, const char* parentDirectorName)
    {
        CTrackViewTrack* pTrack = GetTrack(paramName, trackIndex, nodeName, parentDirectorName);

        std::set<float> keyTimeSet = GetKeyTimeSet(pTrack);
        if (keyIndex < 0 || keyIndex >= keyTimeSet.size())
        {
            throw std::runtime_error("Invalid key index");
        }

        auto keyTimeIter = keyTimeSet.begin();
        std::advance(keyTimeIter, keyIndex);
        const float keyTime = *keyTimeIter;

        return PyTrackViewGetInterpolatedValue(paramName, trackIndex, keyTime, nodeName, parentDirectorName);
    }
}

// General
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewSetRecording, trackview, set_recording,
    "Activates/deactivates TrackView recording mode",
    "trackview.set_recording(bool bRecording)");

// Sequences
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewNewSequence, trackview, new_sequence,
    "Creates a new sequence of the given type (0=Object Entity Sequence (Legacy), 1=Component Entity Sequence (PREVIEW)) with the given name.",
    "trackview.new_sequence(str sequenceName, int sequenceType)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewDeleteSequence, trackview, delete_sequence,
    "Deletes the specified sequence.",
    "trackview.delete_sequence(str sequenceName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewSetCurrentSequence, trackview, set_current_sequence,
    "Sets the specified sequence as a current one in TrackView.",
    "trackview.set_current_sequence(str sequenceName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetNumSequences, trackview, get_num_sequences,
    "Gets the number of sequences.",
    "trackview.get_num_sequences()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetSequenceName, trackview, get_sequence_name,
    "Gets the name of a sequence by its index.",
    "trackview.get_sequence_name(int sequenceIndex)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetSequenceTimeRange, trackview, get_sequence_time_range,
    "Gets the time range of a sequence as a pair",
    "trackview.get_sequence_time_range(string sequenceName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewSetSequenceTimeRange, trackview, set_sequence_time_range,
    "Sets the time range of a sequence",
    "trackview.set_sequence_time_range(string sequenceName, float start, float end)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewPlaySequence, trackview, play_sequence,
    "Plays the current sequence in TrackView.",
    "trackview.play_sequence()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewStopSequence, trackview, stop_sequence,
    "Stops any sequence currently playing in TrackView.",
    "trackview.stop_sequence()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewSetSequenceTime, trackview, set_time,
    "Sets the time of the sequence currently playing in TrackView.",
    "trackview.set_time(float time)");

// Nodes
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewAddNode, trackview, add_node,
    "Adds a new node with the given type & name to the current sequence.",
    "trackview.add_node(str nodeTypeName, str nodeName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewAddSelectedEntities, trackview, add_selected_entities,
    "Adds an entity node(s) from viewport selection to the current sequence.",
    "trackview.add_selected_entities()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewAddLayerNode, trackview, add_layer_node,
    "Adds a layer node from the current layer to the current sequence.",
    "trackview.add_layer_node()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewDeleteNode, trackview, delete_node,
    "Deletes the specified node from the current sequence.",
    "trackview.delete_node(str nodeName, str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewAddTrack, trackview, add_track,
    "Adds a track of the given parameter ID to the node.",
    "trackview.add_track(str paramType, str nodeName, str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewDeleteTrack, trackview, delete_track,
    "Deletes a track of the given parameter ID (in the given index in case of a multi-track) from the node.",
    "trackview.delete_track(str paramType, int index, str nodeName, str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetNumNodes, trackview, get_num_nodes,
    "Gets the number of sequences.",
    "trackview.get_num_nodes(str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetNodeName, trackview, get_node_name,
    "Gets the name of a sequence by its index.",
    "trackview.get_node_name(int nodeIndex, str parentDirectorName)");

// Tracks
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetNumTrackKeys, trackview, get_num_track_keys,
    "Gets number of keys of the specified track",
    "trackview.get_num_track_keys(str paramName, int trackIndex, str nodeName, str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetKeyValue, trackview, get_key_value,
    "Gets the value of the specified key",
    "trackview.get_key_value(str paramName, int trackIndex, int keyIndex, str nodeName, str parentDirectorName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyTrackViewGetInterpolatedValue, trackview, get_interpolated_value,
    "Gets the interpolated value of a track at the specified time",
    "trackview.get_interpolated_value(str paramName, int trackIndex, float time, str nodeName, str parentDirectorName)");
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

#include "AnimGraphTransitionConditionFixture.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/AnimGraphVector2Condition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <MCore/Source/ReflectionSerializer.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>

#include <functional>
#include <unordered_map>

namespace EMotionFX
{
    // This function controls the way that the Google Test code formats objects
    // of type AnimGraphNode*. This causes test failure messages involving
    // AnimGraphNode pointers to contain the node name instead of just the
    // pointer address.
    void PrintTo(AnimGraphNode* const object, ::std::ostream* os)
    {
        *os << object->GetName();
    }

    // Google Test does not consider inheritance when calling PrintTo(), so
    // multiple functions must be defined for subclasses
    void PrintTo(AnimGraphMotionNode* const object, ::std::ostream* os)
    {
        PrintTo(static_cast<AnimGraphNode*>(object), os);
    }

    // This class is used as a simple holder for attribute values to pass to an
    // MCore::Attribute instance at test time. The test data for parameterized
    // tests use this class instead of using MCore::Attribute directly.
    // MCore::Attribute's API doesn't support stack-allocated instances since
    // it has a private constructor, and the Attribute::Create() method uses
    // MCore's allocator, which is not set up when the test data is
    // initialized.
    class Variant
    {
    public:
        enum Type
        {
            Bool,
            String,
            StringVector,
            StdString,
            Float,
            Vector2,

            // NodeName represents the name of a node, where the serialized
            // type stores a node id. The id is not known until runtime, so the
            // setter for a NodeName type looks up the node id from the name
            // stored, and then sets the property name with the node id
            NodeName,
            AZu8,
            AZu32,
            AZu64
        };

        Variant(bool value) : type(Bool)
        {
            data.b = value;
        }

        Variant(const char* value, Type type = String) : type(type)
        {
            data.string = value;
        }

        Variant(float f) : type(Float)
        {
            data.f = f;
        }

        Variant(float x, float y) : type(Vector2), y(y)
        {
            data.f = x;
        }

        Variant(const std::string& string) : type(StdString)
        {
            m_string = string;
        }

        Variant(const std::vector<std::string>& stringVector) : type(StringVector), m_stringVector(stringVector)
        {
        }

        Variant(AZ::u8 value) : type(AZu8)
        {
            data.u8 = value;
        }

        Variant(AZ::u32 value) : type(AZu32)
        {
            data.u32 = value;
        }

        Variant(AZ::u64 value) : type(AZu64)
        {
            data.u64 = value;
        }

        Type GetType() const { return type; }

        template<typename T>
        T Get() const;

    private:
        union Data
        {
            bool b;
            float f;
            const char* string;
            AZ::u8 u8;
            AZ::u32 u32;
            AZ::u64 u64;
        } data;

        // For Vector2, the x value is stored in the above-declared union. This
        // is the Vector2 y value.
        float y = 0.0f;

        std::string m_string;

        std::vector<std::string> m_stringVector;

        Type type;
    };

    template<>
    bool Variant::Get<bool>() const
    {
        AZ_Assert(type == Bool, "Get<bool>() called on Variant that is not a Bool");
        return data.b;
    }

    template<>
    float Variant::Get<float>() const
    {
        AZ_Assert(type == Float, "Get<float>() called on Variant that is not a float");
        return data.f;
    }

    template<>
    const char* Variant::Get<const char*>() const
    {
        AZ_Assert(type == String || type == NodeName, "Get<const char*>() called on Variant that is not a String");
        return data.string;
    }

    template<>
    const AZ::Vector2 Variant::Get<const AZ::Vector2>() const
    {
        AZ_Assert(type == Vector2, "Get<AZ::Vector2>() called on Variant that is not a Vector2");
        return AZ::Vector2(data.f, y);
    }

    template <>
    const AZStd::string Variant::Get<const AZStd::string>() const
    {
        return AZStd::string(m_string.c_str(), m_string.size());
    }

    template <>
    const std::string Variant::Get<const std::string>() const
    {
        return m_string;
    }

    template<>
    AZStd::vector<AZStd::string> Variant::Get<AZStd::vector<AZStd::string>>() const
    {
        AZ_Assert(type == StringVector, "Get<AZStd::vector<AZStd::string>>() called on Variant that is not a StringVector");
        AZStd::vector<AZStd::string> stringVector;
        for (const std::string& string : m_stringVector)
        {
            stringVector.emplace_back(string.c_str(), string.size());
        }

        return stringVector;
    }

    template<>
    const std::vector<std::string> Variant::Get<const std::vector<std::string>>() const
    {
        return m_stringVector;
    }

    template <>
    AZ::u8 Variant::Get<AZ::u8>() const
    {
        AZ_Assert(type == AZu8, "Get<AZ::u8>() called on Variant that is not a AZ::u8");
        return data.u8;
    }

    template <>
    AZ::u32 Variant::Get<AZ::u32>() const
    {
        AZ_Assert(type == AZu32, "Get<AZ::u32>() called on Variant that is not a AZ::u32");
        return data.u32;
    }

    template <>
    AZ::u64 Variant::Get<AZ::u64>() const
    {
        AZ_Assert(type == AZu64, "Get<NodeId>() called on Variant that is not a NodeId");
        return data.u64;
    }

    using AttributeKeyValuePairs = std::unordered_map<std::string, const Variant>;
    using AttributeKeyValuePair = AttributeKeyValuePairs::value_type;

    template <typename ValueType>
    void SetAttributeValueImpl(AZ::SerializeContext* context, AnimGraphObject* object, const AttributeKeyValuePair& value)
    {
        const std::string& propertyName = std::get<0>(value);
        MCore::ReflectionSerializer::SetIntoMember(context, object, propertyName.c_str(), value.second.Get<ValueType>());
    }

    // The test data does not instantiate the conditions when the data is
    // declared. Instead, the values of the condition's properties are
    // declared, and then set by the test during SetUp()
    void SetAttributeValue(AZ::SerializeContext* context, AnimGraphObject* object, const AttributeKeyValuePair& attribute)
    {
        switch (attribute.second.GetType()) {
            case Variant::Bool:
                SetAttributeValueImpl<bool>(context, object, attribute);
                break;
            case Variant::Float:
                SetAttributeValueImpl<float>(context, object, attribute);
                break;
            case Variant::StringVector:
                SetAttributeValueImpl<AZStd::vector<AZStd::string>>(context, object, attribute);
                break;
            case Variant::StdString:
                SetAttributeValueImpl<const AZStd::string>(context, object, attribute);
                break;
            case Variant::NodeName:
            {
                AnimGraph* animGraph = object->GetAnimGraph();
                AZ::u64 id = animGraph->RecursiveFindNodeByName(attribute.second.Get<const char*>())->GetId();
                Variant idVariant = Variant(id);
                SetAttributeValue(context, object, AttributeKeyValuePair {attribute.first, idVariant});
                break;
            }
            case Variant::AZu8:
                SetAttributeValueImpl<AZ::u8>(context, object, attribute);
                break;
            case Variant::AZu32:
                SetAttributeValueImpl<AZ::u32>(context, object, attribute);
                break;
            case Variant::AZu64:
                SetAttributeValueImpl<AZ::u64>(context, object, attribute);
                break;
            default:
                ASSERT_TRUE(false) << "Request to set unsupported value type " << attribute.second.GetType();
        }
    }

    // This is used to map from frame numbers to a list of AnimGraphNodes. Test
    // data defines the expected list of active nodes per frame using this
    // type.
    using ActiveNodesMap = std::unordered_map<int, std::vector<const char*>>;

    // This function is defined by test data to allow for modifications to the
    // anim graph at each frame. The second parameter is the current frame
    // number. It is called with a frame number of -1 as the first thing in the
    // test before the first Update() call, to allow for initial values to be
    // set.
    using FrameCallback = std::function<void(AnimGraphInstance*, int)>;

    // The TransitionConditionFixtureP fixture is parameterized
    // on this type
    struct ConditionFixtureParams
    {
        ConditionFixtureParams(
            const AttributeKeyValuePairs& attributeValues,
            const ActiveNodesMap& activeNodesMap,
            const FrameCallback& frameCallback = [](AnimGraphInstance*, int) {}
        ) : attributeValues(attributeValues), activeNodes(activeNodesMap), callback(frameCallback)
        {
        }

        // attributes to set on the condition
        const AttributeKeyValuePairs attributeValues;

        // List of nodes that are active on each frame
        const ActiveNodesMap activeNodes;

        const FrameCallback callback;
    };

    void PrintTo(const Variant& object, ::std::ostream* os)
    {
        switch (object.GetType())
        {
            case Variant::Bool:
                *os << object.Get<bool>();
                break;
            case Variant::String:
            case Variant::NodeName:
                *os << object.Get<const char*>();
                break;
            case Variant::StringVector:
            {
                const std::vector<std::string>& stringVector = object.Get<const std::vector<std::string>>();
                std::copy(stringVector.begin(), stringVector.end() -1, std::ostream_iterator<std::string>(*os, ", "));
                *os << stringVector.back();
                break;
            }
            case Variant::StdString:
                *os << object.Get<const std::string>();
                break;
            case Variant::Float:
                *os << object.Get<float>();
                break;
            case Variant::Vector2:
            {
                const AZ::Vector2& vector2 = object.Get<const AZ::Vector2>();
                *os << '{' << vector2.GetX() << ", " << vector2.GetY() << '}';
                break;
            }
            case Variant::AZu8:
            {
                AZStd::string value;
                AZStd::to_string(value, object.Get<AZ::u8>());
                *os << value.c_str();
                break;
            }
            case Variant::AZu32:
            {
                AZStd::string value;
                AZStd::to_string(value, object.Get<AZ::u32>());
                *os << value.c_str();
                break;
            }
            case Variant::AZu64:
            {
                AZStd::string value;
                AZStd::to_string(value, object.Get<AZ::u64>());
                *os << value.c_str();
                break;
            }
            default:
                break;
        }
    }

    void PrintTo(const ConditionFixtureParams& object, ::std::ostream* os)
    {
        for (const AttributeKeyValuePair& attribute : object.attributeValues)
        {
            *os << attribute.first << ": [";
            PrintTo(attribute.second, os);
            *os << "] ";
        }
    }

    template<class ConditionType>
    class TransitionConditionFixtureP : public AnimGraphTransitionConditionFixture,
                                        public ::testing::WithParamInterface<ConditionFixtureParams>
    {
    public:
        const float fps;
        const float updateInterval;
        const int numUpdates;

        TransitionConditionFixtureP()
            : fps(60.0f)
            , updateInterval(1.0f / fps)
            , numUpdates(static_cast<int>(3.0f * fps))
        {
        }

        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            mAnimGraph->AddParameter(parameter);
        }

        void AddNodesToAnimGraph() override
        {
            const AttributeKeyValuePairs& conditionAttributes = GetParam().attributeValues;

            AddParameter<FloatSliderParameter>("FloatParam", 0.1f);
            AddParameter<Vector2Parameter>("Vector2Param", AZ::Vector2(0.1f, 0.1f));
            AddParameter<TagParameter>("TagParam1", false);
            AddParameter<TagParameter>("TagParam2", false);

            // Create the appropriate condition type from this test's
            // parameters
            AnimGraphTransitionCondition* condition = aznew ConditionType();
            mTransition->AddCondition(condition);
            condition->SetAnimGraph(mAnimGraph);

            for (const auto& attribute : conditionAttributes)
            {
                SetAttributeValue(GetSerializeContext(), condition, attribute);
            }

            mTransition->SetBlendTime(0.5f);
        }

    protected:
        void RunEMotionFXUpdateLoop()
        {
            const ActiveNodesMap& activeNodes = GetParam().activeNodes;
            const FrameCallback& callback = GetParam().callback;

            // Allow tests to set starting values for parameters
            callback(mAnimGraphInstance, -1);

            // Run the EMotionFX update loop for 3 seconds at 60 fps
            for (int frameNum = 0; frameNum < numUpdates; ++frameNum)
            {
                // Allow for test-data defined updates to the graph state
                callback(mAnimGraphInstance, frameNum);

                if (frameNum == 0)
                {
                    // Make sure the first frame is initialized correctly
                    // The EMotion FX update is needed before we can extract
                    // the currently active nodes from it
                    // We use an update delta time of zero on the first frame,
                    // to make sure we have a valid internal state on the first
                    // frame
                    GetEMotionFX().Update(0.0f);
                }
                else
                {
                    GetEMotionFX().Update(updateInterval);
                }

                // Check the state for the current frame
                if (activeNodes.count(frameNum))
                {
                    std::vector<AnimGraphNode*> expectedActiveNodes;
                    for (const char* name : activeNodes.at(frameNum))
                    {
                        expectedActiveNodes.push_back(mAnimGraph->RecursiveFindNodeByName(name));
                    }

                    AnimGraphNode* sourceNode = nullptr;
                    AnimGraphNode* targetNode = nullptr;
                    mStateMachine->GetActiveStates(mAnimGraphInstance, &sourceNode, &targetNode);

                    std::vector<AnimGraphNode*> gotActiveNodes;
                    if (sourceNode)
                    {
                        gotActiveNodes.push_back(sourceNode);
                    }
                    if (targetNode)
                    {
                        gotActiveNodes.push_back(targetNode);
                    }

                    EXPECT_EQ(gotActiveNodes, expectedActiveNodes) << "on frame " << frameNum << ", time " << frameNum * updateInterval;
                }
            }
            {
                // Ensure that we reached the target state after 3 seconds
                AnimGraphNode* sourceNode = nullptr;
                AnimGraphNode* targetNode = nullptr;
                mStateMachine->GetActiveStates(mAnimGraphInstance, &sourceNode, &targetNode);
                EXPECT_EQ(sourceNode, mMotionNode1) << "MotionNode1 is not the single active node";
            }
        }
    };

    class StateConditionFixture : public TransitionConditionFixtureP<AnimGraphStateCondition>
    {
    public:
        // The base class TransitionConditionFixtureP just sets up a simple
        // anim graph with two motion nodes and a transition between them. This
        // graph is not sufficient to test the state condition, as there are no
        // states available to test against. This fixture creates a slightly
        // more complex graph.
        void AddNodesToAnimGraph() override
        {
            //                       +-------------------+
            //                       | childStateMachine |
            //                       +-------------------+
            //        0.5s time     ^                     \     state condition defined
            //        condition--->o                       o<---by test data
            //  0.5s blend time-->/                         v<--0.5s blend time
            //+-------------------+                         +-------------------+
            //|testSkeletalMotion0|------------------------>|testSkeletalMotion1|
            //+-------------------+           ^             +-------------------+
            //                          transition with
            //                            no condition
            //
            // -------------------+----------------------------------------------
            // Child State Machine|          1.0 sec time
            // -------------------+            condition
            //               +---------------+    v     +----------+
            // entry state-->|ChildMotionNode|----o---->|Exit state|
            //               +---------------+ ^        +----------+
            //                                 transitions to exit states have
            //                                 a blend time of 0

            AddParameter<FloatSliderParameter>("FloatParam", 0.1f);
            AddParameter<Vector2Parameter>("Vector2Param", AZ::Vector2(0.1f, 0.1f));

            const AttributeKeyValuePairs& conditionAttributes = GetParam().attributeValues;

            // Create another state machine inside the top-level one
            AnimGraphMotionNode* childMotionNode = aznew AnimGraphMotionNode();
            childMotionNode->SetName("ChildMotionNode");
            childMotionNode->AddMotionId("testSkeletalMotion0");

            AnimGraphExitNode* childExitNode = aznew AnimGraphExitNode();
            childExitNode->SetName("ChildExitNode");

            AnimGraphTimeCondition* motionToExitCondition = aznew AnimGraphTimeCondition();
            motionToExitCondition->SetCountDownTime(1.0f);

            AnimGraphStateTransition* motionToExitTransition = aznew AnimGraphStateTransition();
            motionToExitTransition->SetSourceNode(childMotionNode);
            motionToExitTransition->SetTargetNode(childExitNode);
            motionToExitTransition->SetBlendTime(0.0f);
            motionToExitTransition->AddCondition(motionToExitCondition);

            mChildState = aznew AnimGraphStateMachine();
            mChildState->SetName("ChildStateMachine");
            mChildState->AddChildNode(childMotionNode);
            mChildState->AddChildNode(childExitNode);
            mChildState->SetEntryState(childMotionNode);
            mChildState->AddTransition(motionToExitTransition);

            AnimGraphTimeCondition* motion0ToChildStateCondition = aznew AnimGraphTimeCondition();
            motion0ToChildStateCondition->SetCountDownTime(0.5f);

            AnimGraphStateTransition* motion0ToChildStateTransition = aznew AnimGraphStateTransition();
            motion0ToChildStateTransition->SetSourceNode(mMotionNode0);
            motion0ToChildStateTransition->SetTargetNode(mChildState);
            motion0ToChildStateTransition->SetBlendTime(0.5f);
            motion0ToChildStateTransition->AddCondition(motion0ToChildStateCondition);

            AnimGraphStateTransition* childStateToMotion1Transition = aznew AnimGraphStateTransition();
            childStateToMotion1Transition->SetSourceNode(mChildState);
            childStateToMotion1Transition->SetTargetNode(mMotionNode1);
            childStateToMotion1Transition->SetBlendTime(0.5f);

            mStateMachine->AddChildNode(mChildState);
            mStateMachine->AddTransition(motion0ToChildStateTransition);
            mStateMachine->AddTransition(childStateToMotion1Transition);

            // Create the appropriate condition type from this test's
            // parameters
            AnimGraphTransitionCondition* condition = aznew AnimGraphStateCondition();
            condition->SetAnimGraph(mAnimGraph);
            childStateToMotion1Transition->AddCondition(condition);

            for (const auto& attribute : conditionAttributes)
            {
                SetAttributeValue(GetSerializeContext(), condition, attribute);
            }
        }

    protected:
        AnimGraphStateMachine* mChildState;
    };

    // The test data changes various parameters of the conditions being tested,
    // but they frequently result in the anim graph changing in an identical
    // manner (such as moving from testSkeletalMotionNode0 to
    // testSkeletalMotionNode1 at the same point in time). These following
    // functions centralize some of the expected behavior.
    template<class AttributeType>
    void changeParamTo(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, const Variant& newValue, const char* paramName)
    {
        // The type we want to use to set the new value is whatever the return
        // type of GetValue() is
        using ValueType = typename std::remove_reference<typename std::result_of<decltype(&AttributeType::GetValue)(AttributeType)>::type>::type;

        if (currentFrame == testFrame)
        {
            AttributeType* parameter = static_cast<AttributeType*>(animGraphInstance->FindParameter(paramName));
            parameter->SetValue(newValue.Get<ValueType>());
        }
    }


    void ChangeVector2ParamSpecial(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, const AZ::Vector2& frame30Value, const AZ::Vector2& otherValue)
    {
        if (currentFrame != testFrame)
        {
            MCore::AttributeVector2* parameter = static_cast<MCore::AttributeVector2*>(animGraphInstance->FindParameter("Vector2Param"));
            parameter->SetValue(otherValue);
        }
        else
        {
            MCore::AttributeVector2* parameter = static_cast<MCore::AttributeVector2*>(animGraphInstance->FindParameter("Vector2Param"));
            parameter->SetValue(frame30Value);
        }
    }

    void ChangeNodeAttribute(AnimGraphInstance* animGraphInstance, int currentFrame, int testFrame, const AttributeKeyValuePair& newValue)
    {
        if (currentFrame == testFrame)
        {
            const AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            AnimGraphNode* node = animGraph->RecursiveFindNodeByName("testSkeletalMotion0");
            AZ_Assert(node, "There is no node named 'testSkeletalMotion0'");

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            AnimGraphMotionNode::Reflect(&serializeContext);

            SetAttributeValue(&serializeContext, node, newValue);
            node->OnUpdateUniqueData(animGraphInstance);
        }
    }

    static const auto changeNodeToLooping = std::bind(
        ChangeNodeAttribute,
        std::placeholders::_1,
        std::placeholders::_2,
        -1,
        AttributeKeyValuePair {"loop", true}
    );

    static const auto changeNodeToNonLooping = std::bind(
        ChangeNodeAttribute,
        std::placeholders::_1,
        std::placeholders::_2,
        -1,
        AttributeKeyValuePair {"loop", false}
    );

    static const auto changeFloatParamToPointFiveOnFrameThirty = std::bind(
        changeParamTo<MCore::AttributeFloat>,
        std::placeholders::_1,
        std::placeholders::_2,
        30,
        0.5f,
        "FloatParam"
    );

    static const auto changeFloatParamToNegativePointFiveOnFrameThirty = std::bind(
        changeParamTo<MCore::AttributeFloat>,
        std::placeholders::_1,
        std::placeholders::_2,
        30,
        -0.5f,
        "FloatParam"
    );

    static const auto changeVector2Param = std::bind(
        changeParamTo<MCore::AttributeVector2>,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4,
        "Vector2Param"
    );

    static const ActiveNodesMap moveToMotion1AtFrameThirty
    {
        {0, {"testSkeletalMotion0"}},
        {29, {"testSkeletalMotion0"}},
        {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
        {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
        {60, {"testSkeletalMotion1"}}
    };

    // Remember that the test runs the update loop at 60 fps. All the frame
    // numbers in the ActiveNodesPerFrameMaps are based on this value.
    // testSkeletalMotion0 is exactly 1 second long.
    static const std::vector<ConditionFixtureParams> motionTransitionConditionData
    {
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_EVENT)},
                {"eventType", std::string {"TestEvent"}},
                {"eventParameter", std::string {"TestParameter"}}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {44, {"testSkeletalMotion0"}},
                {45, {"testSkeletalMotion0", "testSkeletalMotion1"}},   // The event gets triggered on frame 44, but the condition only will only be reevaluated the next frame, so we have one frame delay.
                {46, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {74, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {75, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_HASENDED)}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_HASREACHEDMAXNUMLOOPS)},
                {"numLoops", AZ::u32(1.0f)}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0"}},  // Motion will not have reached 1.0 as playtime yet, because it lags a frame behind. The actual time value gets updated in PostUpdate which is after the evaluation of the condition.
                {61, {"testSkeletalMotion0"}},  // Motion will be at 1.0 play time exactly, the loop is not detected yet.
                {62, {"testSkeletalMotion0", "testSkeletalMotion1"}},   // The loop has been detected
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {91, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {92, {"testSkeletalMotion1"}}
            },
            changeNodeToLooping
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_PLAYTIME)},
                {"playTime", 0.2f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {11, {"testSkeletalMotion0"}},
                {12, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {41, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {42, {"testSkeletalMotion1"}}
            },
            changeNodeToNonLooping
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_PLAYTIMELEFT)},
                {"playTime", 0.2f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {47, {"testSkeletalMotion0"}},
                {48, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {77, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {78, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"motionNodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphMotionCondition::FUNCTION_ISMOTIONASSIGNED)}
            },
            ActiveNodesMap {
                // This condition will always evaluate to true. The transition
                // will start immediately.
                {0, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {29, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {30, {"testSkeletalMotion1"}}
            }
        )
        // TODO AnimGraphMotionCondition::FUNCTION_ISMOTIONNOTASSIGNED
    };

    static const std::vector<ConditionFixtureParams> parameterTransitionConditionData
    {
        // FUNCTION_EQUAL tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_EQUAL)},
                {"testValue", 0.1f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {29, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {30, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_EQUAL)},
                {"testValue", 0.5f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_NOTEQUAL tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTEQUAL)},
                {"testValue", 0.1f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_INRANGE tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_INRANGE)},
                {"testValue", 0.4f},
                {"rangeValue", 0.6f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_NOTINRANGE tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTINRANGE)},
                {"testValue", -0.2f},
                {"rangeValue", 0.2f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_LESS tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESS)},
                {"testValue", 0.0f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToNegativePointFiveOnFrameThirty
        ),

        // FUNCTION_GREATER tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATER)},
                {"testValue", 0.1f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_GREATEREQUAL tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.5f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.49f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToPointFiveOnFrameThirty
        ),

        // FUNCTION_LESSEQUAL tests
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string {"FloatParam"}},
                {"function", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", -0.1f},
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {59, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {60, {"testSkeletalMotion1"}}
            },
            changeFloatParamToNegativePointFiveOnFrameThirty
        )
    };

    static const std::vector<ConditionFixtureParams> playTimeTransitionConditionData
    {
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"nodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"mode", static_cast<AZ::u8>(AnimGraphPlayTimeCondition::MODE_REACHEDEND)}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {59, {"testSkeletalMotion0"}},
                {60, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {89, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"nodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"mode", static_cast<AZ::u8>(AnimGraphPlayTimeCondition::MODE_REACHEDTIME)},
                {"playTime", 0.3f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {17, {"testSkeletalMotion0"}},
                {18, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {47, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {48, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"nodeId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"mode", static_cast<AZ::u8>(AnimGraphPlayTimeCondition::MODE_HASLESSTHAN)},
                {"playTime", 0.3f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {41, {"testSkeletalMotion0"}},
                {42, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {71, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {72, {"testSkeletalMotion1"}}
            }
        )
    };

    static const std::vector<ConditionFixtureParams> stateTransitionConditionData
    {
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"stateId", Variant("ChildStateMachine", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphStateCondition::FUNCTION_EXITSTATES)}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                {60, {"ChildStateMachine"}},
                {89, {"ChildStateMachine"}},
                {90, {"ChildStateMachine", "testSkeletalMotion1"}},
                {119, {"ChildStateMachine", "testSkeletalMotion1"}},
                {120, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"stateId", Variant("ChildMotionNode", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphStateCondition::FUNCTION_END)}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                {60, {"ChildStateMachine"}},
                {89, {"ChildStateMachine"}},
                {90, {"ChildStateMachine"}},
                {91, {"ChildStateMachine", "testSkeletalMotion1"}}, // The end state is captured by an event, but this event will be caught one frame later, so on 91 instead of 90.
                {119, {"ChildStateMachine", "testSkeletalMotion1"}},
                {120, {"ChildStateMachine", "testSkeletalMotion1"}},
                {121, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"stateId", Variant("ChildStateMachine", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphStateCondition::FUNCTION_ENTERING)}
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"stateId", Variant("ChildStateMachine", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphStateCondition::FUNCTION_ENTER)}
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"stateId", Variant("testSkeletalMotion0", Variant::NodeName)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphStateCondition::FUNCTION_END)}
            },
            ActiveNodesMap {
                // Stay in entry state for 0.5s
                {0, {"testSkeletalMotion0"}},
                {29, {"testSkeletalMotion0"}},
                // Transition into ChildStateMachine for 0.5s
                // As soon as this transition activates, the state condition to
                // move to testSkeletalMotion1 becomes true
                {30, {"testSkeletalMotion0", "ChildStateMachine"}},
                {59, {"testSkeletalMotion0", "ChildStateMachine"}},
                // Even though ChildStateMachine is not yet to the exit state,
                // the condition in the root state machine to leave that state
                // is true, so the transition to testSkeletalMotion1 starts
                {60, {"ChildStateMachine", "testSkeletalMotion1"}},
                {89, {"ChildStateMachine", "testSkeletalMotion1"}},
                {90, {"testSkeletalMotion1"}}
            }
        )
    };

    static const std::vector<ConditionFixtureParams> tagTransitionConditionData
    {
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"function", static_cast<AZ::u8>(AnimGraphTagCondition::FUNCTION_ALL)},
                {"tags", std::vector<std::string> {"TagParam1", "TagParam2"}}
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, true, "TagParam1");
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 15, true, "TagParam2");
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"function", static_cast<AZ::u8>(AnimGraphTagCondition::FUNCTION_NOTALL)},
                {"tags", std::vector<std::string> {"TagParam1", "TagParam2"}}
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                // initialize tags to on
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, true, "TagParam1");
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, true, "TagParam2");

                // turn TagParam1 off on frame 30
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, false, "TagParam1");
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"function", static_cast<AZ::u8>(AnimGraphTagCondition::FUNCTION_NONE)},
                {"tags", std::vector<std::string> {"TagParam1", "TagParam2"}}
            },
            moveToMotion1AtFrameThirty,
            [] (AnimGraphInstance* animGraphInstance, int currentFrame) {
                // initialize tags to on
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, true, "TagParam1");
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, -1, true, "TagParam2");

                // turn TagParam2 off on frame 15
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 15, false, "TagParam2");

                // turn TagParam1 off on frame 30
                changeParamTo<MCore::AttributeBool>(animGraphInstance, currentFrame, 30, false, "TagParam1");
            }
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"function", static_cast<AZ::u8>(AnimGraphTagCondition::FUNCTION_ONEORMORE)},
                {"tags", std::vector<std::string> {"TagParam1", "TagParam2"}}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeParamTo<MCore::AttributeBool>, std::placeholders::_1, std::placeholders::_2, 30, true, "TagParam1")
        ),
    };

    static const std::vector<ConditionFixtureParams> timeTransitionConditionData
    {
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {{"countDownTime"}, 1.3f}
            },
            ActiveNodesMap {
                {0, {"testSkeletalMotion0"}},
                {77, {"testSkeletalMotion0"}},
                {78, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {107, {"testSkeletalMotion0", "testSkeletalMotion1"}},
                {108, {"testSkeletalMotion1"}}
            }
        )
    };

    static const std::vector<ConditionFixtureParams> vector2TransitionConditionData
    {
        // --------------------------------------------------------------------
        // FUNCTION_EQUAL
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_EQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.5f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_EQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.5f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_EQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(std::sqrt(0.25f / 2.0f), std::sqrt(0.25f / 2.0f)))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_NOTEQUAL
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTEQUAL)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.5f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTEQUAL)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.5f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTEQUAL)},
                {"testValue", std::sqrt(0.1f*0.1f + 0.1f*0.1f)}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(std::sqrt(0.25f / 2.0f), std::sqrt(0.25f / 2.0f)))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_LESS
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESS)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.05f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESS)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.05f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESS)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.05f, 0.05f))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_GREATER
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATER)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.15f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATER)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.15f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATER)},
                {"testValue", 0.2f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.25f, 0.0f))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_GREATEREQUAL
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.2f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.2f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.2f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.3f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.2f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.2f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.2f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.3f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(1.0f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_GREATEREQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.5f, 0.0f))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_LESSEQUAL
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.05f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.5f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.0f, 0.05f), AZ::Vector2(1.0f, 1.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.0f, 0.5f), AZ::Vector2(1.0f, 1.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.1f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.05f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_LESSEQUAL)},
                {"testValue", 0.5f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(ChangeVector2ParamSpecial, std::placeholders::_1, std::placeholders::_2, 30, AZ::Vector2(0.5f, 0.0f), AZ::Vector2(1.0f, 1.0f))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_INRANGE
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_INRANGE)},
                {"testValue", 0.2f},
                {"rangeValue", 0.3f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.25f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_INRANGE)},
                {"testValue", 0.2f},
                {"rangeValue", 0.3f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.25f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_INRANGE)},
                {"testValue", 0.2f},
                {"rangeValue", 0.3f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.15f, 0.15f))
        ),

        // --------------------------------------------------------------------
        // FUNCTION_NOTINRANGE
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETX)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTINRANGE)},
                {"testValue", 0.05f},
                {"rangeValue", 0.15f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.25f, 0.0f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_GETY)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTINRANGE)},
                {"testValue", 0.05f},
                {"rangeValue", 0.15f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.0f, 0.25f))
        ),
        ConditionFixtureParams(
            AttributeKeyValuePairs {
                {"parameterName", std::string("Vector2Param")},
                {"operation", static_cast<AZ::u8>(AnimGraphVector2Condition::OPERATION_LENGTH)},
                {"testFunction", static_cast<AZ::u8>(AnimGraphParameterCondition::FUNCTION_NOTINRANGE)},
                {"testValue", 0.05f},
                {"rangeValue", 0.15f}
            },
            moveToMotion1AtFrameThirty,
            std::bind(changeVector2Param, std::placeholders::_1, std::placeholders::_2, 30, Variant(0.15f, 0.15f))
        ),
    };

    using MotionConditionFixture = TransitionConditionFixtureP<AnimGraphMotionCondition>;
    TEST_P(MotionConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestMotionCondition, MotionConditionFixture,
        ::testing::ValuesIn(motionTransitionConditionData)
    );

    using ParameterConditionFixture = TransitionConditionFixtureP<AnimGraphParameterCondition>;
    TEST_P(ParameterConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestParameterCondition, ParameterConditionFixture,
        ::testing::ValuesIn(parameterTransitionConditionData)
    );

    using PlayTimeConditionFixture = TransitionConditionFixtureP<AnimGraphPlayTimeCondition>;
    TEST_P(PlayTimeConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestPlayTimeCondition, PlayTimeConditionFixture,
        ::testing::ValuesIn(playTimeTransitionConditionData)
    );

    TEST_P(StateConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestStateCondition, StateConditionFixture,
        ::testing::ValuesIn(stateTransitionConditionData)
    );

    using TagConditionFixture = TransitionConditionFixtureP<AnimGraphTagCondition>;
    TEST_P(TagConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestTagCondition, TagConditionFixture,
        ::testing::ValuesIn(tagTransitionConditionData)
    );


    using TimeConditionFixture = TransitionConditionFixtureP<AnimGraphTimeCondition>;
    TEST_P(TimeConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestTimeCondition, TimeConditionFixture,
        ::testing::ValuesIn(timeTransitionConditionData)
    );

    using Vector2ConditionFixture = TransitionConditionFixtureP<AnimGraphVector2Condition>;
    TEST_P(Vector2ConditionFixture, TestTransitionCondition)
    {
        RunEMotionFXUpdateLoop();
    }
    INSTANTIATE_TEST_CASE_P(TestVector2Condition, Vector2ConditionFixture,
        ::testing::ValuesIn(vector2TransitionConditionData)
    );

} // end namespace EMotionFX

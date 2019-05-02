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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/FlowKinectNodes_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/FlowKinectNodes_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

////////////////////////////////////////////////////////////////////////////////////////////////////
// Kinect gesture system experiments.
#define TEST_KINECT_GESTURE_SYSTEM 0
#if TEST_KINECT_GESTURE_SYSTEM
void TestKinectGestureSystem();
#endif //TEST_KINECT_GESTURE_SYSTEM
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlowNode_KinectAlignment
    : public IKinectInputListener
    , public CFlowBaseNode<eNCT_Instanced>
{
public:

    CFlowNode_KinectAlignment(SActivationInfo* pActivationInfo)
        : m_alignTime(0.0f)
        , m_currentTime(0.0f)
        , m_nodeID(pActivationInfo->myID)
        , m_pGraph(pActivationInfo->pGraph)
    {
    }

    virtual ~CFlowNode_KinectAlignment()
    {
        IInput* input = gEnv->pSystem->GetIInput();
        if (input)
        {
            IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput();
            if (pKinectInput)
            {
                pKinectInput->UnregisterInputListener(this);
            }
        }
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_KinectAlignment(pActInfo); }

    enum EInputPorts
    {
        eIP_Enable = 0,
        eIP_Disable,
        eIP_ForceAlign,
        eIP_AlignTime,
    };

    enum EOutputPorts
    {
        eOP_Started = 0,
        eOP_Completed,
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputConfig[] =
        {
            InputPortConfig_Void        ("Enable", _HELP("Enable alignment watcher")),
            InputPortConfig_Void        ("Disable", _HELP("Disable alignment watcher")),
            InputPortConfig_Void        ("ForceAlign", _HELP("Force beginning of new alignment phase.")),
            InputPortConfig<float>  ("AlignTime", 1.0f, _HELP("Time spent each time skeletal alignment is started")),
            { 0 }
        };

        static const SOutputPortConfig outputConfig[] =
        {
            OutputPortConfig<bool>  ("Started", _HELP("Triggers when a new alignment is started.")),
            OutputPortConfig<bool>  ("Completed", _HELP("Triggers when an alignment completes.")),
            { 0 }
        };

        config.sDescription = _HELP("Get default skeleton joint lengths when new closest tracked skeleton is detected");
        config.pInputPorts  = inputConfig;
        config.pOutputPorts = outputConfig;

        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActivationInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            m_alignTime = GetPortFloat(pActivationInfo, eIP_AlignTime);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActivationInfo, eIP_Enable))
            {
                if (IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput())
                {
                    pKinectInput->RegisterInputListener(this, "CFlowNode_KinectAlignment");
                    ActivateOutput(pActivationInfo, eOP_Started, true);
                }
            }
            else if (IsPortActive(pActivationInfo, eIP_Disable))
            {
                if (IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput())
                {
                    pKinectInput->UnregisterInputListener(this);
                }
            }
            else if (IsPortActive(pActivationInfo, eIP_ForceAlign))
            {
                EnableNodeUpdate();
            }
            else if (IsPortActive(pActivationInfo, eIP_AlignTime))
            {
                m_alignTime = GetPortFloat(pActivationInfo, eIP_AlignTime);
            }

            break;
        }

        case eFE_Update:
        {
            //If we are here then the node is active, decrease the alignment time until it hits zero
            m_currentTime = max(0.0f, m_currentTime - gEnv->pTimer->GetFrameTime());

            //Check skeleton status, if it doesn't meet the criteria, then reset the alignment time and warn the user
            //Have a decent alignment, saved the distances as defaults
            if (IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput())
            {
                uint32 closestSkeleton = pKinectInput->GetClosestTrackedSkeleton();
                if (closestSkeleton != KIN_SKELETON_INVALID_TRACKING_ID)
                {
                    SKinSkeletonRawData skeletonRawData;
                    if (pKinectInput->GetSkeletonRawData(closestSkeleton, skeletonRawData))
                    {
                        //Check some basic constraints about the skeleton that should be rational for the "rest" pose

                        Vec4 cHipPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_HIP_CENTER];
                        Vec4 cSpinePos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_SPINE];
                        Vec4 cShoulder = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_SHOULDER_CENTER];

                        Vec4 lWristPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_WRIST_LEFT];
                        Vec4 lElbowPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_ELBOW_LEFT];
                        Vec4 lShldrPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_SHOULDER_LEFT];

                        Vec4 rWristPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_WRIST_RIGHT];
                        Vec4 rElbowPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_ELBOW_RIGHT];
                        Vec4 rShldrPos = skeletonRawData.vSkeletonPositions[KIN_SKELETON_POSITION_SHOULDER_RIGHT];

                        const char* invalidStateReason = NULL;

                        //Check that wrists are below elbows
                        if (lWristPos.y >= lElbowPos.y || rWristPos.y >= rElbowPos.y)
                        {
                            invalidStateReason = "Wrists above elbows";
                        }

                        //Wrists are below spine
                        if (!invalidStateReason && (lWristPos.y > cSpinePos.y || rWristPos.y > cSpinePos.y))
                        {
                            invalidStateReason = "Wrists above spine";
                        }

                        //Wrists are in horitzontal plane with body
                        if (!invalidStateReason &&
                            (fabs(lWristPos.z - cShoulder.z) > 0.1 ||
                             fabs(rWristPos.z - cShoulder.z) > 0.1)
                            )
                        {
                            invalidStateReason = "Wrists are in front of body";
                        }

                        if (!invalidStateReason &&
                            skeletonRawData.eSkeletonPositionTrackingState[KIN_SKELETON_POSITION_HIP_CENTER] == KIN_SKELETON_POSITION_TRACKED &&
                            fabs(cShoulder.x - cHipPos.x) > 0.03
                            )
                        {
                            invalidStateReason = "Spine is not straight";
                        }

                        //Check that wrist distance from shoulder center is symmetrical within some threshold
                        float xLWristToHipC = fabs(lWristPos.x - cShoulder.x);
                        float xRWristToHipC = fabs(rWristPos.x - cShoulder.x);
                        if (!invalidStateReason && fabs(xLWristToHipC - xRWristToHipC) > 0.05)
                        {
                            invalidStateReason = "Wrists not evenly spaced from body";
                        }

                        //Morgan - Disabled as this does not support all body types
                        //Wrist x-distance from shoulder is very small
                        //if(!invalidStateReason && ( fabs(lWristPos.x - lShldrPos.x) > 0.1 || fabs(rWristPos.x - rShldrPos.x) > 0.1))
                        //  invalidStateReason = 4;

                        //WARN
                        if (invalidStateReason)
                        {
                            float color[] = {1.0f, 0.0f, 0.0f, 1.0f};
                            gEnv->pRenderer->Draw2dLabel(100, 200, 2.5, color, false, "Warning: Pose invalid, stand with arms in rest position. Failure Reason: %s", invalidStateReason);
                            m_currentTime = m_alignTime;
                        }
                        else if (m_currentTime <= 0.0f)
                        {
                            pKinectInput->UpdateSkeletonAlignment(closestSkeleton);

                            ActivateOutput(pActivationInfo, eOP_Completed, true);
                            m_pGraph->SetRegularlyUpdated(m_nodeID, false);
                        }
                        else
                        {
                            float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
                            gEnv->pRenderer->Draw2dLabel(100, 200, 2.5, color, false, "Aligning: Stand straight in rest pose, with your arms at rest at your sides.");
                        }
                    }
                }
                //If the current skeleton is invalid, then turn this node's monitoring off
                else
                {
                    m_pGraph->SetRegularlyUpdated(m_nodeID, false);
                }
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
    }

    // IKinectInputListener
    virtual bool OnKinectRawInputEvent(const SKinSkeletonFrame& skeletonFrame){return false; }
    virtual void OnRailProgress(const SKinRailState& railState){}
    virtual void OnVoiceCommand(string voiceCommand){};
    virtual void OnKinectClosestSkeletonChanged(uint32 skeletonIndex)
    {
        if (skeletonIndex != KIN_SKELETON_INVALID_TRACKING_ID)
        {
            EnableNodeUpdate();
        }
    }
    // ~IKinectInputListener

    virtual void OnKinectSkeletonMoved(uint32 skeletonIndex, float distanceMoved){};

    virtual void EnableNodeUpdate()
    {
        m_currentTime = m_alignTime;
        m_pGraph->SetRegularlyUpdated(m_nodeID, true);
    }

private:

    float m_alignTime;
    float m_currentTime;
    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;
};

REGISTER_FLOW_NODE("Kinect:Alignment", CFlowNode_KinectAlignment);

const static char* ms_jointEnum("enum_int:hip_center=0,spine=1,shoulder_center=2,head=3,shoulder_left=4,elbow_left=5,wrist_left=6,hand_left=7,shoulder_right=8,elbow_right=9,wrist_right=10,hand_right=11,hip_left=12,knee_left=13,ankle_left=14,foot_left=15,hip_right=16,knee_right=17,ankle_right=18,foot_right=19,hand_refined_left=20,hand_refined_right=21,null=22");

class CFlowNode_KinectSkeleton
    : public CFlowBaseNode<eNCT_Instanced>
{
public:

    CFlowNode_KinectSkeleton(SActivationInfo* pActivationInfo)
        : m_frequency(0.0f)
    {
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_KinectSkeleton(pActInfo); }

    enum EInputPorts
    {
        eIP_Sync = 0,
        eIP_Auto,
        eIP_Freq,
        eIP_Joint,
        eIP_RefJoint
    };

    enum EOutputPorts
    {
        eOP_Pos = 0,
        eOP_X,
        eOP_Y,
        eOP_Z
    };

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputConfig[] =
        {
            InputPortConfig_Void        ("Sync", _HELP("Synchronize")),
            InputPortConfig<bool>       ("Auto", false, _HELP("Auto update")),
            InputPortConfig<float>  ("Freq", 0.0f, _HELP("Auto update frequency (0 to update every frame)")),
            InputPortConfig<int>        ("Joint", KIN_SKELETON_POSITION_COUNT, _HELP("Joint"), NULL, _UICONFIG(ms_jointEnum)),
            InputPortConfig<int>        ("RefJoint", KIN_SKELETON_POSITION_COUNT, _HELP("Reference joint"), NULL, _UICONFIG(ms_jointEnum)),
            { 0 }
        };

        static const SOutputPortConfig outputConfig[] =
        {
            OutputPortConfig<Vec3>  ("Pos", _HELP("Position")),
            OutputPortConfig<float> ("X", _HELP("Position X")),
            OutputPortConfig<float> ("Y", _HELP("Position Y")),
            OutputPortConfig<float> ("Z", _HELP("Position Z")),
            { 0 }
        };

        config.sDescription = _HELP("Get status of joint in Kinect skeleton");
        config.pInputPorts  = inputConfig;
        config.pOutputPorts = outputConfig;

        config.SetCategory(EFLN_APPROVED);

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        // Kinect gesture system experiments.
#if TEST_KINECT_GESTURE_SYSTEM
        TestKinectGestureSystem();
#endif //TEST_KINECT_GESTURE_SYSTEM
        ////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActivationInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));

            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActivationInfo, eIP_Auto))
            {
                SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));
            }
            else if (IsPortActive(pActivationInfo, eIP_Freq))
            {
                m_frequency = GetPortFloat(pActivationInfo, eIP_Freq);
            }
            else if (IsPortActive(pActivationInfo, eIP_Sync))
            {
                Sync(pActivationInfo);
            }

            break;
        }

        case eFE_Update:
        {
            CTimeValue  delta = gEnv->pTimer->GetFrameStartTime() - m_prevTime;

            if (delta.GetSeconds() >= m_frequency)
            {
                Sync(pActivationInfo);
            }

            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->Add(*this);
    }

private:

    void SetAutoUpdate(SActivationInfo* pActivationInfo, bool enable)
    {
        pActivationInfo->pGraph->SetRegularlyUpdated(pActivationInfo->myID, enable);

        if (enable)
        {
            m_prevTime = gEnv->pTimer->GetFrameStartTime();
        }
    }

    void Sync(SActivationInfo* pActivationInfo)
    {
        int32   joint = GetPortInt(pActivationInfo, eIP_Joint), refJoint = GetPortInt(pActivationInfo, eIP_RefJoint);

        if ((joint >= 0) && (joint < KIN_SKELETON_POSITION_COUNT))
        {
            if (IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput())
            {
                uint32 closestSkeleton = pKinectInput->GetClosestTrackedSkeleton();

                if (closestSkeleton != KIN_SKELETON_INVALID_TRACKING_ID)
                {
                    SKinSkeletonRawData skeletonRawData;
                    if (pKinectInput->GetSkeletonRawData(closestSkeleton, skeletonRawData))
                    {
                        Vec4    jointPos = skeletonRawData.vSkeletonPositions[joint];

                        if ((refJoint >= 0) && (refJoint < KIN_SKELETON_POSITION_COUNT))
                        {
                            jointPos = jointPos - skeletonRawData.vSkeletonPositions[refJoint];
                        }

                        ActivateOutput(pActivationInfo, eOP_Pos, Vec3(jointPos.x, jointPos.y, jointPos.z));
                        ActivateOutput(pActivationInfo, eOP_X, jointPos.x);
                        ActivateOutput(pActivationInfo, eOP_Y, jointPos.y);
                        ActivateOutput(pActivationInfo, eOP_Z, jointPos.z);
                    }
                }
            }
        }

        m_prevTime = gEnv->pTimer->GetFrameStartTime();
    }

    float               m_frequency;

    CTimeValue  m_prevTime;
};

REGISTER_FLOW_NODE("Kinect:Skeleton", CFlowNode_KinectSkeleton);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Kinect gesture system experiments. Just something I was playing around with one evening, but
// might be a good starting point for a more advanced system in the future.
//
// TODO : Ability to define and recycle rules?
//        Vec3 support?
#if TEST_KINECT_GESTURE_SYSTEM
namespace stl
{
    template <class COMPARE>
    struct hash_compare<string, COMPARE>
    {
        enum
        {
            bucket_size = 4, min_buckets = 8
        };

        inline size_t operator () (const string& rhs) const
        {
            return CCrc32::Compute(rhs.c_str());
        }

        inline bool operator() (const string& lhs, const string& rhs) const
        {
            return COMPARE()(lhs, rhs);
        }
    };
}

class CKinectPose
{
public:

    bool Parse(const XmlNodeRef& xmlNode);

    float Evaluate(const SKinSkeletonRawData& skeleton) const;

private:

    enum EParamType
    {
        ePT_Nil = 0,
        ePT_Int32,
        ePT_Float,
        ePT_CVar
    };

    class CParam
    {
    public:

        inline CParam()
            : m_type(ePT_Nil)
        {
        }

        inline CParam(int32 rhs)
            : m_type(ePT_Int32)
            , m_int32(rhs)
        {
        }

        inline CParam(float rhs)
            : m_type(ePT_Float)
            , m_float(rhs)
        {
        }

        inline CParam(const ICVar* rhs)
            : m_type(ePT_CVar)
            , m_pCVar(rhs)
        {
        }

        inline int32 ToInt32() const
        {
            switch (m_type)
            {
            case ePT_Int32:
            {
                return m_int32;
            }

            case ePT_Float:
            {
                return static_cast<int32>(m_float);
            }

            case ePT_CVar:
            {
                return m_pCVar ? m_pCVar->GetIVal() : 0;
            }

            default:
            {
                return 0;
            }
            }
        }

        inline float ToFloat() const
        {
            switch (m_type)
            {
            case ePT_Int32:
            {
                return static_cast<float>(m_int32);
            }

            case ePT_Float:
            {
                return m_float;
            }

            case ePT_CVar:
            {
                return m_pCVar ? m_pCVar->GetFVal() : 0;
            }

            default:
            {
                return 0.0f;
            }
            }
        }

        inline CParam& operator = (const CParam& rhs)
        {
            memcpy(this, &rhs, sizeof(CParam));

            return *this;
        }

        inline CParam& operator = (int32 rhs)
        {
            m_type      = ePT_Int32;
            m_int32 = rhs;

            return *this;
        }

        inline CParam& operator = (float rhs)
        {
            m_type  = ePT_Float;
            m_float = rhs;

            return *this;
        }

        inline CParam& operator = (const ICVar* rhs)
        {
            m_type  = ePT_CVar;
            m_pCVar = rhs;

            return *this;
        }

        friend inline bool operator && (const CParam& lhs, const CParam& rhs)
        {
            return lhs.ToInt32() && rhs.ToInt32();
        }

        friend inline bool operator || (const CParam& lhs, const CParam& rhs)
        {
            return lhs.ToInt32() || rhs.ToInt32();
        }

        friend inline bool operator < (const CParam& lhs, const CParam& rhs)
        {
            return lhs.ToFloat() < rhs.ToFloat();
        }

        friend inline bool operator > (const CParam& lhs, const CParam& rhs)
        {
            return lhs.ToFloat() > rhs.ToFloat();
        }

        friend inline CParam operator + (const CParam& lhs, const CParam& rhs)
        {
            return CParam(lhs.ToFloat() + rhs.ToFloat());
        }

        friend inline CParam operator - (const CParam& lhs, const CParam& rhs)
        {
            return CParam(lhs.ToFloat() - rhs.ToFloat());
        }

    private:

        EParamType  m_type;

        union
        {
            int32               m_int32;

            float               m_float;

            const ICVar* m_pCVar;
        };
    };

    enum ENodeType
    {
        eNT_Null = 0,
        eNT_Const,
        eNT_CVar,
        eNT_JointX,
        eNT_JointY,
        eNT_JointZ,
        eNT_And,
        eNT_Or,
        eNT_GreaterThan,
        eNT_LessThan,
        eNT_Add,
        eNT_Subtract,
        eNT_Count
    };

    struct SNode
    {
        inline SNode()
            : type(eNT_Null)
        {
        }

        ENodeType   type;

        CParam      params[2];
    };

    typedef std::vector<SNode> TNodeVector;

    bool ParseNode(const XmlNodeRef& xmlNode, int32 iNode);

    ENodeType GetNodeType(const string& name) const;

    int32 GetJointId(const string& name) const;

    CParam ProcessNode(const SKinSkeletonRawData& skeleton, int32 iNode) const;

    static const string ms_nodeTypeNames[eNT_Count], ms_jointNames[KIN_SKELETON_POSITION_COUNT];

    TNodeVector m_nodes;
};

class CKinectGesture
{
public:

    bool Parse(const XmlNodeRef& xmlNode);

private:

    typedef std::vector<string> TPoseVector;

    TPoseVector m_poses;
};

class CKinectGestureSystem
{
public:

    bool LoadGestureLibrary(const string& fileName);

    const CKinectPose* GetPose(const string& name) const;

    const CKinectGesture* GetGesture(const string& name) const;

private:

    typedef AZStd::unordered_map<string, CKinectPose> TPoseMap;

    typedef AZStd::unordered_map<string, CKinectGesture> TGestureMap;

    TPoseMap        m_poses;

    TGestureMap m_gestures;
};

const string CKinectPose::ms_nodeTypeNames[eNT_Count] =
{
    "null",
    "const",
    "c_var",
    "joint_x",
    "joint_y",
    "joint_z",
    "and",
    "or",
    "greater_than",
    "less_than",
    "add",
    "subtract"
};

const string CKinectPose::ms_jointNames[KIN_SKELETON_POSITION_COUNT] =
{
    "hip_center",
    "spine",
    "shoulder_center",
    "head",
    "shoulder_left",
    "elbow_left",
    "wrist_left",
    "hand_left",
    "shoulder_right",
    "elbow_right",
    "wrist_right",
    "hand_right",
    "hip_left",
    "knee_left",
    "ankle_left",
    "foot_left",
    "hip_right",
    "knee_right",
    "ankle_right",
    "foot_right"
};

bool CKinectPose::Parse(const XmlNodeRef& xmlNode)
{
    if (!strcmp(xmlNode->getTag(), "pose"))
    {
        if (xmlNode->getChildCount() > 1)
        {
            // Warning!!!
        }

        m_nodes.push_back(SNode());

        if (ParseNode(xmlNode->getChild(0), 0))
        {
            return true;
        }
        else
        {
            m_nodes.clear();
        }
    }
    else
    {
        // Warning!!!
    }

    return false;
}

float CKinectPose::Evaluate(const SKinSkeletonRawData& skeleton) const
{
    return m_nodes.size() ? ProcessNode(skeleton, 0).ToFloat() : 0.0f;
}

bool CKinectPose::ParseNode(const XmlNodeRef& xmlNode, int32 iNode)
{
    CRY_ASSERT((iNode >= 0) && (static_cast<size_t>(iNode < m_nodes.size())));

    if (m_nodes[iNode].type = GetNodeType(xmlNode->getTag()))
    {
        switch (m_nodes[iNode].type)
        {
        case eNT_Const:
        {
            const char* pTag = NULL, * pValue = NULL;

            if (xmlNode->getAttributeByIndex(0, &pTag, &pValue))
            {
                if (!strcmp(pTag, "int32"))
                {
                    m_nodes[iNode].params[0] = static_cast<int32>(atoi(pValue));

                    return true;
                }
                else if (!strcmp(pTag, "float"))
                {
                    m_nodes[iNode].params[0] = static_cast<float>(atof(pValue));

                    return true;
                }
                else
                {
                    // Warning!!!

                    return false;
                }
            }
            else
            {
                // Warning!!!

                return false;
            }
        }

        case eNT_CVar:
        {
            string  name = xmlNode->getAttr("name");

            if (ICVar* pCVar = gEnv->pConsole->GetCVar(name.c_str()))
            {
                m_nodes[iNode].params[0] = pCVar;

                return true;
            }
            else
            {
                // Warning!!!

                return false;
            }
        }

        case eNT_JointX:
        case eNT_JointY:
        case eNT_JointZ:
        {
            int32   jointId = GetJointId(xmlNode->getAttr("name"));

            if (jointId != KIN_SKELETON_POSITION_COUNT)
            {
                m_nodes[iNode].params[0] = jointId;

                return true;
            }
            else
            {
                // Warning!!!

                return false;
            }
        }

        case eNT_And:
        case eNT_Or:
        case eNT_GreaterThan:
        case eNT_LessThan:
        case eNT_Add:
        case eNT_Subtract:
        {
            if (xmlNode->getChildCount() == 2)
            {
                m_nodes[iNode].params[0] = static_cast<int32>(m_nodes.size());

                m_nodes.push_back(SNode());

                if (!ParseNode(xmlNode->getChild(0), m_nodes[iNode].params[0].ToInt32()))
                {
                    // Warning!!!

                    return false;
                }

                m_nodes[iNode].params[1] = static_cast<int32>(m_nodes.size());

                m_nodes.push_back(SNode());

                if (!ParseNode(xmlNode->getChild(1), m_nodes[iNode].params[1].ToInt32()))
                {
                    // Warning!!!

                    return false;
                }

                return true;
            }
            else
            {
                // Warning!!!

                return false;
            }
        }

        default:
        {
            return false;
        }
        }
    }
    else
    {
        // Warning!!!

        return false;
    }
}

CKinectPose::ENodeType CKinectPose::GetNodeType(const string& name) const
{
    for (int32 nodeType = eNT_Null; nodeType < eNT_Count; ++nodeType)
    {
        if (name == ms_nodeTypeNames[nodeType])
        {
            return static_cast<ENodeType>(nodeType);
        }
    }

    return eNT_Null;
}

int32 CKinectPose::GetJointId(const string& name) const
{
    for (int32 jointId = KIN_SKELETON_POSITION_HIP_CENTER; jointId < KIN_SKELETON_POSITION_COUNT; ++jointId)
    {
        if (name == ms_jointNames[jointId])
        {
            return jointId;
        }
    }

    return KIN_SKELETON_POSITION_COUNT;
}

CKinectPose::CParam CKinectPose::ProcessNode(const SKinSkeletonRawData& skeleton, int32 iNode) const
{
    CRY_ASSERT((iNode >= 0) && (static_cast<size_t>(iNode < m_nodes.size())));

    const SNode& node = m_nodes[iNode];

    switch (node.type)
    {
    case eNT_Const:
    case eNT_CVar:
    {
        return node.params[0];
    }

    case eNT_JointX:
    {
        return CParam(skeleton.vSkeletonPositions[node.params[0].ToInt32()].x);
    }

    case eNT_JointY:
    {
        return CParam(skeleton.vSkeletonPositions[node.params[0].ToInt32()].y);
    }

    case eNT_JointZ:
    {
        return CParam(skeleton.vSkeletonPositions[node.params[0].ToInt32()].z);
    }

    case eNT_And:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) && ProcessNode(skeleton, node.params[1].ToInt32());
    }

    case eNT_Or:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) || ProcessNode(skeleton, node.params[1].ToInt32());
    }

    case eNT_GreaterThan:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) > ProcessNode(skeleton, node.params[1].ToInt32()) ? 1.0f : 0.0f;
    }

    case eNT_LessThan:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) < ProcessNode(skeleton, node.params[1].ToInt32()) ? 1.0f : 0.0f;
    }

    case eNT_Add:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) + ProcessNode(skeleton, node.params[1].ToInt32());
    }

    case eNT_Subtract:
    {
        return ProcessNode(skeleton, node.params[0].ToInt32()) - ProcessNode(skeleton, node.params[1].ToInt32());
    }

    default:
    {
        return CParam();
    }
    }
}

bool CKinectGesture::Parse(const XmlNodeRef& node)
{
    return true;
}

bool CKinectGestureSystem::LoadGestureLibrary(const string& fileName)
{
    if (const XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFromFile(fileName.c_str()))
    {
        if (!strcmp(rootNode->getTag(), "gesture_library"))
        {
            for (uint32 iChildNode = 0, iEndChildNode = rootNode->getChildCount(); iChildNode != iEndChildNode; ++iChildNode)
            {
                const XmlNodeRef    childNode = rootNode->getChild(iChildNode);

                if (!strcmp(childNode->getTag(), "poses"))
                {
                    for (uint32 iPoseNode = 0, iEndPoseNode = childNode->getChildCount(); iPoseNode != iEndPoseNode; ++iPoseNode)
                    {
                        const XmlNodeRef    poseNode = childNode->getChild(iPoseNode);

                        if (!strcmp(poseNode->getTag(), "pose"))
                        {
                            std::pair<TPoseMap::iterator, bool> result = m_poses.insert(TPoseMap::value_type(poseNode->getAttr("name"), CKinectPose()));

                            if (result.second)
                            {
                                TPoseMap::iterator& iPose = result.first;

                                if (!iPose->second.Parse(poseNode))
                                {
                                    // Warning!!!

                                    m_poses.erase(iPose);
                                }
                            }
                            else
                            {
                                // Warning!!!
                            }
                        }
                        else
                        {
                            // Warning!!!
                        }
                    }
                }
                else if (!strcmp(childNode->getTag(), "gestures"))
                {
                    for (uint32 iGestureNode = 0, iEndGestureNode = childNode->getChildCount(); iGestureNode != iEndGestureNode; ++iGestureNode)
                    {
                        const XmlNodeRef    gestureNode = childNode->getChild(iGestureNode);

                        if (!strcmp(gestureNode->getTag(), "gesture"))
                        {
                            std::pair<TGestureMap::iterator, bool>  result = m_gestures.insert(TGestureMap::value_type(gestureNode->getAttr("name"), CKinectGesture()));

                            if (result.second)
                            {
                                TGestureMap::iterator& iGesture = result.first;

                                if (!iGesture->second.Parse(gestureNode))
                                {
                                    // Warning!!!

                                    m_gestures.erase(iGesture);
                                }
                            }
                            else
                            {
                                // Warning!!!
                            }
                        }
                        else
                        {
                            // Warning!!!
                        }
                    }
                }
                else
                {
                    // Warning!!!
                }
            }

            return true;
        }
        else
        {
            // Warning!!!

            return false;
        }
    }
    else
    {
        // Warning!!!

        return false;
    }
}

const CKinectPose* CKinectGestureSystem::GetPose(const string& name) const
{
    TPoseMap::const_iterator    iPose = m_poses.find(name);

    return (iPose != m_poses.end()) ? &iPose->second : NULL;
}

void TestKinectGestureSystem()
{
    CKinectGestureSystem    kinectGestureSystem;

    if (kinectGestureSystem.LoadGestureLibrary("gestures.xml"))
    {
        if (const CKinectPose* pPose = kinectGestureSystem.GetPose("raise_right_arm"))
        {
            if (IKinectInput* pKinectInput = gEnv->pSystem->GetIInput()->GetKinectInput())
            {
                SKinSkeletonRawData skeleton;

                for (uint32 iSkeleton = 0; iSkeleton < KIN_SKELETON_COUNT; ++iSkeleton)
                {
                    if (pKinectInput->GetSkeletonRawData(iSkeleton, skeleton))
                    {
                        pPose->Evaluate(skeleton);
                    }
                }
            }
        }
    }
}
#endif //TEST_KINECT_GESTURE_SYSTEM

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

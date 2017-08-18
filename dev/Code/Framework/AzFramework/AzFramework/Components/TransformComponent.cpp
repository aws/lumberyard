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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Network/NetworkContext.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <GridMate/Serialize/DataMarshal.h>

#include <Math/MathUtils.h>

namespace AZ
{
    class BehaviorTransformNotificationBusHandler : public TransformNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTransformNotificationBusHandler, "{9CEF4DAB-F359-4A3E-9856-7780281E0DAA}", AZ::SystemAllocator
            , OnTransformChanged
            , OnParentChanged
            , OnChildAdded
            , OnChildRemoved
        );

        void OnTransformChanged(const Transform& localTM, const Transform& worldTM) override
        {
            Call(FN_OnTransformChanged, localTM, worldTM);
        }

        void OnParentChanged(EntityId oldParent, EntityId newParent) override
        {
            Call(FN_OnParentChanged, oldParent, newParent);
        }

        void OnChildAdded(EntityId child) override
        {
            Call(FN_OnChildAdded, child);
        }

        void OnChildRemoved(EntityId child) override
        {
            Call(FN_OnChildRemoved, child);
        }
    };
} // namespace AZ

namespace AzFramework
{
    //=========================================================================
    // TransformReplicaChunk
    // [3/9/2016]
    //=========================================================================
    class TransformReplicaChunk
        : public GridMate::ReplicaChunkBase
    {
    public:
        AZ_CLASS_ALLOCATOR(TransformReplicaChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "TransformReplicaChunk"; }

        TransformReplicaChunk()
            : m_parentId("ParentId")
            , m_localTransform("LocalTransformData")
        {
        }

        bool IsReplicaMigratable() override
        {
            return true;
        }

        GridMate::DataSet<AZ::u64>::BindInterface<TransformComponent, &TransformComponent::OnNewNetParentData> m_parentId;
        GridMate::DataSet<AZ::Transform>::BindInterface<TransformComponent, &TransformComponent::OnNewNetTransformData> m_localTransform;

        AZ::Transform m_initialWorldTM;

        class Descriptor
            : public ExternalChunkDescriptor<TransformReplicaChunk>
        {
        public:
            ReplicaChunkBase* CreateFromStream(UnmarshalContext& context) override
            {
                // Pre/Post construct allow DataSets and RPCs to bind to the chunk.
                TransformReplicaChunk* transformChunk = aznew TransformReplicaChunk;
                context.m_iBuf->Read(transformChunk->m_initialWorldTM);
                return transformChunk;
            }

            void DiscardCtorStream(UnmarshalContext& context) override
            {
                AZ::Transform discard;
                context.m_iBuf->Read(discard);
            }

            void MarshalCtorData(ReplicaChunkBase* chunk, WriteBuffer& wb) override
            {
                TransformReplicaChunk* transformChunk = static_cast<TransformReplicaChunk*>(chunk);
                TransformComponent* transformComponent = static_cast<TransformComponent*>(transformChunk->GetHandler());
                if (transformComponent)
                {
                    wb.Write(transformComponent->GetWorldTM());
                }
                else
                {
                    wb.Write(transformChunk->m_initialWorldTM);
                }
            }
        };
    };

    //=========================================================================
    bool TransformComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            // IsStatic field added at v3.
            // It should be false for old versions of TransformComponent.
            classElement.AddElementWithData(context, "IsStatic", false);
        }

        return true;
    }

    //=========================================================================
    // TransformComponent
    // [8/9/2013]
    //=========================================================================
    TransformComponent::TransformComponent()
        : m_parentTM(nullptr)
        , m_onNewParentKeepWorldTM(true)
        , m_parentActivationTransformMode(ParentActivationTransformMode::MaintainOriginalRelativeTransform)
        , m_isStatic(false)
    {
        m_localTM = AZ::Transform::CreateIdentity();
        m_worldTM = AZ::Transform::CreateIdentity();
    }

    TransformComponent::TransformComponent(const TransformComponentConfiguration& configuration)
        : m_localTM(configuration.m_transform)
        , m_worldTM(configuration.m_worldTransform)
        , m_parentId(configuration.m_parentId)
        , m_parentTM(nullptr)
        , m_onNewParentKeepWorldTM(true)
        , m_parentActivationTransformMode(configuration.m_parentActivationTransformMode)
        , m_isStatic(configuration.m_isStatic)
    {
        SetSyncEnabled(configuration.m_isBoundToNetwork);
    }

    TransformComponent::~TransformComponent()
    {
    }

    void TransformComponent::Activate()
    {
        AZ::TransformBus::Handler::BusConnect(m_entity->GetId());
        AZ::TransformNotificationBus::Bind(m_notificationBus, m_entity->GetId());

        const bool keepWorldTm = (m_parentActivationTransformMode == ParentActivationTransformMode::MaintainCurrentWorldTransform);
        SetParentImpl(m_parentId, keepWorldTm);
    }

    void TransformComponent::Deactivate()
    {
        EBUS_EVENT_ID(m_parentId, AZ::TransformNotificationBus, OnChildRemoved, GetEntityId());

        UnbindFromNetwork();

        m_notificationBus = nullptr;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
        }
        AZ::TransformBus::Handler::BusDisconnect();
    }

    void TransformComponent::SetLocalTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetLocalTMImpl(tm);
            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetWorldTM(const AZ::Transform& tm)
    {
        if (AreMoveRequestsAllowed())
        {
            SetWorldTMImpl(tm);
            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetParent(AZ::EntityId id)
    {
        if (!IsNetworkControlled())
        {
            SetParentImpl(id, true);

            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetParentRelative(AZ::EntityId id)
    {
        if (!IsNetworkControlled())
        {
            SetParentImpl(id, m_isStatic);

            UpdateReplicaChunk();
        }
    }

    void TransformComponent::SetWorldTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetTranslation(newPosition);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetLocalTranslation(const AZ::Vector3& newPosition)
    {
        AZ::Transform newLocalTransform = m_localTM;
        newLocalTransform.SetTranslation(newPosition);
        SetLocalTM(newLocalTransform);
    }

    AZ::Vector3 TransformComponent::GetWorldTranslation()
    {
        return m_worldTM.GetPosition();
    }

    AZ::Vector3 TransformComponent::GetLocalTranslation()
    {
        return m_localTM.GetPosition();
    }

    void TransformComponent::MoveEntity(const AZ::Vector3& offset)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetPosition();
        SetWorldTranslation(worldPosition + offset);
    }

    void TransformComponent::SetWorldX(float x)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetPosition();
        SetWorldTranslation(AZ::Vector3(x, worldPosition.GetY(), worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldY(float y)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetPosition();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), y, worldPosition.GetZ()));
    }

    void TransformComponent::SetWorldZ(float z)
    {
        const AZ::Vector3& worldPosition = m_worldTM.GetPosition();
        SetWorldTranslation(AZ::Vector3(worldPosition.GetX(), worldPosition.GetY(), z));
    }

    float TransformComponent::GetWorldX()
    {
        return GetWorldTranslation().GetX();
    }

    float TransformComponent::GetWorldY()
    {
        return GetWorldTranslation().GetY();
    }

    float TransformComponent::GetWorldZ()
    {
        return GetWorldTranslation().GetZ();
    }

    void TransformComponent::SetLocalX(float x)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetX(x);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalY(float y)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetY(y);
        SetLocalTranslation(newLocalTranslation);
    }

    void TransformComponent::SetLocalZ(float z)
    {
        AZ::Vector3 newLocalTranslation = m_localTM.GetTranslation();
        newLocalTranslation.SetZ(z);
        SetLocalTranslation(newLocalTranslation);
    }

    float TransformComponent::GetLocalX()
    {
        float localX = m_localTM.GetTranslation().GetX();
        return localX;
    }

    float TransformComponent::GetLocalY()
    {
        float localY = m_localTM.GetTranslation().GetY();
        return localY;
    }

    float TransformComponent::GetLocalZ()
    {
        float localZ = m_localTM.GetTranslation().GetZ();
        return localZ;
    }

    void TransformComponent::SetRotation(const AZ::Vector3& eulerAnglesRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotation is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AzFramework::ConvertEulerRadiansToQuaternion(eulerAnglesRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ_Warning("TransformComponent", false, "SetRotationQuaternion is deprecated, please use SetLocalRotationQuaternion");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(quaternion);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationX(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationX is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationX(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationY(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationY is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationY(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetRotationZ(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "SetRotationZ is deprecated, please use SetLocalRotation");

        AZ::Transform newWorldTransform = m_worldTM;
        newWorldTransform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationZ(eulerAngleRadian));
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::RotateByX(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByX is deprecated, please use RotateAroundLocalX");

        RotateAroundLocalX(eulerAngleRadian);
    }

    void TransformComponent::RotateByY(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByY is deprecated, please use RotateAroundLocalY");

        RotateAroundLocalY(eulerAngleRadian);
    }

    void TransformComponent::RotateByZ(float eulerAngleRadian)
    {
        AZ_Warning("TransformComponent", false, "RotateByZ is deprecated, please use RotateAroundLocalZ");

        RotateAroundLocalZ(eulerAngleRadian);
    }

    AZ::Vector3 TransformComponent::GetRotationEulerRadians()
    {
        AZ_Warning("TransformComponent", false, "GetRotationEulerRadians is deprecated, please use GetWorldRotation");

        return AzFramework::ConvertTransformToEulerRadians(m_worldTM);
    }

    AZ::Quaternion TransformComponent::GetRotationQuaternion()
    {
        AZ_Warning("TransformComponent", false, "GetRotationQuaternion is deprecated, please use GetWorldRotationQuaternion");

        return AZ::Quaternion::CreateFromTransform(m_worldTM);
    }

    float TransformComponent::GetRotationX()
    {
        AZ_Warning("TransformComponent", false, "GetRotationX is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetX();
    }

    float TransformComponent::GetRotationY()
    {
        AZ_Warning("TransformComponent", false, "GetRotationY is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetY();
    }

    float TransformComponent::GetRotationZ()
    {
        AZ_Warning("TransformComponent", false, "GetRotationZ is deprecated, please use GetWorldRotation");

        return GetRotationEulerRadians().GetZ();
    }

    AZ::Vector3 TransformComponent::GetWorldRotation()
    {
        AZ::Transform rotate = m_worldTM;
        rotate.ExtractScaleExact();
        AZ::Vector3 angles = AzFramework::ConvertTransformToEulerRadians(rotate);
        return angles;
    }

    AZ::Quaternion TransformComponent::GetWorldRotationQuaternion()
    {
        AZ::Transform rotate = m_worldTM;
        rotate.ExtractScaleExact();
        AZ::Quaternion quat = AZ::Quaternion::CreateFromTransform(rotate);
        return quat;
    }

    void TransformComponent::SetLocalRotation(const AZ::Vector3& eulerRadianAngles)
    {
        AZ::Transform newLocalTM = AzFramework::ConvertEulerRadiansToTransformPrecise(eulerRadianAngles);
        AZ::Vector3 scale = m_localTM.RetrieveScaleExact();
        newLocalTM.MultiplyByScale(scale);
        newLocalTM.SetTranslation(m_localTM.GetTranslation());
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalRotationQuaternion(const AZ::Quaternion& quaternion)
    {
        AZ::Transform newLocalTM = AZ::Transform::CreateFromQuaternion(quaternion);
        AZ::Vector3 scale = m_localTM.RetrieveScaleExact();
        newLocalTM.MultiplyByScale(scale);
        newLocalTM.SetTranslation(m_localTM.GetTranslation());
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalX(float eulerAngleRadian)
    {
        AZ::Transform currentLocalTM = m_localTM;
        AZ::Vector3 xAxis = currentLocalTM.GetBasisX();
        AZ::Quaternion xRotate = AZ::Quaternion::CreateFromAxisAngle(xAxis, eulerAngleRadian);

        AZ::Vector3 translation = currentLocalTM.GetTranslation();
        AZ::Vector3 scale = currentLocalTM.ExtractScaleExact();

        AZ::Quaternion currentRotate = AZ::Quaternion::CreateFromTransform(currentLocalTM);
        AZ::Quaternion newRotate = xRotate * currentRotate;
        newRotate.NormalizeExact();

        AZ::Transform newLocalTM = AZ::Transform::CreateFromQuaternion(newRotate);
        newLocalTM.MultiplyByScale(scale);
        newLocalTM.SetTranslation(translation);

        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalY(float eulerAngleRadian)
    {
        AZ::Transform currentLocalTM = m_localTM;
        AZ::Vector3 yAxis = currentLocalTM.GetBasisY();
        AZ::Quaternion yRotate = AZ::Quaternion::CreateFromAxisAngle(yAxis, eulerAngleRadian);

        AZ::Vector3 translation = currentLocalTM.GetTranslation();
        AZ::Vector3 scale = currentLocalTM.ExtractScaleExact();

        AZ::Quaternion currentRotate = AZ::Quaternion::CreateFromTransform(currentLocalTM);
        AZ::Quaternion newRotate = yRotate * currentRotate;
        newRotate.NormalizeExact();

        AZ::Transform newLocalTM = AZ::Transform::CreateFromQuaternion(newRotate);
        newLocalTM.MultiplyByScale(scale);
        newLocalTM.SetTranslation(translation);

        SetLocalTM(newLocalTM);
    }

    void TransformComponent::RotateAroundLocalZ(float eulerAngleRadian)
    {
        AZ::Transform currentLocalTM = m_localTM;
        AZ::Vector3 zAxis = currentLocalTM.GetBasisZ();
        AZ::Quaternion zRotate = AZ::Quaternion::CreateFromAxisAngle(zAxis, eulerAngleRadian);

        AZ::Vector3 translation = currentLocalTM.GetTranslation();
        AZ::Vector3 scale = currentLocalTM.ExtractScaleExact();

        AZ::Quaternion currentRotate = AZ::Quaternion::CreateFromTransform(currentLocalTM);
        AZ::Quaternion newRotate = zRotate * currentRotate;
        newRotate.NormalizeExact();

        AZ::Transform newLocalTM = AZ::Transform::CreateFromQuaternion(newRotate);
        newLocalTM.MultiplyByScale(scale);
        newLocalTM.SetTranslation(translation);

        SetLocalTM(newLocalTM);
    }

    AZ::Vector3 TransformComponent::GetLocalRotation()
    {
        AZ::Transform rotate = m_localTM;
        rotate.ExtractScaleExact();
        AZ::Vector3 angles = AzFramework::ConvertTransformToEulerRadians(rotate);
        return angles;
    }

    AZ::Quaternion TransformComponent::GetLocalRotationQuaternion()
    {
        AZ::Transform rotate = m_localTM;
        rotate.ExtractScaleExact();
        AZ::Quaternion quat = AZ::Quaternion::CreateFromTransform(rotate);
        return quat;
    }

    void TransformComponent::SetScale(const AZ::Vector3& scale)
    {
        AZ_Warning("TransformCompnent", false, "SetScale is deprecated, please use SetLocalScale");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 prevScale = newWorldTransform.ExtractScale();
        if (!prevScale.IsClose(scale))
        {
            newWorldTransform.MultiplyByScale(scale);
            SetWorldTM(newWorldTransform);
        }
    }

    void TransformComponent::SetScaleX(float scaleX)
    {
        AZ_Warning("TransformCompnent", false, "SetScaleX is deprecated, please use SetLocalScaleX");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetX(scaleX);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }

    void TransformComponent::SetScaleY(float scaleY)
    {
        AZ_Warning("TransformCompnent", false, "SetScaleY is deprecated, please use SetLocalScaleY");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetY(scaleY);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }
    
    void TransformComponent::SetScaleZ(float scaleZ)
    {
        AZ_Warning("TransformCompnent", false, "SetScaleZ is deprecated, please use SetLocalScaleZ");

        AZ::Transform newWorldTransform = m_worldTM;
        AZ::Vector3 scale = newWorldTransform.ExtractScale();
        scale.SetZ(scaleZ);
        newWorldTransform.MultiplyByScale(scale);
        SetWorldTM(newWorldTransform);
    }

    AZ::Vector3 TransformComponent::GetScale()
    {
        AZ_Warning("TransformCompnent", false, "GetScale is deprecated, please use GetLocalScale");

        return m_worldTM.RetrieveScale();
    }

    float TransformComponent::GetScaleX()
    {
        AZ_Warning("TransformCompnent", false, "GetScaleX is deprecated, please use GetLocalScale");
        
        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetX();
    }

    float TransformComponent::GetScaleY()
    {
        AZ_Warning("TransformCompnent", false, "GetScaleY is deprecated, please use GetLocalScale");

        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetY();
    }

    float TransformComponent::GetScaleZ()
    {
        AZ_Warning("TransformCompnent", false, "GetScaleZ is deprecated, please use GetLocalScale");

        AZ::Vector3 scale = m_worldTM.RetrieveScale();
        return scale.GetZ();
    }

    void TransformComponent::SetLocalScale(const AZ::Vector3& scale)
    {
        AZ::Transform newLocalTM = m_localTM;
        newLocalTM.ExtractScaleExact();
        newLocalTM.MultiplyByScale(scale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleX(float scaleX)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetX(scaleX);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleY(float scaleY)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetY(scaleY);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    void TransformComponent::SetLocalScaleZ(float scaleZ)
    {
        AZ::Transform newLocalTM = m_localTM;
        AZ::Vector3 newScale = newLocalTM.ExtractScaleExact();
        newScale.SetZ(scaleZ);
        newLocalTM.MultiplyByScale(newScale);
        SetLocalTM(newLocalTM);
    }

    AZ::Vector3 TransformComponent::GetLocalScale()
    {
        AZ::Vector3 scale = m_localTM.RetrieveScaleExact();
        return scale;
    }

    AZ::Vector3 TransformComponent::GetWorldScale()
    {
        AZ::Vector3 scale = m_worldTM.RetrieveScaleExact();
        return scale;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetChildren()
    {
        AZStd::vector<AZ::EntityId> children;
        EBUS_EVENT_ID(GetEntityId(), AZ::TransformHierarchyInformationBus, GatherChildren, children);
        return children;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = GetChildren();
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            EBUS_EVENT_ID(descendants[i], AZ::TransformHierarchyInformationBus, GatherChildren, descendants);
        }
        return descendants;
    }

    AZStd::vector<AZ::EntityId> TransformComponent::GetEntityAndAllDescendants()
    {
        AZStd::vector<AZ::EntityId> descendants = { GetEntityId() };
        for (size_t i = 0; i < descendants.size(); ++i)
        {
            EBUS_EVENT_ID(descendants[i], AZ::TransformHierarchyInformationBus, GatherChildren, descendants);
        }
        return descendants;
    }

    bool TransformComponent::IsStaticTransform()
    {
        return m_isStatic;
    }

    void TransformComponent::OnTransformChanged(const AZ::Transform& parentLocalTM, const AZ::Transform& parentWorldTM)
    {
        OnTransformChangedImpl(parentLocalTM, parentWorldTM);
    }

    void TransformComponent::GatherChildren(AZStd::vector<AZ::EntityId>& children)
    {
        children.push_back(GetEntityId());
    }

    void TransformComponent::OnEntityActivated(const AZ::EntityId& parentEntityId)
    {
        OnEntityActivatedImpl(parentEntityId);
        UpdateReplicaChunk();
    }

    void TransformComponent::OnEntityDeactivated(const AZ::EntityId& parentEntityId)
    {
        if (!IsNetworkControlled())
        {
            OnEntityDeactivateImpl(parentEntityId);
            UpdateReplicaChunk();
        }
        else
        {
            // If this transform is network controlled, then the localTM is updated by the network,
            // so update m_parentTM and compute worldTM instead.
            AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
            m_parentTM = nullptr;
            ComputeWorldTM();
        }
    }

    GridMate::ReplicaChunkPtr TransformComponent::GetNetworkBinding()
    {
        TransformReplicaChunk* replicaChunk = GridMate::CreateReplicaChunk<TransformReplicaChunk>();
        replicaChunk->SetHandler(this);
        m_replicaChunk = replicaChunk;

        UpdateReplicaChunk();

        return m_replicaChunk;
    }

    void TransformComponent::SetNetworkBinding(GridMate::ReplicaChunkPtr replicaChunk)
    {
        AZ_Assert(m_replicaChunk == nullptr, "Being bound to two ReplicaChunks");

        bool isTransformChunk = replicaChunk != nullptr;

        AZ_Assert(isTransformChunk, "Being bound to invalid chunk type");
        if (isTransformChunk)
        {
            replicaChunk->SetHandler(this);
            m_replicaChunk = replicaChunk;

            TransformReplicaChunk* transformReplicaChunk = static_cast<TransformReplicaChunk*>(m_replicaChunk.get());

            m_parentId = AZ::EntityId(transformReplicaChunk->m_parentId.Get());

            m_worldTM = transformReplicaChunk->m_initialWorldTM;
            m_localTM = transformReplicaChunk->m_localTransform.Get();
        }

        m_onNewParentKeepWorldTM = false;
    }

    void TransformComponent::UnbindFromNetwork()
    {
        if (m_replicaChunk)
        {
            m_replicaChunk->SetHandler(nullptr);
            m_replicaChunk = nullptr;
        }
    }

    void TransformComponent::OnNewNetTransformData(const AZ::Transform& transform, const GridMate::TimeContext& /*tc*/)
    {        
        SetLocalTMImpl(transform);
    }

    void TransformComponent::OnNewNetParentData(const AZ::u64& parentId, const GridMate::TimeContext& /*tc*/)
    {
        SetParentImpl(AZ::EntityId(parentId), false);
    }

    bool TransformComponent::IsNetworkControlled() const
    {
        return m_replicaChunk && m_replicaChunk->GetReplica() && !m_replicaChunk->IsMaster();
    }

    void TransformComponent::UpdateReplicaChunk()
    {
        if (!IsNetworkControlled() && m_replicaChunk)
        {
            TransformReplicaChunk* transformReplicaChunk = static_cast<TransformReplicaChunk*>(m_replicaChunk.get());
            transformReplicaChunk->m_localTransform.Set(GetLocalTM());
            transformReplicaChunk->m_parentId.Set(static_cast<AZ::u64>(GetParentId()));
        }
    }

    void TransformComponent::SetParentImpl(AZ::EntityId parentId, bool isKeepWorldTM)
    {
        AZ::EntityId oldParent = m_parentId;
        if (m_parentId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TransformHierarchyInformationBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
        }

        m_parentId = parentId;
        if (m_parentId.IsValid())
        {
            m_onNewParentKeepWorldTM = isKeepWorldTM;

            AZ::TransformNotificationBus::Handler::BusConnect(m_parentId);
            AZ::TransformHierarchyInformationBus::Handler::BusConnect(m_parentId);
            AZ::EntityBus::Handler::BusConnect(m_parentId);
        }
        else
        {
            m_parentTM = nullptr;

            if (isKeepWorldTM)
            {
                SetWorldTM(m_worldTM);
            }
            else
            {
                SetLocalTM(m_localTM);
            }

            if (oldParent.IsValid())
            {
                EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
            }
        }

        EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnParentChanged, oldParent, parentId);

        if (oldParent != parentId) // Don't send removal notification while activating.
        {
            EBUS_EVENT_ID(oldParent, AZ::TransformNotificationBus, OnChildRemoved, GetEntityId());
        }

        EBUS_EVENT_ID(parentId, AZ::TransformNotificationBus, OnChildAdded, GetEntityId());
    }

    void TransformComponent::SetLocalTMImpl(const AZ::Transform& tm)
    {
        m_localTM = tm;
        ComputeWorldTM();  // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::SetWorldTMImpl(const AZ::Transform& tm)
    {
        m_worldTM = tm;
        ComputeLocalTM(); // We can user dirty flags and compute it later on demand
    }

    void TransformComponent::OnTransformChangedImpl(const AZ::Transform& /*parentLocalTM*/, const AZ::Transform& parentWorldTM)
    {
        // Called when our parent transform changes
        // Ignore the event until we've already derived our local transform.
        if (m_parentTM)
        {            
            m_worldTM = parentWorldTM * m_localTM;
            EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
        }
    }

    void TransformComponent::OnEntityActivatedImpl(const AZ::EntityId& parentEntityId)
    {
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
        
        AZ::Entity* parentEntity = nullptr;
        EBUS_EVENT_RESULT(parentEntity, AZ::ComponentApplicationBus, FindEntity, parentEntityId);
        AZ_Assert(parentEntity, "We expect to have a parent entity associated with the provided parent's entity Id.");
        if (parentEntity)
        {
            m_parentTM = parentEntity->GetTransform();

            AZ_Warning("TransformComponent", !m_isStatic || m_parentTM->IsStaticTransform(),
                "Entity '%s' %s has static transform, but parent has non-static transform. This may lead to unexpected movement.",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());

            if (m_onNewParentKeepWorldTM)
            {
                ComputeLocalTM();
            }
            else
            {
                ComputeWorldTM();
            }
        }
    }

    void TransformComponent::OnEntityDeactivateImpl(const AZ::EntityId& parentEntityId)
    {
        (void)parentEntityId;
        AZ_Assert(parentEntityId == m_parentId, "We expect to receive notifications only from the current parent!");
        m_parentTM = nullptr;
        ComputeLocalTM();
    }

    void TransformComponent::ComputeLocalTM()
    {
        if (m_parentTM)
        {
            m_localTM = m_parentTM->GetWorldTM().GetInverseFull() * m_worldTM;
        }
        else
        {
            m_localTM = m_worldTM;
        }
        EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
    }

    void TransformComponent::ComputeWorldTM()
    {
        if (m_parentTM)
        {
            m_worldTM = m_parentTM->GetWorldTM() * m_localTM;
        }
        else if (!m_parentId.IsValid())
        {
            m_worldTM = m_localTM;
        }        

        EBUS_EVENT_PTR(m_notificationBus, AZ::TransformNotificationBus, OnTransformChanged, m_localTM, m_worldTM);
    }

    bool TransformComponent::AreMoveRequestsAllowed() const
    {
        if (IsNetworkControlled())
        {
            return false;
        }

        // Don't allow static transform to be moved while entity is activated.
        // But do allow a static transform to be moved when the entity is deactivated.
        if (m_isStatic && m_entity && (m_entity->GetState() > AZ::Entity::ES_INIT))
        {
            return false;
        }

        return true;
    }

    void TransformComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void TransformComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<TransformComponent, AZ::Component, NetBindable>()
                ->Version(3, &TransformComponentVersionConverter)
                ->Field("Parent", &TransformComponent::m_parentId)
                ->Field("Transform", &TransformComponent::m_worldTM)
                ->Field("LocalTransform", &TransformComponent::m_localTM)
                ->Field("ParentActivationTransformMode", &TransformComponent::m_parentActivationTransformMode)
                ->Field("IsStatic", &TransformComponent::m_isStatic)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if(behaviorContext)
        {
            behaviorContext->EBus<AZ::TransformNotificationBus>("TransformNotificationBus")->
                Handler<AZ::BehaviorTransformNotificationBusHandler>();

            behaviorContext->Class<TransformComponent>()->RequestBus("TransformBus");

            behaviorContext->EBus<AZ::TransformBus>("TransformBus")
                ->Event("GetLocalTM", &AZ::TransformBus::Events::GetLocalTM)
                ->Event("GetWorldTM", &AZ::TransformBus::Events::GetWorldTM)
                ->Event("GetParentId", &AZ::TransformBus::Events::GetParentId)
                ->Event("GetLocalAndWorld", &AZ::TransformBus::Events::GetLocalAndWorld)
                ->Event("SetLocalTM", &AZ::TransformBus::Events::SetLocalTM)
                ->Event("SetWorldTM", &AZ::TransformBus::Events::SetWorldTM)
                ->Event("SetParent", &AZ::TransformBus::Events::SetParent)
                ->Event("SetParentRelative", &AZ::TransformBus::Events::SetParentRelative)
                ->Event("SetWorldTranslation", &AZ::TransformBus::Events::SetWorldTranslation)
                ->Event("SetLocalTranslation", &AZ::TransformBus::Events::SetLocalTranslation)
                ->Event("GetWorldTranslation", &AZ::TransformBus::Events::GetWorldTranslation)
                    ->Attribute("Position", AZ::Edit::Attributes::PropertyPosition)
                ->VirtualProperty("Position", "GetWorldTranslation", "SetWorldTranslation")
                ->Event("GetLocalTranslation", &AZ::TransformBus::Events::GetLocalTranslation)
                ->Event("MoveEntity", &AZ::TransformBus::Events::MoveEntity)
                ->Event("SetWorldX", &AZ::TransformBus::Events::SetWorldX)
                ->Event("SetWorldY", &AZ::TransformBus::Events::SetWorldY)
                ->Event("SetWorldZ", &AZ::TransformBus::Events::SetWorldZ)
                ->Event("GetWorldX", &AZ::TransformBus::Events::GetWorldX)
                ->Event("GetWorldY", &AZ::TransformBus::Events::GetWorldY)
                ->Event("GetWorldZ", &AZ::TransformBus::Events::GetWorldZ)
                ->Event("SetLocalX", &AZ::TransformBus::Events::SetLocalX)
                ->Event("SetLocalY", &AZ::TransformBus::Events::SetLocalY)
                ->Event("SetLocalZ", &AZ::TransformBus::Events::SetLocalZ)
                ->Event("GetLocalX", &AZ::TransformBus::Events::GetLocalX)
                ->Event("GetLocalY", &AZ::TransformBus::Events::GetLocalY)
                ->Event("GetLocalZ", &AZ::TransformBus::Events::GetLocalZ)
                ->Event("RotateByX", &AZ::TransformBus::Events::RotateByX)
                ->Event("RotateByY", &AZ::TransformBus::Events::RotateByY)
                ->Event("RotateByZ", &AZ::TransformBus::Events::RotateByZ)
                ->Event("SetEulerRotation", &AZ::TransformBus::Events::SetRotation) // Deprecated
                ->Event("SetRotationQuaternion", &AZ::TransformBus::Events::SetRotationQuaternion) // Deprecated
                ->Event("SetRotationX", &AZ::TransformBus::Events::SetRotationX) // Deprecated
                ->Event("SetRotationY", &AZ::TransformBus::Events::SetRotationY) // Deprecated
                ->Event("SetRotationZ", &AZ::TransformBus::Events::SetRotationZ) // Deprecated
                ->Event("GetEulerRotation", &AZ::TransformBus::Events::GetRotationEulerRadians) // Deprecated
                ->Event("GetRotationQuaternion", &AZ::TransformBus::Events::GetRotationQuaternion) // Deprecated
                ->Event("GetRotationX", &AZ::TransformBus::Events::GetRotationX) // Deprecated
                ->Event("GetRotationY", &AZ::TransformBus::Events::GetRotationY) // Deprecated
                ->Event("GetRotationZ", &AZ::TransformBus::Events::GetRotationZ) // Deprecated
                ->Event("GetWorldRotation", &AZ::TransformBus::Events::GetWorldRotation)
                ->Event("GetWorldRotationQuaternion", &AZ::TransformBus::Events::GetWorldRotationQuaternion)
                ->Event("SetLocalRotation", &AZ::TransformBus::Events::SetLocalRotation)
                ->Event("SetLocalRotationQuaternion", &AZ::TransformBus::Events::SetLocalRotationQuaternion)
                ->Event("RotateAroundLocalX", &AZ::TransformBus::Events::RotateAroundLocalX)
                ->Event("RotateAroundLocalY", &AZ::TransformBus::Events::RotateAroundLocalY)
                ->Event("RotateAroundLocalZ", &AZ::TransformBus::Events::RotateAroundLocalZ)
                ->Event("GetLocalRotation", &AZ::TransformBus::Events::GetLocalRotation)
                ->Event("GetLocalRotationQuaternion", &AZ::TransformBus::Events::GetLocalRotationQuaternion)
                    ->Attribute("Rotation", AZ::Edit::Attributes::PropertyRotation)
                ->VirtualProperty("Rotation", "GetLocalRotationQuaternion", "SetLocalRotationQuaternion")
                ->Event("SetScale", &AZ::TransformBus::Events::SetScale)    // Deprecated
                ->Event("SetScaleX", &AZ::TransformBus::Events::SetScaleX)  // Deprecated
                ->Event("SetScaleY", &AZ::TransformBus::Events::SetScaleY)  // Deprecated
                ->Event("SetScaleZ", &AZ::TransformBus::Events::SetScaleZ)  // Deprecated
                ->Event("GetScale", &AZ::TransformBus::Events::GetScale)    // Deprecated
                ->Event("GetScaleX", &AZ::TransformBus::Events::GetScaleX)  // Deprecated
                ->Event("GetScaleY", &AZ::TransformBus::Events::GetScaleY)  // Deprecated
                ->Event("GetScaleZ", &AZ::TransformBus::Events::GetScaleZ)  // Deprecated
                ->Event("SetLocalScale", &AZ::TransformBus::Events::SetLocalScale)
                ->Event("SetLocalScaleX", &AZ::TransformBus::Events::SetLocalScaleX)
                ->Event("SetLocalScaleY", &AZ::TransformBus::Events::SetLocalScaleY)
                ->Event("SetLocalScaleZ", &AZ::TransformBus::Events::SetLocalScaleZ)
                ->Event("GetLocalScale", &AZ::TransformBus::Events::GetLocalScale)
                    ->Attribute("Scale", AZ::Edit::Attributes::PropertyScale)
                ->VirtualProperty("Scale", "GetLocalScale", "SetLocalScale")
                ->Event("GetWorldScale", &AZ::TransformBus::Events::GetWorldScale)
                ->Event("GetChildren", &AZ::TransformBus::Events::GetChildren)
                ->Event("GetAllDescendants", &AZ::TransformBus::Events::GetAllDescendants)
                ->Event("GetEntityAndAllDescendants", &AZ::TransformBus::Events::GetEntityAndAllDescendants)
                ->Event("IsStaticTransform", &AZ::TransformBus::Events::IsStaticTransform)
                ;
        }

        NetworkContext* netContext = azrtti_cast<NetworkContext*>(reflection);
        if (netContext)
        {
            netContext->Class<TransformComponent>()
                ->Chunk<TransformReplicaChunk, TransformReplicaChunk::Descriptor>()
                    ->Field("ParentId", &TransformReplicaChunk::m_parentId)
                    ->Field("LocalTransformData", &TransformReplicaChunk::m_localTransform);
        }

    }
} // namespace AZ

#endif  // AZ_UNITY_BUILD

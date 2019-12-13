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

#include "LmbrCentral_precompiled.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include "Source/Rendering/EditorMeshComponent.h"

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    // base physical entity which can be derived from to detect other specific use-cases
    struct PhysicalEntityPlaceHolder
        : public IPhysicalEntity
    {
        pe_type GetType() const override { return PE_NONE; } 
        int AddRef() override { return 0; }
        int Release() override { return 0; }
        int SetParams(const pe_params* params, int bThreadSafe = 0) override { return 0; }
        int GetParams(pe_params* params) const override { return 0; } 
        int GetStatus(pe_status* status) const override { return 0; } 
        int Action(const pe_action*, int bThreadSafe = 0) override { return 0; } 
        int AddGeometry(phys_geometry* pgeom, pe_geomparams* params, int id = -1, int bThreadSafe = 0) override { return 0; }
        void RemoveGeometry(int id, int bThreadSafe = 0) override {}
        PhysicsForeignData GetForeignData(int itype = 0) const override { return PhysicsForeignData{}; }
        int GetiForeignData() const override { return 0; }
        int GetStateSnapshot(class CStream& stm, float time_back = 0, int flags = 0) override { return 0; }
        int GetStateSnapshot(TSerialize ser, float time_back = 0, int flags = 0) override { return 0; }
        int SetStateFromSnapshot(class CStream& stm, int flags = 0) override { return 0; }
        int PostSetStateFromSnapshot() override { return 0; }
        unsigned int GetStateChecksum() override { return 0; }
        void SetNetworkAuthority(int authoritive = -1, int paused = -1) override {}
        int SetStateFromSnapshot(TSerialize ser, int flags = 0) override { return 0; }
        int SetStateFromTypedSnapshot(TSerialize ser, int type, int flags = 0) override { return 0; }
        int GetStateSnapshotTxt(char* txtbuf, int szbuf, float time_back = 0) override { return 0; }
        void SetStateFromSnapshotTxt(const char* txtbuf, int szbuf) override {}
        int DoStep(float time_interval) override { return 0; }
        int DoStep(float time_interval, int iCaller) override { return 0; }
        void StartStep(float time_interval) override {}
        void StepBack(float time_interval) override {}
        IPhysicalWorld* GetWorld() const override { return nullptr; }
        void GetMemoryStatistics(ICrySizer* pSizer) const override {}
    };

    // special test fake to validate incoming pe_params
    struct PhysicalEntitySetParamsCheck
        : public PhysicalEntityPlaceHolder
    {
        int SetParams(const pe_params* params, int bThreadSafe = 0) override
        {
            if (params->type == pe_params_pos::type_id)
            {
                pe_params_pos* params_pos = (pe_params_pos*)params;

                Vec3 s;
                if (Matrix34* m34 = params_pos->pMtx3x4)
                {
                    s.Set(m34->GetColumn(0).len(), m34->GetColumn(1).len(), m34->GetColumn(2).len());
                    Matrix33 m33(m34->GetColumn(0) / s.x, m34->GetColumn(1) / s.y, m34->GetColumn(2) / s.z);
                    // ensure passed in params_pos->pMtx3x4 is orthonormal
                    // ref - see Cry_Quat.h - explicit ILINE Quat_tpl<F>(const Matrix33_tpl<F>&m)
                    m_isOrthonormal = m33.IsOrthonormalRH(0.1f);
                }
            }

            return 0;
        }

        bool m_isOrthonormal = false;
    };

    class TestEditorMeshComponent
        : public EditorMeshComponent
    {
    public:
        AZ_EDITOR_COMPONENT(TestEditorMeshComponent, "{6C6B593A-1946-4239-AE16-E8B96D9835E5}", EditorMeshComponent)

        static void Reflect(AZ::ReflectContext* context);

        TestEditorMeshComponent() = default;

        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override
        {
            // to stop the physics entity getting rebuilt ensure m_physScale stays in sync with world scale
            m_physScale = world.RetrieveScale();
            EditorMeshComponent::OnTransformChanged(local, world);
        }

        void OverridePhysicalEntity(IPhysicalEntity* physicalEntity)
        {
            m_physicalEntity = physicalEntity;
        }

        void SetScale(const AZ::EntityId entityId, const AZ::Vector3& scale)
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            const AZ::Transform newWorldFromLocal = worldFromLocal * AZ::Transform::CreateScale(scale);

            AZ::TransformBus::Event(
                entityId, &AZ::TransformBus::Events::SetWorldTM, newWorldFromLocal);
        }
    };

    void TestEditorMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestEditorMeshComponent>()
                ->Version(0);
        }
    }

    class EditorMeshComponentTestFixture
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_testMeshComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_testMeshComponentDescriptor =
                AZStd::unique_ptr<ComponentDescriptor>(TestEditorMeshComponent::CreateDescriptor());
            m_transformComponentDescriptor =
                AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());

            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_testMeshComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_testMeshComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    TEST_F(EditorMeshComponentTestFixture, OrthonormalTransformIsPassedToPhysicalEntity)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup a simple hierarchy
        AZ::Entity parent;
        AZ::Entity child;

        parent.Init();
        child.Init();

        AZ::EntityId parentId = parent.GetId();
        AZ::EntityId childId = child.GetId();

        AZ::TransformBus::Event(
            childId, &AZ::TransformInterface::SetParent, parentId);

        parent.CreateComponent<TransformComponent>();
        child.CreateComponent<TransformComponent>();

        // add test mesh component to 'sense' change
        TestEditorMeshComponent* parentTestMeshComponent =
            parent.CreateComponent<TestEditorMeshComponent>();
        TestEditorMeshComponent* childTestMeshComponent =
            child.CreateComponent<TestEditorMeshComponent>();

        parent.Activate();
        child.Activate();

        // apply scale to entities
        parentTestMeshComponent->SetScale(parentId, AZ::Vector3(2.0f, 0.5f, 3.0f));
        childTestMeshComponent->SetScale(childId, AZ::Vector3(3.0f, 2.5f, 0.2f));

        // attach face physical entity (to detect changes)
        auto parentPhysicalEntitySetParamChecker = AZStd::make_unique<PhysicalEntitySetParamsCheck>();
        parentTestMeshComponent->OverridePhysicalEntity(parentPhysicalEntitySetParamChecker.get());

        auto childPhysicalEntitySetParamChecker = AZStd::make_unique<PhysicalEntitySetParamsCheck>();
        childTestMeshComponent->OverridePhysicalEntity(childPhysicalEntitySetParamChecker.get());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const AZ::Transform rotation =
            AZ::Transform::CreateFromQuaternion(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::DegToRad(40.0f)));

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            worldFromLocal, childId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformBus::Event(
            childId, &AZ::TransformBus::Events::SetWorldTM, worldFromLocal * rotation);

        // apply new scale to parent after orientating child entity
        parentTestMeshComponent->SetScale(parentId, AZ::Vector3(0.6f, 0.5f, 3.0f));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // ensure matricies are still orthonormal
        EXPECT_TRUE(childPhysicalEntitySetParamChecker->m_isOrthonormal);
        EXPECT_TRUE(parentPhysicalEntitySetParamChecker->m_isOrthonormal);

        // clear test physical entity
        childTestMeshComponent->OverridePhysicalEntity(nullptr);
        parentTestMeshComponent->OverridePhysicalEntity(nullptr);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
} // namespace UnitTest

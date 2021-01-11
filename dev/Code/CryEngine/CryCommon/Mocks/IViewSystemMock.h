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
#include <IViewSystem.h>
#include <Cry_Camera.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/containers/map.h>


#pragma warning( push )
#pragma warning( disable : 4373 ) // previous versions of the compiler did not override when parameters only differed by const / volatile qualifiers

class ViewMock
    : public IView
{
public:
    MOCK_METHOD0(Release, void());
    MOCK_METHOD2(Update, void(float frameTime, bool isActive));
    MOCK_METHOD1(LinkTo, void(AZ::Entity* follow));
    MOCK_METHOD1(LinkTo, void(IGameObject* follow));
    MOCK_METHOD1(LinkTo, void(IEntity* follow));
    MOCK_METHOD0(Unlink, void());
    MOCK_METHOD0(GetLinkedId, AZ::EntityId());

    CCamera m_camera;
    CCamera& GetCamera() { return m_camera; }
    const CCamera& GetCamera() const { return m_camera; }

    MOCK_METHOD1(Serialize, void(TSerialize ser));
    MOCK_METHOD0(PostSerialize, void());

    SViewParams m_currentParams;
    void SetCurrentParams(SViewParams& params)
    {
        m_currentParams = params;
    }
    
    const SViewParams* GetCurrentParams()
    {
        return &m_currentParams;
    }

    void SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec = true, bool bUpdateOnly = false, bool bGroundOnly = false)
    {
        return;
    }
    MOCK_METHOD1(SetViewShakeEx, void(const SShakeParams& params));
    MOCK_METHOD1(StopShake, void(int shakeID));
    MOCK_METHOD0(ResetShaking, void());
    MOCK_METHOD0(ResetBlending, void());
    MOCK_METHOD1(SetFrameAdditiveCameraAngles, void(const Ang3& addFrameAngles));
    MOCK_METHOD1(SetScale, void(const float scale));
    MOCK_METHOD1(SetZoomedScale, void(const float scale));
    MOCK_METHOD1(SetActive, void(const bool bActive));
    MOCK_METHOD1(UpdateAudioListener, void(const Matrix34& rMatrix));

    bool ProjectWorldPointToScreen(const AZ::Vector3& worldPoint, AZ::Vector3& outScreenPoint)
    {
        outScreenPoint = AZ::Vector3::CreateZero();
        return true;
    }

    bool UnprojectScreenPointToWorld(const AZ::Vector3& screenPoint, AZ::Vector3& outWorldPoint)
    {
        outWorldPoint = AZ::Vector3::CreateZero();
        return true;
    }
    
    bool ProjectWorldPointToViewport(const AZ::Vector3& worldPoint, const AZ::Vector4& viewport, AZ::Vector3& outViewportPoint)
    {
        outViewportPoint = AZ::Vector3::CreateZero();
        return true;
    }
    
    bool UnprojectViewportPointToWorld(const AZ::Vector3& viewportPoint, const AZ::Vector4& viewport, AZ::Vector3& outWorldPoint)
    {
        outWorldPoint = AZ::Vector3::CreateZero();
        return true;
    }
    
    void GetProjectionMatrix(const float nearClipPlane, const float farClipPlane, AZ::Matrix4x4& outProjectionMatrix)
    {
        outProjectionMatrix = AZ::Matrix4x4::CreateIdentity();
    }
};

class ViewSystemMock
    : public IViewSystem
{
public:
    const unsigned int InvalidView = 0;

    MOCK_METHOD0(Release, void());
    MOCK_METHOD1(Update, void(float frameTime));

    IView* CreateView() 
    { 
        auto newView = new ::testing::NiceMock<ViewMock>();
        m_idToViewMap.insert({m_nextViewId, newView});
        m_nextViewId++;
        return newView;
    }

    MOCK_METHOD1(AddView, unsigned int(IView *pView));

    void RemoveView(IView* pView)
    {
        auto viewId = GetViewId(pView);
        if (viewId != InvalidView)
        {
            m_idToViewMap.erase(viewId);
        }
    }

    void RemoveView(unsigned int viewId)
    {
        m_idToViewMap.erase(viewId);
    }

    void SetActiveView(IView* pView)
    {
        m_activeViewId = GetViewId(pView);
    }

    void SetActiveView(unsigned int viewId)
    {
        auto targetView = GetView(viewId);
        if (targetView)
        {
            SetActiveView(targetView);
        }
    }

    IView* GetView(unsigned int viewId)
    {
        auto viewIterator = m_idToViewMap.find(viewId);
        if (viewIterator != m_idToViewMap.end())
        {
            return viewIterator->second;
        }
        return nullptr;
    }

    IView* GetActiveView()
    {
        return GetView(m_activeViewId);
    }

    unsigned int GetViewId(IView* pView)
    {
        for (ViewMapType::iterator viewIterator = m_idToViewMap.begin(); viewIterator != m_idToViewMap.end(); ++viewIterator)
        {
            if (viewIterator->second == pView)
            {
                return viewIterator->first;
            }
        }

        return InvalidView;
    }

    unsigned int GetActiveViewId()
    {
        return m_activeViewId;
    }

    MOCK_METHOD2(GetViewByEntityId, IView* (const AZ::EntityId& id, bool forceCreate));

    MOCK_METHOD1(AddListener, bool(IViewSystemListener* pListener));
    MOCK_METHOD1(RemoveListener, bool(IViewSystemListener* pListener));

    MOCK_METHOD1(Serialize, void(TSerialize ser));
    MOCK_METHOD0(PostSerialize, void());

    MOCK_METHOD0(GetDefaultZNear, float());

    MOCK_METHOD3(SetBlendParams, void(float fBlendPosSpeed, float fBlendRotSpeed, bool performBlendOut));

    MOCK_METHOD2(SetOverrideCameraRotation, void(bool bOverride, Quat rotation));

    MOCK_CONST_METHOD0(IsPlayingCutScene, bool());

    MOCK_METHOD0(UpdateSoundListeners, void());

    MOCK_METHOD1(SetDeferredViewSystemUpdate, void(bool const bDeferred));
    MOCK_CONST_METHOD0(UseDeferredViewSystemUpdate, bool());
    MOCK_METHOD1(SetControlAudioListeners, void(bool const bActive));
    MOCK_METHOD1(ForceUpdate, void(float elapsed));

private:
    unsigned int m_activeViewId = InvalidView;

    using ViewMapType = AZStd::map<unsigned int, IView*>;
    ViewMapType m_idToViewMap;
    unsigned int m_nextViewId = 1;
};
#pragma warning( pop )

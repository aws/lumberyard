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

#pragma once

#if (defined(WIN32) && !defined(WIN64))

#if defined(WIN32)

#include <MSR_NuiApi.h>

#endif

#if !defined(RELEASE) && (defined(WIN32) || defined(WIN64))
#define KINECT_XBOX_CONNECT // ACCEPTED_USE
class CKinectXboxSyncThread;
#endif

#include <CryThread.h>
#include <CryListenerSet.h>
#include <BoostHelpers.h>


DECLARE_SMART_POINTERS(CryCriticalSection);

#ifdef KINECT_USE_HANDLES
struct SKinGripInternal
{
    std::vector<int> railIds;
    NUI_HANDLES_GRIP nuiGrip;
};
#endif

class CKinectInput
    : public IKinectInput
    , public ISystemEventListener
{
public:
    CKinectInput();
    ~CKinectInput();

    bool Init();
    void Update();

    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

    virtual void RegisterInputListener(IKinectInputListener* pInputListener, const char* name){m_eventListeners.Add(pInputListener, name); }
    virtual void UnregisterInputListener(IKinectInputListener* pInputListener){m_eventListeners.Remove(pInputListener); }

    bool RegisterArcRail(int gripId, int railId, const Vec2& vScreenPos, const Vec3& vDir, float fLenght, float fDeadzoneLength, float fToleranceConeAngle);
    void UnregisterArcRail(int gripId);
    bool RegisterHoverTimeRail(int gripId, int railId, const Vec2& vScreenPos, float fHoverTime, float fTimeTillCommit, SKinGripShape* pGripShape = NULL);
    void UnregisterHoverTimeRail(int gripId);
    void UnregisterAllRails();
    inline bool IsEnabled() {return m_enabled; }
    bool GetBodySpaceHandles(SKinBodyShapeHandles& bodyShapeHandles);

    //Skeleton
    void EnableSeatedSkeletonTracking(bool bValue);
    uint32 GetClosestTrackedSkeleton() const {return m_closestSkeletonIndex; }
    bool GetSkeletonRawData(uint32 iUser, SKinSkeletonRawData& skeletonRawData) const;
    bool GetSkeletonDefaultData(uint32 iUser, SKinSkeletonDefaultData& skeletonDefaultData) const;

    void UpdateSkeletonAlignment(uint32 skeletonIndex);

    void EnableWaveGestureTracking(bool bEnable);
    float GetWaveGestureProgress(DWORD* pTrackingId);

    // Identity
    bool IdentityDetectedIntentToPlay(DWORD dwTrackingId);
    bool IdentityIdentify(DWORD dwTrackingId, KinIdentifyCallback callbackFunc, void* pData);

    // Speech
    bool SpeechEnable();
    void SpeechDisable();
    void SetEnableGrammar(const string& grammarName, bool enable) {}
    bool KinSpeechSetEventInterest(unsigned long ulEvents);
    bool KinSpeechLoadDefaultGrammar();
    bool KinSpeechStartRecognition();
    void KinSpeechStopRecognition();
    const char* GetUserStatusMessage() {return "IDENTIFYING"; }

    //~IKinectInput

#ifdef KINECT_USE_IDENTITY
    bool OnIdentityCallBack(NUI_IDENTITY_MESSAGE* pMessage);
#endif

private:
    void CopyNUISkeletonFrameToNativeFormat(const NUI_SKELETON_FRAME& sourceSkeleton, SKinSkeletonFrame& sinkSkeleton);
    void PushRawNativeDataToListeners();

#ifdef KINECT_USE_HANDLES
    void UpdateScreenHands(const NUI_SKELETON_FRAME& source, SKinSkeletonFrame& sink, NUI_HANDLES_ARMS* arms, CONST NUI_IMAGE_FRAME* pDepthFrame320x240, CONST NUI_IMAGE_FRAME* pDepthFrame80x60);
#endif

    void UpdateGrips();
    void UpdateRails(float fTimeLastGetNextFrame);
    void PushRailData();

#ifdef KINECT_USE_SPEECH
    void OnSpeechEvent(NUI_SPEECH_EVENT event);
#endif

    HRESULT RenderSkeletons(FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor);
    HRESULT RenderSingleSkeleton(DWORD dwSkeletonIndex, FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor);

    void DebugDraw();
    void DebugDrawPlaySpace(float originX, float originY, float scale);

private:

    //Update the current closest skeleton
    void UpdateClosestSkeleton();

    //Check to see if skeleton positions have moved and inform listeners
    void CheckSkeletonPositions();

#ifdef KINECT_XBOX_CONNECT // ACCEPTED_USE
    void SetupXboxKinectThread();
#endif

#ifdef KINECT_USE_HANDLES
    NUI_HANDLES_ARMS m_nuiArms[KIN_SKELETON_COUNT];
#endif
    HANDLE m_hDepth320x240;
    HANDLE m_hDepth80x60;
    HANDLE m_hImage;
    //SYNC_THREADS;
    const NUI_IMAGE_FRAME* m_pDepthFrame320x240;
    const NUI_IMAGE_FRAME* m_pDepthFrame80x60;
    const NUI_IMAGE_FRAME* m_pImageFrame;

    HRESULT m_hrImage;
    HRESULT m_hr320x240;
    HRESULT m_hr80x60;

    HANDLE m_hFrameEndEvent;

    NUI_SKELETON_FRAME m_nuiSkeletonFrame;
    SKinSkeletonFrame m_skeletonFrame;

    Vec4 m_lastSkeletonPosition;

    //Filters for skeleton data
    ISkeletonFilter* m_skeletonFilter[KIN_SKELETON_COUNT];

    typedef CListenerSet<IKinectInputListener*> TKinectEventListeners;
    TKinectEventListeners m_eventListeners;

#ifdef KINECT_USE_HANDLES
    typedef std::map<int, SKinGripInternal> TKINGrips;

    TKINGrips m_arcGrips;
    TKINGrips m_timeGrips;
#endif

#ifdef KINECT_XBOX_CONNECT // ACCEPTED_USE
    CKinectXboxSyncThread* m_pXboxKinectSyncThread;
#endif

    bool    m_enabled;
    bool    m_bUseSeatedST;

    int m_iDepth320x240;
    int m_iImage;

    uint32 m_closestSkeletonIndex;

#ifdef KINECT_USE_IDENTITY
    KinIdentifyCallback m_callbackFunc;
    void* m_pCallbackData;
#endif

#ifdef KINECT_USE_SPEECH
    NUI_SPEECH_GRAMMAR m_Grammar;
#endif
};

#endif

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

#include <EMotionFX/Source/DebugDraw.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(DebugDraw, Integration::EMotionFXAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(DebugDraw::ActorInstanceData, Integration::EMotionFXAllocator, 0)

    void DebugDraw::ActorInstanceData::Clear()
    {
        Lock();
        ClearLines();
        Unlock();
    }

    void DebugDraw::ActorInstanceData::Lock()
    {
        m_mutex.lock();
    }

    void DebugDraw::ActorInstanceData::Unlock()
    {
        m_mutex.unlock();
    }

    void DebugDraw::ActorInstanceData::ClearLines()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_lines.clear();
    }

    //-------------------------------------------------------------------------

    void DebugDraw::Lock()
    { 
        m_mutex.lock();
    }

    void DebugDraw::Unlock()
    { 
        m_mutex.unlock();
    }

    void DebugDraw::Clear()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        for (auto& data : m_actorInstanceData)
        {
            data.second->Clear();
        }
    }

    DebugDraw::ActorInstanceData* DebugDraw::RegisterActorInstance(ActorInstance* actorInstance)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        AZ_Assert(m_actorInstanceData.find(actorInstance) == m_actorInstanceData.end(), "This actor instance has already been registered.");
        ActorInstanceData* newData = aznew ActorInstanceData(actorInstance);
        m_actorInstanceData.insert(AZStd::make_pair(actorInstance, newData));
        return newData;
    }

    void DebugDraw::UnregisterActorInstance(ActorInstance* actorInstance)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_actorInstanceData.erase(actorInstance);
    }

    DebugDraw::ActorInstanceData* DebugDraw::GetActorInstanceData(ActorInstance* actorInstance)
    {        
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        const auto it = m_actorInstanceData.find(actorInstance);
        if (it != m_actorInstanceData.end())
        {
            return it->second;
        }
        return RegisterActorInstance(actorInstance);
    }

    const DebugDraw::ActorInstanceDataSet& DebugDraw::GetActorInstanceData() const
    {
        return m_actorInstanceData;
    }

}   // namespace EMotionFX
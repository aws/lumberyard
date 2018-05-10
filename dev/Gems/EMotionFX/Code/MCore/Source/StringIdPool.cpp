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

// include the required headers
#include "StringIdPool.h"
#include "LogManager.h"

namespace MCore
{
    StringIdPool::StringIdPool()
    {
        Reserve(10000);
        GenerateIdForString("");
    }


    StringIdPool::~StringIdPool()
    {
        Clear();
    }


    void StringIdPool::Clear()
    {
        Lock();

        const size_t numStrings = mStrings.size();
        for (size_t i = 0; i < numStrings; ++i)
        {
            delete mStrings[i];
        }
        mStrings.clear();

        mStringToIndex.clear();
        Unlock();
    }


    AZ::u32 StringIdPool::GenerateIdForStringWithoutLock(const AZStd::string& objectName)
    {
        // Try to insert it, if we hit a collision, we have the element.
        auto iterator = mStringToIndex.emplace(objectName, static_cast<AZ::u32>(mStrings.size()));
        if (!iterator.second)
        {
            // could not insert, we have the element
            return iterator.first->second;
        }

        // Create the new string object and push it to the string list.
        AZStd::string* newString = new AZStd::string(objectName);
        mStrings.push_back(newString);

        // The string was already added to the hashmap
        return iterator.first->second;
    }


    AZ::u32 StringIdPool::GenerateIdForString(const AZStd::string& objectName)
    {
        Lock();
        const AZ::u32 result = GenerateIdForStringWithoutLock(objectName);
        Unlock();
        return result;
    }


    const AZStd::string& StringIdPool::GetName(uint32 id)
    {
        Lock();
        MCORE_ASSERT(id != MCORE_INVALIDINDEX32);
        const AZStd::string* stringAddress = mStrings[id];
        Unlock();
        return *stringAddress;
    }

    const AZStd::string& StringIdPool::GetStringById(AZ::u32 id)
    {
        Lock();
        MCORE_ASSERT(id != MCORE_INVALIDINDEX32);
        AZStd::string* stringAddress = mStrings[id];
        Unlock();
        return *stringAddress;
    }


    void StringIdPool::Reserve(size_t numStrings)
    {
        Lock();
        mStrings.reserve(numStrings);
        Unlock();
    }


    // Wait with execution until we can set the lock.
    void StringIdPool::Lock()
    {
        mMutex.Lock();
    }


    // Release the lock again.
    void StringIdPool::Unlock()
    {
        mMutex.Unlock();
    }


    void StringIdPool::Log(bool includeEntries)
    {
        AZ_Printf("EMotionFX", "StringIdPool: NumEntries=%d\n", mStrings.size());

        if (includeEntries)
        {
            const size_t numStrings = mStrings.size();
            for (size_t i = 0; i < numStrings; ++i)
            {
                AZ_Printf("EMotionFX", "   #%d: String='%s', Id=%d\n", i, mStrings[i]->c_str(), GenerateIdForString(mStrings[i]->c_str()));
            }
        }
    }
} // namespace MCore
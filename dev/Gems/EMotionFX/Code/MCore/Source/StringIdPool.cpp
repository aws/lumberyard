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
#include "StringIDGenerator.h"
#include "LogManager.h"

namespace MCore
{
    // constructor
    StringIDGenerator::StringIDGenerator()
    {
        mNames.SetMemoryCategory(MCORE_MEMCATEGORY_IDGENERATOR);
        Reserve(10000);
        mStringToIndex.Init(1000);
        GenerateIDForString("");
    }


    // destructor
    StringIDGenerator::~StringIDGenerator()
    {
        Clear();
    }


    // clear all contents
    void StringIDGenerator::Clear()
    {
        Lock();

        const uint32 numStrings = mNames.GetLength();
        for (uint32 i = 0; i < numStrings; ++i)
        {
            delete mNames[i];
        }

        mNames.Clear();
        mStringToIndex.Clear();

        Unlock();
    }


    // get a unique id for the given string
    uint32 StringIDGenerator::GenerateIDForStringWithoutLock(const char* objectName)
    {
        if ((objectName == nullptr || strlen(objectName) == 0) && mNames.GetLength() > 0)
        {
            return 0;
        }

        // try to locate it
        uint32 elementIndex;
        uint32 entryIndex;
        if (mStringToIndex.FindEntry(StringRef(objectName), &elementIndex, &entryIndex))
        {
            return mStringToIndex.GetEntry(elementIndex, entryIndex).GetValue();
        }

        String* newString = new String(objectName);
        mNames.Add(newString);
        const uint32 id = mNames.GetLength() - 1;

        // it doesn't exist, so add it
        mStringToIndex.Add(StringRef(newString->AsChar()), id);
        return id;

        /*  // check if the ID already exists
            uint32 equalIndex = MCORE_INVALIDINDEX32;

            const uint32 numStrings = mNames.GetLength();
            for (uint32 i=0; i<numStrings && (equalIndex==MCORE_INVALIDINDEX32); ++i)
            {
                if (mNames[i]->CheckIfIsEqual( objectName ))
                    equalIndex = i;
            }

            if (equalIndex != MCORE_INVALIDINDEX32)
                return equalIndex;

            mNames.Add( new String(objectName) );

            // return its ID, which is the index into the array
            return mNames.GetLength() - 1;
        */
    }


    // thread safe generate
    uint32 StringIDGenerator::GenerateIDForString(const char* objectName)
    {
        Lock();
        const uint32 result = GenerateIDForStringWithoutLock(objectName);
        Unlock();
        return result;
    }


    // get the name
    const String& StringIDGenerator::GetName(uint32 id)
    {
        Lock();
        MCORE_ASSERT(id != MCORE_INVALIDINDEX32);
        String* stringAddress = mNames[id];
        Unlock();
        return *stringAddress;
    }


    // pre-alloc space in the array
    void StringIDGenerator::Reserve(uint32 numNames)
    {
        Lock();
        mNames.Reserve(numNames);
        Unlock();
    }


    // wait with execution until we can set the lock
    void StringIDGenerator::Lock()
    {
        mMutex.Lock();
    }


    // release the lock again
    void StringIDGenerator::Unlock()
    {
        mMutex.Unlock();
    }


    void StringIDGenerator::Log(bool includeEntries)
    {
        const float average = mStringToIndex.CalcAverageNumEntries();
        const float loadBalance = mStringToIndex.CalcLoadBalance();
        LogInfo("MCore::StringIDGenerator() - Logging Stats:");
        LogInfo("   + TotalEntries   = %d", mStringToIndex.GetTotalNumEntries());
        LogInfo("   + NumRows        = %d", mStringToIndex.GetNumTableElements());
        LogInfo("   + Average        = %f", average);
        LogInfo("   + Load Balance   = %f", loadBalance);

        if (includeEntries)
        {
            for (uint32 i = 0; i < mStringToIndex.GetNumTableElements(); ++i)
            {
                LogInfo("   #%d = %d entries", i, mStringToIndex.GetNumEntries(i));
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AzStringIdGenerator::AzStringIdGenerator()
    {
        Reserve(10000);
        GenerateIdForString("");
    }


    AzStringIdGenerator::~AzStringIdGenerator()
    {
        Clear();
    }


    void AzStringIdGenerator::Clear()
    {
        Lock();

        const size_t numStrings = mStrings.size();
        for (size_t i = 0; i< numStrings; ++i)
        {
            delete mStrings[i];
        }
        mStrings.clear();

        mStringToIndex.clear();
        Unlock();
    }


    AZ::u32 AzStringIdGenerator::GenerateIdForStringWithoutLock(const char* objectName)
    {
        if ((!objectName || strlen(objectName) == 0) && mStrings.size() > 0)
        {
            return 0;
        }

        // Try to locate it.
        auto iterator = mStringToIndex.find(objectName);
        if (iterator != mStringToIndex.end())
        {
            return iterator->second;
        }

        // Create the new string object and push it to the string list.
        AZStd::string* newString = new AZStd::string(objectName);
        mStrings.push_back(newString);

        // The string doesn't exist yet, add it to the hash map.
        const AZ::u32 id = static_cast<AZ::u32>(mStrings.size() - 1);
        mStringToIndex.insert(AZStd::make_pair<AZStd::string, AZ::u32>(objectName, id));
        return id;
    }


    AZ::u32 AzStringIdGenerator::GenerateIdForString(const char* objectName)
    {
        Lock();
        const AZ::u32 result = GenerateIdForStringWithoutLock(objectName);
        Unlock();
        return result;
    }


    const AZStd::string& AzStringIdGenerator::GetStringById(AZ::u32 id)
    {
        Lock();
        MCORE_ASSERT(id != MCORE_INVALIDINDEX32);
        AZStd::string* stringAddress = mStrings[id];
        Unlock();
        return *stringAddress;
    }


    void AzStringIdGenerator::Reserve(size_t numStrings)
    {
        Lock();
        mStrings.reserve(numStrings);
        Unlock();
    }


    // Wait with execution until we can set the lock.
    void AzStringIdGenerator::Lock()
    {
        mMutex.Lock();
    }


    // Release the lock again.
    void AzStringIdGenerator::Unlock()
    {
        mMutex.Unlock();
    }


    void AzStringIdGenerator::Log(bool includeEntries)
    {
        AZ_Printf("EMotionFX", "AzStringIdGenerator: NumEntries=%d", mStrings.size());

        if (includeEntries)
        {
            const size_t numStrings = mStrings.size();
            for (size_t i = 0; i < numStrings; ++i)
            {
                AZ_Printf("EMotionFX", "   #%d: String='%s', Id=%d", i, mStrings[i]->c_str(), GenerateIdForString(mStrings[i]->c_str()));
            }
        }
    }
} // namespace MCore
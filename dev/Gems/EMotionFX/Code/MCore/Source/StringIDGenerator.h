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

#include "StandardHeaders.h"
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "MemoryManager.h"
#include "UnicodeString.h"
#include "MultiThreadManager.h"
#include "HashTable.h"


namespace MCore
{
    class MCORE_API StringIDGenerator
    {
        MCORE_MEMORYOBJECTCATEGORY(StringIDGenerator, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_IDGENERATOR);
        friend class Initializer;
        friend class MCoreSystem;

    public:
        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is not thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        uint32 GenerateIDForStringWithoutLock(const char* objectName);

        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        uint32 GenerateIDForString(const char* objectName);

        /**
         * Return the name of the given id.
         * @param id The unique id to search for the name.
         * @return The name of the given object.
         */
        const String& GetName(uint32 id);

        /**
         * Reserve space for a given amount of names.
         * @param numNames The number of names to reserve space for.
         */
        void Reserve(uint32 numNames);

        void Log(bool includeEntries = true);

        void Clear();
        void Lock();
        void Unlock();

    private:
        Array<String*>                  mNames;                 /**< String array which contains the names of the nodes. They are pointers as we store some pointers to the strings sometimes, and growing the array can change the addresses. */
        HashTable<StringRef, uint32>    mStringToIndex;         /**< The string to index table, where the index maps into mNames array and is directly the ID. */
        Mutex                           mMutex;                 /**< The multithread lock. */

        /**
         * Default constructor.
         */
        StringIDGenerator();

        /**
         * Destructor.
         */
        ~StringIDGenerator();
    };


    class MCORE_API AzStringIdGenerator
    {
        friend class Initializer;
        friend class MCoreSystem;

    public:
        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is not thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        AZ::u32 GenerateIdForStringWithoutLock(const char* objectName);

        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        AZ::u32 GenerateIdForString(const char* objectName);

        /**
         * Return the name of the given id.
         * @param id The unique id to search for the name.
         * @return The name of the given object.
         */
        const AZStd::string& GetStringById(AZ::u32 id);

        /**
         * Reserve space for a given amount of strings.
         * @param numStrings The number of strings to reserve space for.
         */
        void Reserve(size_t numStrings);

        void Log(bool includeEntries = true);

        void Clear();
        void Lock();
        void Unlock();

    private:
        class StringRef
        {
        public:
            MCORE_INLINE StringRef(const char* text)                    { mData = const_cast<char*>(text); }
            MCORE_INLINE const char* AsChar() const                     { return mData; }
            MCORE_INLINE uint32 GetLength() const                       { return (uint32)strlen(mData); }
            MCORE_INLINE bool operator==(const StringRef& other)        { return (strcmp(mData, other.mData) == 0); }

        private:
            char* mData;
        };

        AZStd::vector<AZStd::string*>                   mStrings;
        AZStd::unordered_map<AZStd::string, AZ::u32>    mStringToIndex;         /**< The string to index table, where the index maps into mNames array and is directly the ID. */
        Mutex                                           mMutex;                 /**< The multithread lock. */

        AzStringIdGenerator();
        ~AzStringIdGenerator();
    };
} // namespace MCore

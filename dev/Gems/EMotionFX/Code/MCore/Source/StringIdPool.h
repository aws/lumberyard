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
#include "MultiThreadManager.h"


namespace MCore
{
    class MCORE_API StringIdPool
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
        AZ::u32 GenerateIdForStringWithoutLock(const AZStd::string& objectName);

        /**
         * Generate a unique id for the given string which contains the object name.
         * This method is thread safe.
         * @param objectName The name of the node to generate an id for.
         * @return The unique id of the given object.
         */
        AZ::u32 GenerateIdForString(const AZStd::string& objectName);

        /**
        * Return the name of the given id.
        * @param id The unique id to search for the name.
        * @return The name of the given object.
        */
        const AZStd::string& GetName(uint32 id);

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

        AZStd::vector<AZStd::string*>                   mStrings;
        AZStd::unordered_map<AZStd::string, AZ::u32>    mStringToIndex;         /**< The string to index table, where the index maps into mNames array and is directly the ID. */
        Mutex                                           mMutex;                 /**< The multithread lock. */

        StringIdPool();
        ~StringIdPool();
    };
} // namespace MCore

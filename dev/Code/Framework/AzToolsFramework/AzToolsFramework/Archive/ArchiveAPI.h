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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Uuid.h>

namespace AzToolsFramework
{
    // use bind if you need additional context.
    typedef AZStd::function<void(bool success)> ArchiveResponseCallback;

    //! ArchiveCommands
    //! This bus handles messages relating to archive commands
    //! archive commands are ASYNCHRONOUS
    //! archive formats officially supported are .zip
    //! do not block the main thread waiting for a response, it is not okay
    //! you will not get a message delivered unless you tick the tickbus anyway!
    class ArchiveCommands
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<ArchiveCommands>;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        virtual ~ArchiveCommands() {}

        //! Start an async task to extract an archive to the target directory
        //! taskHandles are used to cancel a task at some point in the future and are provided by the caller per task.
        //! Multiple tasks can be associated with the same handle
        virtual void ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback) = 0;
    
        //! Cancels tasks associtated with the given handle. Blocks until all tasks are cancelled.
        virtual void CancelTasks(AZ::Uuid taskHandle) = 0;
    };
}; // namespace AzToolsFramework

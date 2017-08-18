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

// Description : This is the interface for the realtime remote update system
//               the main purpose for it is to allow users to enable or
//               disable it as needed


#ifndef CRYINCLUDE_CRYCOMMON_IREALTIMEREMOTEUPDATE_H
#define CRYINCLUDE_CRYCOMMON_IREALTIMEREMOTEUPDATE_H
#pragma once


struct IRealtimeUpdateGameHandler
{
    // <interfuscator:shuffle>
    virtual ~IRealtimeUpdateGameHandler(){}
    virtual bool UpdateGameData(XmlNodeRef oXmlNode, unsigned char* auchBinaryData) = 0;
    // </interfuscator:shuffle>
};

struct IRealtimeRemoteUpdate
{
    // <interfuscator:shuffle>
    virtual ~IRealtimeRemoteUpdate(){}
    // Description:
    //   Enables or disables the realtime remote update system.
    // See Also:
    // Arguments:
    //   boEnable: if true enable the realtime remote update system
    //                       if false, disables the realtime realtime remote
    //                          update system
    // Return:
    //   bool - true if the operation succeeded, false otherwise.

    virtual bool    Enable(bool boEnable = true) = 0;

    // Description:
    //   Checks if the realtime remote update system is enabled.
    // See Also:
    // Arguments:
    //  Nothing
    // Return:
    //   bool - true if the system is running, otherwise false.
    virtual bool    IsEnabled() = 0;

    // Description:
    //    Method allowing us to use a pass-through mechanism for testing.
    // Arguments:
    //   pBuffer: contains the received message.
    //   lenght:  contains the lenght of the received message.
    // Return:
    //  Nothing.
    virtual void    OnNotificationNetworkReceive(const void* pBuffer, size_t length) = 0;

    // Description:
    //      Method used to add a game specific handler to the live preview
    // Arguments:
    //      Interface to the game side handler that will be used for live preview
    // Return:
    //      Nothing.
    virtual void AddGameHandler(IRealtimeUpdateGameHandler* handler) = 0;

    // Description:
    //      Method used to remove the game specific handler from the live preview
    // Arguments:
    //      Interface to the game side handler that will be used for live preview
    // Return:
    //      Nothing.
    virtual void RemoveGameHandler(IRealtimeUpdateGameHandler* handler) = 0;

    // Description:
    //   Method used to check if the editor is currently syncing data to the
    // the current engine instance withing the timeout of 30 seconds.
    // Return Value:
    //   True if the editor is syncing. Otherwise, false.
    virtual bool IsSyncingWithEditor() = 0;

    // Description:
    //   Method used to do everything which was scheduled to be done in the
    // main thread.
    // Return Value:
    //   none
    virtual void Update() = 0;
    // </interfuscator:shuffle>
};




#endif // CRYINCLUDE_CRYCOMMON_IREALTIMEREMOTEUPDATE_H

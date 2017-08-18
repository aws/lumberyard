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

#ifndef CRYINCLUDE_CRYCOMMON_ILIVECREATECOMMON_H
#define CRYINCLUDE_CRYCOMMON_ILIVECREATECOMMON_H
#pragma once

namespace LiveCreate
{
    struct IHost;
    struct IHostInfo;
    struct IManager;
    struct IPlatformHandler;
    struct IPlatformHandlerFactory;

    enum ELogMessageType
    {
        eLogType_Normal,
        eLogType_Warning,
        eLogType_Error
    };

    struct IManagerListenerEx
    {
        // <interfuscator:shuffle>
        virtual ~IManagerListenerEx(){};

        // Manager was successfully connected to a remote host
        virtual void OnHostConnected(IHostInfo* pHostInfo){};

        // Manager was disconnected from a remote host
        virtual void OnHostDisconnected(IHostInfo* pHostInfo){};

        // Manager connection was confirmed and we are ready to send LiveCreate data
        virtual void OnHostReady(IHostInfo* pHost) {};

        // LiveCreate host is busy (probably loading the level)
        virtual void OnHostBusy(IHostInfo* pHost) {};

        // Manager wide sending status flag has changed
        virtual void OnSendingStatusChanged(bool bCanSend) {};

        // Internal message logging
        virtual void OnLogMessage(ELogMessageType aType, const char* pMessage){};

        // </interfuscator:shuffle>
    };

    //------------------------------------------------------------------------------------
}

#endif // CRYINCLUDE_CRYCOMMON_ILIVECREATECOMMON_H

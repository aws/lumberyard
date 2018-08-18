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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_NETWORKSTALLTICKER_H
#define CRYINCLUDE_CRYACTION_NETWORK_NETWORKSTALLTICKER_H
#pragma once


//--------------------------------------------------------------------------
// a special ticker thread to run during load and unload of levels

#ifdef USE_NETWORK_STALL_TICKER_THREAD


#include <ISystem.h>
#include <ICmdLine.h>

#include "IGameFramework.h"


class CNetworkStallTickerThread
    : public CrySimpleThread<>                              //in multiplayer mode
{
public:
    CNetworkStallTickerThread()
    {
        m_threadRunning = true;
    }

    virtual void Run();

    void FlagStop()
    {
        m_threadRunning = false;
    }

    void Cancel()
    {
    }

private:
    bool m_threadRunning;
};


#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD
//--------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYACTION_NETWORK_NETWORKSTALLTICKER_H

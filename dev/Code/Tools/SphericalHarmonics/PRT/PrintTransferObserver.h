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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_PRINTTRANSFEROBSERVER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_PRINTTRANSFEROBSERVER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)

#include <PRT/IObserver.h>

namespace NSH
{
    namespace NTransfer
    {
        struct IObservableTransfer;
    }

    //!< implements a simple observer which prints out the current state to std output
    class CPrintTransferObserver
        : public IObserverBase
    {
    public:
        CPrintTransferObserver(NTransfer::IObservableTransfer* pS);//!< constructor with -subject to observe- argument

        ~CPrintTransferObserver();                                              //!< detaches from subject

        virtual void Update(IObservable* pChangedSubject);  //!< update function called from subject

        virtual void Display(); //!< simple print function, output percentage of progress

        void ResetDisplayState();   //!< forces display to a complete output

    private:
        NTransfer::IObservableTransfer* m_pSubject;     //!< subject to observe
        float m_fLastProgress;                                              //!< last progress
        uint32 m_LastRunningThreads;                                    //!< last number of running theads
        bool m_FirstTime;                                                           //!< indicates whether something has been displayed already or not
        bool m_Progress;                                                            //!< indicates whether progress was outputted so far or not

        CPrintTransferObserver();   //!< standard constructor forbidden
    };
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_PRINTTRANSFEROBSERVER_H

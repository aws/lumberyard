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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "PrintTransferObserver.h"
#include "Transfer.h"

NSH::CPrintTransferObserver::CPrintTransferObserver(NTransfer::IObservableTransfer* pS)
    : m_pSubject(pS)
    , m_FirstTime(true)
    , m_fLastProgress(0.f)
    , m_LastRunningThreads(0)
{
    assert(pS);
    m_pSubject->Attach(this);
}

NSH::CPrintTransferObserver::~CPrintTransferObserver()
{
    m_pSubject->Detach(this);
}

void NSH::CPrintTransferObserver::Update(IObservable* pChangedSubject)
{
    if (pChangedSubject == m_pSubject)
    {
        Display();
    }
}

void NSH::CPrintTransferObserver::Display()
{
    NTransfer::STransferStatus state;
    m_pSubject->GetProcessingState(state);
    // display operation
    if (m_FirstTime)
    {
        printf("meshes to process: %d\n", state.totalMeshCount);
        printf("total vertices to process: %d\n", state.overallVertexCount);
        if (state.totalPassCount > 0)
        {
            printf("passes to process: %d\n", state.totalPassCount);
        }
        if (state.totalRCThreads > 1)
        {
            printf("ray casting threads running: %d of %d   progress: ray casting...", state.rcThreadsRunning, state.totalRCThreads);
            m_Progress = false;
        }
        else
        {
            m_Progress = true;
            printf("progress: 0.0 %%");
        }
        m_FirstTime = false;
    }
    float progress = 0.f;
    if (state.overallVertexIndex > 0)
    {
        progress = 100.f * (float)state.overallVertexIndex / (float)state.overallVertexCount;
        progress = (state.totalPassCount > 1) ? progress * (1.f / (float)state.totalPassCount) : progress; //scale by passes
        progress += (state.passIndex > 0) ? 100.f * (float)state.passIndex / (float)state.totalPassCount : 0.f;
    }

    if (state.overallVertexCount > 0)
    {
        //update threads running
        if (state.totalRCThreads > 1 && m_LastRunningThreads != state.rcThreadsRunning)
        {
            m_LastRunningThreads = state.rcThreadsRunning;
            if (m_Progress)
            {
                if (m_fLastProgress >= 9.95f)
                {
                    for (int i = 0; i < 25; ++i)
                    {
                        printf("%c", 8);                             // clear character and go back
                    }
                }
                else
                {
                    for (int i = 0; i < 24; ++i)
                    {
                        printf("%c", 8);                             // clear character and go back
                    }
                }
            }
            else
            {
                for (int i = 0; i < 33; ++i)
                {
                    printf("%c", 8);                             // clear character and go back
                }
            }
            printf("%d of %d", state.rcThreadsRunning, state.totalRCThreads);
            if (state.overallVertexIndex > 0)
            {
                printf("   progress: %2.1f %%", progress);
                if (!m_Progress)
                {
                    m_Progress = true;
                    printf("        ");
                    for (int i = 0; i < 8; ++i)
                    {
                        printf("%c", 8);                             // clear character and go back
                    }
                }
                m_fLastProgress = progress;
            }
            else
            {
                printf("   progress: ray casting...");
            }
        }
        else
        {
            //use escape sequence to output
            if (progress > m_fLastProgress + 0.09f)
            {
                if (!m_Progress)
                {
                    for (int i = 0; i < 14; ++i)
                    {
                        printf("%c", 8);                             // clear character and go back
                    }
                    printf("              ");
                    for (int i = 0; i < 14; ++i)
                    {
                        printf("%c", 8);                             // clear character and go back
                    }
                    m_Progress = true;
                }
                else
                {
                    if (m_fLastProgress >= 9.95f)
                    {
                        for (int i = 0; i < 6; ++i)
                        {
                            printf("%c", 8);                             // clear character and go back
                        }
                    }
                    else
                    {
                        for (int i = 0; i < 5; ++i)
                        {
                            printf("%c", 8);                             // clear character and go back
                        }
                    }
                }
                printf("%2.1f %%", progress);
                m_fLastProgress = progress;
            }
        }
    }
}

void NSH::CPrintTransferObserver::ResetDisplayState()
{
    m_FirstTime = true;
}

#endif
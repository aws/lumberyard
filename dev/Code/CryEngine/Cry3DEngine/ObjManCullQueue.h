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

// Description : Declaration and entry point for asynchronous obj-culling queue

#ifndef CRYINCLUDE_CRY3DENGINE_OBJMANCULLQUEUE_H
#define CRYINCLUDE_CRY3DENGINE_OBJMANCULLQUEUE_H
#pragma once

#include <platform.h>
#include <IEntityRenderState.h>
#include <Cry_Camera.h>

#include <Cry_Geo.h>

class CZBufferCuller;
class CCullBuffer;

namespace NCullQueue
{
    class CCullTask;
#ifdef USE_CULL_QUEUE
    enum
    {
        MAX_CULL_QUEUE_ITEM_COUNT = 4096
    };                                      //128 KB
#else
    enum
    {
        MAX_CULL_QUEUE_ITEM_COUNT = 1
    };
#endif

    struct SCullItem
    {
        AABB objBox;
        uint32 BufferID;
        OcclusionTestClient* pOcclTestVars;
    } _ALIGN(16);

    class SCullQueue
    {
    private:
        //State variables for asynchronous double-buffering
        volatile bool m_TestItemQueueReady;     //State variable for the queue of items to test. Filled from the main thread during scene parsing.
        volatile bool m_BufferAndCameraReady;   //State variable for the data to test the items in the queue against. Namely the coverage buffer (depth buffer) and a camera for projection which are set by the render thread.

        //Data for processing next time ProcessInternal is called
        uint32 m_MainFrameID;
        CZBufferCuller* m_pCullBuffer;
        const CCamera* m_pCam;



        volatile uint32 cullBufWriteHeadIndex;
        volatile uint32 cullBufReadHeadIndex;
        Vec3 sunDir;

        SCullItem cullItemBuf[MAX_CULL_QUEUE_ITEM_COUNT + 1] _ALIGN(16);

    public:
        void ProcessInternal(uint32 mainFrameID, CZBufferCuller* const pCullBuffer, const CCamera* const pCam);

        SCullQueue();
        ~SCullQueue();

        uint32 Size(){return cullBufWriteHeadIndex; }
        bool        HasItemsToProcess() { return cullBufReadHeadIndex < cullBufWriteHeadIndex; }
        //Routine to reset the coverage buffer state variables. Called when the renderer's buffers are swapped ready for a new frame's worth of work on the Render and Main threads.
        ILINE void ResetSignalVariables()
        {
            m_TestItemQueueReady = false;
            m_BufferAndCameraReady = false;
        }

        ILINE bool BufferAndCameraReady() {return m_BufferAndCameraReady; }

        ILINE void AddItem(const AABB& objBox, const Vec3& lightDir, OcclusionTestClient* pOcclTestVars, uint32 mainFrameID)
        {
            // Quick quiet-nan check
            assert(objBox.max.x == objBox.max.x);
            assert(objBox.max.y == objBox.max.y);
            assert(objBox.max.z == objBox.max.z);
            assert(objBox.min.x == objBox.min.x);
            assert(objBox.min.y == objBox.min.y);
            assert(objBox.min.z == objBox.min.z);

            if (cullBufWriteHeadIndex < MAX_CULL_QUEUE_ITEM_COUNT - 1)
            {
                SCullItem& RESTRICT_REFERENCE rItem = cullItemBuf[cullBufWriteHeadIndex];
                rItem.objBox                = objBox;
                sunDir                          =   lightDir;
                rItem.BufferID          = 14;//1<<1 1<<2 1<<3  shadows
                rItem.pOcclTestVars = pOcclTestVars;
                ++cullBufWriteHeadIndex;//store here to make it more likely SCullItem has been written
                return;
            }

            pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;//not enough space to hold item
        }

        ILINE void AddItem(const AABB& objBox, float fDistance, OcclusionTestClient* pOcclTestVars, uint32 mainFrameID)
        {
            assert(objBox.max.x == objBox.max.x);
            assert(objBox.max.y == objBox.max.y);
            assert(objBox.max.z == objBox.max.z);
            assert(objBox.min.x == objBox.min.x);
            assert(objBox.min.y == objBox.min.y);
            assert(objBox.min.z == objBox.min.z);

            if (cullBufWriteHeadIndex < MAX_CULL_QUEUE_ITEM_COUNT - 1)
            {
                SCullItem& RESTRICT_REFERENCE rItem = cullItemBuf[cullBufWriteHeadIndex];
                rItem.objBox                = objBox;
                rItem.BufferID          = 1;    //1<<0 zbuffer
                rItem.pOcclTestVars = pOcclTestVars;
                ++cullBufWriteHeadIndex;//store here to make it more likely SCullItem has been written
                return;
            }

            pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;//not enough space to hold item
        }
        //Sets a signal variable. If both signal variables are set then process() is called
        void FinishedFillingTestItemQueue();
        //Sets a signal variable and sets up the parameters for the next test. If both signal variables are set then process() is called
        void SetTestParams(uint32 mainFrameID, const CCullBuffer* pCullBuffer, const CCamera* pCam);

        void Process();
        void Wait();
    };
};


#endif // CRYINCLUDE_CRY3DENGINE_OBJMANCULLQUEUE_H

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

#ifndef CRYINCLUDE_CRYANIMATION_MEMORY_POOL_H
#define CRYINCLUDE_CRYANIMATION_MEMORY_POOL_H
#pragma once

namespace Memory {
    class CContext;

    //

    class CPool
    {
    private:
        friend class CContext;

    protected:
        CPool(CContext& context);
        virtual ~CPool();

    public:
        void Release() { delete this; }

        virtual void* Allocate(uint32 length) { return NULL; }
        virtual void Free(void* pAddress) { }

        virtual void Update() { }

        virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    private:
        CContext* m_pContext;
        CPool* m_pNext;
    };

    class CPoolFrameLocal
        : public CPool
    {
    private:
        class CBucket
        {
        public:
            static CBucket* Create(uint32 length);

        private:
            CBucket(uint32 length);
            ~CBucket();

        public:
            void Release() { delete this; }

            void Reset();
            void* Allocate(uint32 length);

            ILINE void GetMemoryUsage(ICrySizer* pSizer) const;

        private:
            CBucket(const CBucket&);
            CBucket& operator = (const CBucket&);

        private:
            uint8* m_buffer;
            uint32 m_bufferSize;
            uint32 m_length;
            uint32 m_used;
        };

    public:
        static CPoolFrameLocal* Create(CContext& context, uint32 bucketLength);

    private:
        CPoolFrameLocal(CContext& context, uint32 bucketLength);
        ~CPoolFrameLocal();

    public:
        void ReleaseBuckets();

        void Reset();

    private:
        CBucket* CreateBucket();

        // CPool
    public:
        virtual void* Allocate(uint32 length);

        virtual void Update();

        virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    private:
        uint32 m_bucketLength;
        std::vector<CBucket*> m_buckets;
        uint32 m_bucketCurrent;
    };

    ILINE void CPoolFrameLocal::CBucket::GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_buffer);
    }
} // namespace Memory

#endif // CRYINCLUDE_CRYANIMATION_MEMORY_POOL_H

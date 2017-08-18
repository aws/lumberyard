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

// Description : Static instance list. A utility class for automatic
//               registration of types data during static initialization.


#ifndef __STATICINSTANCELIST_H__
#define __STATICINSTANCELIST_H__

template <typename TYPE>
class CStaticInstanceList
{
public:

    inline CStaticInstanceList()
    {
        SInstanceList&  instanceList = GetInstanceList();

        m_pNextInstance                         = instanceList.pFirstInstance;
        instanceList.pFirstInstance = this;
    }

    virtual ~CStaticInstanceList() {}

    static inline TYPE* GetFirstInstance()
    {
        return static_cast<TYPE*>(GetInstanceList().pFirstInstance);
    }

    inline TYPE* GetNextInstance() const
    {
        return static_cast<TYPE*>(m_pNextInstance);
    }

private:

    struct SInstanceList
    {
        inline SInstanceList()
            : pFirstInstance(NULL)
        {}

        CStaticInstanceList*    pFirstInstance;
    };

    static inline SInstanceList&    GetInstanceList()
    {
        static SInstanceList    instanceList;
        return instanceList;
    }

    CStaticInstanceList*    m_pNextInstance;
};

#endif //__STATICINSTANCELIST_H__
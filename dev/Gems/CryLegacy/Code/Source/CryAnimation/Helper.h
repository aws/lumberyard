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

#ifndef CRYINCLUDE_CRYANIMATION_HELPER_H
#define CRYINCLUDE_CRYANIMATION_HELPER_H
#pragma once

namespace Helper {
    template <class T>
    T* FindFromSorted(T* pValues, uint32 count, const T& valueToFind)
    {
        T* pLast = &pValues[count];
        T* pValue = std::lower_bound(pValues, pLast, valueToFind);
        if (pValue == pLast || *pValue != valueToFind)
        {
            return NULL;
        }

        return pValue;
    }

    template <class T>
    T* FindFromSorted(std::vector<T>& values, const T& valueToFind)
    {
        return FindFromSorted(&values[0], values.size(), valueToFind);
    }
} // namespace Helper

#endif // CRYINCLUDE_CRYANIMATION_HELPER_H

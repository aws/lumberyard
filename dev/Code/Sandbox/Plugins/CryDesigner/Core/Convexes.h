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

#pragma once

namespace CD
{
    typedef std::vector<CD::SVertex> Convex;

    class Convexes
        : public CRefCountBase
    {
    public:

        void AddConvex(const CD::Convex& convex) { m_Convexes.push_back(convex); }
        int GetConvexCount() const { return m_Convexes.size(); }
        Convex& GetConvex(int nIndex) { return m_Convexes[nIndex]; }

    private:

        mutable std::vector<CD::Convex> m_Convexes;
    };
};
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
#include <cmath>

#include <qpoint.h>

namespace GraphCanvas
{
    class QtVectorMath
    {
    public:
        static QPointF Transpose(const QPointF& point)
        {
            return QPointF(point.y(), -point.x());
        }
        
        static float GetLength(const QPointF& point)
        {
            return std::sqrtf(point.x() * point.x() + point.y() * point.y());
        }
        
        static QPointF Normalize(const QPointF& point)
        {
            float length = GetLength(point);
            
            if (length > 0.0f)
            {
                return point/length;
            }
            else
            {
                return point;
            }
        }
    };
}
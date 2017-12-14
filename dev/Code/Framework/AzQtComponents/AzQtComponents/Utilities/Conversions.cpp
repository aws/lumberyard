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

#include <AzQtComponents/Utilities/Conversions.h>


namespace AzQtComponents
{
    QColor ToQColor(const AZ::Color& color)
    {
        QColor result;
        result.setRgbF(static_cast<float>(color.GetR()), static_cast<float>(color.GetG()), static_cast<float>(color.GetB()), static_cast<float>(color.GetA()));
        return result;
    }

    AZ::Color FromQColor(const QColor& color)
    {
        return AZ::Color(static_cast<float>(color.redF()), static_cast<float>(color.greenF()), static_cast<float>(color.blueF()), static_cast<float>(color.alphaF()));
    }
} // namespace AzQtComponents
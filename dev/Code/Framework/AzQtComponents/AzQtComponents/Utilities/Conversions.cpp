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
        return QColor::fromRgbF(static_cast<float>(color.GetR()), static_cast<float>(color.GetG()), static_cast<float>(color.GetB()), static_cast<float>(color.GetA()));
    }

    AZ::Color FromQColor(const QColor& color)
    {
        const QColor rgb = color.toRgb();
        return AZ::Color(static_cast<float>(rgb.redF()), static_cast<float>(rgb.greenF()), static_cast<float>(rgb.blueF()), static_cast<float>(rgb.alphaF()));
    }
} // namespace AzQtComponents

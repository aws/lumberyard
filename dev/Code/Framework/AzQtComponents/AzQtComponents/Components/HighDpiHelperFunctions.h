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
#pragma once

// This file specializes a few private high dpi functions that have become template functions
// after upstream HDPI/multiscreen related changes have been backported.

#include <qnamespace.h> // For AZ_QT_VERSION
#include <QtWidgets/private/qstylehelper_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

// Define AzQStyleHelper::dpiScaled(qreal, qreal) for Qt 5.12.4.0-az and wrap it for Qt 5.12.4.1-az
namespace AzQStyleHelper
{

inline qreal dpi(const QStyleOption* option)
{
    return QStyleHelper::dpi(option);
}

inline qreal dpiScaled(qreal value, qreal dpi)
{
    return QStyleHelper::dpiScaled(value, dpi);
}

}

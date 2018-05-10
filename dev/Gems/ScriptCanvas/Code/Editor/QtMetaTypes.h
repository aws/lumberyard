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

#include <QMetaType>

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableCore.h>

// VariableId is a UUID typedef for now. So we don't want to double reflect the UUID.
Q_DECLARE_METATYPE(AZ::Uuid);
Q_DECLARE_METATYPE(ScriptCanvas::Data::Type);
Q_DECLARE_METATYPE(ScriptCanvas::VariableId);
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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLSAVER_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLSAVER_H
#pragma once


struct IVehicleData;

XmlNodeRef VehicleDataSave(const char* definitionFile, IVehicleData* pData);
bool VehicleDataSave(const char* definitionFile, const char* dataFile, IVehicleData* pData);

// This Save method merges the vehicle data using the original source xml
// without losing data that is unknown to the vehicle definition
XmlNodeRef VehicleDataMergeAndSave(const char* originalXml, XmlNodeRef definition, IVehicleData* pData);

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLSAVER_H

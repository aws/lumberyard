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

#ifndef CRYINCLUDE_CRYINPUT_NATURALPOINTINPUTNULL_H
#define CRYINCLUDE_CRYINPUT_NATURALPOINTINPUTNULL_H
#pragma once

//////////////////////////////////////////////////////////////////////////
struct CNaturalPointInputNull
    : public INaturalPointInput
{
public:
    CNaturalPointInputNull(){};
    virtual ~CNaturalPointInputNull(){};

    virtual bool Init(){return true; }
    virtual void Update(){};
    virtual bool IsEnabled(){return true; }

    virtual void Recenter(){};

    // Summary;:
    //      Get raw skeleton data
    virtual bool GetNaturalPointData(NP_RawData& npRawData) const {return true; }
};

#endif // CRYINCLUDE_CRYINPUT_NATURALPOINTINPUTNULL_H

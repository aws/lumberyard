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

// Description : BMPHelper


#ifndef CRYINCLUDE_CRYACTION_PLAYERPROFILES_BMPHELPER_H
#define CRYINCLUDE_CRYACTION_PLAYERPROFILES_BMPHELPER_H
#pragma once

#include <AzCore/IO/FileIO.h>

namespace BMPHelper
{
    // load a BMP. if pByteData is 0, only reports dimensions
    // when pFile is given, restores read location after getting dimensions only
    bool LoadBMP(const char* filename, uint8* pByteData, int& width, int& height, int& depth, bool bForceInverseY = false);
    bool LoadBMP(AZ::IO::HandleType fileHandle, uint8* pByteData, int& width, int& height, int& depth, bool bForceInverseY = false);

    // save pByteData BGR[A] into a new file 'filename'. if bFlipY flips y.
    // if depth==4 assumes BGRA
    bool SaveBMP(const char* filename, uint8* pByteData, int width, int height, int depth, bool inverseY);
    // save pByteData BGR[A] into a file pFile. if bFlipY flips y.
    // if depth==4 assumes BGRA
    bool SaveBMP(AZ::IO::HandleType fileHandle, uint8* pByteData, int width, int height, int depth, bool inverseY);
    // calculate size of BMP incl. Header and padding bytes
    size_t CalcBMPSize(int width, int height, int depth);
};

#endif // CRYINCLUDE_CRYACTION_PLAYERPROFILES_BMPHELPER_H

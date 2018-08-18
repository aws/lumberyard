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

#ifndef CRYINCLUDE_CRYACTION_PLAYERPROFILES_RICHSAVEGAMETYPES_INFO_H
#define CRYINCLUDE_CRYACTION_PLAYERPROFILES_RICHSAVEGAMETYPES_INFO_H
#pragma once


#include "RichSaveGameTypes.h"

STRUCT_INFO_BEGIN(CryBitmapFileHeader)
STRUCT_VAR_INFO(bfType, TYPE_INFO(uint16))
STRUCT_VAR_INFO(bfSize, TYPE_INFO(uint32))
STRUCT_VAR_INFO(bfReserved1, TYPE_INFO(uint16))
STRUCT_VAR_INFO(bfReserved2, TYPE_INFO(uint16))
STRUCT_VAR_INFO(bfOffBits, TYPE_INFO(uint32))
STRUCT_INFO_END(CryBitmapFileHeader)

STRUCT_INFO_BEGIN(CryBitmapInfoHeader)
STRUCT_VAR_INFO(biSize, TYPE_INFO(uint32))
STRUCT_VAR_INFO(biWidth, TYPE_INFO(int32))
STRUCT_VAR_INFO(biHeight, TYPE_INFO(int32))
STRUCT_VAR_INFO(biPlanes, TYPE_INFO(uint16))
STRUCT_VAR_INFO(biBitCount, TYPE_INFO(uint16))
STRUCT_VAR_INFO(biCompression, TYPE_INFO(uint32))
STRUCT_VAR_INFO(biSizeImage, TYPE_INFO(uint32))
STRUCT_VAR_INFO(biXPelsPerMeter, TYPE_INFO(int32))
STRUCT_VAR_INFO(biYPelsPerMeter, TYPE_INFO(int32))
STRUCT_VAR_INFO(biClrUsed, TYPE_INFO(uint32))
STRUCT_VAR_INFO(biClrImportant, TYPE_INFO(uint32))
STRUCT_INFO_END(CryBitmapInfoHeader)

STRUCT_INFO_BEGIN(RichSaveGames::GUID)
STRUCT_VAR_INFO(Data1, TYPE_INFO(uint32))
STRUCT_VAR_INFO(Data2, TYPE_INFO(uint16))
STRUCT_VAR_INFO(Data3, TYPE_INFO(uint16))
STRUCT_VAR_INFO(Data4, TYPE_ARRAY(8, TYPE_INFO(unsigned char)))
STRUCT_INFO_END(RichSaveGames::GUID)

STRUCT_INFO_BEGIN(RichSaveGames::RICH_GAME_MEDIA_HEADER)
STRUCT_VAR_INFO(dwMagicNumber, TYPE_INFO(uint32))
STRUCT_VAR_INFO(dwHeaderVersion, TYPE_INFO(uint32))
STRUCT_VAR_INFO(dwHeaderSize, TYPE_INFO(uint32))
STRUCT_VAR_INFO(liThumbnailOffset, TYPE_INFO(int64))
STRUCT_VAR_INFO(dwThumbnailSize, TYPE_INFO(uint32))
STRUCT_VAR_INFO(guidGameId, TYPE_INFO(RichSaveGames::GUID))
STRUCT_VAR_INFO(szGameName, TYPE_ARRAY(RM_MAXLENGTH, TYPE_INFO(wchar_t)))
STRUCT_VAR_INFO(szSaveName, TYPE_ARRAY(RM_MAXLENGTH, TYPE_INFO(wchar_t)))
STRUCT_VAR_INFO(szLevelName, TYPE_ARRAY(RM_MAXLENGTH, TYPE_INFO(wchar_t)))
STRUCT_VAR_INFO(szComments, TYPE_ARRAY(RM_MAXLENGTH, TYPE_INFO(wchar_t)))
STRUCT_INFO_END(RichSaveGames::RICH_GAME_MEDIA_HEADER)


#endif // CRYINCLUDE_CRYACTION_PLAYERPROFILES_RICHSAVEGAMETYPES_INFO_H

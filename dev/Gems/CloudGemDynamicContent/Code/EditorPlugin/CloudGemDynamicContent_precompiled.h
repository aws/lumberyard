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

#include <AzCore/PlatformDef.h>

#define eCryModule eCryM_Game

#include <platform.h>
#include <CryName.h>
#include <I3DEngine.h>
#include <ISystem.h>
#include <ISerialize.h>
#include <IGem.h>
#include <EditorDefs.h>
#include <PythonWorker.h>
#include <QtViewPane.h>
#include <Util/PathUtil.h>
#include <IAWSResourceManager.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QList>
#include <QVariantList>
#include <QMainWindow>
#include <QScopedPointer>
#include <QAbstractTableModel>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QQueue>
#include <QMimeData>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>



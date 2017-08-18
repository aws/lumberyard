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

#ifndef CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H
#define CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H
#pragma once

/*! Implements exporting of all loaded resources to specified directory.
 *
 */
class CGameResourcesExporter
{
public:
    void ChooseDirectoryAndSave();
    void ChooseDirectory();
    void Save(const QString& outputDirectory);

    void GatherAllLoadedResources();
    void SetUsedResources(CUsedResources& resources);

    void SaveLayerResources(const QString& subDirectory, CObjectLayer* pLayer, bool bAllChilds);

    static void ExportSelectedLayerResources();
    static void ExportPerLayerResourceList();

private:
    static void GatherLayerResourceList_r(CObjectLayer* pLayer, CUsedResources& resources);

private:
    typedef QStringList Files;
    static Files m_files;

    QString m_path;

    //////////////////////////////////////////////////////////////////////////
    // Functions that gather files from editor subsystems.
    //////////////////////////////////////////////////////////////////////////
    void GetFilesFromObjects();
    void GetFilesFromVarBlock(CVarBlock* pVB);
    void GetFilesFromVariable(IVariable* pVar);
    void GetFilesFromMaterials();
    void GetFilesFromParticles();
};

#endif // CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H


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

#include "StdAfx.h"
#include "GameResourcesExporter.h"
#include "GameEngine.h"

#include "Objects/ObjectLayerManager.h"
#include "Objects/EntityObject.h"
#include "Material/MaterialManager.h"
#include "Particles/ParticleManager.h"

#include <QFileDialog>

//////////////////////////////////////////////////////////////////////////
// Static data.
//////////////////////////////////////////////////////////////////////////
CGameResourcesExporter::Files CGameResourcesExporter::m_files;

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ChooseDirectoryAndSave()
{
    ChooseDirectory();
    if (!m_path.isEmpty())
    {
        Save(m_path);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ChooseDirectory()
{
    m_path = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Choose Target (root/MasterCD) Folder"));
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GatherAllLoadedResources()
{
    m_files.clear();
    m_files.reserve(100000);        // count from GetResourceList, GetFilesFromObjects, GetFilesFromMaterials ...  is unknown

    IResourceList* pResList = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_Level);
    {
        for (const char* filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
        {
            m_files.push_back(filename);
        }
    }

    GetFilesFromObjects();
    GetFilesFromMaterials();
    GetFilesFromParticles();
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::SetUsedResources(CUsedResources& resources)
{
    for (CUsedResources::TResourceFiles::const_iterator it = resources.files.begin(); it != resources.files.end(); it++)
    {
        m_files.push_back(*it);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::Save(const QString& outputDirectory)
{
    CMemoryBlock data;

    int numFiles = m_files.size();

    CLogFile::WriteLine("===========================================================================");
    CLogFile::FormatLine("Exporting Level %s resources, %d files", GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data(), numFiles);
    CLogFile::WriteLine("===========================================================================");

    // Needed files.
    CWaitProgress wait("Exporting Resources");
    for (int i = 0; i < numFiles; i++)
    {
        QString srcFilename = m_files[i];
        if (!wait.Step((i * 100) / numFiles))
        {
            break;
        }
        wait.SetText(srcFilename.toUtf8().data());

        CLogFile::WriteLine(srcFilename.toUtf8().data());

        CCryFile file;
        if (file.Open(srcFilename.toUtf8().data(), "rb"))
        {
            // Save this file in target folder.
            QString trgFilename = Path::Make(outputDirectory, srcFilename);
            int fsize = file.GetLength();
            if (fsize > data.GetSize())
            {
                data.Allocate(fsize + 16);
            }
            // Read data.
            file.ReadRaw(data.GetBuffer(), fsize);

            // Save this data to target file.
            QString trgFileDir = Path::GetPath(trgFilename);
            CFileUtil::CreateDirectory(trgFileDir.toUtf8().data());
            // Create a file.
            FILE* trgFile = nullptr;
            azfopen(&trgFile, trgFilename.toUtf8().data(), "wb");
            if (trgFile)
            {
                // Save data to new file.
                fwrite(data.GetBuffer(), fsize, 1, trgFile);
                fclose(trgFile);
            }
        }
    }
    CLogFile::WriteLine("===========================================================================");
    m_files.clear();
}

#if defined(WIN64) || defined(APPLE)
template <class Container1, class Container2>
void Append(Container1& a, const Container2& b)
{
    a.reserve (a.size() + b.size());
    for (auto it = b.begin(); it != b.end(); ++it)
    {
        a.insert(a.end(), *it);
    }
}
#else
template <class Container1, class Container2>
void Append(Container1& a, const Container2& b)
{
    a.insert (a.end(), b.begin(), b.end());
}
#endif
//////////////////////////////////////////////////////////////////////////
//
// Go through all editor objects and gathers files from thier properties.
//
//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GetFilesFromObjects()
{
    CUsedResources rs;
    GetIEditor()->GetObjectManager()->GatherUsedResources(rs);

    Append(m_files, rs.files);
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GetFilesFromMaterials()
{
    CUsedResources rs;
    GetIEditor()->GetMaterialManager()->GatherUsedResources(rs);
    Append(m_files, rs.files);
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GetFilesFromParticles()
{
    CUsedResources rs;
    GetIEditor()->GetParticleManager()->GatherUsedResources(rs);
    Append(m_files, rs.files);
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ExportSelectedLayerResources()
{
    CObjectLayer* pSelLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
    if (pSelLayer)
    {
        CGameResourcesExporter exporter;
        exporter.ChooseDirectory();
        exporter.SaveLayerResources(pSelLayer->GetName(), pSelLayer, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GatherLayerResourceList_r(CObjectLayer* pLayer, CUsedResources& resources)
{
    GetIEditor()->GetObjectManager()->GatherUsedResources(resources, pLayer);

    for (int i = 0; i < pLayer->GetChildCount(); i++)
    {
        CObjectLayer* pChildLayer = pLayer->GetChild(i);
        GatherLayerResourceList_r(pChildLayer, resources);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::SaveLayerResources(const QString& subDirectory, CObjectLayer* pLayer, bool bAllChilds)
{
    if (m_path.isEmpty())
    {
        return;
    }

    CUsedResources resources;
    GetIEditor()->GetObjectManager()->GatherUsedResources(resources, pLayer);

    m_files.clear();
    SetUsedResources(resources);

    Save(Path::AddPathSlash(m_path) + subDirectory);

    if (bAllChilds)
    {
        for (int i = 0; i < pLayer->GetChildCount(); i++)
        {
            CObjectLayer* pChildLayer = pLayer->GetChild(i);
            SaveLayerResources(pChildLayer->GetName(), pChildLayer, bAllChilds);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ExportPerLayerResourceList()
{
    std::vector<CObjectLayer*> layers;
    GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);
    for (size_t i = 0; i < layers.size(); ++i)
    {
        CObjectLayer* pLayer = layers[i];

        // Only export topmost layers.
        if (pLayer->GetParent())
        {
            continue;
        }

        CUsedResources resources;
        GatherLayerResourceList_r(pLayer, resources);

        QString listFilename;
        listFilename = QStringLiteral("Layer_%1.txt").arg(pLayer->GetName());

        listFilename = Path::Make(GetIEditor()->GetLevelFolder(), listFilename);

        QStringList files;

        QString levelDir = GetIEditor()->GetLevelFolder();
        for (CUsedResources::TResourceFiles::const_iterator it = resources.files.begin(); it != resources.files.end(); it++)
        {
            QString filePath = Path::MakeGamePath(*it);
            files.push_back(filePath.toLower());
        }
        if (!files.empty())
        {
            FILE* file = nullptr;
            azfopen(&file, listFilename.toUtf8().data(), "wt");
            if (file)
            {
                for (size_t c = 0; c < files.size(); c++)
                {
                    fprintf(file, "%s\n", files[c].toUtf8().data());
                }
                fclose(file);
            }
        }
    }
}

REGISTER_EDITOR_COMMAND(CGameResourcesExporter::ExportSelectedLayerResources, editor, export_layer_resources, "", "");
REGISTER_EDITOR_COMMAND(CGameResourcesExporter::ExportPerLayerResourceList, editor, export_per_layer_resource_list, "", "");

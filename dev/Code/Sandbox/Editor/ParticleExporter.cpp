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
#include "ParticleExporter.h"

#include "Util/PakFile.h"

#include "Include/IEditorParticleManager.h"

//////////////////////////////////////////////////////////////////////////
// Signatures for particles.lst file.
#define PARTICLES_FILE_TYPE 2
#define PARTICLES_FILE_VERSION 4
#define PARTICLES_FILE_SIGNATURE "CRY"

#define PARTICLES_FILE "LevelParticles.xml"

#define EFFECTS_PAK_FILE "Effects.pak"

//////////////////////////////////////////////////////////////////////////
void CParticlesExporter::ExportParticles(const QString& path, const QString& levelName, CPakFile& levelPakFile)
{
    IEditorParticleManager* pPartManager = GetIEditor()->GetParticleManager();
    if (pPartManager->GetLibraryCount() == 0)
    {
        return;
    }

    // Effects pack.
    int i;
    for (i = 0; i < pPartManager->GetLibraryCount(); i++)
    {
        CBaseLibrary* pLib = (CBaseLibrary*)pPartManager->GetLibrary(i);
        if (pLib->IsLevelLibrary())
        {
            XmlNodeRef node = XmlHelpers::CreateXmlNode("ParticleLibrary");
            pLib->Serialize(node, false);
            XmlString xmlData = node->getXML();

            CCryMemFile file;
            file.Write(xmlData.c_str(), xmlData.length());
            QString filename = Path::Make(path, PARTICLES_FILE);
            levelPakFile.UpdateFile(filename.toUtf8().data(), file);
        }
    }

    /*
    ISystem *pISystem = GetIEditor()->GetSystem();

    // If have more then onle library save them into shared Effects.pak
    bool bNeedEffectsPak = pPartManager->GetLibraryCount() > 1;

    bool bEffectsPak = true;

    CString pakFilename = EFFECTS_PAK_FILE;
    CPakFile effectsPak;

    if (bNeedEffectsPak)
    {
        // Close main game pak file if open.
        if (!pISystem->GetIPak()->ClosePack( pakFilename ))
        {
            CLogFile::FormatLine( "Cannot close Pak file %s",(const char*)pakFilename );
            bEffectsPak = false;
        }

        //////////////////////////////////////////////////////////////////////////
        if (CFileUtil::OverwriteFile(pakFilename))
        {
            // Delete old pak file.
            if (!effectsPak.Open( pakFilename,false ))
            {
                CLogFile::FormatLine( "Cannot open Pak file for Writing %s",(const char*)pakFilename );
                bEffectsPak = false;
            }
        }
        else
            bEffectsPak = false;
    }

    // Effects pack.
    int i;
    for (i = 0; i < pPartManager->GetLibraryCount(); i++)
    {
        CParticleLibrary *pLib = (CParticleLibrary*)pPartManager->GetLibrary(i);
        if (pLib->IsLevelLibrary())
        {
            CCryMemFile file;
            ExportParticleLib( pLib,file );
            CString filename = Path::Make( path,PARTICLES_FILE );
            levelPakFile.UpdateFile( filename,file );
        }
        else
        {
            if (bEffectsPak)
            {
                CCryMemFile file;
                CString filename = Path::Make( EFFECTS_FOLDER,pLib->GetName() + ".prt" );
                ExportParticleLib( pLib,file );
                effectsPak.UpdateFile( filename,file );
            }
        }
    }

    // Open Pak, which was closed before.
    pISystem->GetIPak()->OpenPack( pakFilename );
    */
}
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
#include "AnimationSerializer.h"
#include "CryEditDoc.h"
#include "Mission.h"
#include "IMovieSystem.h"

#include "Util/PakFile.h"

CAnimationSerializer::CAnimationSerializer(void)
{
}

CAnimationSerializer::~CAnimationSerializer(void)
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimationSerializer::SaveSequence(IAnimSequence* seq, const char* szFilePath, bool bSaveEmpty)
{
    assert(seq != 0);
    XmlNodeRef sequenceNode = XmlHelpers::CreateXmlNode("Sequence");
    seq->Serialize(sequenceNode, false, false);
    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), sequenceNode, szFilePath);
}

IAnimSequence* CAnimationSerializer::LoadSequence(const char* szFilePath)
{
    IAnimSequence* seq = 0;
    XmlNodeRef sequenceNode = XmlHelpers::LoadXmlFromFile(szFilePath);
    if (sequenceNode)
    {
        seq = GetIEditor()->GetMovieSystem()->LoadSequence(sequenceNode);
    }
    return seq;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationSerializer::SaveAllLegacySequences(const char* szPath, CPakFile& pakFile)
{
    IMovieSystem* movSys = GetIEditor()->GetMovieSystem();
    XmlNodeRef movieNode = XmlHelpers::CreateXmlNode("MovieData");
    for (int i = 0; i < GetIEditor()->GetDocument()->GetMissionCount(); i++)
    {
        CMission* pMission = GetIEditor()->GetDocument()->GetMission(i);
        pMission->ExportLegacyAnimations(movieNode);
    }
    string sFilename = string(szPath) + "MovieData.xml";
    //XmlHelpers::SaveXmlNode(movieNode,sFilename.c_str());

    XmlString xml = movieNode->getXML();
    CCryMemFile file;
    file.Write(xml.c_str(), xml.length());
    pakFile.UpdateFile(sFilename.c_str(), file);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationSerializer::LoadAllSequences(const char* szPath)
{
    QString dir = Path::AddPathSlash(szPath);
    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(dir, "*.seq", files, false);

    for (int i = 0; i < files.size(); i++)
    {
        // Construct the full filepath of the current file
        LoadSequence((dir + files[i].filename).toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationSerializer::SerializeSequences(XmlNodeRef& xmlNode, bool bLoading)
{
    if (bLoading)
    {
        // Load.
        IMovieSystem* movSys = GetIEditor()->GetMovieSystem();
        movSys->RemoveAllSequences();
        int num = xmlNode->getChildCount();
        for (int i = 0; i < num; i++)
        {
            XmlNodeRef seqNode = xmlNode->getChild(i);
            movSys->LoadSequence(seqNode);
        }
    }
    else
    {
        // Save.
        IMovieSystem* movSys = GetIEditor()->GetMovieSystem();
        for (int i = 0; i < movSys->GetNumSequences(); ++i)
        {
            IAnimSequence* seq = movSys->GetSequence(i);
            XmlNodeRef seqNode = xmlNode->newChild("Sequence");
            seq->Serialize(seqNode, false);
        }
    }
}
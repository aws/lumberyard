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

#include "stdafx.h"
#include "XmlArchive.h"
#include "PakFile.h"

//////////////////////////////////////////////////////////////////////////
// CXmlArchive
bool CXmlArchive::Load(const QString& file)
{
    bLoading = true;

    QFile cFile(file);
    if (!cFile.open(QFile::ReadOnly))
    {
        CLogFile::FormatLine("Warning: Loading of %s failed", file.toLatin1().data());
        return false;
    }
    CArchive ar(&cFile, CArchive::load);

    QString str;
    ar >> str;

    try
    {
        pNamedData->Serialize(ar);
    }
    catch (...)
    {
        CLogFile::FormatLine("Error: Can't load xml file: '%s'! File corrupted. Binary file possibly was corrupted by Source Control if it was marked like text format.", file.toLatin1().data());
        return false;
    }

    root = XmlHelpers::LoadXmlFromBuffer(str.toLatin1().data(), str.toLatin1().length());
    if (!root)
    {
        CLogFile::FormatLine("Warning: Loading of %s failed", file.toLatin1().data());
    }

    if (root)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CXmlArchive::Save(const QString& file)
{
    QString filename(file);
    if (GetIEditor()->GetConsoleVar("ed_lowercasepaths"))
    {
        filename = filename.toLower();
    }

    bLoading = false;
    if (!root)
    {
        return;
    }

    QFile cFile(filename);
    // Open the file for writing, create it if needed
    if (!cFile.open(QFile::WriteOnly))
    {
        CLogFile::FormatLine("Warning: Saving of %s failed", filename.toLatin1().data());
        return;
    }
    // Create the archive object
    CArchive ar(&cFile, CArchive::store);

    _smart_ptr<IXmlStringData> pXmlStrData = root->getXMLData(5000000);

    // Need convert to QString for CArchive::operator<<
    QString str = pXmlStrData->GetString();
    ar << str;

    pNamedData->Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
bool CXmlArchive::SaveToPak(const QString& levelPath, CPakFile& pakFile)
{
    _smart_ptr<IXmlStringData> pXmlStrData = root->getXMLData(5000000);

    // Save xml file.
    QString xmlFilename = "Level.editor_xml";
    pakFile.UpdateFile(xmlFilename.toLatin1().data(), (void*)pXmlStrData->GetString(), pXmlStrData->GetStringLength());

    if (pakFile.GetArchive())
    {
        CLogFile::FormatLine("Saving pak file %s", (const char*)pakFile.GetArchive()->GetFullPath());
    }

    pNamedData->Save(pakFile);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlArchive::LoadFromPak(const QString& levelPath, CPakFile& pakFile)
{
    QString xmlFilename = levelPath + "Level.editor_xml";
    root = XmlHelpers::LoadXmlFromFile(xmlFilename.toLatin1().data());
    if (!root)
    {
        return false;
    }

    return pNamedData->Load(levelPath, pakFile);
}

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
#include "stdafx.h"

//Local
#include "ParticleLibraryAutoRecovery.h"

//Editor
#include <EditorDefs.h>
#include <IEditor.h>
#include <Include/IEditorParticleManager.h>

//QT
#include <QTimer>
#include <QSettings>
#include <QMessageBox>
#include <QDebug>
#include <QObject>

#define TIME_BETWEEN_BACKUPS 5 * 60 * 1000

ParticleLibraryAutoRecovery::ParticleLibraryAutoRecovery()
    : m_pLibraryManager(nullptr)
    , m_Timer(nullptr)
{
    CRY_ASSERT(GetIEditor());
    m_pLibraryManager = GetIEditor()->GetParticleManager();

    m_Timer = new QTimer();
    connect(m_Timer, &QTimer::timeout, this, [&](){ Save(); });
    ResetTimer();
}

ParticleLibraryAutoRecovery::~ParticleLibraryAutoRecovery()
{
    SAFE_DELETE(m_Timer);
}

bool ParticleLibraryAutoRecovery::AttemptRecovery()
{
    bool retval = false;
    if (HasExisting())
    {
        QStringList libraryNames = GetBackupNames();
        QString liblist;
        for (QString name : libraryNames)
        {
            liblist.append(name + "\n");
        }

        //Prompt user to recover modified libraries
        QMessageBox msg(QMessageBox::NoIcon, QObject::tr("Unsaved files found!"),
            QObject::tr("The following files were found and recovered:\n\n") + liblist + QObject::tr("\nWould you like to recover the libraries?"),
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Discard, nullptr);

        int response = msg.exec();
        if (response == QMessageBox::StandardButton::Yes)
        {
            Load();
            retval = true;
        }
        else if (response == QMessageBox::StandardButton::Discard)
        {
            Discard();
        }
    }

    return retval;
}

void ParticleLibraryAutoRecovery::Save()
{
    Discard(); //Discard the old

    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetSystem());
    CRY_ASSERT(m_pLibraryManager);

    QSettings settings("Amazon", "Lumberyard");
    QString group = "Recovery/";
    settings.beginGroup(group);
    QString level = GetIEditor()->GetLevelName();
    settings.beginWriteArray(level);
    int libCount = m_pLibraryManager->GetLibraryCount();
    int saveIndex = 0;
    for (int i = 0; i < libCount; i++)
    {
        IDataBaseLibrary* library = m_pLibraryManager->GetLibrary(i);
        if (library->IsModified())
        {
            XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("ParticleLibrary");
            QString file = library->GetFilename();
            library->Serialize(node, false);
            if (node && file.length() > 0)
            {
                qDebug() << "Auto saved" << file << "for recovery";
                settings.setArrayIndex(saveIndex++);
                settings.setValue("filepath", file);
                settings.setValue("xml", node->getXML().c_str());
            }
        }
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();

    ResetTimer();
}

void ParticleLibraryAutoRecovery::Load()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetSystem());
    CRY_ASSERT(m_pLibraryManager);

    QSettings settings("Amazon", "Lumberyard");
    QString group = "Recovery/";
    QString level = GetIEditor()->GetLevelName();
    settings.beginGroup(group);
    int size = settings.beginReadArray(level);
    int arrayIndex = 0;
    for (int i = 0; i < size; i++)
    {
        QString filepath;
        QString xml;
        settings.setArrayIndex(arrayIndex++);
        xml = settings.value("xml").toString();
        filepath = settings.value("filepath").toString();
        filepath.toLower();
        if (!(xml.length() <= 0 && filepath.length() <= 0))
        {
            XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromBuffer(xml.toStdString().c_str(), xml.length());
            if (node)
            {
                //AddLibrary adds a .xml to the end of the library path, this will remove the extra for compatibilty
                filepath.replace(m_pLibraryManager->GetLibsPath().toLower(), "");
                filepath.replace(".xml", "");
                IDataBaseLibrary* library = m_pLibraryManager->AddLibrary(filepath.toStdString().c_str(), false);
                library->Serialize(node, true);
            }
        }
    }
    settings.endArray();
    settings.endGroup();
    ResetTimer();
}

void ParticleLibraryAutoRecovery::Discard()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Recovery/";
    QString level = GetIEditor()->GetLevelName();
    settings.beginGroup(group);
    settings.beginGroup(level);
    settings.remove("");
    settings.endGroup();
    settings.endGroup();
    settings.sync();
}

QStringList ParticleLibraryAutoRecovery::GetBackupNames()
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetSystem());

    QSettings settings("Amazon", "Lumberyard");
    QString group = "Recovery/";
    QString level = GetIEditor()->GetLevelName();
    QStringList names;
    settings.beginGroup(group);
    int size = settings.beginReadArray(level);
    int arrayIndex = 0;
    for (int i = 0; i < size; i++)
    {
        QString filepath;
        QString xml;
        settings.setArrayIndex(arrayIndex++);
        xml = settings.value("xml").toString();
        filepath = settings.value("filepath").toString();
        if (!(xml.length() <= 0 && filepath.length() <= 0))
        {
            XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromBuffer(xml.toStdString().c_str(), xml.length());
            if (node)
            {
                //the recovered file is good, add to list of names
                names.append(filepath);
            }
        }
    }
    settings.endArray();
    settings.endGroup();
    return names;
}

bool ParticleLibraryAutoRecovery::HasExisting()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Recovery/";
    QString level = GetIEditor()->GetLevelName();
    settings.beginGroup(group);
    int size = settings.beginReadArray(level);
    settings.endArray();
    settings.endGroup();
    return size > 0; //there is something in recovery to recover
}

void ParticleLibraryAutoRecovery::ResetTimer()
{
    CRY_ASSERT(m_Timer);
    m_Timer->start(TIME_BETWEEN_BACKUPS);
}
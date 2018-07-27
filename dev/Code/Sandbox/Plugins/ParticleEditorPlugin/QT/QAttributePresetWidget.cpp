#include "stdafx.h"
#include "QAttributePresetWidget.h"
#include <qaction.h>
#include <QtXml/QDomDocument>
#include <QMenu>
#include <qsettings.h>

#include <QT/QAttributePresetWidget.moc>


QAttributePresetWidget::QAttributePresetWidget(QWidget* parent)
    : QWidget(parent)
{
    LoadSessionPresets();
    if (m_presetList.count() == 0)
    {
        BuildDefaultLibrary();
    }
    StoreSessionPresets();
}

QAttributePresetWidget::~QAttributePresetWidget()
{
    m_presetList.clear();
}

void QAttributePresetWidget::BuilPresetMenu(QMenu* menu)
{
    menu->addSeparator();
    for (auto preset : m_presetList)
    {
        QAction* action = menu->addAction(preset->name);
        action->connect(action, &QAction::triggered, this, [=]()
            {
                // Send Add CustomPanel
                emit SignalCustomPanel(preset->doc);
            });
    }
    menu->addSeparator();
}

void QAttributePresetWidget::StoreSessionPresets()
{
    QSettings settings("Amazon", "Lumberyard");

    settings.beginGroup("Attributes Panel Preset/");
    settings.remove("");
    settings.sync();
    for (SAttributePreset* preset : m_presetList)
    {
        settings.beginGroup(preset->name);
        settings.setValue("doc", preset->doc);
        settings.setValue("name", preset->name);
        settings.endGroup();
    }
    settings.endGroup();
}


bool QAttributePresetWidget::LoadSessionPresets()
{
    m_presetList.clear();
    QSettings settings("Amazon", "Lumberyard");

    settings.beginGroup("Attributes Panel Preset/"); //start of all libraries
    QStringList libraries = settings.childGroups();
    if (libraries.count() <= 0)
    {
        return false;
    }
    for (QString lib : libraries)
    {
        settings.beginGroup(lib);

        QString name = settings.value("name", "").toString();
        QString doc = settings.value("doc", "").toString();
        AddPreset(name, doc);
        settings.endGroup();
    }
    settings.endGroup(); //end of all libraries
    return true;
}

void QAttributePresetWidget::AddPreset(QString name, QString data)
{
    for (SAttributePreset* preset : m_presetList)
    {
        if (preset->name == name)
        {
            preset->doc = data;
            return;
        }
    }

    SAttributePreset* preset = new SAttributePreset();
    preset->name = name;
    preset->doc = data;
    m_presetList.push_back(preset);
}

// Create panel settings and add to presetList
void QAttributePresetWidget::BuildDefaultLibrary()
{
    while (m_presetList.count() > 0)
    {
        m_presetList.takeAt(0);
    }

    QString content = "<tree> <panel name=\"TestPanel\" custom = \"1\"> \
						<item as = \"Configuration.Config_Min\" name =\"Config Min\" />\
						<item as = \"Configuration.Config_Max\" name =\"Config Max\" />\
						<item as = \"Configuration.Platforms\" name =\"Platforms\" /></panel></tree>";

    AddPreset("TestPanel", content);
}
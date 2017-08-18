#pragma once

#include <QWidget>

class QMenu;
class QDomDocument;

struct SAttributePreset
{
    QString name;
    QString doc;
};

class QAttributePresetWidget
    : public QWidget
{
    Q_OBJECT
public:
    QAttributePresetWidget(QWidget* parent);
    ~QAttributePresetWidget();

    void BuilPresetMenu(QMenu*);
    void StoreSessionPresets();
    bool LoadSessionPresets();

    void AddPreset(QString name, QString data);

    void BuildDefaultLibrary();

signals:
    void SignalCustomPanel(QString);

private:

    QVector<SAttributePreset*> m_presetList;
};

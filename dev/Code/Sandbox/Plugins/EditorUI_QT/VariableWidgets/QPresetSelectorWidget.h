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
#ifndef QPresetSelectorWidget_h__
#define QPresetSelectorWidget_h__

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QBrush>
#include <QLabel>
#include <QComboBox>
#include "QAmazonLineEdit.h"
#include <QButtonGroup>
#include <QScrollArea>
#include <QScrollBar>
#include <QGroupBox>
#include <QMenu>
#include "QPresetWidget.h"
#include <Controls/QToolTipWidget.h>

class QSettings;
class QPresetSelectorWidget
    : public QWidget
{
    Q_OBJECT
public:
    QPresetSelectorWidget(QWidget* parent = 0);
    virtual ~QPresetSelectorWidget();

    void SaveOnClose();

protected:
    virtual QString DefaultLibraryName();
    virtual QWidget* CreateSelectedWidget() = 0;
    virtual void BuildPresetMenu();
    virtual void onValueChanged(unsigned int preset) = 0;
    virtual void StylePreset(QPresetWidget* widget, bool selected = false) = 0;
    virtual bool AddPresetToLibrary(QWidget* value, unsigned int lib, QString name = QString(), bool allowDefault = false, bool InsertAtTop = true){ return false; }
    virtual void AddActionCanceled();

    virtual void RemovePresetFromLibrary(int id, int lib);
    virtual int AddNewLibrary(QString name, int index = -1);
    virtual void RemoveLibrary(int id, int gotoLibId = 0);
    virtual int PresetAlreadyExistsInLibrary(QWidget* value, int lib) = 0;
    virtual void ConnectButtons();
    virtual void DisconnectButtons();
    virtual void BuildLibraryLayout(int lib);
    virtual void BuildPanelLayout(bool editable = false, QString _name = QString());
    virtual void BuildMainMenu();
    virtual void BuildLibSelectMenu();
    virtual void InitPanel();

    virtual bool LoadPreset(QString filepath) = 0;
    virtual bool SavePreset(QString filepath) = 0;
    void LoadDefaultPreset(int libId);
    void SaveDefaultPreset(int libId);
    void StoreSessionPresets();
    bool LoadSessionPresets();

    virtual bool eventFilter(QObject* obj, QEvent* event);

    virtual void BuildDefaultLibrary(){}
    virtual void showEvent(QShowEvent*) override;
    QString GetUniqueLibraryName(QString name);

    //settings should be in a group just before the library
    //i.e. presets/foo/bar would load with settings having
    //a current group of "presets/foo" and libName being "bar"
    virtual bool LoadSessionPresetFromName(QSettings* settings, const QString& libName) = 0;
    //the index used to access the preset
    virtual void SavePresetToSession(QSettings* settings, const int& lib, const int& preset) = 0;
    virtual void DisplayLibrary(int libId); //called whenever something is shown or hidden to fix display glitch
    virtual void resizeEvent(QResizeEvent*) override;
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual QString SettingsBaseGroup() const = 0;
    void PrepareExport(QString filepath);

    virtual void SelectPresetLibrary(int i);

public slots:
    //Common
    virtual void OnPresetClicked(int preset, bool overwriteSelection, bool deselect = false);
    virtual void OnAddPresetClicked();

    //Panel
    virtual void OnPanelNameClicked();          //only for non-editable Panel
    virtual void OnPanelSaveButtonClicked();    //only for Editable Panel

    // Presets and Library Menu
    virtual void OnPresetRightClicked(int i);

protected:
    enum PresetMenuFlag
    {
        MAIN_MENU_LIST_OPTIONS = 1 << 0,
        MAIN_MENU_SIZE_OPTIONS = 1 << 1,
        MAIN_MENU_REMOVE_ALL = 1 << 2,
    };

    enum PresetLayoutType
    {
        PRESET_LAYOUT_DEFAULT = 1 << 0,
        PRESET_LAYOUT_FIXED_COLUMN_2 = 1 << 1,
    };

    static const int DEFAULT_PRESET_INDEX = 0;

    PresetMenuFlag m_menuFlags;
    PresetLayoutType m_layoutType;

    //Delete Menu
    QMenu* presetMenu;

    //Layouts
    QGridLayout libraryLayout;
    QHBoxLayout panelLayout;
    QVBoxLayout layout; //used to house panel and selected preset layout

    QVector<QVector<QPresetWidget*> >presets;
    QVector<bool>                   newLibFlag;
    QVector<bool>                   unsavedFlag;
    QVector<bool>                   presetIsSelectedForDeletion;
    int                             lastPresetIndexSelected;
    QVector<QString>                libNames;
    QVector<QString>                libFilePaths;
    QLabel                          addPresetBackground;
    QWidget*                        addPresetForeground;
    QAmazonLineEdit                     addPresetName;
    QPushButton                     addPresetBtn;
    //used to remove interaction when user hits enter in Descriptor widget or otherwise

    //panel
    QFrame panelFrame;
    QLabel panelName;
    QAmazonLineEdit panelEditName;
    QPushButton panelMenuBtn;

    QMenu* m_menu;
    QMenu* libSelectMenu;
    QMenu* swabSizeMenu;

    //scrollbar
    QScrollArea* scrollArea;
    QWidget*     scrollBox;
    QWidget* m_sizeFix;

    //view types
    enum ViewType
    {
        GRID_VIEW,
        LIST_VIEW
    };

    struct presetSizeSettings
    {
        int width;
        int height;
        int numPerGridViewRow;
        presetSizeSettings(int _width, int _height, int _numPerGridViewRow)
            : width(_width)
            , height(_height)
            , numPerGridViewRow(_numPerGridViewRow)
        {
        }
        presetSizeSettings()
            : width(32)
            , height(32)
            , numPerGridViewRow(7)
        {
        }
    };
    enum presetSizeEnum
    {
        PRESET_SMALL = 0,
        PRESET_MEDIUM = 1,
        PRESET_LARGE = 2
    };

    ViewType viewType;
    presetSizeSettings presetSizes[3];
    presetSizeEnum currentPresetSize;
    QString lastDirectory;
    unsigned int m_currentLibrary;
    bool m_amCreatingLib;
    const unsigned int m_defaultLibraryId = 0;
    QToolTipWidget* m_tooltip;
};
#endif // QPresetSelectorWidget_h__

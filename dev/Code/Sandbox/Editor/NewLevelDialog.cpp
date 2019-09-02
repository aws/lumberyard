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
#include "NewLevelDialog.h"
#include <ui_NewLevelDialog.h>

// for constants
#include "NewTerrainDialog.h"

#include <QDir>
#include <QFileInfo>
#include <QtWidgets/QPushButton>

// Folder in which levels are stored
static const char kNewLevelDialog_LevelsFolder[] = "Levels";

// CNewLevelDialog dialog

CNewLevelDialog::CNewLevelDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ilevelFolders(0)
    , m_useTerrain(false)
    , m_terrainResolution(0)
    , m_terrainUnits(0)
    , m_bUpdate(false)
    , ui(new Ui::CNewLevelDialog)
    , m_initialized(false)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("New Level"));

    // Default level folder is root (Levels/)
    m_ilevelFolders = 0;

    // Default Terrain
    m_terrainType=0;

    //Default Map size
    m_terrainSizeX=10240;
    m_terrainSizeY=10240;
    m_terrainSizeZ=1024;

    // Default is 1024x1024
    m_terrainResolution = 3;

    m_bIsResize = false;

    // Level name only supports ASCII characters
    QRegExp rx("[_a-zA-Z0-9-]+");
    QValidator* validator = new QRegExpValidator(rx, this);
    ui->LEVEL->setValidator(validator);

    connect(ui->USE_TERRAIN, &QGroupBox::clicked, this, &CNewLevelDialog::OnBnClickedUseTerrain);
    connect(ui->TERRAIN_TYPE, SIGNAL(activated(int)), this, SLOT(OnCbnSelendokTerrainType()));
    connect(ui->TERRAIN_SIZE_X, SIGNAL(textChanged(QString &)), this, SLOT(OnTerrainSizeXChanged(QString &)));
    connect(ui->TERRAIN_RESOLUTION, SIGNAL(activated(int)), this, SLOT(OnCbnSelendokTerrainResolution()));
    connect(ui->TERRAIN_UNITS, SIGNAL(activated(int)), this, SLOT(OnCbnSelendokTerraniUnits()));
    connect(ui->LEVEL_FOLDERS, SIGNAL(activated(int)), this, SLOT(OnCbnSelendokLevelFolders()));
    connect(ui->LEVEL, &QLineEdit::textChanged, this, &CNewLevelDialog::OnLevelNameChange);

    // First of all, keyboard focus is related to widget tab order, and the default tab order is based on the order in which 
    // widgets are constructed. Therefore, creating more widgets changes the keyboard focus. That is why setFocus() is called last.
    // Secondly, using singleShot() allows setFocus() slot of the QLineEdit instance to be invoked right after the event system
    // is ready to do so. Therefore, it is better to use singleShot() than directly call setFocus().
    QTimer::singleShot(0, ui->LEVEL, SLOT(setFocus()));
}

CNewLevelDialog::~CNewLevelDialog()
{
}

void CNewLevelDialog::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        m_level = ui->LEVEL->text();
        m_levelFolders = ui->LEVEL_FOLDERS->currentText();
        //The checkbox is disabled during resize. Don't bother checking it. 
        if (!m_bIsResize)
        {
            m_useTerrain = ui->USE_TERRAIN->isChecked();
        }
        m_ilevelFolders = ui->LEVEL_FOLDERS->currentIndex();
        m_terrainType =ui->TERRAIN_TYPE->currentIndex();
        m_terrainSizeX=ui->TERRAIN_SIZE_X->text().toInt();
        m_terrainSizeY=ui->TERRAIN_SIZE_Y->text().toInt();
        m_terrainSizeZ=ui->TERRAIN_SIZE_Z->text().toInt();
        m_terrainResolution = ui->TERRAIN_RESOLUTION->currentIndex();
        m_terrainUnits = ui->TERRAIN_UNITS->currentIndex();
    }
    else
    {
        ui->LEVEL->setText(m_level);
        ui->LEVEL_FOLDERS->setCurrentText(m_levelFolders);
        ui->USE_TERRAIN->setChecked(m_useTerrain);
        ui->LEVEL_FOLDERS->setCurrentIndex(m_ilevelFolders);
        ui->TERRAIN_TYPE->setCurrentIndex(m_terrainType);
        ui->TERRAIN_SIZE_X->setText(QString::number(m_terrainSizeX));
        ui->TERRAIN_SIZE_Y->setText(QString::number(m_terrainSizeY));
        ui->TERRAIN_SIZE_Z->setText(QString::number(m_terrainSizeZ));
        ui->TERRAIN_RESOLUTION->setCurrentIndex(m_terrainResolution);
        ui->TERRAIN_UNITS->setCurrentIndex(m_terrainUnits);
    }
}

// CNewLevelDialog message handlers

void CNewLevelDialog::OnInitDialog()
{
    ReloadLevelFolders();

    m_useTerrain = true;

    // Inititialize terrain values.

    ui->TERRAIN_TYPE->addItem("Default");
    ui->TERRAIN_TYPE->addItem("Voxel");
    ui->TERRAIN_TYPE->setCurrentIndex(m_terrainType);

    int i;
    int resolution = Ui::START_TERRAIN_RESOLUTION;
    for (i = 0; i < 6; i++)
    {
        ui->TERRAIN_RESOLUTION->addItem(QString("%1x%1").arg(resolution));
        resolution *= 2;
    }

    UpdateTerrainUi();
    UpdateTerrainUnits();

    if (m_bIsResize)
    {
        setWindowTitle(tr("Resize Terrain"));

        // hide new-level dialog stuff
        ui->STATIC_GROUP1->hide();

        // resize window
        adjustSize();

        //If we're only a Resize Terrain dialog, hide confusing checkbox and text that says "Use Terrain"
        ui->USE_TERRAIN->setCheckable(false);
        ui->USE_TERRAIN->setTitle("");

        ShowWarning(tr("Warning: After terrain resize, undo queue will be lost!"));
    }
    else
    {
        // Disable OK until some text is entered
        if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
        {
            button->setEnabled(false);
        }
    }

    // Save data.
    UpdateData(false);
}


//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ReloadLevelFolders()
{
    QString levelsFolder = QString(Path::GetEditingGameDataFolder().c_str()) + "/" + kNewLevelDialog_LevelsFolder;

    m_itemFolders.clear();
    ui->LEVEL_FOLDERS->clear();
    ui->LEVEL_FOLDERS->addItem(QString(kNewLevelDialog_LevelsFolder) + '/');
    ReloadLevelFoldersRec(levelsFolder);
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ReloadLevelFoldersRec(const QString& currentFolder)
{
    QDir dir(currentFolder);

    QFileInfoList infoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &fi, infoList)
    {
        m_itemFolders.push_back(fi.baseName());
        ui->LEVEL_FOLDERS->addItem(QString(kNewLevelDialog_LevelsFolder) + '/' + fi.baseName());
    }
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::UpdateTerrainUi()
{
    bool default=false;
    bool voxel=false;

    if(m_useTerrain)
    {
        if(m_terrainType==1)//Voxel
            voxel=true;
        else //Default
            default=true;
    }

    ui->TERRAIN_RESOLUTION_LABEL->setVisible(default);
    ui->TERRAIN_RESOLUTION->setVisible(default);

    ui->TERRAIN_UNITS->setVisible(m_useTerrain);
    ui->STATIC4->setVisible(m_useTerrain);

    ui->TERRAIN_SIZE_LABEL->setVisible(voxel);
    ui->TERRAIN_SIZE_X_LABEL->setVisible(voxel);
    ui->TERRAIN_SIZE_X->setVisible(voxel);
    ui->TERRAIN_SIZE_Y_LABEL->setVisible(voxel);
    ui->TERRAIN_SIZE_Y->setVisible(voxel);
    ui->TERRAIN_SIZE_Z_LABEL->setVisible(voxel);
    ui->TERRAIN_SIZE_Z->setVisible(voxel);
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::UpdateTerrainUnits()
{
    uint32 terrainRes = GetTerrainResolution();
    int size = terrainRes * GetTerrainUnits();
    int maxUnit = IntegerLog2(Ui::MAXIMUM_TERRAIN_RESOLUTION / terrainRes);
    int units = Ui::START_TERRAIN_UNITS;

    ui->TERRAIN_UNITS->clear();
    for (int i = 0; i <= maxUnit; i++)
    {
        ui->TERRAIN_UNITS->addItem(QString::number(units));
        units *= 2;
    }
    if (size > Ui::MAXIMUM_TERRAIN_RESOLUTION)
    {
        m_terrainUnits = 0;
    }
    ui->TERRAIN_UNITS->setCurrentText(QString::number(m_terrainUnits));
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::UpdateTerrainInfo()
{
    int sizeX = 0, sizeY = 0, sizeZ = 0;

    if (m_useTerrain)
    {
        if(m_terrainType==1)
        {
            sizeX=m_terrainSizeX;
            sizeY=m_terrainSizeY;
            sizeZ=m_terrainSizeZ;
        }
        else
        {
            int size=GetTerrainResolution() * GetTerrainUnits();

            sizeX=sizeY=size;
            size>Ui::MAXIMUM_TERRAIN_RESOLUTION?ShowWarning(tr("4096x4096 maximum recommended")):ShowWarning("");
        }
    }
    else
    {
        ShowWarning("");
    }

    QString str;


    if((sizeX >= 1000) || (sizeY>=1000) || (sizeZ>=1000))
    {
        if(m_terrainType==1)
        {
            str=tr("Terrain Size: %1 x %2 x %3 Kilometers").arg((float)sizeX/1000.0f, 0, 'f', 3)
                .arg((float)sizeY/1000.0f, 0, 'f', 3)
                .arg((float)sizeZ/1000.0f, 0, 'f', 3);
        }
        else
        {
            str=tr("Terrain Size: %1 x %2 Kilometers").arg((float)sizeX/1000.0f, 0, 'f', 3)
                .arg((float)sizeY/1000.0f, 0, 'f', 3);
        }
    }
    else if (sizeX > 0)
    {
        str = tr("Terrain Size: %1 x %2 Meters").arg(sizeX).arg(sizeY);
    }
    else
    {
        str = tr("Level will have no terrain");
    }

    ui->TERRAIN_INFO->setText(str);
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ShowWarning(const QString& message)
{
    ui->STATIC_WARNING->setVisible(!message.isEmpty());
    ui->STATIC_WARNING->setText(message);
}

//////////////////////////////////////////////////////////////////////////
QString CNewLevelDialog::GetLevel() const
{
    QString output = m_level;

    if (m_itemFolders.size() > 0 && m_ilevelFolders > 0)
    {
        output = m_itemFolders[m_ilevelFolders - 1] + "/" + m_level;
    }

    return output;
}

//////////////////////////////////////////////////////////////////////////
bool CNewLevelDialog::IsUseTerrain() const
{
    return m_useTerrain != false;
}

//////////////////////////////////////////////////////////////////////////
int CNewLevelDialog::GetTerrainType() const
{
    return m_terrainType;
}

//////////////////////////////////////////////////////////////////////////
int CNewLevelDialog::GetTerrainSize(size_t index) const
{
    if(m_terrainType==1)
    {
        switch(index)
        {
        case 0:
        default:
            return m_terrainSizeX;
            break;
        case 1:
            return m_terrainSizeY;
            break;
        case 2:
            return m_terrainSizeZ;
            break;
        }
    }
    else
    {
        switch(index)
        {
        case 0:
        case 1:
        default:
            return GetTerrainResolution();
            break;
        case 2:
            return 0;
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CNewLevelDialog::GetTerrainResolution() const
{
    return Ui::START_TERRAIN_RESOLUTION * (1 << m_terrainResolution);
}

int CNewLevelDialog::GetTerrainUnits() const
{
    return Ui::START_TERRAIN_UNITS * (1 << m_terrainUnits);
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnBnClickedUseTerrain()
{
    UpdateData();
    UpdateTerrainUi();
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnCbnSelendokTerrainType()
{
    UpdateData();
    UpdateTerrainUi();
    UpdateTerrainInfo();
}

void CNewLevelDialog::OnTerrainSizeXChanged(QString &value)
{
    UpdateData();
    UpdateTerrainInfo();
}

void CNewLevelDialog::OnTerrainSizeYChanged(QString &value)
{
    UpdateData();
    UpdateTerrainInfo();
}

void CNewLevelDialog::OnTerrainSizeZChanged(QString &value)
{
    UpdateData();
    UpdateTerrainInfo();
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnCbnSelendokTerrainResolution()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    /*
        if ((m_terrainResolution+(m_terrainUnits+POWER_OFFSET))>MAXIMUM_TERRAIN_POWER_OF_TWO)
        {
            m_terrainUnits=POWER_OFFSET-m_terrainResolution;
            m_cTerrainUnits.SetCurSel(m_terrainUnits);
        }
    */
    UpdateTerrainUnits();

    UpdateTerrainInfo();
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnCbnSelendokTerraniUnits()
{
    // TODO: Add your control notification handler code here
    UpdateData();
    /*
        if ((m_terrainResolution+(m_terrainUnits+POWER_OFFSET))>MAXIMUM_TERRAIN_POWER_OF_TWO)
        {
            m_terrainResolution=POWER_OFFSET-m_terrainUnits;
            m_cTerrainResolution.SetCurSel(m_terrainResolution);
        }
    */
    UpdateTerrainInfo();
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnCbnSelendokLevelFolders()
{
    UpdateData();
}

void CNewLevelDialog::OnLevelNameChange()
{
    m_level = ui->LEVEL->text();

    // QRegExpValidator means the string will always be valid as long as it's not empty:
    const bool valid = !m_level.isEmpty();

    // Use the validity to dynamically change the Ok button's enabled state
    if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        button->setEnabled(valid);
    }
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::SetTerrainResolution(int res)
{
    int i = 0;
    int dim = res / Ui::START_TERRAIN_RESOLUTION;
    for (i = 0; i < 32; i++)
    {
        if ((dim >> i) == 1)
        {
            m_terrainResolution = i;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::SetTerrainUnits(int units)
{
    int i = 0;
    int dim = units / Ui::START_TERRAIN_UNITS;
    for (i = 0; i < 32; i++)
    {
        if ((dim >> i) == 1)
        {
            m_terrainUnits = i;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::IsResize(bool bIsResize)
{
    m_bIsResize = bIsResize;
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }
    QDialog::showEvent(event);
}

#include <NewLevelDialog.moc>

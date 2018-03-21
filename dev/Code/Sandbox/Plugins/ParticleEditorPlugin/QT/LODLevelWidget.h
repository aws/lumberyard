#pragma once

#include <QWidget>
#include <QDockwidget>
#include <QTreewidget>

class QCheckBox;
class QPushButton;
class QDoubleSpinBox;
class QLabel;
struct SLodInfo;
class QHBoxLayout;
class QVBoxLayout;
class QTreeWidget;
struct IParticleEffect;
class CLibraryTreeViewItem;
class CParticleItem;

class LODTreeWidget
    : public QTreeWidget
{
public:
    LODTreeWidget(QWidget* parent = 0);
    int GetContentHeight();

protected:
    virtual void mousePressEvent(QMouseEvent* e);
};

class LODLevelWidget
    : public QDockWidget
{
    Q_OBJECT
public:
    LODLevelWidget(QWidget* parent);

    void Init(SLodInfo* lod);
    ~LODLevelWidget();

    void ClearSelectionColor();

    void SetSelected(const QString& itemName);
    void SelectTopItem();

    void LODStateChanged(int state);

    void updateTreeItemStyle(bool ishighlight, SLodInfo* lod, QString fullname);

    void RefreshDistance();

    SLodInfo* GetLod();

signals:
    void RemoveLod(LODLevelWidget* widget, SLodInfo* lod);
    void SignalLodParticleItemSelected(LODLevelWidget* lodLevel, IParticleEffect* baseParticle, SLodInfo* lod);
    void SignalSelectionChanged(LODLevelWidget* lodLevel, QString itemName, float distance);
    void SignalRefreshGUI();
    void SignalDistanceChanged();
    void SignalUpdateLODIcon(CLibraryTreeViewItem* item);
    void SignalAttributeViewItemDeleted(const QString& name);
    void OnContentChanged();

private slots:
    void onRemoveLod();
    void onActiveChanged(bool active);
    void onDistanceChanged(double distance);
    void onContentSizeChanged();
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    void BuildTreeGUI();
    void UpdateItemModified(CLibraryTreeViewItem* parent, CParticleItem* particle);
    void OnItemContextMenu(const QPoint& pos);
    void BuildTreeItem(CLibraryTreeViewItem* parent, IParticleEffect* particle);
    void RefreshGUIDetour();
    int GetWidgetHeight();

    static CLibraryTreeViewItem* FindTreeItem(CLibraryTreeViewItem* root, const QString& name);

private:
    SLodInfo* m_Lod;

    QVBoxLayout* m_Layout;

    //Title widgets
    QWidget*        m_TitleWidget;
    QHBoxLayout*    m_TitleLayout;
    QCheckBox*      m_TitleActive;
    QLabel*         m_TitleLabel;
    QDoubleSpinBox* m_TitleDistanceBox;
    QPushButton*    m_TitleCrossButton;

    //Body
    LODTreeWidget* m_LodTree;

    bool m_SignalSelectedGuard;
};

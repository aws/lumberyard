#include "mainwidget.h"
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzQtComponents/Components/ToolButtonLineEdit.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/LumberyardStylesheet.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include "MyCombo.h"

#include <QPushButton>
#include <QDebug>
#include <QApplication>
#include <QStyleFactory>
#include <QMainWindow>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSettings>

using namespace AzQtComponents;

QToolBar * toolBar()
{
    auto t = new QToolBar(QString());
    QAction *a = nullptr;
    QMenu *menu = nullptr;
    auto but = new QToolButton();
    menu = new QMenu();
    but->setMenu(menu);
    but->setCheckable(true);
    but->setPopupMode(QToolButton::MenuButtonPopup);
    but->setDefaultAction(new QAction(QStringLiteral("Test"), but));
    but->setIcon(QIcon(":/stylesheet/img/undo.png"));
    t->addWidget(but);

    a = t->addAction(QIcon(":/stylesheet/img/select.png"), QString());
    a->setCheckable(true);
    t->addSeparator();

    auto tbcb = new AzQtComponents::ToolButtonComboBox();
    tbcb->setIcon(QIcon(":/stylesheet/img/angle.png"));
    tbcb->comboBox()->addItem("1");
    tbcb->comboBox()->addItem("2");
    tbcb->comboBox()->addItem("3");
    tbcb->comboBox()->addItem("4");
    t->addWidget(tbcb);

    auto tble = new AzQtComponents::ToolButtonLineEdit();
    tble->setFixedWidth(130);
    tble->setIcon(QIcon(":/stylesheet/img/16x16/Search.png"));
    t->addWidget(tble);

    auto combo = new QComboBox();
    combo->addItem("Test1");
    combo->addItem("Test3");
    combo->addItem("Selection,Area");
    t->addWidget(combo);

    const QStringList iconNames = {
        "Align_object_to_surface",
        "Align_to_grid",
        "Align_to_Object",
        "Angle",
        "Asset_Browser",
        "Audio",
        "Database_view",
        "Delete_named_selection",
        "Flowgraph",
        "Follow_terrain",
        "Freeze",
        "Gepetto",
        "Get_physics_state",
        "Grid",
        "layer_editor",
        "layers",
        "LIghting",
        "lineedit-clear",
        "LOD_generator",
        "LUA",
        "Mannequin",
        "Material",
        "Maximize",
        "Measure",
        "Move",
        "Object_follow_terrain",
        "Object_list",
        "Redo",
        "Reset_physics_state",
        "Scale",
        "select_object",
        "Select",
        "Select_terrain",
        "Simulate_Physics_on_selected_objects",
        "Substance",
        "Terrain",
        "Terrain_Texture",
        "Texture_browser",
        "Time_of_Day",
        "Trackview",
        "Translate",
        "UI_editor",
        "undo",
        "Unfreeze_all",
        "Vertex_snapping" };

    for (auto name : iconNames) {
        auto a = t->addAction(QIcon(QString(":/stylesheet/img/16x16/%1.png").arg(name)), nullptr);
        a->setToolTip(name);
        a->setDisabled(name == "Audio");
    }

    combo = new MyComboBox();
    combo->addItem("Test1");
    combo->addItem("Test3");
    combo->addItem("Selection,Area");
    t->addWidget(combo);

    return t;
}

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_settings("org", "app")
    {
        setAttribute(Qt::WA_DeleteOnClose);
        QVariant geoV = m_settings.value("geo");
        if (geoV.isValid()) {
            restoreGeometry(geoV.toByteArray());
        }
    }

    ~MainWindow()
    {
        qDebug() << "Deleted mainwindow";
        auto geo = saveGeometry();
        m_settings.setValue("geo", geo);
    }
private:
    QSettings m_settings;
};

static void addMenu(QMainWindow *w, const QString &name)
{
    auto action = w->menuBar()->addAction(name);
    auto menu = new QMenu();
    action->setMenu(menu);
    menu->addAction("Item 1");
    menu->addAction("Item 2");
    menu->addAction("Item 3");
    menu->addAction("Longer item 4");
    menu->addAction("Longer item 5");
}

int main(int argv, char **argc)
{
    AzQtComponents::PrepareQtPaths();

    QApplication app(argv, argc);
    app.setStyle(new AzQtComponents::EditorProxyStyle(QStyleFactory::create("Fusion")));

    AzQtComponents::LumberyardStylesheet stylesheet(&app);
    stylesheet.Refresh(&app);

    //qApp->setStyleSheet("file:///:/stylesheet/style.qss");

    auto wrapper = new AzQtComponents::WindowDecorationWrapper();
    QMainWindow *w = new MainWindow();
    w->setWindowTitle("Component Gallery");
    auto widget = new MainWidget(w);
    w->resize(550, 900);
    w->setMinimumWidth(500);
    wrapper->setGuest(w);
    wrapper->enableSaveRestoreGeometry("app", "org", "windowGeometry");
    w->setCentralWidget(widget);
    w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
    w->show();

    auto action = w->menuBar()->addAction("Menu 1");

    auto fileMenu = new QMenu();
    action->setMenu(fileMenu);
    auto openDock = fileMenu->addAction("Open dockwidget");
    QObject::connect(openDock, &QAction::triggered, [&w] {
        auto dock = new AzQtComponents::StyledDockWidget(QLatin1String("Amazon Lumberyard"), w);
        auto button = new QPushButton("Click to dock");
        auto wid = new QWidget();
        auto widLayout = new QVBoxLayout(wid);
        widLayout->addWidget(button);
        wid->resize(300, 200);
        QObject::connect(button, &QPushButton::clicked, [dock, &w]{
            dock->setFloating(!dock->isFloating());
        });
        w->addDockWidget(Qt::BottomDockWidgetArea, dock);
        dock->setWidget(wid);
        dock->resize(300, 200);
        dock->setFloating(true);
        dock->show();
    });


    addMenu(w, "Menu 2");
    addMenu(w, "Menu 3");
    addMenu(w, "Menu 4");

    w->addToolBar(toolBar());

    QObject::connect(widget, &MainWidget::reloadCSS, [] {
        qDebug() << "Reloading CSS";
        qApp->setStyleSheet(QString());
        qApp->setStyleSheet("file:///style.qss");
    });

    return app.exec();
}

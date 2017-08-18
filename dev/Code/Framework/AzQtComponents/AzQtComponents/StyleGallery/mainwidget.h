#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void initializeControls();
    void initializeItemViews();

signals:
    void reloadCSS();

private:
    Ui::MainWidget *ui;
};

#endif // MAINWIDGET_H

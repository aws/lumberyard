#ifndef MY_COMBO_H
#define MY_COMBO_H

#include <QComboBox>

class MyComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit MyComboBox(QWidget *parent = nullptr);
};


#endif

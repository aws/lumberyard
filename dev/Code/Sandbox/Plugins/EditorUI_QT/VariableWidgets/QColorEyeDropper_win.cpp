#include <QApplication>
#include <QCursor>

void setOverrideCursor(const QCursor& cursor)
{
    QApplication::setOverrideCursor(cursor);
}

void changeOverrideCursor(const QCursor& cursor)
{
    QApplication::changeOverrideCursor(cursor);
}

void restoreOverrideCursor()
{
    QApplication::restoreOverrideCursor();
}

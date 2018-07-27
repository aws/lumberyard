#import <Cocoa/Cocoa.h>

#include <QApplication>
#include <QCursor>
#include <QPixmap>

#include <QtMac>

// borrowed from qcocoacursor.mm
NSCursor *createCursorFromPixmap(const QPixmap pixmap, const QPoint hotspot)
{
    NSPoint hotSpot = NSMakePoint(hotspot.x(), hotspot.y());
    NSImage *nsimage;
    if (pixmap.devicePixelRatio() > 1.0) {
        QSize layoutSize = pixmap.size() / pixmap.devicePixelRatio();
        QPixmap scaledPixmap = pixmap.scaled(layoutSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        nsimage = QtMac::toNSImage(scaledPixmap);
        CGImageRef cgImage = QtMac::toCGImageRef(pixmap);
        NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        [nsimage addRepresentation:imageRep];
        [imageRep release];
        CGImageRelease(cgImage);
    } else {
        nsimage = QtMac::toNSImage(pixmap);
    }

    NSCursor *nsCursor = [[NSCursor alloc] initWithImage:nsimage hotSpot: hotSpot];
    [nsimage release];
    return nsCursor;
}

void setOverrideCursor(const QCursor& cursor)
{
    // this is only used with blank cursor by QColorEyeDropper
    Q_ASSERT(cursor.shape() == Qt::BlankCursor);
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    [createCursorFromPixmap(pm, QPoint()) set];
}

void changeOverrideCursor(const QCursor& cursor)
{
    [createCursorFromPixmap(cursor.pixmap(), cursor.hotSpot()) set];
}

void restoreOverrideCursor()
{
    [[NSCursor arrowCursor] set];
}

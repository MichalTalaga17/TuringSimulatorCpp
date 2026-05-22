#include <QApplication>
#include <QStyleFactory>

#include "ui.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication app(argc, argv);

    TuringSimulatorUI window;
    window.show();

    return QApplication::exec();
}

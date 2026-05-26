#include <QApplication>

#include "ui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("TuringSimulatorCpp");
    QApplication::setApplicationName("TuringSimulatorCpp");

    TuringSimulatorUI window;
    window.show();

    return QApplication::exec();
}

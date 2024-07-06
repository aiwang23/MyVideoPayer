#include <QApplication>


#include "mainwindow.h"

// #undef main

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    Window::MainWindow w;
    w.show();

    return application.exec();
}

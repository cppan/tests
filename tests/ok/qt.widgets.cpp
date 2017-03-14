/*
executable_type: win32
dependencies:
    - pvt.cppan.demo.qtproject.qt.base.widgets: 5
    - pvt.cppan.demo.qtproject.qt.base.winmain: 5
    - pvt.cppan.demo.qtproject.qt.base.plugins.platforms.windows: 5
*/

#include <QtWidgets>

int main(int argv, char *args[])
{
    QApplication app(argv, args);

    QPushButton hello( "Hello world!", 0 );
    hello.resize( 300, 90 );

    hello.show();

    return app.exec();
}

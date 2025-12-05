#include <QApplication>
#include "spaceinvaders.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    SpaceInvaders game;
    game.showFullScreen();
    
    return app.exec();
}
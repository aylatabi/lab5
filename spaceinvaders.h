#ifndef SPACE_INVADERS_H
#define SPACE_INVADERS_H

#include <QWidget>
#include <QSocketNotifier>
#include "player.h"
#include "alien.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
class SpaceInvaders : public QWidget
{
    Q_OBJECT

public:
    SpaceInvaders(QWidget *parent = nullptr);
    ~SpaceInvaders();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onControllerInput();
private:

    std::atomic<int> stickValue{0}; 
    int controllerFd;
    QSocketNotifier *controllerNotifier;
    std::vector<Player> player;
    std::vector<Alien> alien;
    std::mutex platform_mtx;
    std::mutex alien_mtx;

    //Platform Thread 
    
    std::thread platformThread;
    std::atomic<int> position_offset{0};
    std::atomic<bool> platformThread_running{true};
    void platformThread_func();

    std::thread cannonThread;
    std::atomic<bool> a_button_pressed{false};
    std::atomic<bool> cannonThread_running{true};
    std::atomic<int> cannon_y_pos{235};
    void cannonThread_func();

    // *********
    int curr_platformPosition;
    std::thread displayThread;
    std::atomic<bool> displayThread_running{true};
    void displayThread_func();

    void drawAlienType_0(QPainter &painter, int position_x, int position_y);
    void drawAlienType_1(QPainter &painter, int position_x, int position_y);
    void drawAlienType_2(QPainter &painter, int position_x, int position_y);

};

#endif 
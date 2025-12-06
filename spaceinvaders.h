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
#include <random>

class SpaceInvaders : public QWidget
{
    Q_OBJECT

public:
    SpaceInvaders(QWidget *parent = nullptr);
    ~SpaceInvaders();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onControllerInput_player1();
    void onControllerInput_player2();
private:
    enum GameState 
    {   
        START,
        GAME,
        END,
    }; 
    
    enum PlayerMode
    {
        NONE,
        SINGLE_PLAYER,
        MULTI_PLAYER,
    };

    GameState current_state = START;
    PlayerMode player_mode = NONE;
    int player_number = 0;
    std::atomic<int> player1_stickValue{0}; 
    std::atomic<int> player2_stickValue{0}; 
    int controllerFd_player1;
    int controllerFd_player2;
    QSocketNotifier *controllerNotifier_player1;
    QSocketNotifier *controllerNotifier_player2;


    std::vector<Player> player;
    std::vector<std::vector<Alien>> alien;
    std::mutex platform_mtx;

    std::mutex alien_mtx;
    
    void drawStartScreen(QPainter &painter);
    
    // PLATFORM 
    std::thread platformThread;
    std::atomic<int> position_offset{0};
    std::atomic<bool> platformThread_running{true};
    void platformThread_func();
    
    int player1_pos;
    int player2_pos;
    
    std::thread cannonThread;
    std::atomic<bool> player1_a_button_pressed{false};
    std::atomic<bool> cannonThread_running{true};
    std::atomic<int> cannon_y_pos{235};
    void cannonThread_func();


    std::atomic<bool> player2_a_button_pressed{false};

    // *********
    int player1_curr_platformPosition;
    int player2_curr_platformPosition;
    void drawPlatforms(QPainter &painter, int player1_pos, int player2_pos);
    std::thread displayThread;
    std::atomic<bool> displayThread_running{true};
    void displayThread_func();

    std::thread attackThread;
    std::atomic<bool> isAttacking{false};
    std::atomic<int> attack_x_pos{0};
    std::atomic<int> attack_y_pos{0};
    std::atomic<bool> attackThread_running{true};
    void attackThread_func();

    void drawAlienType_0(QPainter &painter, int position_x, int position_y);
    void drawAlienType_1(QPainter &painter, int position_x, int position_y);
    void drawAlienType_2(QPainter &painter, int position_x, int position_y);
    bool hit_alien_row_1(int cannon_x_pos);
    bool hit_alien_row_2(int cannon_x_pos);
    bool hit_alien_row_3(int cannon_x_pos);
    bool hit_player(int attack_x_pos, int attack_y_pos, int player_x_pos);

    std::atomic<bool> player1_x_button_pressed{false};
    std::atomic<bool> player2_x_button_pressed{false};
    int player1_shield_time = 0;
    int player1_shield_flag = 0;
    void drawPlayerShield(QPainter &painter, int position_x);
};

#endif 
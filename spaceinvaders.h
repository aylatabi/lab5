#ifndef SPACE_INVADERS_H
#define SPACE_INVADERS_H

#include <QWidget>
#include <QSocketNotifier>
#include "player.h"
#include "alien.h"
#include <string>
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
    std::atomic<bool> platformThread_running{false};
    void platformThread_func();
    
    
    std::atomic<int> player1_cannon_display_x_flag{0};
    int player1_cannon_display_x_pos = 0;
    std::thread player1_cannonThread;
    std::atomic<bool> player1_a_button_pressed{false};
    std::atomic<bool> player1_cannonThread_running{false};
    std::atomic<int> player1_cannon_y_pos{235};
    void player1_cannonThread_func();

    std::atomic<int> player2_cannon_display_x_flag{0};
    int player2_cannon_display_x_pos = 0;
    std::thread player2_cannonThread;
    std::atomic<bool> player2_a_button_pressed{false};
    std::atomic<bool> player2_cannonThread_running{false};
    std::atomic<int> player2_cannon_y_pos{235};
    void player2_cannonThread_func();
    
  

    // *********
    int player1_curr_platformPosition;
    int player2_curr_platformPosition;
    void drawPlatforms(QPainter &painter, int curr_player1_pos, int curr_player2_pos, int curr_player1_health, int curr_player2_health);
    std::thread displayThread;
    std::atomic<bool> displayThread_running{true};
    void displayThread_func();

    std::thread attackThread;
    std::atomic<bool> isAttacking{false};
    std::atomic<int> attack_x_pos{0};
    std::atomic<int> attack_y_pos{0};
    std::atomic<bool> attackThread_running{false};
    void attackThread_func();

    std::thread attackThread2;
    std::atomic<bool> isAttacking2{false};
    std::atomic<int> attack2_x_pos{0};
    std::atomic<int> attack2_y_pos{0};
    std::atomic<bool> attackThread2_running{false};
    void attackThread2_func();

    void drawAlienType_0(QPainter &painter, int position_x, int position_y);
    void drawAlienType_1(QPainter &painter, int position_x, int position_y);
    void drawAlienType_2(QPainter &painter, int position_x, int position_y);
    void drawAlienLaser(QPainter &painter, int x, int y);
    bool hit_alien_row_1(int cannon_x_pos);
    bool hit_alien_row_2(int cannon_x_pos);
    bool hit_alien_row_3(int cannon_x_pos);
    bool hit_player(int attack_x_pos, int attack_y_pos, int player_x_pos);

    std::atomic<bool> player1_x_button_pressed{false};
    int player1_shield_time = 0;
    int player1_shield_flag = 0;
    void drawPlayerShield(QPainter &painter, int position_x);
    int player1_shield_cooldown_time = 0;
    bool player1_shield_cooldown_enabled = false;

    std::atomic<bool> player2_x_button_pressed{false};
   
    int player2_shield_time = 0;
    int player2_shield_flag = 0;
    int player2_shield_cooldown_time = 0;
    bool player2_shield_cooldown_enabled = false;

    // End screen
    int end_screen_selection = 0;
    bool player_won = false;
    void drawEndScreen(QPainter &painter);
    void resetGame();
    void stopGameThreads();
    void initializeAliens();
    bool checkAllAliensDead();
    bool checkAllPlayersDead();

    // Player destruction
    int player1_explosion_frame = 0;
    int player2_explosion_frame = 0;
    int player1_death_x = 0;
    int player2_death_x = 0;
    void drawExplosion(QPainter &painter, int center_x, int frame, bool isPlayer1);
    void drawDestroyedPlatform(QPainter &painter, int position_x, bool isPlayer1);

};

#endif 
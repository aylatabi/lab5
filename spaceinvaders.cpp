#include "spaceinvaders.h"
#include <QPainter>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>


#define XBOX_LEFT_STICK_X 0
#define XBOX_A_BUTTON     304
#define XBOX_X_BUTTON     307


SpaceInvaders::SpaceInvaders(QWidget *parent) : QWidget(parent)
{
    controllerFd_player1 = -1;
    controllerNotifier_player1 = nullptr;

    initializeAliens();

    player.emplace_back(Player("Player1", 30, 170));
   
    // for (size_t row = 0; row < alien.size(); row++)
    // {
    //     for (size_t column = 0; column < alien[row].size(); column++)
    //     {
    //         qDebug() << "Column " << column << " " << (alien[row][column].getPosition_x() + 8);
    //     }
    //     qDebug() << " ";
    // }
    setStyleSheet("background-color: black;");
   
    controllerFd_player1 = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
    if (controllerFd_player1 < 0) {
        qWarning() << "Could not open controller at /dev/input/event1";
        qWarning() << "Continuing without controller input...";
    } else {
        qDebug() << "Controller 1 opened successfully";
    
        controllerNotifier_player1 = new QSocketNotifier(controllerFd_player1, QSocketNotifier::Read, this);
        connect(controllerNotifier_player1, &QSocketNotifier::activated, this, &SpaceInvaders::onControllerInput_player1);
    }

   


    // 

    displayThread = std::thread(&SpaceInvaders::displayThread_func, this);

    // 

    // 


}

SpaceInvaders::~SpaceInvaders()
{
    if (controllerFd_player1 >= 0) {
        ::close(controllerFd_player1);
    }
    platformThread_running = false;
    displayThread_running = false;
    player1_cannonThread_running = false;
    for (int i = 0; i < NUM_ATTACKS; i++) {
        attackThread_running[i] = false;
    }
}

void SpaceInvaders::displayThread_func()
{
    while(displayThread_running)
    {
        if (current_state == GAME)
        {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                player1_curr_platformPosition = player[0].getPosition();
                if (player_mode == MULTI_PLAYER)
                {
                    player2_curr_platformPosition = player[1].getPosition();
                }
            }
        }
      
        
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void SpaceInvaders::platformThread_func()
{
    while(platformThread_running)
    {
        bool player1_alive;
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player1_alive = player[0].getHealth() > 0;
        }

        if (player1_alive)
        {
            int player1_moveSpeed = 0;
            if (player1_stickValue < -8000) {
                player1_moveSpeed = -4;  // Move left
            } else if (player1_stickValue > 8000) {
                player1_moveSpeed = 4;   // Move right
            }

            if (player1_moveSpeed != 0) {
                {
                    std::lock_guard<std::mutex> lock(platform_mtx);
                    player[0].movePlatform(player1_moveSpeed);
                }
            }
        }

        if (player_mode == MULTI_PLAYER)
        {
            bool player2_alive;
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                player2_alive = player[1].getHealth() > 0;
            }

            if (player2_alive)
            {
                int player2_moveSpeed = 0;
                if (player2_stickValue < -8000) {
                    player2_moveSpeed = -4;  // Move left
                } else if (player2_stickValue > 8000) {
                    player2_moveSpeed = 4;   // Move right
                }

                if (player2_moveSpeed != 0) {
                    {
                        std::lock_guard<std::mutex> lock(platform_mtx);
                        player[1].movePlatform(player2_moveSpeed);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void SpaceInvaders::attackThread_func(int index, int delayMin, int delayMax)
{
    int attack_pos_x = 0;
    int attack_pos_y = 0;
    int alien_type = -1;
    int num_increments = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib_column(0, 7);
    std::uniform_int_distribution<> distrib_row(0, 2);
    std::uniform_int_distribution<> distrib_delay(delayMin, delayMax);

    while(attackThread_running[index])
    {
        int delay = distrib_delay(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        int column = distrib_column(gen);
        int row = distrib_row(gen);

        if (row == 1) column = column % 6;
        if (row == 2) column = column % 5;

        {
            std::lock_guard<std::mutex> lock(alien_mtx);
            if (alien[row][column].isAlive())
            {
                attack_pos_x = alien[row][column].getPosition_x();
                attack_pos_y = alien[row][column].getPosition_y();
                alien_type = alien[row][column].getType();
            }
        }
        if (alien[row][column].isAlive())
        {
            if (alien_type == 1)
            {
                attack_x_pos[index] = attack_pos_x + 32;
            }
            else if (alien_type == 2)
            {
                attack_x_pos[index] = attack_pos_x + 24;
            }
            else if (alien_type == 3)
            {
                attack_x_pos[index] = attack_pos_x + 30;
            }

            attack_y_pos[index] = attack_pos_y + 32;
            if (row == 0)
            {
                num_increments = 24;
            }
            else if (row == 1)
            {
                num_increments = 18;
            }
            else if (row == 2)
            {
                num_increments = 12;
            }
            isAttacking[index] = true;
            for (int i = 0; i < num_increments; i++)
            {
                attack_y_pos[index] += 10;
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
            isAttacking[index] = false;
        }

        attack_pos_x = 0;
        attack_pos_y = 0;
    }
}

bool SpaceInvaders::hit_alien_row_3(int cannon_x_pos)
{
    int column = -1;
    bool attacked = false;
    if (58 <= cannon_x_pos && cannon_x_pos <= 106)
    {
        column = 0;
    }
    else if (126 <= cannon_x_pos && cannon_x_pos <= 158)
    {
        column = 1;
    }
    else if (178 <= cannon_x_pos && cannon_x_pos <= 226)
    {
        column = 2;
    }
    else if (246 <= cannon_x_pos && cannon_x_pos <= 290)
    {
        column = 3;
    }
    else if (310 <= cannon_x_pos && cannon_x_pos <= 358)
    {
        column = 4;
    }
    else if (378 <= cannon_x_pos && cannon_x_pos <= 410)
    {
        column = 5;
    }
    if (column > -1)
    {
        {
            std::lock_guard<std::mutex> lock(alien_mtx);
            if (alien[2][column].isAlive())
            {
                alien[2][column].damage();
                attacked = true;
            }   
        }
        
    }
    return attacked;
}

bool SpaceInvaders::hit_alien_row_2(int cannon_x_pos)
{
    int column = -1;
    bool attacked = false;
    if (28 <= cannon_x_pos && cannon_x_pos <= 72)
    {
        column = 0;
    }
    else if (92 <= cannon_x_pos && cannon_x_pos <= 140)
    {
        column = 1;
    }
    else if (160 <= cannon_x_pos && cannon_x_pos <= 204)
    {
        column = 2;
    }
    else if (224 <= cannon_x_pos && cannon_x_pos <= 256)
    {
        column = 3;
    }
    else if (276 <= cannon_x_pos && cannon_x_pos <= 320)
    {
        column = 4;
    }
    else if (340 <= cannon_x_pos && cannon_x_pos <= 388)
    {
        column = 5;
    }
    else if (408 <= cannon_x_pos && cannon_x_pos <= 452)
    {
        column = 6;
    }
    if (column > -1)
    {
        {
            std::lock_guard<std::mutex> lock(alien_mtx);
            if (alien[1][column].isAlive())
            {
                alien[1][column].damage();
                attacked = true;
            }   
        }
    }
    return attacked;
}

bool SpaceInvaders::hit_alien_row_1(int cannon_x_pos)
{
    int column = -1;
    bool attacked = false;
    if ((8 <= cannon_x_pos && cannon_x_pos <= 40))
    {
        column = 0;
    }
    else if (60 <= cannon_x_pos && cannon_x_pos <= 104)
    {
        column = 1;
    }
    else if (124 <= cannon_x_pos && cannon_x_pos <= 156)
    {
        column = 2;
    }
    else if (176 <= cannon_x_pos && cannon_x_pos <= 224)
    {
        column = 3; 
    }
    else if (244 <= cannon_x_pos && cannon_x_pos <= 276)
    {
        column = 4;
    }
    else if (296 <= cannon_x_pos && cannon_x_pos <= 340)
    {
        column = 5;
    }
    else if (360 <= cannon_x_pos && cannon_x_pos <= 392)
    {
        column = 6; 
    }
    else if (412 <= cannon_x_pos && cannon_x_pos <= 460)
    {
        column = 7; 
    }

    if (column > -1)
    {
        {
            std::lock_guard<std::mutex> lock(alien_mtx);
            if (alien[0][column].isAlive())
            {
                alien[0][column].damage();
                attacked = true;
            }   
        }
    }
    
    return attacked;
}

void SpaceInvaders::player2_cannonThread_func()
{
    int cannon_x_pos = 0;
    while(player2_cannonThread_running)
    {
        bool player2_alive;
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player2_alive = player[1].getHealth() > 0;
        }

        if (player2_a_button_pressed && player2_alive)
        {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                cannon_x_pos = player2_curr_platformPosition;

            }

            cannon_x_pos += 20;
            // qDebug() << "cannon x position is " << cannon_x_pos;
            for (int i = 0; i < 11; i+=1)
            {                               
                player2_cannon_y_pos -= 20;
               
                if ((4 <= player2_cannon_y_pos && player2_cannon_y_pos <= 36) && (8 <= cannon_x_pos && cannon_x_pos <= 460))
                {
                    if (hit_alien_row_1(cannon_x_pos)) break;
                }
                if ((56 <= player2_cannon_y_pos && player2_cannon_y_pos <= 88) && (28 <= cannon_x_pos && cannon_x_pos <= 452))
                {
                    if (hit_alien_row_2(cannon_x_pos)) break;        
                }
                if ((108 <= player2_cannon_y_pos && player2_cannon_y_pos <= 140) && (58 <= cannon_x_pos && cannon_x_pos <= 410))
                {
                    if (hit_alien_row_3(cannon_x_pos)) break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(48));
            }
            player2_a_button_pressed = false;
            player2_cannon_display_x_flag = 0;
            player2_cannon_y_pos = 235;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));

}

void SpaceInvaders::player1_cannonThread_func()
{
    int cannon_x_pos = 0;

    while(player1_cannonThread_running)
    {
        bool player1_alive;
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player1_alive = player[0].getHealth() > 0;
        }

        if (player1_a_button_pressed && player1_alive)
        {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                cannon_x_pos = player1_curr_platformPosition;

            }

            cannon_x_pos += 20;
            // qDebug() << "cannon x position is " << cannon_x_pos;
            for (int i = 0; i < 11; i+=1)
            {                               
                player1_cannon_y_pos -= 20;
               
                if ((4 <= player1_cannon_y_pos && player1_cannon_y_pos <= 36) && (8 <= cannon_x_pos && cannon_x_pos <= 460))
                {
                    if (hit_alien_row_1(cannon_x_pos)) break;
                }
                if ((56 <= player1_cannon_y_pos && player1_cannon_y_pos <= 88) && (28 <= cannon_x_pos && cannon_x_pos <= 452))
                {
                    if (hit_alien_row_2(cannon_x_pos)) break;        
                }
                if ((108 <= player1_cannon_y_pos && player1_cannon_y_pos <= 140) && (58 <= cannon_x_pos && cannon_x_pos <= 410))
                {
                    if (hit_alien_row_3(cannon_x_pos)) break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(48));
            }
            player1_a_button_pressed = false;
            player1_cannon_display_x_flag = 0;
            player1_cannon_y_pos = 235;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

bool hit_player(int attack_x_pos, int attack_y_pos, int player_x_pos)
{

    return false;
}

bool SpaceInvaders::checkAllAliensDead()
{
    std::lock_guard<std::mutex> lock(alien_mtx);
    for (size_t row = 0; row < alien.size(); row++)
    {
        for (size_t column = 0; column < alien[row].size(); column++)
        {
            if (alien[row][column].isAlive())
            {
                return false;
            }
        }
    }
    return true;
}

bool SpaceInvaders::checkAllPlayersDead()
{
    std::lock_guard<std::mutex> lock(platform_mtx);
    if (player_mode == SINGLE_PLAYER)
    {
        return player[0].getHealth() <= 0;
    }
    else if (player_mode == MULTI_PLAYER)
    {
        return player[0].getHealth() <= 0 && player[1].getHealth() <= 0;
    }
    return false;
}

void SpaceInvaders::initializeAliens()
{
    alien.clear();
    std::vector<std::vector<int>> alien_types = {
        {2, 0, 2, 1, 2, 0, 2, 1},
        {0, 1, 0, 2, 0, 1, 0},
        {1, 2, 1, 0, 1, 2},
    };

    std::vector<int> alien_widths = {44, 48, 32};
    int positionX = 0;
    int positionY = 0;
    int spacingX = 20;
    int spacingY = 52;
    for (size_t row = 0; row < alien_types.size(); row++)
    {
        if (row == 1) positionX = 20;
        else if (row == 2) positionX = 50;
        else positionX = 0;
        alien.push_back(std::vector<Alien>());
        for (size_t column = 0; column < alien_types[row].size(); column++)
        {
            alien[row].emplace_back(Alien(alien_types[row][column], true, 100, positionX, positionY));
            if (alien_types[row][column] == 0)
            {
                positionX += alien_widths[0] + spacingX;
            }
            else if (alien_types[row][column] == 1)
            {
                positionX += alien_widths[1] + spacingX;
            }
            else if (alien_types[row][column] == 2)
            {
                positionX += alien_widths[2] + spacingX;
            }
        }
        positionY += spacingY;
    }
}

void SpaceInvaders::stopGameThreads()
{
    platformThread_running = false;
    player1_cannonThread_running = false;
    player2_cannonThread_running = false;
    for (int i = 0; i < NUM_ATTACKS; i++) {
        attackThread_running[i] = false;
    }

    if (platformThread.joinable()) platformThread.join();
    if (player1_cannonThread.joinable()) player1_cannonThread.join();
    if (player2_cannonThread.joinable()) player2_cannonThread.join();
    for (int i = 0; i < NUM_ATTACKS; i++) {
        if (attackThread[i].joinable()) attackThread[i].join();
    }
}

void SpaceInvaders::resetGame()
{
    {
        std::lock_guard<std::mutex> lock(alien_mtx);
        initializeAliens();
    }

    {
        std::lock_guard<std::mutex> lock(platform_mtx);
        player[0].resetHealth();
        player[0].setPosition(170);
        if (player_mode == MULTI_PLAYER && player.size() > 1)
        {
            player[1].resetHealth();
            player[1].setPosition(270);
        }
    }

    player1_cannon_y_pos = 235;
    player1_cannon_display_x_flag = 0;
    player1_a_button_pressed = false;

    player2_cannon_y_pos = 235;
    player2_cannon_display_x_flag = 0;
    player2_a_button_pressed = false;

    player1_x_button_pressed = false;
    player1_shield_time = 0;
    player1_shield_flag = 0;
    player1_shield_cooldown_time = 0;
    player1_shield_cooldown_enabled = false;

    player2_x_button_pressed = false;
    player2_shield_time = 0;
    player2_shield_flag = 0;
    player2_shield_cooldown_time = 0;
    player2_shield_cooldown_enabled = false;

    for (int i = 0; i < NUM_ATTACKS; i++) {
        isAttacking[i] = false;
        attack_x_pos[i] = 0;
        attack_y_pos[i] = 0;
    }

    player1_explosion_frame = 0;
    player2_explosion_frame = 0;
    player1_death_x = 0;
    player2_death_x = 0;
    game_over_delay = 0;

    end_screen_selection = 0;
}

void SpaceInvaders::onControllerInput_player2()
{
    struct input_event ev;

    while (read(controllerFd_player2, &ev, sizeof(ev)) == sizeof(ev)) {

        if (ev.type == EV_KEY && ev.code == XBOX_A_BUTTON) {
            if (ev.value) {
               player2_a_button_pressed = true;      
            }
        }
        if (ev.type == EV_KEY && ev.code == XBOX_X_BUTTON) {
            if (ev.value) {
               player2_x_button_pressed = true;      
            }
        } 
        if (ev.type == EV_ABS && ev.code == XBOX_LEFT_STICK_X) {
            player2_stickValue = ev.value;
        }
    }
}

void SpaceInvaders::onControllerInput_player1()
{
    struct input_event ev;
    
    while (read(controllerFd_player1, &ev, sizeof(ev)) == sizeof(ev)) {

        if (ev.type == EV_KEY && ev.code == XBOX_A_BUTTON) {
            if (ev.value) {
                player1_a_button_pressed = true;      
            }
        }
        if (ev.type == EV_KEY && ev.code == XBOX_X_BUTTON) {
            if (ev.value) {
               player1_x_button_pressed = true;      
            }
        } 
        if (ev.type == EV_ABS && ev.code == XBOX_LEFT_STICK_X) {
            player1_stickValue = ev.value;
        }
    }
}

void SpaceInvaders::drawAlienType_0(QPainter &painter, int position_x, int position_y)
{

    painter.setBrush(QBrush(Qt::cyan));
    painter.setPen(Qt::NoPen);
    //left ear
    painter.drawRect(position_x + 16, position_y + 4, 4, 4);
    painter.drawRect(position_x + 20, position_y + 8, 4, 4);
    //right ear
    painter.drawRect(position_x + 40, position_y + 4, 4, 4);
    painter.drawRect(position_x + 36, position_y + 8, 4, 4);

    //face
    //1st row
    painter.drawRect(position_x + 16, position_y + 12, 28, 4);
    //second row
    painter.drawRect(position_x + 12, position_y + 16, 8, 4);
    painter.drawRect(position_x + 24, position_y + 16, 12, 4);
    painter.drawRect(position_x + 40, position_y + 16, 8, 4);
    //third row
    painter.drawRect(position_x + 8, position_y + 20, 44, 4);
    //4th row
    painter.drawRect(position_x + 8, position_y + 24, 4, 4);
    painter.drawRect(position_x + 16, position_y + 24, 28, 4);
    painter.drawRect(position_x + 48, position_y + 24, 4, 4);

    //5th row
    painter.drawRect(position_x + 8, position_y + 28, 4, 4);
    painter.drawRect(position_x + 16, position_y + 28, 4, 4);
    painter.drawRect(position_x + 40, position_y + 28, 4, 4);
    painter.drawRect(position_x + 48, position_y + 28, 4, 4);

    //6th row
    painter.drawRect(position_x + 20, position_y + 32, 8, 4);
    painter.drawRect(position_x + 32, position_y + 32, 8, 4);



}

void SpaceInvaders::drawAlienType_1(QPainter &painter, int position_x, int position_y)
{
    painter.setBrush(QBrush(Qt::yellow));
    painter.setPen(Qt::NoPen);

    // 1st row
    painter.drawRect(position_x + 24, position_y + 4, 16, 4);
    //2nd row
    painter.drawRect(position_x + 12, position_y + 8, 40, 4);
    //3rd row
    painter.drawRect(position_x + 8, position_y + 12, 48, 4);
    //4th row
    painter.drawRect(position_x + 8, position_y + 16, 12, 4);
    painter.drawRect(position_x + 28, position_y + 16, 8, 4);
    painter.drawRect(position_x + 44, position_y + 16, 12, 4);
    //5th row
    painter.drawRect(position_x + 8, position_y + 20, 48, 4);
    //6th row
    painter.drawRect(position_x + 20, position_y + 24, 8, 4);
    painter.drawRect(position_x + 36, position_y + 24, 8, 4);
    //7th row
    painter.drawRect(position_x + 16, position_y + 28, 8, 4);
    painter.drawRect(position_x + 28, position_y + 28, 8, 4);
    painter.drawRect(position_x + 40, position_y + 28, 8, 4);
    //8th row
    painter.drawRect(position_x + 8, position_y + 32, 8, 4);
    painter.drawRect(position_x + 48, position_y + 32, 8, 4);


}

void SpaceInvaders::drawAlienType_2(QPainter &painter, int position_x, int position_y)
{
    painter.setBrush(QBrush(Qt::magenta));
    painter.setPen(Qt::NoPen);
    //1st row
    painter.drawRect(position_x + 20, position_y + 4, 8, 4);
    //2nd row
    painter.drawRect(position_x + 16, position_y + 8, 16, 4);
    //3rd row
    painter.drawRect(position_x + 12, position_y + 12, 24, 4);
    //4th row
    painter.drawRect(position_x + 8, position_y + 16, 8, 4);
    painter.drawRect(position_x + 20, position_y + 16, 8, 4);
    painter.drawRect(position_x + 32, position_y + 16, 8, 4);
    //5th row
    painter.drawRect(position_x + 8, position_y + 20, 32, 4);
    //6th row
    painter.drawRect(position_x + 16, position_y + 24, 4, 4);
    painter.drawRect(position_x + 28, position_y + 24, 4, 4);
    //7th row
    painter.drawRect(position_x + 12, position_y + 28, 4, 4);
    painter.drawRect(position_x + 20, position_y + 28, 8, 4);
    painter.drawRect(position_x + 32, position_y + 28, 4, 4);
    //8th row
    painter.drawRect(position_x + 8, position_y + 32, 4, 4);
    painter.drawRect(position_x + 16, position_y + 32, 4, 4);
    painter.drawRect(position_x + 28, position_y + 32, 4, 4);
    painter.drawRect(position_x + 36, position_y + 32, 4, 4);

}

void SpaceInvaders::drawAlienLaser(QPainter &painter, int x, int y)
{
    painter.setPen(Qt::NoPen);

    // widest layer
    painter.setBrush(QColor(255, 100, 100, 80));
    painter.drawRect(x + 2, y - 2, 6, 2);      // top glow
    painter.drawRect(x + 2, y + 16, 6, 2);     // bottom glow
    painter.drawRect(x, y, 2, 16);             // left glow
    painter.drawRect(x + 8, y, 2, 16);         // right glow

    // mid layer (orange glow)
    painter.setBrush(QColor(255, 150, 50, 180));
    painter.drawRect(x + 2, y, 6, 16);

    // inner core (bright yellow-white) - brightest center
    painter.setBrush(QColor(255, 255, 200, 255));
    painter.drawRect(x + 3, y + 1, 4, 14);

    // hot white center line
    painter.setBrush(QColor(255, 255, 255, 255));
    painter.drawRect(x + 4, y + 2, 2, 12);
}

void SpaceInvaders::drawPlayerShield(QPainter &painter, int position_x)
{
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(Qt::NoPen);

    painter.drawRect(position_x, 230, 4, 4);
    painter.drawRect(position_x + 4, 226, 32, 4);


  
    painter.drawRect(position_x + 36, 230, 4, 4);

}

void SpaceInvaders::drawStartScreen(QPainter &painter)
{
    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 12));
    painter.drawText(20, 70, "Welcome to Space Invaders!");
    painter.drawText(20, 90, "Select Number of Players");


    //Draw Boxes and highlight them based on player 1 left stick input
    if (player1_stickValue < -8000) {
        player_number = 1;  // Move left
    } else if (player1_stickValue > 8000) {
        player_number = 2;   // Move right
    }

    if (player_number == 1)
    {
        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);
        painter.drawRect(60, 150, 90, 60);
    }
    if (player_number == 2)
    {
        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);
        painter.drawRect(330, 150, 90, 60);
    }

    painter.setBrush(QBrush(Qt::black));
    painter.setPen(Qt::NoPen);

    painter.drawRect(70, 160, 70, 40);
    painter.drawRect(340, 160, 70, 40);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 20));
    painter.drawText(95, 185, "1");
    painter.drawText(365, 185, "2");

    //select mode logic

    if (player1_a_button_pressed)
    {
        if (player_number == 1)
        {
            player_mode = SINGLE_PLAYER;
        }
        if (player_number == 2)
        {
            player_mode = MULTI_PLAYER;
            controllerFd_player2 = open("/dev/input/event2", O_RDONLY | O_NONBLOCK);
            if (controllerFd_player2 < 0) {
                qWarning() << "Could not open controller at /dev/input/event1";
                qWarning() << "Continuing without controller input...";
            } else {
                qDebug() << "Controller 2 opened successfully";
                player.emplace_back(Player("Player2", 30, 270));
              
                controllerNotifier_player2 = new QSocketNotifier(controllerFd_player2, QSocketNotifier::Read, this);
                connect(controllerNotifier_player2, &QSocketNotifier::activated, this, &SpaceInvaders::onControllerInput_player2);
              
               
            }
        }
        //go to game
        player1_a_button_pressed = false;
        platformThread_running = true;
        player1_cannonThread_running = true;
        player2_cannonThread_running = true;
        for (int i = 0; i < NUM_ATTACKS; i++) {
            attackThread_running[i] = true;
        }
        platformThread = std::thread(&SpaceInvaders::platformThread_func, this);
        player1_cannonThread = std::thread(&SpaceInvaders::player1_cannonThread_func, this);
        attackThread[0] = std::thread(&SpaceInvaders::attackThread_func, this, 0, 400, 800);
        attackThread[1] = std::thread(&SpaceInvaders::attackThread_func, this, 1, 500, 1000);
        attackThread[2] = std::thread(&SpaceInvaders::attackThread_func, this, 2, 600, 1200);
        if (player_mode == MULTI_PLAYER)
        {
            player2_cannonThread = std::thread(&SpaceInvaders::player2_cannonThread_func, this);
        }
        current_state = GAME;
    }



}

void SpaceInvaders::drawEndScreen(QPainter &painter)
{
    painter.setFont(QFont("Sans", 20));

    if (player_won)
    {
        drawAlienType_0(painter, 50, 20);
        drawAlienType_1(painter, 180, 15);
        drawAlienType_2(painter, 320, 20);

        painter.setPen(Qt::green);
        painter.drawText(150, 85, "YOU WIN!");
        painter.setPen(Qt::white);
        painter.setFont(QFont("Sans", 10));
        painter.drawText(115, 105, "All aliens destroyed!");
    }
    else
    {
        drawAlienType_1(painter, 60, 15);
        drawAlienType_2(painter, 150, 20);
        drawAlienType_0(painter, 230, 20);
        drawAlienType_1(painter, 330, 15);

        painter.setPen(Qt::red);
        painter.drawText(130, 85, "GAME OVER");
        painter.setPen(Qt::white);
        painter.setFont(QFont("Sans", 10));
        painter.drawText(130, 105, "The aliens won...");
    }

    if (player1_stickValue < -8000) {
        end_screen_selection = 0;
    } else if (player1_stickValue > 8000) {
        end_screen_selection = 1;
    }

    if (end_screen_selection == 0)
    {
        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);
        painter.drawRect(20, 170, 160, 60);
    }
    if (end_screen_selection == 1)
    {
        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);
        painter.drawRect(280, 170, 170, 60);
    }

    painter.setBrush(QBrush(Qt::black));
    painter.setPen(Qt::NoPen);
    painter.drawRect(30, 180, 140, 40);
    painter.drawRect(290, 180, 150, 40);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 12));
    painter.drawText(55, 205, "Play Again");
    painter.drawText(315, 205, "Start Screen");

    if (player1_a_button_pressed)
    {
        player1_a_button_pressed = false;
        if (end_screen_selection == 0)
        {
            resetGame();
            current_state = GAME;
        }
        else if (end_screen_selection == 1)
        {
            stopGameThreads();
            resetGame();

            if (player_mode == MULTI_PLAYER && player.size() > 1)
            {
                player.pop_back();
            }
            player_mode = NONE;
            player_number = 0;
            current_state = START;
        }
    }
}

void SpaceInvaders::drawPlatforms(QPainter &painter, int curr_player1_pos, int curr_player2_pos, int curr_player1_health, int curr_player2_health)
{

    painter.setBrush(QColor(255, 0, 170, 255));
    painter.setPen(Qt::NoPen);
    painter.drawRect(curr_player1_pos, 255, 40, 15);
    painter.drawRect(curr_player1_pos + 5, 250, 30, 5);
    painter.drawRect(curr_player1_pos+15, 245, 10, 5);
    painter.drawRect(curr_player1_pos+18, 240, 4, 5);

    //health bar
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(Qt::NoPen);
    painter.drawRect(curr_player1_pos + 4, 257.5, 32, 10);

    painter.setBrush(QBrush(Qt::red));
    painter.setPen(Qt::NoPen);
    painter.drawRect(curr_player1_pos + 5, 258.5, curr_player1_health, 8);

    if (player_mode == MULTI_PLAYER)
    {
        painter.setBrush(QColor(97, 255, 166, 255));
        painter.setPen(Qt::NoPen);
        painter.drawRect(curr_player2_pos, 255, 40, 15);
        painter.drawRect(curr_player2_pos + 5, 250, 30, 5);
        painter.drawRect(curr_player2_pos+15, 245, 10, 5);
        painter.drawRect(curr_player2_pos+18, 240, 4, 5);

        //health bar
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(Qt::NoPen);
        painter.drawRect(curr_player2_pos + 4, 257.5, 32, 10);
        
        painter.setBrush(QBrush(Qt::red));
        painter.setPen(Qt::NoPen);
        painter.drawRect(curr_player2_pos + 5, 258.5, curr_player2_health, 8);
    }

}

void SpaceInvaders::drawExplosion(QPainter &painter, int center_x, int frame, bool isPlayer1)
{
    QColor baseColor = isPlayer1 ? QColor(255, 0, 170, 255) : QColor(97, 255, 166, 255);
    painter.setPen(Qt::NoPen);
    painter.setBrush(baseColor);

    int center_y = 250;

    if (frame <= 15)
    {
        painter.drawRect(center_x + 16, center_y - 4, 4, 4);
        painter.drawRect(center_x + 20, center_y - 4, 4, 4);
        painter.drawRect(center_x + 16, center_y, 4, 4);
        painter.drawRect(center_x + 20, center_y, 4, 4);
    }
    else if (frame <= 35)
    {
        painter.drawRect(center_x + 18, center_y - 16, 4, 4);  // up
        painter.drawRect(center_x + 18, center_y + 8, 4, 4);   // down
        painter.drawRect(center_x, center_y - 4, 4, 4);        // left
        painter.drawRect(center_x + 36, center_y - 4, 4, 4);   // right
        painter.drawRect(center_x + 8, center_y - 12, 4, 4);   // up-left
        painter.drawRect(center_x + 28, center_y - 12, 4, 4);  // up-right
    }
    else
    {
        painter.drawRect(center_x - 8, center_y - 8, 4, 4);
        painter.drawRect(center_x + 44, center_y - 8, 4, 4);
        painter.drawRect(center_x + 18, center_y - 28, 4, 4);
        painter.drawRect(center_x + 4, center_y - 20, 4, 4);
        painter.drawRect(center_x + 32, center_y - 20, 4, 4);
    }
}

void SpaceInvaders::drawDestroyedPlatform(QPainter &painter, int position_x, bool isPlayer1)
{
    QColor baseColor = isPlayer1 ? QColor(255, 0, 170, 255) : QColor(97, 255, 166, 255);
    painter.setPen(Qt::NoPen);
    painter.setBrush(baseColor);

    painter.drawRect(position_x + 4, 264, 4, 4);
    painter.drawRect(position_x + 16, 260, 4, 4);
    painter.drawRect(position_x + 32, 264, 4, 4);
    painter.drawRect(position_x + 12, 268, 4, 4);
    painter.drawRect(position_x + 24, 268, 4, 4);
}

void SpaceInvaders::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    
    int player1_pos;
    int player2_pos;
    int player1_health = 0;
    int player2_health = 0;
    bool isalive = false;
    if (current_state == START)
    {
       drawStartScreen(painter);
    }
    else if (current_state == GAME)
    {       
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player1_pos = player1_curr_platformPosition;
            player1_health = player[0].getHealth();
            if (player_mode == MULTI_PLAYER)
            {
                player2_pos = player2_curr_platformPosition;
                player2_health = player[1].getHealth();
            }
        }

        if (player1_health <= 0)
        {
            if (player1_explosion_frame == 0)
            {
                player1_death_x = player1_pos;
                player1_explosion_frame = 1;
            }
            else if (player1_explosion_frame <= 60)
            {
                drawExplosion(painter, player1_death_x, player1_explosion_frame, true);
                player1_explosion_frame++;
            }
            else
            {
                drawDestroyedPlatform(painter, player1_death_x, true);
            }
        }

        if (player_mode == MULTI_PLAYER && player2_health <= 0)
        {
            if (player2_explosion_frame == 0)
            {
                player2_death_x = player2_pos;
                player2_explosion_frame = 1;
            }
            else if (player2_explosion_frame <= 60)
            {
                drawExplosion(painter, player2_death_x, player2_explosion_frame, false);
                player2_explosion_frame++;
            }
            else
            {
                drawDestroyedPlatform(painter, player2_death_x, false);
            }
        }

        if (player_mode == MULTI_PLAYER)
        {
            drawPlatforms(painter,
                player1_health > 0 ? player1_pos : -100,
                player2_health > 0 ? player2_pos : -100,
                player1_health > 0 ? player1_health : 0,
                player2_health > 0 ? player2_health : 0);
        }
        else
        {
            if (player1_health > 0)
            {
                drawPlatforms(painter, player1_pos, -1, player1_health, -1);
            }
        }

        if (player1_a_button_pressed && player1_health > 0)
        {
            if (player1_cannon_display_x_flag == 0)
            {
                player1_cannon_display_x_pos = player1_pos;
                player1_cannon_display_x_flag = 1;
            }
            painter.setBrush(QColor(255, 0, 170, 255));
            painter.setPen(Qt::NoPen);
            painter.drawRect(player1_cannon_display_x_pos + 18, player1_cannon_y_pos, 4, 5);
        }

        if (player2_a_button_pressed && player2_health > 0)
        {
            if (player2_cannon_display_x_flag == 0)
            {
                player2_cannon_display_x_pos = player2_pos;
                player2_cannon_display_x_flag = 1;
            }
            painter.setBrush(QColor(97, 255, 166, 255));
            painter.setPen(Qt::NoPen);
            painter.drawRect(player2_cannon_display_x_pos + 18, player2_cannon_y_pos, 4, 5);
        }

        //Shield Logic
        if (player1_x_button_pressed && !player1_shield_cooldown_enabled)
        {
            if (player1_shield_flag == 0)
            {
                player1_shield_time = 0;
                player1_shield_flag = 1;
            }
            drawPlayerShield(painter, player1_pos);
            player1_shield_time += 1;

            if (player1_shield_time >= 180)
            {
                player1_x_button_pressed = false;
                player1_shield_flag = 0;
                player1_shield_cooldown_enabled = true;
            }
        }

        if (player2_x_button_pressed && !player2_shield_cooldown_enabled)
        {
            if (player2_shield_flag == 0)
            {
                player2_shield_time = 0;
                player2_shield_flag = 1;
            }
            drawPlayerShield(painter, player2_pos);
            player2_shield_time += 1;
            if (player2_shield_time >= 180)
            {
                player2_x_button_pressed = false;
                player2_shield_flag = 0;
                player2_shield_cooldown_enabled = true;
            }

        }



        if (player1_shield_cooldown_enabled)
        {
            player1_shield_cooldown_time += 1;

                    
            painter.setBrush(QColor(248, 255, 0, 255));
            painter.setPen(Qt::NoPen);  
            painter.drawRect(player1_pos + 5, 252, 5, 4);
            

            if (100 <= player1_shield_cooldown_time)
            {               
                painter.setBrush(QColor(248, 255, 0, 255));
                painter.setPen(Qt::NoPen);  
                painter.drawRect(player1_pos + 15, 252, 5, 4);
            }

            if (200 <= player1_shield_cooldown_time)
            {           
                painter.setBrush(QColor(248, 255, 0, 255));
                painter.setPen(Qt::NoPen);  
                painter.drawRect(player1_pos + 25, 252, 5, 4);
            }
            
            if (player1_shield_cooldown_time >= 300)
            {
                player1_shield_cooldown_time = 0;
                player1_shield_cooldown_enabled = false;
            }
        }

        if (player2_shield_cooldown_enabled)
        {
            player2_shield_cooldown_time += 1;
     
            painter.setBrush(QColor(248, 255, 0, 255));
            painter.setPen(Qt::NoPen);  
            painter.drawRect(player2_pos + 5, 252, 5, 4);

            if (100 <= player2_shield_cooldown_time)
            {               
                painter.setBrush(QColor(248, 255, 0, 255));
                painter.setPen(Qt::NoPen);  
                painter.drawRect(player2_pos + 15, 252, 5, 4);
            }

            if (200 <= player2_shield_cooldown_time)
            {           
                painter.setBrush(QColor(248, 255, 0, 255));
                painter.setPen(Qt::NoPen);  
                painter.drawRect(player2_pos + 25, 252, 5, 4);
            }
            
            if (player2_shield_cooldown_time >= 300)
            {
                player2_shield_cooldown_time = 0;
                player2_shield_cooldown_enabled = false;
            }
        }

        //Alien Logic
        for (size_t row = 0; row < alien.size(); row++)
        {
            for (size_t column = 0; column < alien[row].size(); column++)
            {
                {
                    std::lock_guard<std::mutex> lock(alien_mtx);
                    isalive = alien[row][column].isAlive();
                }
                if (isalive)
                {
                    if (alien[row][column].getType() == 0)
                    {
                        drawAlienType_0(painter, alien[row][column].getPosition_x(), alien[row][column].getPosition_y());
                    }
                    else if (alien[row][column].getType() == 1)
                    {
                        drawAlienType_1(painter, alien[row][column].getPosition_x(), alien[row][column].getPosition_y());
                    }
                    else if (alien[row][column].getType() == 2)
                    {
                        drawAlienType_2(painter, alien[row][column].getPosition_x(), alien[row][column].getPosition_y());
                    }
                }
            }
        }
        painter.setBrush(QBrush(Qt::red));
        painter.setPen(Qt::NoPen);
        
        for (int i = 0; i < NUM_ATTACKS; i++)
        {
            if (isAttacking[i])
            {
                drawAlienLaser(painter, attack_x_pos[i], attack_y_pos[i]);
                if (player1_pos <= attack_x_pos[i] && attack_x_pos[i] <= (player1_pos + 40) && attack_y_pos[i] >= 235)
                {
                    if (player1_x_button_pressed)
                    {
                        isAttacking[i] = false;
                    }
                    else
                    {
                        qDebug() << "Player 1 HIT by attack" << i;
                        std::lock_guard<std::mutex> lock(platform_mtx);
                        player[0].playerHit();
                        isAttacking[i] = false;
                    }
                }
                if (player2_pos <= attack_x_pos[i] && attack_x_pos[i] <= (player2_pos + 40) && attack_y_pos[i] >= 235)
                {
                    if (player2_x_button_pressed)
                    {
                        isAttacking[i] = false;
                    }
                    else
                    {
                        qDebug() << "Player 2 HIT by attack" << i;
                        std::lock_guard<std::mutex> lock(platform_mtx);
                        player[1].playerHit();
                        isAttacking[i] = false;
                    }
                }
            }
        }

        if (checkAllAliensDead())
        {
            player_won = true;
            current_state = END;
        }
        if (checkAllPlayersDead())
        {
            player_won = false;

            // wait for all explosion animations to complete
            bool explosions_done = true;
            if (player1_explosion_frame > 0 && player1_explosion_frame <= 60)
                explosions_done = false;
            if (player_mode == MULTI_PLAYER && player2_explosion_frame > 0 && player2_explosion_frame <= 60)
                explosions_done = false;

            if (explosions_done)
            {
                game_over_delay++;
                if (game_over_delay >= 120)
                {
                    current_state = END;
                }
            }
        }
    }
    else if (current_state == END)
    {
        drawEndScreen(painter);
    }
    
}
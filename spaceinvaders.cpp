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

    std::vector<std::vector<int>> alien_types = {
        {2, 0, 2, 1, 2, 0, 2, 1},
        {0, 1, 0, 2, 0, 1, 0},
        {1, 2, 1, 0, 1, 2},
    };
    
    //                    0, 1, 2
    std::vector<int> alien_widths = {44, 48, 32};
    int positionX = 0;
    int positionY = 0;
    int spacingX = 20;
    int spacingY = 52;
    for (size_t row = 0; row < alien_types.size(); row++)
    {
        // qDebug() << "position Y " << positionY << " row " << row;
        if (row == 1) positionX = 20;
        else if (row == 2) positionX = 50;
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
    
    player.emplace_back(Player("Player1", 30, 220));
   
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

    // cannonThread = std::thread(&SpaceInvaders::cannonThread_func, this);

    // attackThread = std::thread(&SpaceInvaders::attackThread_func, this);


}

SpaceInvaders::~SpaceInvaders()
{
    if (controllerFd_player1 >= 0) {
        ::close(controllerFd_player1);
    }
    platformThread_running = false;
    displayThread_running = false;
    cannonThread_running = false;
    attackThread_running = false;
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
        int player1_moveSpeed = 0;
        if (player1_stickValue < -8000) {
            player1_moveSpeed = -5;  // Move left
        } else if (player1_stickValue > 8000) {
            player1_moveSpeed = 5;   // Move right
        }
        
        if (player1_moveSpeed != 0) {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                player[0].movePlatform(player1_moveSpeed);   
            }
        }

        if (player_mode == MULTI_PLAYER)
        {
            int player2_moveSpeed = 0;
            if (player2_stickValue < -8000) {
                player2_moveSpeed = -5;  // Move left
            } else if (player2_stickValue > 8000) {
                player2_moveSpeed = 5;   // Move right
            }
            
            if (player2_moveSpeed != 0) {
                {
                    std::lock_guard<std::mutex> lock(platform_mtx);
                    player[1].movePlatform(player2_moveSpeed);   
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void SpaceInvaders::attackThread_func()
{
    int attack_pos_x = 0;
    int attack_pos_y = 0;
    int alien_type = -1;
    int num_increments = 0;
   
  
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib_column(0, 7);
    std::uniform_int_distribution<> distrib_row(0, 2);
    while(attackThread_running)
    {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
       
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
            qDebug() << "attack_pos_x" << attack_pos_x << " y is " << attack_pos_y << "type is " << alien_type;
            if (alien_type == 1)
            {
                attack_x_pos = attack_pos_x + 32;
            }
            else if (alien_type == 2)
            {
                attack_x_pos = attack_pos_x + 24;          
            }
            else if (alien_type == 3)
            {
                attack_x_pos = attack_pos_x + 30;
            }

            attack_y_pos = attack_pos_y + 32;
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
            isAttacking = true;
            for (int i = 0; i < num_increments; i++)
            {
                attack_y_pos += 10;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            isAttacking = false;
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



void SpaceInvaders::cannonThread_func()
{
    int cannon_x_pos = 0;
 
    while(cannonThread_running)
    {
        if (player1_a_button_pressed)
        {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                cannon_x_pos = player1_curr_platformPosition + 20;
            }
           
            for (int i = 0; i < 11; i+=1)
            {                               
                cannon_y_pos -= 20;
               
                if ((4 <= cannon_y_pos && cannon_y_pos <= 36) && (8 <= cannon_x_pos && cannon_x_pos <= 460))
                {
                    if (hit_alien_row_1(cannon_x_pos)) break;
                }
                if ((56 <= cannon_y_pos && cannon_y_pos <= 88) && (28 <= cannon_x_pos && cannon_x_pos <= 452))
                {
                    if (hit_alien_row_2(cannon_x_pos)) break;        
                }
                if ((108 <= cannon_y_pos && cannon_y_pos <= 140) && (58 <= cannon_x_pos && cannon_x_pos <= 410))
                {
                    if (hit_alien_row_3(cannon_x_pos)) break;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(32));
            }
            player1_a_button_pressed = false;
            cannon_y_pos = 235;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

bool hit_player(int attack_x_pos, int attack_y_pos, int player_x_pos)
{

    return false;
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
            
                controllerNotifier_player2 = new QSocketNotifier(controllerFd_player2, QSocketNotifier::Read, this);
                connect(controllerNotifier_player2, &QSocketNotifier::activated, this, &SpaceInvaders::onControllerInput_player2);
                player.emplace_back(Player("Player2", 30, 220));
               
            }
        }
        //go to game
        platformThread = std::thread(&SpaceInvaders::platformThread_func, this);
        current_state = GAME;
    }

    

}

void SpaceInvaders::drawPlatforms(QPainter &painter, int player1_pos, int player2_pos)
{
    player1_pos -= 50;
    painter.setBrush(QColor(255, 0, 170, 255));
    painter.setPen(Qt::NoPen);
    painter.drawRect(player1_pos, 255, 40, 15);
    painter.drawRect(player1_pos + 5, 250, 30, 5);
    painter.drawRect(player1_pos+15, 245, 10, 5);
    painter.drawRect(player1_pos+18, 240, 4, 5);

    if (player2_pos != -1)
    {
        player2_pos += 50;
        painter.setBrush(QColor(97, 255, 166, 255));
        painter.setPen(Qt::NoPen);
        painter.drawRect(player2_pos, 255, 40, 15);
        painter.drawRect(player2_pos + 5, 250, 30, 5);
        painter.drawRect(player2_pos+15, 245, 10, 5);
        painter.drawRect(player2_pos+18, 240, 4, 5);
    }
}


void SpaceInvaders::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    if (current_state == START)
    {
       drawStartScreen(painter);
    }
    else if (current_state == GAME)
    {       
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player1_pos = player1_curr_platformPosition;
            if (player_mode == MULTI_PLAYER)
            {               
                player2_pos = player2_curr_platformPosition;
            }
        }

        if (player_mode == MULTI_PLAYER)
        {
            drawPlatforms(painter, player1_pos, player2_pos);
        }
        else
        {
            drawPlatforms(painter, player1_pos, -1);
        }
    }
    else
    {

        int pos;
        int player1_health = 0;
        bool isalive = false;
      
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            player1_health = player[0].getHealth();
            pos = player1_curr_platformPosition;
                // qWarning() << "display thread running";
        }
        // qWarning() << "Drawing at position:" << pos;
        // painter.setPen(Qt::white);
        // painter.setFont(QFont("Sans", 12));
        // painter.drawText(10, 70, "Move stick left/right, press A to 'shoot'");

        //platform + cannon
        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);
        painter.drawRect(pos, 255, 40, 15);
        painter.drawRect(pos + 5, 250, 30, 5);
        painter.drawRect(pos+15, 245, 10, 5);
        painter.drawRect(pos+18, 240, 4, 5);

        painter.setBrush(QBrush(Qt::white));
        painter.setPen(Qt::NoPen);
        //health bar
        painter.drawRect(pos + 5, 258.5, 30, 8);

        painter.setBrush(QBrush(Qt::red));
        painter.setPen(Qt::NoPen);
        painter.drawRect(pos + 5, 258.5, player1_health, 8);

        painter.setBrush(QBrush(Qt::green));
        painter.setPen(Qt::NoPen);

        if (player1_a_button_pressed)
        {     
            painter.drawRect(pos + 18, cannon_y_pos, 4, 5);
        }

        if (player1_x_button_pressed)
        {
            if (player1_shield_flag == 0)
            {
                player1_shield_time = 0;
                player1_shield_flag = 1;
            }
            drawPlayerShield(painter, pos);
            player1_shield_time += 1;

            if (player1_shield_time >= 180)
            {
                player1_x_button_pressed = false;
                player1_shield_flag = 0;
            }
        }
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
        
        if (isAttacking)
        {
            painter.drawRect(attack_x_pos, attack_y_pos, 10, 10);
            
                if (pos <= attack_x_pos && attack_x_pos <= (pos + 40) && attack_y_pos >= 235)
                {
                    if (player1_x_button_pressed)
                    {
                        isAttacking = false;
                    }
                    else
                    {
                        {
                            // qDebug() << "Player HIT";
                            std::lock_guard<std::mutex> lock(platform_mtx);
                            player[0].playerHit();
                            isAttacking = false;
                        }
                    }

                
                }
        
        
        }
    }
    // drawAlienType_0(painter, 0, 0);
    // drawAlienType_1(painter, 60, 0);
    // // drawAlienType_2(painter, 0, 0);
    
}
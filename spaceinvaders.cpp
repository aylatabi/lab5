#include "spaceinvaders.h"
#include <QPainter>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>


#define XBOX_LEFT_STICK_X 0
#define XBOX_A_BUTTON     304
SpaceInvaders::SpaceInvaders(QWidget *parent) : QWidget(parent)
{
    controllerFd = -1;
    controllerNotifier = nullptr;

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
    
    player.emplace_back(Player("Player1", 100, 220));
   
    // for (size_t row = 0; row < alien.size(); row++)
    // {
    //     for (size_t column = 0; column < alien[row].size(); column++)
    //     {
    //         qDebug() << "Column " << column << " " << (alien[row][column].getPosition_x() + 8);
    //     }
    //     qDebug() << " ";
    // }
    setStyleSheet("background-color: black;");
   
    controllerFd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
    if (controllerFd < 0) {
        qWarning() << "Could not open controller at /dev/input/event1";
        qWarning() << "Continuing without controller input...";
    } else {
        qDebug() << "Controller opened successfully";
    
        controllerNotifier = new QSocketNotifier(controllerFd, QSocketNotifier::Read, this);
        connect(controllerNotifier, &QSocketNotifier::activated, this, &SpaceInvaders::onControllerInput);
    }

    platformThread = std::thread(&SpaceInvaders::platformThread_func, this);

    displayThread = std::thread(&SpaceInvaders::displayThread_func, this);

    cannonThread = std::thread(&SpaceInvaders::cannonThread_func, this);

}

SpaceInvaders::~SpaceInvaders()
{
    if (controllerFd >= 0) {
        ::close(controllerFd);
    }
    platformThread_running = false;
    displayThread_running = false;
    cannonThread_running = false;
}

void SpaceInvaders::displayThread_func()
{
    while(displayThread_running)
    {
        {
            std::lock_guard<std::mutex> lock(platform_mtx);
            curr_platformPosition = player[0].getPosition();
            // qWarning() << "display thread running";
        }
        
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void SpaceInvaders::platformThread_func()
{
    while(platformThread_running)
    {
        int moveSpeed = 0;
        if (stickValue < -8000) {
            moveSpeed = -5;  // Move left
        } else if (stickValue > 8000) {
            moveSpeed = 5;   // Move right
        }
        
        if (moveSpeed != 0) {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                player[0].movePlatform(moveSpeed);
                // qWarning() << "platform thread running";
            }
            
                        
        //    // update();  // Trigger repaint
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}



bool SpaceInvaders::hit_alien_row_3(int cannon_x_pos)
{
    int column = -1;
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
        if (alien[2][column].isAlive())
        {
            alien[2][column].damage();
            return true;
        }
    }
    return false;
}

bool SpaceInvaders::hit_alien_row_2(int cannon_x_pos)
{
    int column = -1;
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
        if (alien[1][column].isAlive())
        {
            alien[1][column].damage();
            return true;
        }
    }
    return false;
}

bool SpaceInvaders::hit_alien_row_1(int cannon_x_pos)
{
    int column = -1;
   
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
        if (alien[0][column].isAlive())
        {
            alien[0][column].damage();
            return true;
        }
    }
    
    return false;
}
void SpaceInvaders::cannonThread_func()
{
    int cannon_x_pos = 0;
 
    while(cannonThread_running)
    {
        if (a_button_pressed)
        {
            {
                std::lock_guard<std::mutex> lock(platform_mtx);
                cannon_x_pos = curr_platformPosition + 20;
            }
            // qDebug() << "cannon x position center" << cannon_x_pos;
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
            a_button_pressed = false;
            cannon_y_pos = 235;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

void SpaceInvaders::onControllerInput()
{
    struct input_event ev;
    
    while (read(controllerFd, &ev, sizeof(ev)) == sizeof(ev)) {

        if (ev.type == EV_KEY && ev.code == XBOX_A_BUTTON) {
            if (ev.value) {
               a_button_pressed = true;      
            }
        }
        if (ev.type == EV_ABS && ev.code == XBOX_LEFT_STICK_X) {
            stickValue = ev.value;
            
            // Move platform based on stick position
            // Stick range is roughly -32768 to 32767
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
void SpaceInvaders::paintEvent(QPaintEvent *event)
{
    int pos;
    bool isalive = false;
    Q_UNUSED(event);
    QPainter painter(this);
    {
        std::lock_guard<std::mutex> lock(platform_mtx);
        pos = curr_platformPosition;
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

    if (a_button_pressed)
    {
        painter.drawRect(pos + 18, cannon_y_pos, 4, 5);
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
    // drawAlienType_0(painter, 0, 0);
    // drawAlienType_1(painter, 60, 0);
    // // drawAlienType_2(painter, 0, 0);
    
}
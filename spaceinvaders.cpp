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
        {2, 0, 2, 1, 2, 0, 2, 1, 2},
        // {0, 1, 0, 2, 0, 1, 0, 2, 0},
        // {1, 2, 1, 0, 1, 2, 1, 0, 1},
    };
    
    //                    0, 1, 2
    std::vector<int> alien_widths = {44, 48, 32};
    int positionX = 0;
    int positionY = 0;
    int spacingX = 20;
    int spacingY = 52;
    for (size_t row = 0; row < alien_types.size(); row++)
    {
        for (size_t column = 0; column < alien_types[row].size(); column++)
        {
            alien.emplace_back(Alien(alien_types[row][column], true, 100, positionX, positionY));
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
        positionX = 0;
        positionY += spacingY;
    }
    
    player.emplace_back(Player("Player1", 100, 220));
   
    
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

void SpaceInvaders::cannonThread_func()
{
    int cannon_x_pos = 0;
 
    while(cannonThread_running)
    {
        if (a_button_pressed)
        {
            for (int i = 0; i < 13; i+=1)
            {
                {
                    std::lock_guard<std::mutex> lock(platform_mtx);
                    cannon_x_pos = curr_platformPosition;
                }
                cannon_y_pos -= 10;
                if (cannon_y_pos <= 36)
                {
                    if (5 <= cannon_x_pos && cannon_x_pos <= 50)
                    {
                        {
                            std::lock_guard<std::mutex> lock(alien_mtx);
                            alien
                    
                        }
                    }

                   
                    
                  
                    
                }
                break;
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
    
    for (size_t i = 0; i < alien.size(); i++)
    {
        if (alien[i].isAlive())
        {
            if (alien[i].getType() == 0)
            {
                drawAlienType_0(painter, alien[i].getPosition_x(), alien[i].getPosition_y());
            }
            else if (alien[i].getType() == 1)
            {
                drawAlienType_1(painter, alien[i].getPosition_x(), alien[i].getPosition_y());
            }
            else if (alien[i].getType() == 2)
            {
                drawAlienType_2(painter, alien[i].getPosition_x(), alien[i].getPosition_y());
            }
        }
      

    }
    // drawAlienType_0(painter, 0, 0);
    // drawAlienType_1(painter, 60, 0);
    // // drawAlienType_2(painter, 0, 0);
    
}
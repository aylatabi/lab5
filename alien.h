#ifndef ALIEN_H
#define ALIEN_H

class Alien
{
public:
    Alien(int alien_type, bool enabled, int health, int position_x, int position_y)
        : alien_type(alien_type), enabled(enabled), health(health), position_x(position_x), position_y(position_y) {}
    int getPosition_x() const {return position_x;}
    int getPosition_y() const {return position_y;}
    int getType() const {return alien_type;}
    bool isAlive() const {return enabled;}
    void damage() {health -= 100; if (health == 0) {enabled = false;}}

private:
    int alien_type;
    bool enabled;
    int health;
    int position_x;
    int position_y;
};

#endif
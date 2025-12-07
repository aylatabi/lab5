#ifndef PLAYER_H
#define PLAYER_H

#include <string>

class Player 
{
public:
    Player(const std::string& name, int health, int position)
        : name(name), health(health), position(position) {}
    int getPosition() const { return position; }
    int getHealth() const {return health;}
    void playerHit() {health -= 15;}
    void movePlatform(int offset);
private:
    std::string name;
    int health;
    int position;
};

#endif


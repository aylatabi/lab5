#include "player.h"


void Player::movePlatform(int offset)
{
    position += offset;
    if (position < 0)
    {
        position = 0;
    }
    else if (position > 440)
    {
        position = 440;
    }

}


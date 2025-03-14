#include "Game/Player.hpp"

#include "Engine/Math/MathUtils.hpp"

#include <limits>
#include <numeric>
#include <utility>

Player::Player(const PlayerDesc& playerDesc) noexcept
    : desc(playerDesc)
    , _scoreRemainingForOneUp(desc.oneUpScore)
{
    /* DO NOTHING */
}

void Player::IncrementLives() noexcept {
    desc.lives = (std::min)(++desc.lives, (std::numeric_limits<decltype(desc.lives)>::max)());
}

void Player::DecrementLives() noexcept {
    desc.lives = (std::max)(decltype(desc.lives){0}, --desc.lives);
}

long long Player::GetScore() const noexcept {
    return desc.score;
}

void Player::AdjustScore(long long amount) noexcept {
    if(!(amount > 0 && desc.score + amount < desc.score) ||
       !(amount < 0 && desc.score - amount > desc.score)) {
        desc.score += amount;
        _scoreRemainingForOneUp -= amount;
        if(_scoreRemainingForOneUp < 0ll) {
            _scoreRemainingForOneUp = MathUtils::Wrap(_scoreRemainingForOneUp, 0ll, desc.oneUpScore);
        }
    }
}

long long Player::GetLives() const noexcept {
    return desc.lives;
}

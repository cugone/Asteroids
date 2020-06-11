#include "Game/Player.hpp"

void Player::IncrementLives() noexcept {
    ++lives;
}

void Player::DecrementLives() noexcept {
    if(lives) {
        --lives;
    }
}

long long Player::GetScore() const noexcept {
    return _score;
}

void Player::AdjustScore(long long amount) noexcept {
    _score += amount;
}

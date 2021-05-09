#pragma once

struct PlayerDesc {
    long long lives{3ull};
    long long score{0ull};
    long long oneUpScore{10000ull};
};

class Player {
public:
    Player() noexcept = default;
    explicit Player(const PlayerDesc& playerDesc) noexcept;

    long long GetLives() const noexcept;
    void IncrementLives() noexcept;
    void DecrementLives() noexcept;

    long long GetScore() const noexcept;
    void AdjustScore(long long amount) noexcept;

    PlayerDesc desc{};
protected:
private:
    long long _scoreRemainingForOneUp{};
};


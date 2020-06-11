#pragma once

class Player {
public:
    void IncrementLives() noexcept;
    void DecrementLives() noexcept;
    long long GetScore() const noexcept;
    void AdjustScore(long long amount) noexcept;

    unsigned long long lives{3ull};

protected:
private:
    long long _score{};
};

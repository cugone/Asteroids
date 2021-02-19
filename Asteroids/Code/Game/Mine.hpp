#pragma once

#include "Game/Entity.hpp"

class Mine : public Entity {
public:

    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render(Renderer& renderer) const noexcept override;
    void EndFrame() noexcept override;
    void OnCreate() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept;

protected:
private:
};

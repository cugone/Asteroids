#include "Game/Ufo.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Bullet.hpp"
#include "Game/Mine.hpp"
#include "Game/Game.hpp"
#include "Game/Ship.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

Ufo::Ufo(Type type, Vector2 position)
    : GameEntity()
    , _type(type)
{
    SetCosmeticRadius(GetCosmeticRadiusFromType(_type));
    SetPhysicalRadius(GetPhysicalRadiusFromType(_type));
    SetPosition(position);
    SetVelocity(Vector2::X_Axis * 100.0f);
    SetHealth(GetHealthFromType(_type));
    faction = GameEntity::Faction::Enemy;
    _bulletSpeed = GetBulletSpeedFromTypeAndDifficulty(_type);
    _fireRate.SetFrequency(GetFireRateFromTypeAndDifficulty(_type));
    _style = GetStyleFromType(_type);
    scoreValue = GetValueFromType(_type);

    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("ufo");
    desc.spriteSheet = GetSpriteSheet();
    desc.durationSeconds = TimeUtils::FPSeconds{0.3f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = GetFrameLengthFromTypeAndStyle(_type, _style);
    desc.startSpriteIndex = GetStartIndexFromTypeAndStyle(_type, _style);

    if(auto cbs = GetMaterial()->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        ufo_state_cb = &cbs[0].get();
    }

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void Ufo::BeginFrame() noexcept {
    GameEntity::BeginFrame();
    _fireTarget = CalculateFireTarget();
    if(_fireRate.CheckAndReset()) {
        _canFire = true;
    }
}

void Ufo::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    GameEntity::Update(deltaSeconds);
    _timeSinceLastHit += deltaSeconds;
    _sprite->Update(deltaSeconds);

    if(_canFire) {
        OnFire();
    }

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};
    const auto scale = GetScaleFromType(_type);
    {
        const auto S = Matrix4::CreateScaleMatrix(scale * half_extents);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        m_transform = Matrix4::MakeSRT(S, R, T);
    }

    auto& builder = m_mesh_builder;
    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(Rgba::White);

    builder.SetUV(Vector2{uvs.maxs.x, uvs.maxs.y});
    builder.AddVertex(Vector2{+0.5f, +0.5f});

    builder.SetUV(Vector2{uvs.mins.x, uvs.maxs.y});
    builder.AddVertex(Vector2{-0.5f, +0.5f});

    builder.SetUV(Vector2{uvs.mins.x, uvs.mins.y});
    builder.AddVertex(Vector2{-0.5f, -0.5f});

    builder.SetUV(Vector2{uvs.maxs.x, uvs.mins.y});
    builder.AddVertex(Vector2{+0.5f, -0.5f});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(GetMaterial());

}

void Ufo::Render() const noexcept {
    ufo_state.wasHitUfoIndex.x = WasHit();
    ufo_state.wasHitUfoIndex.y = GetUfoIndexFromStyle(_style);
    ufo_state_cb->Update(*ServiceLocator::get<IRendererService>().GetDeviceContext(), &ufo_state);
    GameEntity::Render();
}

void Ufo::EndFrame() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(const auto& found = std::find(std::begin(game->ufos), std::end(game->ufos), this);
            (found != std::end(game->ufos) &&
                (*found)->IsDead()))
        {
            *found = nullptr;
        }
        GameEntity::EndFrame();
    }
}

void Ufo::OnCreate() noexcept {
    AudioSystem::SoundDesc desc{};
    desc.volume = 1.0f;
    desc.frequency = 1.0f;
    desc.loopCount = -1;
    desc.groupName = g_audiogroup_sound;
    _warble_sound = g_theAudioSystem->CreateSound(g_sound_warblepath);
    g_theAudioSystem->Play(*_warble_sound, desc);
}

void Ufo::OnCollision(GameEntity* a, GameEntity* b) noexcept {
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case GameEntity::Faction::Player:
    {
        if(const auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
            a->DecrementHealth();
            OnHit();
        }
        if(const auto* asMine = dynamic_cast<Mine*>(b); asMine != nullptr) {
            a->Kill();
        }
        break;
    }
    default:
        break;
    }
}

void Ufo::OnFire() noexcept {
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ufo::OnHit() noexcept {
    if(TimeUtils::FPFrames{1.0f} < _timeSinceLastHit) {
        _timeSinceLastHit = _timeSinceLastHit.zero();
    }
    AudioSystem::SoundDesc desc{};
    desc.groupName = g_audiogroup_sound;
    g_theAudioSystem->Play(g_sound_hitpath, desc);
    ufo_state.wasHitUfoIndex.x = WasHit();
}

void Ufo::OnDestroy() noexcept {
    for(auto* channel : _warble_sound->GetChannels()) {
        channel->Stop();
    }
    GameEntity::OnDestroy();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->MakeExplosion(GetPosition());
    }
}

float Ufo::WasHit() const noexcept {
    return _timeSinceLastHit.count() == 0.0f ? 1.0f : 0.0f;
}

void Ufo::MakeBullet() const noexcept {
    const auto source = GetPosition();
    const auto angle = (_fireTarget - source).CalcHeadingDegrees();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->MakeBullet(this, source, Vector2::CreateFromPolarCoordinatesDegrees(_bulletSpeed, angle));
    }
}

float Ufo::GetCosmeticRadiusFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 15.0f;
    case Type::Big: return 30.0f;
    case Type::Boss: return 60.0f;
    default: return 15.0f;
    }
}

float Ufo::GetPhysicalRadiusFromType(Type type) noexcept {
    const auto cr = GetCosmeticRadiusFromType(type);
    switch(type) {
    case Type::Small: return 10.0f;
    case Type::Big: return  20.0f;
    case Type::Boss: return  30.0f;
    default: return cr;
    }
}

float Ufo::GetScaleFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1.0f;
    case Type::Big: return  2.0f;
    case Type::Boss: return  4.0f;
    default: return 1.0f;
    }
}

std::weak_ptr<SpriteSheet> Ufo::GetSpriteSheet() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        return game->ufo_sheet;
    }
    return {};
}

unsigned int Ufo::GetFireRateFromTypeAndDifficulty(Type type) noexcept {
    switch(type) {
    case Type::Small: return 2u;
    case Type::Big: return 1u;
    case Type::Boss: return 3u;
    default: return 0u;
    }
}

float Ufo::GetBulletSpeedFromTypeAndDifficulty(Type type) const noexcept {
    const auto typeMultiplier = [type]()->float {
        switch(type) {
        case Type::Small: return 2.0f;
        case Type::Big: return 1.0f;
        case Type::Boss: return 3.0f;
        default: return 1.0f;
        }
    }();
    const auto difficultyMultiplier = []() -> float{
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            switch(game->gameOptions.GetDifficulty()) {
            case Difficulty::Easy: return 0.50f;
            case Difficulty::Normal: return 1.0f;
            case Difficulty::Hard: return 2.0f;
            default: return 1.0f;
            };
        }
        return 1.0f;
    }();
    return difficultyMultiplier * typeMultiplier * _bulletSpeed;
}

Vector2 Ufo::CalculateFireTarget() const noexcept {
    const auto defaultTarget = GetPosition() + Vector2::CreateFromPolarCoordinatesDegrees(1.0f, MathUtils::GetRandomInRange<float>(0.0f, 359.0f));
    switch(_type) {
    case Type::Small: {
        const auto loc = [this, &defaultTarget]() {
            if(auto* game = GetGameAs<Game>(); game != nullptr) {
                if(auto ship = game->GetShip()) {
                    return ship->GetPosition();
                }
            }
            return defaultTarget;
        }();
        return loc;
    }
    case Type::Big:
    {
        return defaultTarget;
    }
    case Type::Boss:
    {
        const auto loc = [this, &defaultTarget]() {
            if(auto* game = GetGameAs<Game>(); game != nullptr) {
                if(const auto ship = game->GetShip()) {
                    const auto target = ship->GetPosition();
                    const auto source = GetPosition();
                    const auto angle = (target - source).CalcHeadingDegrees();
                    const auto offset_range = 90.0f;
                    const auto offset = MathUtils::GetRandomNegOneToOne<float>() * offset_range;
                    return GetPosition() + Vector2::CreateFromPolarCoordinatesDegrees(1.0f, angle + offset);
                }
            }
            return defaultTarget;
        }();
        return loc;
    }
    default: return defaultTarget;
    }
}

float Ufo::GetUfoIndexFromStyle(Style style) noexcept {
    switch(style) {
    case Style::Blue: return 0.0f;
    case Style::Green: return 1.0f;
    case Style::Yellow: return 2.0f;
    //case Style::Cyan:
    //    break;
    //case Style::Magenta:
    //    break;
    //case Style::Last_Big:
    //    break;
    //case Style::First_Boss:
    //    break;
    //case Style::Orange:
    //    break;
    //case Style::Last_Boss:
    //    break;
    default: return 0.0f;
    }
}

int Ufo::GetHealthFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1;
    case Type::Big: return 2;
    case Type::Boss: return 3;
    default: return 1;
    }
}

Material* Ufo::GetMaterial() const noexcept {
    return g_theRenderer->GetMaterial("ufo");
}

int Ufo::GetStartIndexFromTypeAndStyle(Type type, Style style) noexcept {
    switch(type) {
    case Type::Small:
    {
        switch(style) {
        case Style::Blue: return 0;
        case Style::Green: return 4;
        case Style::Yellow: return 8;
        }
    }
    case Type::Big:
    {
        switch(style) {
        case Style::Cyan: return 0;
        case Style::Magenta: return 4;
        default: return 4;
        }
    }
    case Type::Boss:
    {
        switch(style) {
        case Style::Orange: return 0;
        default: return 0;
        }
    }
    default: return 0;
    }
}

int Ufo::GetFrameLengthFromTypeAndStyle(Type type, Style style) noexcept {
    switch(type) {
    case Type::Small:
    {
        switch(style) {
        case Style::Blue: return 4;
        case Style::Green: return 4;
        case Style::Yellow: return 4;
        default: return 4;
        }
    }
    case Type::Big:
    {
        switch(style) {
        case Style::Cyan: return 4;
        case Style::Magenta: return 4;
        default: return 4;
        }
    }
    case Type::Boss:
    {
        switch(style) {
        case Style::Orange: return 8;
        default: return 8;
        }
    }
    default: return 4;
    }
}

long long Ufo::GetValueFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1000LL;
    case Type::Big: return 200LL;
    case Type::Boss: return 2000LL;
    default: return 0LL;
    }
}

Ufo::Style Ufo::GetStyleFromType(Type type) noexcept {
    switch(type) {
    case Type::Small:
    {
        const auto i = MathUtils::GetRandomInRange<int>(static_cast<int>(Style::First_Small), static_cast<int>(Style::Last_Small));
        return static_cast<Style>(i);
    }
    case Type::Big:
    {
        const auto i = MathUtils::GetRandomInRange<int>(static_cast<int>(Style::First_Big), static_cast<int>(Style::Last_Big));
        return static_cast<Style>(i);
    }
    case Type::Boss:
    {
        const auto i = MathUtils::GetRandomInRange<int>(static_cast<int>(Style::First_Boss), static_cast<int>(Style::Last_Boss));
        return static_cast<Style>(i);
    }
    default:
    {
        return Style::Blue;
    }
    }
}


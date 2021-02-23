
struct GraphicsOptions {
    float WindowWidth = 1600.0f;
    float WindowHeight = 900.0f;
    float WindowAspectRatio = WindowWidth / WindowHeight;
    float Fov = 70.0f;
    float MaxShakeAngle = 10.0f;
    float MaxShakeOffsetHorizontal = 50.0f;
    float MaxShakeOffsetVertical = 50.0f;
    float MaxMouseSensitivityX = 0.1f;
    float MaxShakeSensitivityY = 0.1f;
    bool InvertMouseY = false;
    bool InvertMouseX = false;
    bool vsync = true;
};

static GraphicsOptions defaultGraphicsOptions{};
static GraphicsOptions currentGraphicsOptions{};

static std::string g_options_filepath{"Data/Config/options.config"};
static std::string g_options_str{
R"(difficulty=normal
controlpref=mouse
sound=5
music=5
cameraShakeStrength=1.0
maxShakeOffsetHorizontal=25.0
maxShakeOffsetVertical=25.0
maxShakeAngle=2.5
)"
};

static std::string g_title_str{"Asteroids"};
static std::string g_audiogroup_sound{"sound"};
static std::string g_audiogroup_music{"music"};
static std::string g_sound_folderpath{"Data/Audio/Sound/"};
static std::string g_sound_shootpath{"Data/Audio/Sound/Laser_Shoot.wav"};
static std::string g_sound_hitpath{"Data/Audio/Sound/Hit.wav"};
static std::string g_sound_explosionpath{"Data/Audio/Sound/Explosion.wav"};
static std::string g_sound_warblepath{"Data/Audio/Sound/Warble.wav"};
static std::string g_music_folderpath{"Data/Audio/Music/"};
static std::string g_music_bgmpath{"Data/Audio/Music/bgm.wav"};
static std::string g_material_folderpath{"Data/Materials/"};



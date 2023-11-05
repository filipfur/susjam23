#pragma once

#include "glapplication.h"
#include "pipeline.h"
#include "glmesh.h"

class App : public lithium::Application
{
public:
    App();

    virtual ~App() noexcept;

    virtual void update(float dt) override;

    virtual void onWindowSizeChanged(int width, int height) override;

protected:
    virtual void onFpsCount(int fps) override
    {
        //printf("FPS: %d\n", fps);
    }

private:
    std::shared_ptr<Pipeline> _pipeline{nullptr};
    std::vector<std::shared_ptr<lithium::Object>> _objects;
    std::shared_ptr<lithium::Object> _background;
    float _cameraYaw{0.0f};
    float _cameraPitch{0.0f};
    glm::vec3 _cameraTarget{0.0f};
    std::shared_ptr<lithium::Input::KeyCache> _keyCache;

    glm::vec3 _playerPos{-0.5f, 0.0f, 0.0f};
    glm::vec2 _playerVel;

    bool _playerCrawling{false};
    
    enum class JumpState
    {
        GROUNDED,
        JUMPING,
        FALLING
    } _playerJumpState{JumpState::GROUNDED};

    struct Projectile
    {
        glm::vec2 position;
        glm::vec2 velocity;
        bool used{false};
        bool inactive{false};
    };

    struct Collectable
    {
        glm::vec2 position;
        bool used{false};
        bool inactive{false};
        bool picked{false};
        float picking{0.0f};
    };

    struct Enemy
    {
        glm::vec2 position;
        bool used{false};
        bool inactive{false};
        bool facingLeft{false};
        bool chasingPlayer{false};
        float deathTimer{0.0f};
        int health{1};
    };

    Projectile _playerProjectiles[10];
    Collectable _collectables[10];
    Enemy _enemies[10];

    float _shakeTimer{0.0f};
    float _shake{0.0f};
};

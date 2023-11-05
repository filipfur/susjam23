#include "app.h"

#include "glplane.h"

App::App() : Application{"lithium-lab", glm::ivec2{1440, 800}, lithium::Application::Mode::MULTISAMPLED_4X, false},
    _collectables{}, _enemies{}
{
    // Create the render pipeline
    _pipeline = std::make_shared<Pipeline>(defaultFrameBufferResolution());

    // Create and add a background plane to the render pipeline, and stage it for rendering.
    _background = std::make_shared<lithium::Object>(std::shared_ptr<lithium::Mesh>(lithium::Plane2D()),
        std::vector<lithium::Object::TexturePointer>{});
    _background->setGroupId(Pipeline::BACKGROUND);
    _pipeline->attach(_background.get());
    _background->stage();

    // Key cache for rotating the camera left and right.
    _keyCache = std::make_shared<lithium::Input::KeyCache>(
        std::initializer_list<int>{GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN,
            GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_SPACE});
    input()->setKeyCache(_keyCache);

    // Escape key to close the application.
    input()->addPressedCallback(GLFW_KEY_ESCAPE, [this](int key, int mods) {
        this->close();
        return true;
    });

    input()->addPressedCallback(GLFW_KEY_LEFT_CONTROL, [this](int key, int mods) {
        _playerCrawling = true;
        return true;
    });

    input()->addReleasedCallback(GLFW_KEY_LEFT_CONTROL, [this](int key, int mods) {
        _playerCrawling = false;
        return true;
    });

    input()->addPressedCallback(GLFW_KEY_Q, [this](int key, int mods) {
        for(auto& p : _playerProjectiles)
        {
            if(!p.used)
            {
                p.used = true;
                p.inactive = false;
                p.position = _playerPos;
                p.velocity = glm::vec2(_playerPos.z * 1.0f, 0.0f);
                break;
            }
        }
        return true;
    });

    input()->addPressedCallback(GLFW_KEY_K, [this](int key, int mods) {
        _shakeTimer = 0.2f;
        return true;
    });

    int next = 0;
    for(auto &c : {
        glm::vec2(1.2f, 0.08f),
        glm::vec2(1.3f, 0.08f),
        glm::vec2(1.4f, 0.08f),
        glm::vec2(1.8f, 0.32f),
        glm::vec2(2.0f, 0.32f),
        glm::vec2(6.0f, 0.08f),
        glm::vec2(6.1f, 0.08f),
        glm::vec2(6.1f, 0.16f),
        glm::vec2(6.2f, 0.08f),
    })
    {
        _collectables[next].used = true;
        _collectables[next].position = c;
        next++;
    }

    next = 0;
    for(auto &c : {
        glm::vec2(0.5f, 0.0f),
        glm::vec2(5.0f, 0.0f),
    })
    {
        _enemies[next].used = true;
        _enemies[next].position = c;
        next++;
    }

    _background->setShaderCallback([this](lithium::Renderable* r, lithium::ShaderProgram* sp) {
        sp->setUniform("u_playerPos", _playerPos);

        for(int index=0; index < 10; ++index)
        {
            auto& p = _playerProjectiles[index];
            if(p.inactive && p.used == false)
            {
                continue;
            }
            p.inactive = false;
            const std::string label = "u_projectiles[" + std::to_string(index) + "]";
            sp->setUniform(label + ".used", p.used);
            sp->setUniform(label + ".position", p.position);
            p.inactive = !p.used;
        }
        for(int index=0; index < 10; ++index)
        {
            auto& c = _collectables[index];
            if(c.inactive && c.used == false)
            {
                continue;
            }
            c.inactive = false;
            const std::string label = "u_collectables[" + std::to_string(index) + "]";
            sp->setUniform(label + ".used", c.used);
            sp->setUniform(label + ".position", c.position);
        }
        for(int index=0; index < 10; ++index)
        {
            auto& e = _enemies[index];
            if(e.inactive && e.used == false)
            {
                continue;
            }
            e.inactive = false;
            const std::string label = "u_enemies[" + std::to_string(index) + "]";
            sp->setUniform(label + ".used", e.used);
            sp->setUniform(label + ".position", e.position);
            sp->setUniform(label + ".facingLeft", e.facingLeft);
            sp->setUniform(label + ".chasingPlayer", e.chasingPlayer);
            sp->setUniform(label + ".deathTimer", e.deathTimer);
        }
        sp->setUniform("u_shake", _shake);
    });

    // Set the camera oirigin position and target.
    _pipeline->camera()->setTarget(_cameraTarget);

    setMaxFps(120.0f);

    printf("%s\n", glGetString(GL_VERSION));
}

App::~App() noexcept
{
    _pipeline = nullptr;
    _background = nullptr;
    _objects.clear();
}

void App::update(float dt)
{
    lithium::Updateable::update(dt);
    // Apply a rotation to the cube.
    for(auto o : _objects)
    {
        o->update(dt);
        /*o->setQuaternion(o->quaternion() * glm::angleAxis(0.5f * dt, glm::vec3(1,0,0))
            * glm::angleAxis(0.5f * dt, glm::vec3(0,1,0))
            * glm::angleAxis(0.5f * dt, glm::vec3(0,0,1)));*/
    }

    // Rotate the camera around the origin on player input.
    if(_keyCache->isPressed(GLFW_KEY_LEFT))
    {
        _cameraYaw += glm::pi<float>() * 0.5f * dt;
    }
    else if(_keyCache->isPressed(GLFW_KEY_RIGHT))
    {
        _cameraYaw -= glm::pi<float>() * 0.5f * dt;

    }

    for(auto& p : _playerProjectiles)
    {
        if(p.used)
        {
            p.position += p.velocity * dt;
            if(abs(p.position.x - _playerPos.x) > 4.0f)
            {
                p.inactive = false;
                p.used = false;
            }

            for(auto& e : _enemies)
            {
                if(e.used)
                {
                    glm::vec2 delta = glm::vec2(p.position.x, p.position.y) - e.position;
                    if(delta.x * delta.x + delta.y * delta.y < 0.005f)
                    {
                        p.inactive = false;
                        p.used = false;
                        e.health -= 1;
                        break;
                    }
                }
            }
        }
    }

    for(auto& c : _collectables)
    {
        if(c.used)
        {
            if(c.picked)
            {
                c.picking -= dt;
                c.position.y += 1.0f * dt;
                if(c.picking <= 0)
                {
                    c.picking = 0.0f;
                    c.picked = false;
                    c.used = false;
                }
            }
            else
            {
                float dx = c.position.x - _playerPos.x;
                float dy = c.position.y - _playerPos.y + 0.1f;
                if(abs(dx) < 0.05f && abs(dy) < 0.25f)
                {
                    c.picked = true;
                    c.picking = 0.16f;
                }
            }
        }
    }

    for(auto& e : _enemies)
    {
        if(e.used)
        {
            glm::vec2 delta = glm::vec2(_playerPos.x, _playerPos.y) - e.position;
            if(delta.x * delta.x + delta.y * delta.y < 0.005f)
            {
                _shakeTimer = 0.3f;
                e.used = false;
                continue;
            }
            float dx;
            if(e.health <= 0)
            {
                if(e.deathTimer == 0.0f)
                {
                    e.deathTimer = 0.4f;
                }
                e.deathTimer -= dt;
                if(e.deathTimer <= 0)
                {
                    e.used = false;
                }
            }
            else if(e.chasingPlayer)
            {
                dx = _playerPos.x - e.position.x;
                e.position.x += glm::sign(dx) * 0.4f * dt;
            }
            else
            {
                dx = sin(time()) * 0.2f;
                e.position.x += dx * dt;
            }
            e.facingLeft = dx < 0;

            float pdx = _playerPos.x - e.position.x;
            if(e.facingLeft && pdx > -0.5f && pdx < 0)
            {
                e.chasingPlayer = true;
            }
        }
    }

    if(_keyCache->isPressed(GLFW_KEY_A) && _playerVel.x <= 0.0f)
    {
        _playerVel.x -= 2.4f * dt;
        _playerVel.x = std::max(_playerVel.x, -1.0f);
        _playerPos.z = -1.0f;
    }
    else if(_keyCache->isPressed(GLFW_KEY_D) && _playerVel.x >= 0.0f)
    {
        _playerVel.x += 2.4f * dt;
        _playerVel.x = std::min(_playerVel.x, 1.0f);
        _playerPos.z = 1.0f;
    }
    else
    {
        _playerVel.x = glm::mix(_playerVel.x, 0.0f, 12.0f * dt);
        if(_playerVel.x * _playerVel.x < 0.01f)
        {
            _playerVel.x = 0.0f;
        }
    }

    if(_keyCache->isPressed(GLFW_KEY_SPACE))
    {
        if(_playerJumpState == JumpState::GROUNDED)
        {
            _playerJumpState = JumpState::JUMPING;
            _playerVel.y = 2.0f;
        }
    }

    _playerPos.x += _playerVel.x * dt;
    _playerPos.y += _playerVel.y * dt;

    if(_playerPos.x < -0.94f)
    {
        _playerPos.x = -0.94f;
        if(_playerVel.x < 0)
        {
            _shakeTimer = -_playerVel.x * 0.32f;
            _playerVel.x = -_playerVel.x;
        }
    }

    if(_playerPos.y > 0 && _playerJumpState != JumpState::GROUNDED)
    {
        _playerVel.y -= 10.0f * dt;
        if(_playerVel.y < 0)
        {
            _playerJumpState = JumpState::FALLING;
        }
    }
    else
    {
        if(!_playerCrawling)
            _playerPos.y = 0.0f;
        _playerVel.y = 0.0f;
        _playerJumpState = JumpState::GROUNDED;
    }

    if(_playerJumpState == JumpState::GROUNDED)
    {
        if(_playerCrawling)
        {
            _playerPos.y -= 0.5f * dt;
            _playerPos.y = std::max(_playerPos.y, -0.04f);
        }
        else
        {
            _playerPos.y += 0.5f * dt;
            _playerPos.y = std::min(_playerPos.y, 0.0f);
        }
    }

    if(_keyCache->isPressed(GLFW_KEY_UP))
    {
        _cameraPitch += glm::pi<float>() * 0.5f * dt;
    }
    else if(_keyCache->isPressed(GLFW_KEY_DOWN))
    {
        _cameraPitch -= glm::pi<float>() * 0.5f * dt;
    }

    if(_shakeTimer > 0)
    {
        _shakeTimer -= dt;
        _shake = rand() % 1000000 * 0.000001f;
        if(_shakeTimer <= 0)
        {
            _shake = 0.0f;
            _shakeTimer = 0.0f;
        }
    }

    static const float cameraRadius = 8.0f;

    glm::vec3 cameraPosition;
    cameraPosition.x = _cameraTarget.x + cameraRadius * cos(_cameraYaw) * cos(_cameraPitch);
    cameraPosition.y = _cameraTarget.y + cameraRadius * sin(_cameraPitch);
    cameraPosition.z = _cameraTarget.z + cameraRadius * sin(_cameraYaw) * cos(_cameraPitch);

    _pipeline->setTime(time());

    _pipeline->camera()->setPosition(cameraPosition);
    _pipeline->render();
}

void App::onWindowSizeChanged(int width, int height)
{
    _pipeline->setResolution(glm::ivec2{width, height});
}
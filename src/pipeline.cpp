#include "pipeline.h"
#include "glplane.h"

namespace
{
    const char* vertSrc = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 texCoord;
void main()
{
    texCoord = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

    const char* fragSrc = R"(
#version 330 core

out vec4 fragColor;

in vec2 texCoord;

struct Projectile
{
    vec2 position;
    vec2 velocity;
    bool used;
};

struct Collectable
{
    vec2 position;
    bool used;
};

struct Enemy
{
    vec2 position;
    bool used;
    bool facingLeft;
    bool chasingPlayer;
    float deathTimer;
};

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_playerPos;
uniform float u_shake;
uniform Projectile u_projectiles[10];
uniform Collectable u_collectables[10];
uniform Enemy u_enemies[10];

const vec3 bgColor = vec3(0.0, 0.5, 1.0);
const vec3 fgColor = vec3(1.0, 1.0, 1.0);
const float lineRadius = 0.006;

float waveSuperposition(float x) {
    float y = 0.0;

    // Define wave properties
    float frequencies[3] = float[](2.0, 4.0, 0.5);
    float amplitudes[3] = float[](0.002, 0.001, 0.003);
    float phases[3] = float[](0.0, 1.0, 0.5);

    // Sum multiple sine waves
    for (int j = 0; j < 3; ++j) {
        y += amplitudes[j] * sin(frequencies[j] * 16.0 * x * 2.0 * 3.1415926535897932384626433832795 + phases[j] * u_time * 16.0);
    }

    return y;
}

void main()
{
    vec2 st = gl_FragCoord.xy / u_resolution.xy;
    st.x *= u_resolution.x / u_resolution.y;

    st.x -= 0.5;

    vec2 uv = st;

    bool facingLeft = u_playerPos.z < 0;

    //st.x += sin(u_time) * 0.2;

    float posX = 0.5;

    //posX += cos(u_time) * 0.2;'

    float delta = sin(u_time) * 0.5;
    delta = -u_playerPos.x;
    //delta = 0.0;

    st.x += int(u_playerPos.x + 0.5);
    uv.x += int(u_playerPos.x + 0.5);

    //bool facingLeft = delta < 0.0;
    /*st.y += mix(sin(st.x) * 0.1, sin(st.x * 16.0) * 0.02, smoothstep(0.5, 1.0, st.x))
        + mix(0.0, st.x * st.x * 0.05, max(4.0 - st.x, 0.0));*/

    st.y += sin(st.x * 64.0 * cos(27.0 * st.x) * u_shake) * 0.01 * u_shake;

    if(st.x > 5.0)
    {
        //st.y += mix(0.0, sin(st.x * 64.0) * 0.01, max(6.0 - st.x, 0.0));
        st.y += mix(0.0, waveSuperposition(st.x), max(6.0 - st.x, 0.0));
    }
    else if(st.x > 3.5)
    {
        //st.y += mix(0.0, sin(st.x * 64.0) * 0.01, min(st.x - 3.5, 1.0));
        st.y += waveSuperposition(st.x);
    }
    else if(st.x > 3.4)
    {
        st.y += -0.2 + (st.x - 3.4) * 2.0;
    }
    else if(st.x > 3.0)
    {
        st.y -= 0.2;
    }
    else if(st.x > 2.6)
    {
        st.y -= -0.2 + (st.x - 2.6);
    }
    else if(st.x > 0.6)
    {
        st.y += 0.2;
    }
    else if(st.x > 0.4)
    {
        st.y += (st.x - 0.4);   
    }

    float projectileRadius = 0.05;
    for(int i=0; i < 10; ++i)
    {
        if(u_projectiles[i].used)
        {
            vec2 p = u_projectiles[i].position + 0.5;
            vec2 v = u_projectiles[i].velocity;

            if(p.x > st.x - projectileRadius && p.x < st.x + projectileRadius)
            {
                if(p.y > st.y - projectileRadius && p.y < st.y + projectileRadius)
                {
                    st.y -= cos(abs(st.x - p.x) / 0.025) * 0.025 + 0.01;
                }
            }
        }
    }

    for(int i=0; i < 10; ++i)
    {
        float enemyRadius = 0.05;
        float scale = u_enemies[i].deathTimer / 0.4;
        if(u_enemies[i].deathTimer > 0)
        {
            enemyRadius *= scale;
        }
        if(u_enemies[i].used)
        {
            vec2 p = u_enemies[i].position + 0.5;

            if(p.x > st.x - enemyRadius && p.x < st.x + enemyRadius)
            {
                if(p.y > st.y - enemyRadius * 2.5 && p.y < st.y + enemyRadius)
                {
                    st.y -= cos(abs(st.x - p.x) / (enemyRadius * 0.5)) * enemyRadius * 1.5 + enemyRadius * 0.68;
                    float frtime = fract(u_time);
                    if(u_enemies[i].chasingPlayer || frtime < 0.3)
                    {
                        st.y += sin(st.x * 64.0 * cos(27.0 * st.x) * u_time) * 0.01 * (frtime + 0.2);
                    }
                }
            }
        }
    }

    for(int i=0; i < 10; ++i)
    {
        float collectableRadius = 0.008;
        if(u_collectables[i].used)
        {
            vec2 p = u_collectables[i].position + 0.5;
            p.y += sin(u_time * 8.0) * 0.01;
            collectableRadius += sin(u_time * 8.0) * 0.002 + 0.001;
            vec2 d = p - st;
            if(p.x > st.x - collectableRadius && p.x < st.x + collectableRadius)
            {
                if(p.y > st.y - collectableRadius && p.y < st.y + collectableRadius)
                {
                    if(length(d) < collectableRadius + 0.0016)
                    {
                        fragColor = vec4(1.0, 1.0, 0.0, 1.0);
                        return;
                    }
                }
            }
        }
    }

    st.x += delta;

    float a = max(-u_playerPos.y, 0.0);

    st.y += mix(0.0, sin(st.x * 10.0) * 0.15 - u_playerPos.y,
        smoothstep(posX - mix(0.07 + a, 0.04 + a + smoothstep(0.58, 0.61, st.y - u_playerPos.y) * 0.08, facingLeft), posX, st.x)
        - smoothstep(posX, posX + mix(0.04 + a + smoothstep(0.58, 0.61, st.y - u_playerPos.y) * 0.16, 0.07 + a, facingLeft), st.x));

    float x = smoothstep(0.5, 0.501, length(st.y + lineRadius))
        - smoothstep(0.5, 0.501, length(st.y - lineRadius));


    uv.y += sin(uv.x) * 0.3;
    x += smoothstep(1.0, 1.001, length(uv.y + lineRadius))
        - smoothstep(1.0, 1.001, length(uv.y - lineRadius));
        
    fragColor = vec4(mix(bgColor, fgColor, x), 1.0);
    fragColor.rgb = mix(fragColor.rgb, 1.0 - fragColor.rgb, u_shake);


    /*float hitBox = (step(-0.43, uv.x) - step(-0.16, uv.x)) * mod(uv.x * 64.0, 2);

    fragColor.rgb = mix(fragColor.rgb, vec3(1,0,0),
        (smoothstep(0.96, 0.961, length(uv.y + lineRadius))
        - smoothstep(0.96, 0.961, length(uv.y - lineRadius))) * hitBox);*/
}
)";

}

Pipeline::Pipeline(const glm::ivec2& resolution) : lithium::RenderPipeline{resolution},
    _camera{new lithium::SimpleCamera(glm::perspective(glm::radians(45.0f), (float)resolution.x / (float)resolution.y, 0.1f, 100.0f))}
{
    enableDepthTesting();
    enableBlending();
    blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    enableMultisampling();

    _screenShader = std::make_shared<lithium::ShaderProgram>("shaders/screen.vert", "shaders/screen.frag");

    /*
    _screenShader = std::make_shared<lithium::ShaderProgram>(
        std::shared_ptr<lithium::VertexShader>(lithium::VertexShader::fromSource(vertSrc)),
        std::shared_ptr<lithium::FragmentShader>(lithium::FragmentShader::fromSource(fragSrc)));
    */

    _screenMesh = std::shared_ptr<lithium::Mesh>(lithium::Plane2D());

    _screenGroup = createRenderGroup([this](lithium::Renderable* renderable) -> bool {
        return renderable->groupId() == BACKGROUND;
    });

    clearColor(0.0f, 0.0f, 0.0f, 1.0f);

    _mainStage = addRenderStage(std::make_shared<lithium::RenderStage>(nullptr, [this](){
        static const GLuint viewOffset{static_cast<GLuint>(sizeof(glm::mat4))};
        static const GLuint eyePosOffset{static_cast<GLuint>(sizeof(glm::mat4) * 2)};

        clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        disableDepthWriting();
        _screenShader->setUniform("u_resolution", glm::vec2(this->resolution()));
        _screenShader->setUniform("u_time", time());
        _screenGroup->render(_screenShader.get());
        enableDepthWriting();
    }));

}

void Pipeline::setResolution(const glm::ivec2& resolution)
{
    lithium::RenderPipeline::setResolution(resolution);
}

Pipeline::~Pipeline() noexcept
{
    _screenShader = nullptr;
    _screenMesh = nullptr;
}
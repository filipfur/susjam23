#pragma once

#include <memory>
#include "glsimplecamera.h"
#include "glrenderpipeline.h"
#include "glframebuffer.h"
#include "gluniformbufferobject.h"

class Pipeline : public lithium::RenderPipeline
{
public:
    enum Group
    {
        DEFAULT,
        BACKGROUND
    };

    Pipeline(const glm::ivec2& resolution);

    ~Pipeline() noexcept;

    std::shared_ptr<lithium::SimpleCamera> camera()
    {
        return _camera;
    }

    virtual void setResolution(const glm::ivec2& resolution) override;

    void setTime(float time)
    {
        _time = time;
    }

    float time() const
    {
        return _time;
    }

private:
    /* Shaders */
    std::shared_ptr<lithium::ShaderProgram> _screenShader{nullptr};
    std::shared_ptr<lithium::SimpleCamera> _camera{nullptr};

    /*Render groups*/
    std::shared_ptr<lithium::RenderGroup> _screenGroup;

    /*Render stages*/
    std::shared_ptr<lithium::RenderStage> _mainStage;
    
    /* Meshes */
    std::shared_ptr<lithium::Mesh> _screenMesh;

    float _time{0.0f};
};
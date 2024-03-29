#ifndef FLUX_GL_RENDERER_HH
#define FLUX_GL_RENDERER_HH

// Flux includes
#include "Flux/ECS.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "glm/fwd.hpp"

// GL includes
#include <glad/glad.h>
#include <glm/glm.hpp>

// STL includes
#include <string>

namespace Flux { namespace GLRenderer {

    /**
    Context struct with all the data used by the GL renderer
    */
    struct GLCtx
    {
        // GLFWwindow* window;

        int width;
        int height;
        std::string title;

        int mouse_mode;

        glm::vec2 offset;
        glm::vec2 mouse_pos;
    };

    extern GLCtx* current_window;

    /**
    Component where GLRenderer stores all GL-related data
    */
    struct GLMeshCom: Component
    {
        FLUX_COMPONENT(GLMeshCom, glmesh);
        ~GLMeshCom();

        uint32_t VBO;
        uint32_t IBO;
        uint32_t VAO;

        uint32_t num_vertices;
        uint32_t num_indices;

        uint32_t draw_type;
    };

    /**
    Component for shaders
    */
    struct GLShaderCom: Component
    {
        FLUX_COMPONENT(GLShaderCom, glshader);
        ~GLShaderCom();

        // Shader stuff
        uint32_t shader_program;

        // Special high priority uniform locations
        uint32_t mvp_location;
        uint32_t mv_location;
        uint32_t m_location;
        uint32_t cam_pos_location;
        uint32_t has_texture_location;

        uint32_t light_indexes_location;
    };
    
    /** Little struct for storing info on textures */
    struct GLTextureStore
    {
        int location;
        Flux::Resources::ResourceRef<Flux::Renderer::TextureRes> resource;
    };

    /**
    Component that keeps track of uniform positions.
    */
    struct GLUniformCom: Component
    {
        // More info: Open GL 4 Shading Language Cookbook, page 57
        FLUX_COMPONENT(GLUniformCom, gluniform);

        int block_size;

        std::vector<uint32_t> indices;

        std::vector<int> offset;

        /** OpenGL handle for the uniform buffer */
        uint32_t handle;

        /** Textures */
        std::vector<GLTextureStore> textures;
    };

    /**
    Component that keeps track of the GL Objects of textures
    */
    struct GLTextureCom: public Component
    {
        FLUX_COMPONENT(GLTextureCom, gltexture);

        uint32_t handle;
    };

    /**
    Little component that tells the renderer that GL has already been setup
    */
    struct GLEntityCom: Component
    {
        FLUX_COMPONENT(GLEntityCom, glentity);
    };

    /**
    Creates the window, and the GL context.
    For the GLRenderer, there can only be one context, so it is stored globally
    */
    void createWindow(const int& width, const int& height, const std::string& title);

    /**
    Sets up the OpenGL context
    */
    void _startGL();

    /**
    Close the current window, and free all it's resources
    */
    void destroyWindow();

    /**
    Tells the renderer to start the process or rendering a frame. 
    Should be called before any systems.
    If this function returns true, then the frame can be drawn. If it returns false, the window has been destroyed
    */
    bool startFrame();

    /**
    The windowing system's part of starting a frame
    */
    bool _windowStartFrame();

    /**
    The windowing system's part of ending a frame
    */
    void _windowEndFrame();
    
    /**
    Tells the renderer that the frame is over
    */
    void endFrame();

    /**
    Adds the rendering system to the given ECS.
    This allows for entities to have a visual component.
    Returns the system ID
    */
    int addGLRenderer(ECSCtx* ctx);

    class GLRendererSystem: public System
    {
    private:
        void initGLMaterial(Flux::Renderer::MeshCom* mesh);
        void dealWithUniforms(Flux::Renderer::MeshCom* mesh, Flux::Renderer::MaterialRes* mat_res, GLShaderCom* shader_res);
        void dealWithLights();
        bool setup_lighting = false;

        glm::mat4 projection;

        Renderer::LightSystem* lights;
        uint32_t light_buffer;

    public:
        GLRendererSystem();
        void onSystemAdded(ECSCtx* ctx) override;
        void onSystemStart() override;

        void runSystem(EntityRef entity, float delta) override;

    };
    

}}

// FLUX_DEFINE_COMPONENT(Flux::GLRenderer::GLMeshCom, gl_mesh);
// FLUX_DEFINE_COMPONENT(Flux::GLRenderer::GLShaderCom, gl_shader);
// FLUX_DEFINE_COMPONENT(Flux::GLRenderer::GLUniformCom, gl_uniform);
// FLUX_DEFINE_COMPONENT(Flux::GLRenderer::GLEntityCom, gl_entity);

#endif
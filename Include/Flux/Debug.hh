#ifndef FLUX_DEBUG_HH
#define FLUX_DEBUG_HH

#include <vector>
#include "Flux/ECS.hh"
#include "Flux/Renderer.hh"
#include "glm/glm.hpp"

namespace Flux { namespace Debug 
{
    void enableDebugDraw(ECSCtx* ctx);

    void run(float delta);

    enum Colors
    {
        Red, Green, Blue, Orange, Pink, Purple, Black, White, Brown
    };

    void drawMesh(std::vector<Renderer::Vertex> vertices, std::vector<uint32_t> indices, Colors color, bool wireframe=false);

    void drawSphere(const glm::vec3& pos, float radius, Colors color, bool wireframe=false, int lat_lines = 10, int lon_lines = 10);

    void drawPoint(const glm::vec3& pos, float radius, Colors color, bool wireframe=false);
}}

#endif
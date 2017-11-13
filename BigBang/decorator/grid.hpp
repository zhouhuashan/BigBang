#pragma once

#include "decorator/decorator.hpp"

namespace WarGrey::SCADA {
    private class GridDecorator sealed : public WarGrey::SCADA::IUniverseDecorator {
    public:
        GridDecorator(float grid_width = 16.0F, float grid_height = 0.0F);

    public:
        void draw_before(
            WarGrey::SCADA::Universe* master,
            Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
            float Width, float Height) override;

    private:
        float width;
        float height;
    };
}

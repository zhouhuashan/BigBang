#pragma once

#include "graphlet/primitive.hpp"

namespace WarGrey::SCADA {
    private class Alarmlet : public WarGrey::SCADA::IGraphlet {
    public:
        Alarmlet(float size);
        ~Alarmlet() noexcept;

    public:
        void draw(Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds, float x, float y, float Width, float Height) override;
        void fill_extent(float x, float y, float* w = nullptr, float* h = nullptr) override;

    private:
        float size;
    };
}
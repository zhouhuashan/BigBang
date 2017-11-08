#pragma once

#include "snip/pipe/pipesnip.hpp"

namespace WarGrey::SCADA {
    private class GlueCleanerlet : public WarGrey::SCADA::IPipeSnip {
    public:
        GlueCleanerlet(float width, float height, float thickness = 0.0F,
            double color = 120.0, double saturation = 0.607,
            double light = 0.339, double highlight = 0.839);

    public:
        void load() override;
        void draw(Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds, float x, float y, float Width, float Height) override;
        void fill_extent(float x, float y, float* w = nullptr, float* h = nullptr,
            float* d = nullptr, float* s = nullptr, float* l = nullptr, float* r = nullptr)
            override;

    public:
        Windows::Foundation::Rect get_input_port() override;
        Windows::Foundation::Rect get_output_port() override;

    private:
        float width;
        float height;
        float pipe_thickness;

    private:
        Windows::UI::Color color;
        Windows::UI::Color highlight_color;
        Windows::UI::Color body_color;
        Windows::UI::Color endpoint_color;
        Microsoft::Graphics::Canvas::Brushes::CanvasLinearGradientBrush^ hat_brush;
        Microsoft::Graphics::Canvas::Brushes::CanvasLinearGradientBrush^ hatbody_brush;
        Microsoft::Graphics::Canvas::Brushes::CanvasLinearGradientBrush^ pipe_brush;
        Microsoft::Graphics::Canvas::Geometry::CanvasCachedGeometry^ pipe;
        Microsoft::Graphics::Canvas::Geometry::CanvasCachedGeometry^ endpoint;
    };
}
#pragma once

#include "snip.hpp"
#include "text.hpp"

namespace WarGrey::SCADA {
	private enum ArrowPosition { Start, End };

    private class Liquidlet : public WarGrey::SCADA::Snip {
    public:
        Liquidlet(float length, WarGrey::SCADA::ArrowPosition = WarGrey::SCADA::Start,
			double color = 38.825, double saturation = 1.000, double lightness = 0.500);

    public:
		void load() override;
        void draw(Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds, float x, float y, float Width, float Height) override;
        void fill_extent(float x, float y, float* w = nullptr, float* h = nullptr) override;

    private:
		WarGrey::SCADA::ArrowPosition position;
		float length;
		float thickness;
		float arrow_size;
		float arrowhead_size;
		float scale_width;
		float scale_height;

	private:
        float in_temperature;
		float out_temperature;

	private:
		Microsoft::Graphics::Canvas::Geometry::CanvasCachedGeometry^ arrow;
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat^ font;
		Microsoft::Graphics::Canvas::Brushes::ICanvasBrush^ arrow_brush;
		Microsoft::Graphics::Canvas::Brushes::ICanvasBrush^ pipe_brush;
    };
}

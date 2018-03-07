#pragma once

#include "forward.hpp"
#include "object.hpp"

namespace WarGrey::SCADA {
	private class IPlanetDecorator abstract : public WarGrey::SCADA::SharedObject {
    public:
        virtual void draw_before(
            WarGrey::SCADA::IPlanet* master,
            Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
            float Width, float Height) {};

        virtual void draw_after(
            WarGrey::SCADA::IPlanet* master,
            Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
            float Width, float Height) {};

        virtual void draw_before_snip(
            WarGrey::SCADA::ISnip* snip,
            Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
            float x, float y, float width, float height, bool selected) {};

        virtual void draw_after_snip(
            WarGrey::SCADA::ISnip* snip,
            Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
            float x, float y, float width, float height, bool selected) {};

	protected:
		~IPlanetDecorator() noexcept {}
	};

	private class ComposeDecorator final : public WarGrey::SCADA::IPlanetDecorator {
	public:
		ComposeDecorator(IPlanetDecorator* first, IPlanetDecorator* second);
		ComposeDecorator(const IPlanetDecorator** decorators, unsigned int count);

	public:
		void draw_before(
			WarGrey::SCADA::IPlanet* master,
			Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
			float Width, float Height) override;

		void draw_after(
			WarGrey::SCADA::IPlanet* master,
			Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
			float Width, float Height) override;

		void draw_before_snip(
			WarGrey::SCADA::ISnip* snip,
			Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
			float x, float y, float width, float height, bool selected) override;

		void draw_after_snip(
			WarGrey::SCADA::ISnip* snip,
			Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds,
			float x, float y, float width, float height, bool selected) override;

	protected:
		~ComposeDecorator() noexcept;

	private:
		WarGrey::SCADA::IPlanetDecorator** decorators;
		unsigned int count;
	};
}

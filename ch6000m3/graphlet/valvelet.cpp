#include "valvelet.hpp"

#include "shape.hpp"
#include "paint.hpp"
#include "geometry.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::UI;

using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;

static double dynamic_mask_interval = 1.0 / 8.0;
static ValveState default_pump_state = ValveState::Closed;
static CanvasSolidColorBrush^ default_sketeton_color = darkgray_brush();

ValveStyle WarGrey::SCADA::make_default_valve_style(ValveState state) {
	ValveStyle s;

	s.skeleton_color = default_sketeton_color;

	switch (state) {
	case ValveState::Handle: s.body_color = cadetblue_brush(); break;
	case ValveState::Open: s.body_color = green_brush(); break;
	case ValveState::Opening: s.mask_color = green_brush(); break;
	case ValveState::Unopenable: s.skeleton_color = red_brush(); s.mask_color = green_brush(); break;
	//case ValveState::Remote: s.border_color = cyan_brush(); break;
	//case ValveState::Stopped: break; // this is the default to draw
	//case ValveState::Stopping: s.mask_color = forest_brush(); break;
	//case ValveState::Unstoppable: s.mask_color = forest_brush(); break;
	case ValveState::FalseOpen: s.border_color = red_brush(); s.body_color = forest_brush(); break;
	case ValveState::FalseClosed: s.border_color = red_brush(); s.body_color = dimgray_brush(); break;
	}

	return s;
}

/*************************************************************************************************/
Valvelet::Valvelet(float radius, double degrees) : Valvelet(default_pump_state, radius, degrees) {}

Valvelet::Valvelet(ValveState default_state, float radius, double degrees)
	: IStatelet(default_state, &make_default_valve_style), size(radius * 2.0F), degrees(degrees), thickness(2.0F) {
	
	this->tradius = radius - this->thickness * 2.0F;
	this->on_state_change(default_state);
}

void Valvelet::construct() {
	this->skeleton = triangle(this->tradius, this->degrees);
	this->body = geometry_freeze(this->skeleton);
}

void Valvelet::update(long long count, long long interval, long long uptime) {
	switch (this->get_state()) {
	case ValveState::Opening: {
		this->mask_percentage
			= ((this->mask_percentage < 0.0) || (this->mask_percentage >= 1.0))
			? 0.0
			: this->mask_percentage + dynamic_mask_interval;

		this->mask = trapezoid(this->tradius, this->degrees, this->mask_percentage);
	} break;
	case ValveState::Closing: {
		this->mask_percentage
			= ((this->mask_percentage <= 0.0) || (this->mask_percentage > 1.0))
			? 1.0
			: this->mask_percentage - dynamic_mask_interval;

		this->mask = trapezoid(this->tradius, this->degrees, this->mask_percentage);
	} break;
	}
}

void Valvelet::on_state_change(ValveState state) {
	switch (state) {
	case ValveState::Unopenable: {
		if (this->unstartable_mask == nullptr) {
			this->unstartable_mask = trapezoid(this->tradius, this->degrees, 0.382);
		}
		this->mask = this->unstartable_mask;
	} break;
	case ValveState::Unclosable: {
		if (this->unstoppable_mask == nullptr) {
			this->unstoppable_mask = trapezoid(this->tradius, this->degrees, 0.618);
		}
		this->mask = this->unstoppable_mask;
	} break;
	default: {
		this->mask = nullptr;
		this->mask_percentage = -1.0;
	}
	}
}

void Valvelet::fill_extent(float x, float y, float* w, float* h) {
	SET_BOXES(w, h, size);
}

double Valvelet::get_direction_degrees() {
	return this->degrees;
}

void Valvelet::draw(CanvasDrawingSession^ ds, float x, float y, float Width, float Height) {
	const ValveStyle style = this->get_style();
	auto skeleton_color = (style.skeleton_color != nullptr) ? style.skeleton_color : default_sketeton_color;
	auto body_color = (style.border_color != nullptr) ? style.border_color : system_background_brush();

	float radius = this->size * 0.5F - this->thickness;
	float cx = x + radius + this->thickness;
	float cy = y + radius + this->thickness;
	float bx = cx - radius + this->thickness;
	float by = cy - radius + this->thickness;

	if (style.border_color != nullptr) {
		ds->DrawCircle(cx, cy, radius, style.border_color, this->thickness);
	}

	ds->DrawCachedGeometry(this->body, bx, by, body_color);
	ds->DrawGeometry(this->skeleton, bx, by, skeleton_color, this->thickness);

	if (style.mask_color != nullptr) {
		auto mask = ((this->mask == nullptr) ? this->skeleton : this->mask);
		
		ds->FillGeometry(mask, bx, by, style.mask_color);
		ds->DrawGeometry(mask, bx, by, style.mask_color, this->thickness);
	}
}

﻿#include "virtualization/keyboard.hpp"
#include "planet.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::UI;
using namespace Windows::UI::Text;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::Text;

IKeyboard::IKeyboard(IPlanet* master, unsigned int keynum) : master(master), keynum(keynum) {
	this->enable_events(true);
	this->on_goodbye();
}

IKeyboard::~IKeyboard() {
	if (this->cells != nullptr) {
		delete [] this->cells;
	}
}

Syslog* IKeyboard::get_logger() {
	return this->master->get_logger();
}

void IKeyboard::construct() {
	syslog(Log::Info, "construct");
	this->create();

	this->cells = new KeyboardCell[this->keynum];
	for (unsigned int i = 0; i < this->keynum; i++) {
		this->fill_cell(this->cells + i, i);
	}
}

int IKeyboard::find_cell(float mouse_x, float mouse_y) {
	int found = -1;

	for (unsigned char i = 0; i < this->keynum; i++) {
		float cx = this->cells[i].x;
		float cy = this->cells[i].y;
		float cwidth = this->cells[i].width;
		float cheight = this->cells[i].height;

		if ((cx < mouse_x) && (mouse_x < cx + cwidth) && (cy < mouse_y) && (mouse_y < cy + cheight)) {
			found = i;
		}
	}

	return found;
}

void IKeyboard::show(bool shown) {
	this->_shown = shown;
}

bool IKeyboard::shown() {
	return this->_shown;
}
﻿#include <unordered_map>

#include "page/graphlets.hpp"
#include "configuration.hpp"

#include "graphlet/symbol/pump/hydraulic_pumplet.hpp"
#include "graphlet/symbol/valve/manual_valvelet.hpp"
#include "graphlet/symbol/valve/tagged_valvelet.hpp"
#include "graphlet/symbol/door/hopper_doorlet.hpp"

#include "decorator/page.hpp"

#include "module.hpp"

using namespace WarGrey::SCADA;

using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;

static Platform::String^ all_captions[] = {
	"winch", "pump", "valve", "m_valve", "hopperdoor", "upperdoor"
};

private class Stage final {
public:
	Stage(GraphletOverview* master) : master(master), progress(0.0) {
		this->font = make_bold_text_format("Microsoft YaHei", large_font_size);
	}

public:
	void load(float width, float height) {
		float unitsize = 32.0F;
		
		for (size_t i = 0; i < sizeof(all_captions)/sizeof(Platform::String^); i++) {
			this->captions[i] = make_label(_speak(all_captions[i]) + ":", this->font);
		}

		this->load_primitives(this->pumps, this->plabels, unitsize);
		this->load_primitives(this->valves, this->vlabels, unitsize);
		this->load_primitives(this->mvalves, this->mvlabels, unitsize);
		this->load_primitives(this->hdoors, this->hdlabels, unitsize);
		this->load_primitives(this->udoors, this->udlabels, unitsize);
	}

	void reflow(float width, float height, float vinset) {
		float unitsize, halfunit, cellsize;
		float label_max_width = 0.0F;
		float offset = vinset * 0.5F;
		float x0 = offset;
		float y0 = vinset + offset;

		for (size_t i = 0; i < GRAPHLETS_LENGTH(this->captions); i++) {
			if (this->captions[i] != nullptr) {
				float label_width;

				this->captions[i]->fill_extent(0.0F, 0.0F, &label_width);
				label_max_width = fmax(label_max_width, label_width);
			}
		}

		this->pumps[_E0(HydraulicPumpStatus)]->fill_extent(0.0F, 0.0F, &unitsize);
		halfunit = unitsize * 0.5F;
		cellsize = unitsize * 1.618F;

		for (size_t i = 0; i < GRAPHLETS_LENGTH(this->captions); i++) {
			if (this->captions[i] != nullptr) {
				float y = y0 + halfunit + float(i) * cellsize;

				this->master->move_to(this->captions[i], x0 + label_max_width, y, GraphletAnchor::RC);
			}
		}

		x0 += (label_max_width + offset + halfunit);
		y0 += unitsize;
		this->reflow_primitives(this->pumps,  this->plabels,   x0, y0 + cellsize * 1.0F, cellsize);
		this->reflow_primitives(this->valves, this->vlabels,   x0, y0 + cellsize * 2.0F, cellsize);
		this->reflow_primitives(this->mvalves, this->mvlabels, x0, y0 + cellsize * 3.0F, cellsize);
		this->reflow_primitives(this->hdoors, this->hdlabels,  x0, y0 + cellsize * 4.0F, cellsize);
		this->reflow_primitives(this->udoors, this->udlabels,  x0, y0 + cellsize * 5.0F, cellsize);
	}

public:
	void update(long long count, long long interval, long long uptime) {
		this->progress += 0.1;

		if (this->progress > 1.0) {
			this->progress = 0.0;
		}

		hdoors[DoorStatus::Opening]->set_value(this->progress);
		udoors[DoorStatus::Opening]->set_value(this->progress);
		hdoors[DoorStatus::Closing]->set_value(1.0 - this->progress);
		udoors[DoorStatus::Closing]->set_value(1.0 - this->progress);
	}

private:
	Labellet* make_label(Platform::String^ text, CanvasTextFormat^ font = nullptr) {
		return this->master->insert_one(new Labellet(text, font));
	}

	template<typename T, typename S>
	void load_primitives(std::unordered_map<S, T*>& gs, std::unordered_map<S, Labellet*>& ls, float unitsize) {
		for (S s = _E0(S); s < S::_; s++) {
			gs[s] = this->master->insert_one(new T(s, unitsize));
			ls[s] = make_label(_speak(s));
		}
	}

	template<typename T, typename S>
	void reflow_primitives(std::unordered_map<S, T*>& gs, std::unordered_map<S, Labellet*>& ls, float x0, float y, float cellsize) {
		for (S i = _E0(S); i < S::_; i++) {
			float x = x0 + _F(i) * cellsize;

			this->master->move_to(gs[i], x, y, GraphletAnchor::CB);
			this->master->move_to(ls[i], x, y, GraphletAnchor::CT);
		}
	}

private: // never delete these graphlets manually.
	Labellet* captions[sizeof(all_captions) / sizeof(Platform::String^)];
	std::unordered_map<HydraulicPumpStatus, HydraulicPumplet*> pumps;
	std::unordered_map<HydraulicPumpStatus, Labellet*> plabels;
	std::unordered_map<ManualValveStatus, Valvelet*> valves;
	std::unordered_map<ManualValveStatus, Labellet*> vlabels;
	std::unordered_map<TValveStatus, TValvelet*> mvalves;
	std::unordered_map<TValveStatus, Labellet*> mvlabels;
	std::unordered_map<DoorStatus, HopperDoorlet*> hdoors;
	std::unordered_map<DoorStatus, Labellet*> hdlabels;
	std::unordered_map<DoorStatus, UpperHopperDoorlet*> udoors;
	std::unordered_map<DoorStatus, Labellet*> udlabels;

private:
	GraphletOverview* master;
	CanvasTextFormat^ font;

private:
	double progress;
};

/*************************************************************************************************/
static std::unordered_map<GraphletOverview*, Stage*> stages;

GraphletOverview::GraphletOverview() : Planet(__MODULE__) {
	this->append_decorator(new PageDecorator());
}

GraphletOverview::~GraphletOverview() {
	auto maybe_stage = stages.find(this);

	if (maybe_stage != stages.end()) {
		delete maybe_stage->second;

		stages.erase(maybe_stage);
	}
}

void GraphletOverview::load(CanvasCreateResourcesReason reason, float width, float height) {
	if (stages.find(this) == stages.end()) {
		Stage* stage = new Stage(this);
		
		stages.insert(std::pair<GraphletOverview*, Stage*>(this, stage));

		{ // load graphlets
			stage->load(width, height);

			this->statusline = new Statuslinelet(default_logging_level);
			this->statusbar = new Statusbarlet(this->name());
			this->insert(this->statusbar);
			this->insert(this->statusline);
		}
	}
}

void GraphletOverview::reflow(float width, float height) {
	auto maybe_stage = stages.find(this);
	
	if (maybe_stage != stages.end()) {
		Stage* stage = maybe_stage->second;
		float vinset = statusbar_height();

		this->move_to(this->statusline, 0.0F, height, GraphletAnchor::LB);
		
		stage->reflow(width, height, vinset);
	}
}

void GraphletOverview::on_elapse(long long count, long long interval, long long uptime) {
	auto maybe_stage = stages.find(this);

	if (maybe_stage != stages.end()) {
		maybe_stage->second->update(count, interval, uptime);
	}
}

bool GraphletOverview::can_select(IGraphlet* g) {
#ifdef _Debug
	return true;
#else
	return false;
#endif
}

﻿#include <map>

#include "page/flushs.hpp"
#include "configuration.hpp"
#include "menu.hpp"

#include "module.hpp"
#include "text.hpp"
#include "paint.hpp"
#include "brushes.hxx"
#include "turtle.hpp"

#include "graphlet/shapelet.hpp"
#include "graphlet/buttonlet.hpp"
#include "graphlet/symbol/door/hatchlet.hpp"
#include "graphlet/symbol/door/hopper_doorlet.hpp"
#include "graphlet/symbol/pump/water_pumplet.hpp"
#include "graphlet/symbol/valve/gate_valvelet.hpp"
#include "graphlet/symbol/valve/manual_valvelet.hpp"

#include "schema/ai_doors.hpp"
#include "schema/ai_pumps.hpp"
#include "schema/ai_water_pumps.hpp"

#include "schema/di_doors.hpp"
#include "schema/di_pumps.hpp"
#include "schema/di_valves.hpp"
#include "schema/di_water_pumps.hpp"

#include "schema/do_doors.hpp"
#include "schema/do_valves.hpp"
#include "schema/do_water_pumps.hpp"

#include "decorator/page.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::Foundation;

using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;
using namespace Microsoft::Graphics::Canvas::Geometry;

private enum FSMode { WindowUI = 0, Dashboard };

private enum class FSGVOperation { Open, Close, VirtualOpen, VirtualClose, StopButterflyValves, CloseButterflyValves, _ };
private enum class FSHDOperation { Open, Stop, Close, Disable, _ };

private enum class FSSCommand { LeftShift, RightShift, _ };

private enum class FSPSOperation {
	Prepare, Start, Stop, Reset,
	PS_PS, PS_SB, PS_2,
	S2_PS, S2_SB, S2_2, P2_2, I2_2,
	PS_H, S2_H, P2_H,
	_ 
};

private enum class FSSBOperation {
	Prepare, Start, Stop, Reset,
	SB_PS, SB_SB, SB_2,
	S2_PS, S2_SB, S2_2, P2_2, I2_2,
	SB_H, S2_H, P2_H,
	_
};

static CanvasSolidColorBrush^ water_color = Colours::Green;

// WARNING: order matters
private enum class FS : unsigned int {
	Port, Starboard,

	// Pumps
	PSPump, SBPump,

	// Pump dimensions
	D, E,

	// Valves
	HBV01, HBV02, HBV03, HBV08, HBV09, HBV11, HBV12, HBV13, HBV14, HBV15, HBV16, HBV17, HBV18,
	HBV04, HBV05, HBV06, HBV07, HBV10,
	SBV1, SBV2, SBV3, SBV4,

	// Upper Hopper Doors
	SB1, SB2, SB3, SB4, SB5, SB6, SB7,
	PS1, PS2, PS3, PS4, PS5, PS6, PS7,
	
	// key labels
	PSSea, SBSea,

	_,
	// anchors used for unnamed corners
	h3ps, h3sb, h4, h5, h10,
	h11, h12, h13, h14, h15, h16, h17, h18,
	lb11, lb12, lb13, lb14, lb15, lb16, lb17, lb18, // left-bottom corner of hopper
	rb11, rb12, rb13, rb14, rb15, rb16, rb17, rb18, // right-bottom corner of hopper
	water, room, h5ps, h1sb,

	// anchors used for non-interconnected nodes
	nic
};

private class Flush final
	: public PLCConfirmation
	, public IMenuCommand<FSGVOperation, Credit<GateValvelet, FS>, PLCMaster*>
	, public IMenuCommand<FSHDOperation, Credit<UpperHopperDoorlet, FS>, PLCMaster*>
	, public IMenuCommand<FSPSOperation, Credit<WaterPumplet, FS>, PLCMaster*>
	, public IMenuCommand<FSSBOperation, Credit<WaterPumplet, FS>, PLCMaster*> {
public:
	Flush(FlushsPage* master) : master(master) {}

public:
	void pre_read_data(Syslog* logger) override {
		this->master->enter_critical_section();
		this->master->begin_update_sequence();

		this->station->clear_subtacks();
	}

	void on_analog_input(const uint8* DB2, size_t count2, const uint8* DB203, size_t count203, Syslog* logger) override {
		this->pressures[FS::D]->set_value(RealData(DB203, pump_D_pressure), GraphletAnchor::CC);
		this->pressures[FS::E]->set_value(RealData(DB203, pump_E_pressure), GraphletAnchor::CC);

		this->pressures[FS::HBV04]->set_value(RealData(DB203, sb_water_pump_discharge_pressure), GraphletAnchor::CB);
		this->flows[FS::HBV04]->set_value(RealData(DB203, sb_water_pump_flow), GraphletAnchor::CT);
		this->pressures[FS::HBV05]->set_value(RealData(DB203, ps_water_pump_discharge_pressure), GraphletAnchor::CB);
		this->flows[FS::HBV05]->set_value(RealData(DB203, sb_water_pump_flow), GraphletAnchor::CT);

		this->powers[FS::PSPump]->set_value(RealData(DB203, ps_water_pump_power), GraphletAnchor::CB);
		this->rpms[FS::PSPump]->set_value(RealData(DB203, ps_water_pump_rpm), GraphletAnchor::CT);
		this->powers[FS::SBPump]->set_value(RealData(DB203, sb_water_pump_power), GraphletAnchor::CB);
		this->rpms[FS::SBPump]->set_value(RealData(DB203, sb_water_pump_rpm), GraphletAnchor::CT);
		
		this->set_door_progress(FS::PS1, RealData(DB203, upper_door_PS1_progress));
		this->set_door_progress(FS::PS2, RealData(DB203, upper_door_PS2_progress));
		this->set_door_progress(FS::PS3, RealData(DB203, upper_door_PS3_progress));
		this->set_door_progress(FS::PS4, RealData(DB203, upper_door_PS4_progress));
		this->set_door_progress(FS::PS5, RealData(DB203, upper_door_PS5_progress));
		this->set_door_progress(FS::PS6, RealData(DB203, upper_door_PS6_progress));
		this->set_door_progress(FS::PS7, RealData(DB203, upper_door_PS7_progress));

		this->set_door_progress(FS::SB1, RealData(DB203, upper_door_SB1_progress));
		this->set_door_progress(FS::SB2, RealData(DB203, upper_door_SB2_progress));
		this->set_door_progress(FS::SB3, RealData(DB203, upper_door_SB3_progress));
		this->set_door_progress(FS::SB4, RealData(DB203, upper_door_SB4_progress));
		this->set_door_progress(FS::SB5, RealData(DB203, upper_door_SB5_progress));
		this->set_door_progress(FS::SB6, RealData(DB203, upper_door_SB6_progress));
		this->set_door_progress(FS::SB7, RealData(DB203, upper_door_SB7_progress));
	}

	void on_digital_input(const uint8* DB4, size_t count4, const uint8* DB205, size_t count205, WarGrey::SCADA::Syslog* logger) override {
		DI_water_pump(this->pumps[FS::PSPump], DB4, ps_water_pump_feedback, DB205, ps_water_pump_details);
		DI_water_pump(this->pumps[FS::SBPump], DB4, sb_water_pump_feedback, DB205, sb_water_pump_details);

		DI_pump_dimension(this->pressures[FS::D], DB4, pump_D_feedback);
		DI_pump_dimension(this->pressures[FS::E], DB4, pump_E_feedback);

		DI_gate_valve(this->gvalves[FS::HBV01], DB4, butterfly_valve_HBV01_feedback, DB205, butterfly_valve_HBV01_status);
		DI_gate_valve(this->gvalves[FS::HBV02], DB4, butterfly_valve_HBV02_feedback, DB205, butterfly_valve_HBV02_status);
		DI_gate_valve(this->gvalves[FS::HBV03], DB4, butterfly_valve_HBV03_feedback, DB205, butterfly_valve_HBV03_status);
		DI_gate_valve(this->gvalves[FS::HBV04], DB4, butterfly_valve_HBV04_feedback, DB205, butterfly_valve_HBV04_status);
		DI_gate_valve(this->gvalves[FS::HBV05], DB4, butterfly_valve_HBV05_feedback, DB205, butterfly_valve_HBV05_status);
		DI_gate_valve(this->gvalves[FS::HBV06], DB4, butterfly_valve_HBV06_feedback, DB205, butterfly_valve_HBV06_status);
		DI_gate_valve(this->gvalves[FS::HBV07], DB4, butterfly_valve_HBV07_feedback, DB205, butterfly_valve_HBV07_status);
		DI_gate_valve(this->gvalves[FS::HBV08], DB4, butterfly_valve_HBV08_feedback, DB205, butterfly_valve_HBV08_status);
		DI_gate_valve(this->gvalves[FS::HBV09], DB4, butterfly_valve_HBV09_feedback, DB205, butterfly_valve_HBV09_status);
		DI_gate_valve(this->gvalves[FS::HBV10], DB4, butterfly_valve_HBV10_feedback, DB205, butterfly_valve_HBV10_status);
		DI_gate_valve(this->gvalves[FS::HBV11], DB4, butterfly_valve_HBV11_feedback, DB205, butterfly_valve_HBV11_status);
		DI_gate_valve(this->gvalves[FS::HBV12], DB4, butterfly_valve_HBV12_feedback, DB205, butterfly_valve_HBV12_status);
		DI_gate_valve(this->gvalves[FS::HBV13], DB4, butterfly_valve_HBV13_feedback, DB205, butterfly_valve_HBV13_status);
		DI_gate_valve(this->gvalves[FS::HBV14], DB4, butterfly_valve_HBV14_feedback, DB205, butterfly_valve_HBV14_status);
		DI_gate_valve(this->gvalves[FS::HBV15], DB4, butterfly_valve_HBV15_feedback, DB205, butterfly_valve_HBV15_status);
		DI_gate_valve(this->gvalves[FS::HBV16], DB4, butterfly_valve_HBV16_feedback, DB205, butterfly_valve_HBV16_status);
		DI_gate_valve(this->gvalves[FS::HBV17], DB4, butterfly_valve_HBV17_feedback, DB205, butterfly_valve_HBV17_status);
		DI_gate_valve(this->gvalves[FS::HBV18], DB4, butterfly_valve_HBV18_feedback, DB205, butterfly_valve_HBV18_status);

		DI_hopper_door(this->uhdoors[FS::PS1], DB205, upper_door_PS1_status);
		DI_hopper_door(this->uhdoors[FS::PS2], DB205, upper_door_PS2_status);
		DI_hopper_door(this->uhdoors[FS::PS3], DB205, upper_door_PS3_status);
		DI_hopper_door(this->uhdoors[FS::PS4], DB205, upper_door_PS4_status);
		DI_hopper_door(this->uhdoors[FS::PS5], DB205, upper_door_PS5_status);
		DI_hopper_door(this->uhdoors[FS::PS6], DB205, upper_door_PS6_status);
		DI_hopper_door(this->uhdoors[FS::PS7], DB205, upper_door_PS7_status);

		DI_hopper_door(this->uhdoors[FS::SB1], DB205, upper_door_SB1_status);
		DI_hopper_door(this->uhdoors[FS::SB2], DB205, upper_door_SB2_status);
		DI_hopper_door(this->uhdoors[FS::SB3], DB205, upper_door_SB3_status);
		DI_hopper_door(this->uhdoors[FS::SB4], DB205, upper_door_SB4_status);
		DI_hopper_door(this->uhdoors[FS::SB5], DB205, upper_door_SB5_status);
		DI_hopper_door(this->uhdoors[FS::SB6], DB205, upper_door_SB6_status);
		DI_hopper_door(this->uhdoors[FS::SB7], DB205, upper_door_SB7_status);

		DI_shift_button(this->shifts[FSSCommand::LeftShift], DB205, left_shifting_details);
		DI_shift_button(this->shifts[FSSCommand::RightShift], DB205, right_shifting_details);
	}

	void post_read_data(Syslog* logger) override {
		{ // flow water
			FS h3[] = { FS::h3sb, FS::SBPump };
			bool ps_okay = (this->pumps[FS::PSPump]->get_status() == WaterPumpStatus::Running);
			bool sb_okay = (this->pumps[FS::SBPump]->get_status() == WaterPumpStatus::Running);
			FS h1[] = { FS::h1sb, FS::SBPump };
			
			this->station->append_subtrack(FS::HBV01, FS::SBSea, water_color);
			this->station->append_subtrack(FS::HBV02, FS::PSSea, water_color);

			this->try_flow_water(FS::HBV01, h1, water_color);
			this->try_flow_water(FS::HBV02, FS::PSPump, water_color);

			if (this->pumps[FS::PSPump]->get_status() == WaterPumpStatus::Running) {
				FS h35[] = { FS::PSPump, FS::HBV03, FS::h5ps, FS::HBV05 };

				this->station->append_subtrack(h35, water_color);
				this->nintercs[FS::nic]->set_color(water_color);
			} else {
				this->nintercs[FS::nic]->set_color(default_pipe_color);
			}

			this->try_flow_water(FS::HBV03, h3, water_color);
			this->try_flow_water(FS::HBV05, FS::HBV07, FS::HBV08, water_color);
			this->try_flow_water(FS::HBV07, FS::Port, water_color);
			this->try_flow_water(FS::HBV08, FS::HBV10, water_color);
			
			if (this->pumps[FS::SBPump]->get_status() == WaterPumpStatus::Running) {
				this->station->append_subtrack(FS::SBPump, FS::HBV04, water_color);
			}

			this->try_flow_water(FS::HBV04, FS::HBV06, FS::HBV09, water_color);
			this->try_flow_water(FS::HBV06, FS::Starboard, water_color);
			this->try_flow_water(FS::HBV09, FS::HBV10, water_color);
			this->try_flow_water(FS::HBV10, FS::HBV18, water_color);

			for (FS HBV = FS::HBV11; HBV <= FS::HBV18; HBV++) {
				unsigned int distance = _I(HBV) - _I(FS::HBV11);
				FS lb = _E(FS, _I(FS::lb11) + distance);
				FS rb = _E(FS, _I(FS::rb11) + distance);

				this->try_flow_water(HBV, rb, ((HBV == FS::HBV18) ? FS::_ : lb), water_color);
			}
		}

		this->master->end_update_sequence();
		this->master->leave_critical_section();
	}

public:
	bool can_execute(FSHDOperation cmd, Credit<UpperHopperDoorlet, FS>* door, PLCMaster* plc, bool acc_executable) override {
		return plc->connected();
	}

	void execute(FSHDOperation cmd, Credit<UpperHopperDoorlet, FS>* door, PLCMaster* plc) override {
		plc->send_command(DO_upper_door_command(cmd, door->id));
	}

public:
	bool can_execute(FSGVOperation cmd, Credit<GateValvelet, FS>* valve, PLCMaster* plc, bool acc_executable) override {
		return plc->connected();
	}

	void execute(FSGVOperation cmd, Credit<GateValvelet, FS>* valve, PLCMaster* plc) override {
		plc->send_command(DO_butterfly_valve_command(cmd, valve->id));
	}

public:
	bool can_execute(FSPSOperation cmd, Credit<WaterPumplet, FS>* pump, PLCMaster* plc, bool acc_executable) override {
		return plc->connected();
	}

	void execute(FSPSOperation cmd, Credit<WaterPumplet, FS>* pump, PLCMaster* plc) override {
		plc->send_command(DO_ps_water_pump_command(cmd));
	}

	bool can_execute(FSSBOperation cmd, Credit<WaterPumplet, FS>* pump, PLCMaster* plc, bool acc_executable) override {
		return plc->connected();
	}

	void execute(FSSBOperation cmd, Credit<WaterPumplet, FS>* pump, PLCMaster* plc) override {
		plc->send_command(DO_sb_water_pump_command(cmd));
	}

public:
	void construct(float gwidth, float gheight) {
		this->caption_font = make_bold_text_format("Microsoft YaHei", normal_font_size);
		this->label_font = make_bold_text_format("Microsoft YaHei", small_font_size);
		this->pump_style = make_highlight_dimension_style(large_metrics_font_size, 6U, 0, Colours::Background);
		this->highlight_style = make_highlight_dimension_style(large_metrics_font_size, 6U, 0, Colours::Green);

		this->plain_style.number_font = make_bold_text_format("Cambria Math", large_metrics_font_size);
		this->plain_style.unit_font = make_bold_text_format("Cambria", normal_font_size);
	}
 
public:
	void load(float width, float height, float gwidth, float gheight) {
		float radius = resolve_gridsize(gwidth, gheight);
		auto hstyle = make_dash_stroke(CanvasDashStyle::Dash);
		Turtle<FS>* pTurtle = new Turtle<FS>(gwidth, gheight, false, FS::HBV10);
		Turtle<FS>* rTurtle = new Turtle<FS>(gwidth, gheight, false);
		Turtle<FS>* wTurtle = new Turtle<FS>(gwidth, gheight, false);

		pTurtle->move_right(2, FS::h10);
		
		pTurtle->turn_right_up()->move_up(2.5F, FS::HBV08)->move_up(2.5F)->turn_up_right(FS::h5);
		pTurtle->turn_left_up()->move_up(2.5F, FS::SBV4)->move_up(2.5F)->turn_up_left();
		pTurtle->move_left(10, FS::HBV07)->move_left(10, FS::Port)->jump_back(FS::h10);

		pTurtle->turn_right_down()->move_down(5, FS::HBV09)->move_down(5)->turn_down_right(FS::h4);
		pTurtle->turn_left_down()->move_down(2.5F, FS::SBV3)->move_down(2.5F)->turn_down_left();
		pTurtle->move_left(10, FS::HBV06)->move_left(10, FS::Starboard)->jump_back(FS::h5);

		pTurtle->move_right(4, FS::HBV05)->move_right(8)->jump_right()->move_right(6)->turn_right_down(FS::h5ps)->move_down(6);
		pTurtle->turn_down_left(FS::h3ps)->move_left(6)->turn_left_up(FS::PSPump);
		pTurtle->move_up(4, FS::HBV02)->move_up(2.5F, FS::nic)->move_up(3.5F, FS::SBV2)->move_up(2, FS::PSSea)->jump_back(FS::h3ps);

		pTurtle->turn_right_down()->move_down(2.5F, FS::HBV03)->move_down(2.5F);
		pTurtle->turn_down_left(FS::h3sb)->move_left(6, FS::SBPump)->jump_back();
		pTurtle->turn_right_down()->move_down(3, FS::h1sb)->turn_down_left()->move_left(6)->turn_left_down();
		pTurtle->move_down(FS::HBV01)->move_down(2, FS::SBV1)->move_down(2, FS::SBSea)->jump_back(FS::h4);

		pTurtle->move_right(4, FS::HBV04)->move_right(5)->turn_right_up()->move_up(2.5F)->turn_up_right()->move_right(2);
		
		pTurtle->jump_back(FS::HBV10);

		for (FS HBV = FS::HBV11; HBV <= FS::HBV18; HBV++) {
			unsigned int distance = _I(HBV) - _I(FS::HBV11);
			float half_width = 2.0F;
			float half_height = 2.5F;
			float gapsize = 0.382F;
			float room_height = (6.0F + half_height) * 2.0F;
			float water_height = room_height - 5.0F;
			FS hbv = _E(FS, _I(FS::h11) + distance);
			FS lb = _E(FS, _I(FS::lb11) + distance);
			FS rb = _E(FS, _I(FS::rb11) + distance);

			pTurtle->move_left(half_width);
			pTurtle->move_left(gapsize)->move_left(half_width, hbv);
			pTurtle->move_down(half_height, HBV)->move_down(half_height);
			pTurtle->jump_right(half_width, rb)->move_left(half_width);

			if (HBV != FS::HBV18) {
				pTurtle->move_left(half_width, lb)->jump_back(hbv);

				rTurtle->jump_left(gapsize)->move_left()->jump_left(half_width);
				rTurtle->move_left(hbv)->move_down(room_height);
				rTurtle->move_right()->jump_right(half_width)->move_right();
				rTurtle->move_up(room_height)->jump_back(hbv);

				wTurtle->jump_back()->jump_left(half_width)->jump_left(gapsize)->jump_left(half_width, hbv);
				wTurtle->move_down(gapsize)->move_left(half_width)->move_left(gapsize);
				wTurtle->move_down(water_height)->move_right(gapsize)->move_right(half_width)->move_down(gapsize);

				if (HBV != FS::HBV11) {
					wTurtle->jump_up(gapsize)->move_right(half_width)->jump_up(water_height)->move_left(half_width);
				}
			} else {
				pTurtle->jump_back(FS::HBV18)->jump_right(half_width, FS::water)->jump_right(gapsize, FS::room);
			}
		}
		
		this->hopper_room = this->master->insert_one(new Tracklet<FS>(rTurtle, default_pipe_thickness, Colours::DimGray, hstyle));
		this->hopper_water = this->master->insert_one(new Tracklet<FS>(wTurtle, default_pipe_thickness, Colours::DimGray, hstyle));
		this->station = this->master->insert_one(new Tracklet<FS>(pTurtle, default_pipe_thickness, default_pipe_color));

		this->load_buttons(this->shifts);

		{ // load doors
			this->load_doors(this->uhdoors, this->progresses, FS::PS1, FS::PS7, radius);
			this->load_doors(this->uhdoors, this->progresses, FS::SB1, FS::SB7, radius);
		}

		{ // load valves
			this->load_valves(this->mvalves, this->labels, this->captions, FS::SBV1, FS::SBV4, radius * 0.618F, 90.0);
			this->load_valves(this->gvalves, this->labels, this->captions, FS::HBV01, FS::HBV18, radius, 90.0);
			this->load_valves(this->gvalves, this->labels, this->captions, FS::HBV04, FS::HBV10, radius, 00.0);
		}

		{ // load special nodes
			auto pscolor = Colours::make(default_ps_color);
			auto sbcolor = Colours::make(default_sb_color);
			float dh_radius = gwidth * 2.0F;
			float nic_radius = radius * 0.25F;

			this->load_label(this->captions, FS::PSSea, Colours::Silver);
			this->load_label(this->captions, FS::SBSea, Colours::Silver);

			this->load_pump(this->pumps, this->captions, FS::PSPump, +radius, 180.0);
			this->load_pump(this->pumps, this->captions, FS::SBPump, -radius, 180.0);
			
			this->ps_sea = this->master->insert_one(new Hatchlet(gwidth, gheight, pscolor));
			this->sb_sea = this->master->insert_one(new Hatchlet(gwidth, gheight, sbcolor));

			this->ps_draghead = this->master->insert_one(
				new Segmentlet(-90.0, 90.0, dh_radius, gheight,
					nullptr, pscolor, default_pipe_thickness));

			this->sb_draghead = this->master->insert_one(
				new Segmentlet(-90.0, 90.0, dh_radius, gheight,
					nullptr, sbcolor, default_pipe_thickness));

			for (FS id = FS::nic; id <= FS::nic; id++) {
				this->nintercs[id] = this->master->insert_one(
					new Omegalet(180.0, nic_radius, default_pipe_thickness, default_pipe_color));
			}
		}

		{ // load other dimensions
			float cylinder_height = gheight * 5.0F;

			this->load_dimension(this->pressures, FS::HBV04, "bar");
			this->load_dimension(this->flows, FS::HBV04, "m3ph");
			this->load_dimension(this->pressures, FS::HBV05, "bar");
			this->load_dimension(this->flows, FS::HBV05, "m3ph");

			this->load_dimensions(this->pressures, FS::D, FS::E, "bar");
		}
	}

public:
	void reflow(float width, float height, float gwidth, float gheight, float vinset) {
		this->master->move_to(this->station, width * 0.5F, height * 0.5F, GraphletAnchor::CC);
		this->station->map_graphlet_at_anchor(this->hopper_room, FS::room, GraphletAnchor::LC);
		this->station->map_graphlet_at_anchor(this->hopper_water, FS::water, GraphletAnchor::LC);

		this->station->map_graphlet_at_anchor(this->ps_draghead, FS::Port, GraphletAnchor::RC);
		this->station->map_graphlet_at_anchor(this->sb_draghead, FS::Starboard, GraphletAnchor::RC);
		this->station->map_graphlet_at_anchor(this->ps_sea, FS::PSSea, GraphletAnchor::CB);
		this->station->map_graphlet_at_anchor(this->sb_sea, FS::SBSea, GraphletAnchor::CT);
		this->master->move_to(this->captions[FS::PSSea], this->ps_sea, GraphletAnchor::CT, GraphletAnchor::CB);
		this->master->move_to(this->captions[FS::SBSea], this->sb_sea, GraphletAnchor::CB, GraphletAnchor::CT);

		for (auto it = this->nintercs.begin(); it != this->nintercs.end(); it++) {
			/** NOTE
			 * Lines are brush-based shape, they do not have stroke, `Shapelet` does not know how width they are,
			 * thus, we have to do aligning on our own.
			 */
			this->station->map_graphlet_at_anchor(it->second, it->first, GraphletAnchor::CB, 0.0F, default_pipe_thickness * 0.5F);
		}

		this->reflow_doors(this->uhdoors, this->progresses, FS::PS1, FS::PS7, GraphletAnchor::CT);
		this->reflow_doors(this->uhdoors, this->progresses, FS::SB1, FS::SB7, GraphletAnchor::CB);

		{ // reflow buttons
			IGraphlet* shift_target = this->progresses[FS::SB4];
			
			this->master->move_to(this->shifts[FSSCommand::LeftShift], shift_target, GraphletAnchor::LB, GraphletAnchor::RT, 0.0F, gheight);
			this->master->move_to(this->shifts[FSSCommand::RightShift], shift_target, GraphletAnchor::RB, GraphletAnchor::LT, 0.0F, gheight);
		}

		for (auto it = this->pumps.begin(); it != this->pumps.end(); it++) {
			float ox, oy;

			it->second->fill_pump_origin(&ox, &oy);
			this->station->map_credit_graphlet(it->second, GraphletAnchor::CC, -ox, -oy);
			this->master->move_to(this->captions[it->first], it->second, GraphletAnchor::CB, GraphletAnchor::CT);
		}
		
		{ // reflow valves
			float gridsize = resolve_gridsize(gwidth, gheight);

			for (auto it = this->gvalves.begin(); it != this->gvalves.end(); it++) {
				this->reflow_valve(0.0F, 0.0F, gridsize, it->first, it->second);
			}

			for (auto it = this->mvalves.begin(); it != this->mvalves.end(); it++) {
				this->reflow_valve(0.0F, 0.0F, gridsize, it->first, it->second);
			}
		}

		{ // reflow dimensions
			float xoff = gwidth * 2.0F;
			float ps_oy = this->pumps[FS::PSPump]->get_radiusY() * 0.5F;
			
			this->station->map_credit_graphlet(this->pressures[FS::HBV04], GraphletAnchor::LB, xoff);
			this->station->map_credit_graphlet(this->flows[FS::HBV04], GraphletAnchor::LT, xoff);
			this->station->map_credit_graphlet(this->pressures[FS::HBV05], GraphletAnchor::LB, xoff);
			this->station->map_credit_graphlet(this->flows[FS::HBV05], GraphletAnchor::LT, xoff);

			this->station->map_credit_graphlet(this->powers[FS::PSPump], GraphletAnchor::LB, xoff, ps_oy);
			this->station->map_credit_graphlet(this->rpms[FS::PSPump], GraphletAnchor::LT, xoff, ps_oy);
			this->station->map_credit_graphlet(this->powers[FS::SBPump], GraphletAnchor::LB, xoff);
			this->station->map_credit_graphlet(this->rpms[FS::SBPump], GraphletAnchor::LT, xoff);

			this->master->move_to(this->pressures[FS::D], this->progresses[FS::PS5], GraphletAnchor::CT, GraphletAnchor::CB, 0.0F, -gheight);
			this->master->move_to(this->pressures[FS::E], this->progresses[FS::PS3], GraphletAnchor::CT, GraphletAnchor::CB, 0.0F, -gheight);
		}
	}

private:
	template<class G, typename E>
	void load_valves(std::map<E, G*>& gs, std::map<E, Credit<Labellet, E>*>& ls, std::map<E, Credit<Labellet, E>*>& cs
		, E id0, E idn, float radius, double degrees) {
		for (E id = id0; id <= idn; id++) {
			this->load_label(ls, id.ToString(), id, Colours::Silver, this->label_font);
			this->load_label(cs, id, Colours::Silver, this->label_font);

			gs[id] = this->master->insert_one(new G(radius, degrees), id);
		}
	}

	template<class D, typename E>
	void load_doors(std::map<E, Credit<D, E>*>& ds, std::map<E, Credit<Percentagelet, E>*>& ps, E id0, E idn, float radius) {
		for (E id = id0; id <= idn; id++) {
			ds[id] = this->master->insert_one(new Credit<D, E>(radius), id);
			ps[id] = this->master->insert_one(new Credit<Percentagelet, E>(this->percentage_style), id);
		}
	}

	template<class G, typename E>
	void load_pump(std::map<E, G*>& gs, std::map<E, Credit<Labellet, E>*>& ls, E id, float rx, double degrees) {
		this->load_label(ls, id, Colours::Salmon, this->caption_font);
	
		gs[id] = this->master->insert_one(new G(rx, degrees), id);

		this->load_dimension(this->powers, id, "kwatt");
		this->load_dimension(this->rpms, id, "rpm");
	}

	template<class B, typename CMD>
	void load_buttons(std::map<CMD, Credit<B, CMD>*>& bs, float width = 128.0F, float height = 32.0F) {
		for (CMD cmd = _E(CMD, 0); cmd < CMD::_; cmd++) {
			bs[cmd] = this->master->insert_one(new Credit<B, CMD>(speak(cmd, "menu"), width, height), cmd);
		}
	}

	template<class S, typename E>
	void load_shift(std::map<E, Credit<S, E>*> &ss, E id, float size, double degrees) {
		ss[id] = this->master->insert_one(new Credit<S, E>(size, degrees, Colours::SpringGreen), id);
	}

	template<typename E>
	void load_dimension(std::map<E, Credit<Dimensionlet, E>*>& ds, E id, Platform::String^ unit) {
		this->load_dimension(ds, id, unit, this->plain_style);
	}

	template<typename E>
	void load_dimension(std::map<E, Credit<Dimensionlet, E>*>& ds, E id, Platform::String^ unit, DimensionStyle& style) {
		ds[id] = this->master->insert_one(new Credit<Dimensionlet, E>(style, unit), id);
	}

	template<typename E>
	void load_dimensions(std::map<E, Credit<Dimensionlet, E>*>& ds, E id0, E idn, Platform::String^ unit) {
		for (E id = id0; id <= idn; id++) {
			ds[id] = this->master->insert_one(new Credit<Dimensionlet, E>(unit, id.ToString()), id);

			ds[id]->set_style(DimensionStatus::Normal, this->pump_style);
			ds[id]->set_style(DimensionStatus::Highlight, this->highlight_style);
		}
	}

	template<typename E>
	void load_label(std::map<E, Credit<Labellet, E>*>& ls, Platform::String^ caption, E id
		, CanvasSolidColorBrush^ color, CanvasTextFormat^ font = nullptr) {
		ls[id] = this->master->insert_one(new Credit<Labellet, E>(caption, font, color), id);
	}

	template<typename E>
	void load_label(std::map<E, Credit<Labellet, E>*>& ls, E id, CanvasSolidColorBrush^ color, CanvasTextFormat^ font = nullptr) {
		this->load_label(ls, _speak(id), id, color, font);
	}

private:
	template<class D, typename E>
	void reflow_doors(std::map<E, Credit<D, E>*>& ds, std::map<E, Credit<Percentagelet, E>*>& ps, E id0, E idn, GraphletAnchor ya) {
		GraphletAnchor d_anchor = GraphletAnchor::CT;
		GraphletAnchor p_anchor = GraphletAnchor::CB;
		unsigned int distance = _I(FS::HBV11) - _I(id0);
		float x, y, py;

		this->master->fill_graphlet_location(this->hopper_room, nullptr, &y, ya);
		this->station->fill_anchor_location(FS::HBV11, &x, &py);
		
		if (y > py) { // Starboard
			d_anchor = GraphletAnchor::CB;
			p_anchor = GraphletAnchor::CT;
		}

		for (E id = id0; id <= idn; id++) {
			this->station->fill_anchor_location(_E(FS, _I(id) + distance), &x, nullptr);
			
			this->master->move_to(ds[id], x, y, d_anchor);
			this->master->move_to(ps[id], x, y, p_anchor);
		}
	}

	template<class D, typename E>
	void reflow_valve(float x0, float y0, float gridsize, E id, D* valve) {
		GraphletAnchor anchor;
		float label_height, margin, dx, dy, vy, hy;

		switch (id) {
		case FS::HBV01: case FS::HBV02: case FS::HBV08: case FS::HBV09: case FS::SBV3: case FS::SBV4: {
			valve->fill_margin(x0, y0, nullptr, &margin, nullptr, nullptr);
			dx = x0 + gridsize - margin; dy = y0; anchor = GraphletAnchor::LB;
		}; break;
		case FS::HBV03: case FS::SBV1: case FS::SBV2: {
			valve->fill_margin(x0, y0, nullptr, nullptr, nullptr, &margin);
			dx = x0 - gridsize + margin; dy = y0; anchor = GraphletAnchor::RB;
		}; break;
		case FS::HBV04: case FS::HBV07: {
			valve->fill_margin(x0, y0, nullptr, nullptr, &margin, nullptr);
			dx = x0; dy = y0 + gridsize - margin; anchor = GraphletAnchor::CT;
		}; break;
		case FS::HBV05: case FS::HBV06: case FS::HBV10: {
			this->labels[id]->fill_extent(x0, y0, nullptr, &label_height);
			valve->fill_margin(x0, y0, &margin, nullptr, nullptr, nullptr);
			dx = x0; dy = y0 - gridsize - label_height + margin; anchor = GraphletAnchor::CB;
		}; break;
		default: { // HBV11 - HBV18
			this->labels[id]->fill_extent(x0, y0, nullptr, &label_height);
			this->station->fill_anchor_location(id, nullptr, &vy);
			this->station->fill_anchor_location(_E(FS, _I(id) - _I(FS::HBV11) + _I(FS::h11)), nullptr, &hy);
			dx = x0; dy = (hy - vy) - label_height; anchor = GraphletAnchor::CB;
		}
		}

		this->station->map_credit_graphlet(valve, GraphletAnchor::CC, x0, y0);
		this->station->map_credit_graphlet(this->captions[id], anchor, dx, dy);
		this->master->move_to(this->labels[id], this->captions[id], GraphletAnchor::CB, GraphletAnchor::CT);
	}

private:
	void set_door_progress(FS id, float value) {
		this->uhdoors[id]->set_value(value / 100.0F);
		this->progresses[id]->set_value(value, GraphletAnchor::CC);

		AI_hopper_door(this->uhdoors[id], value, upper_door_open_threshold, upper_door_closed_threshold);
	}

private:
	void try_flow_water(FS vid, FS eid1, FS eid2, CanvasSolidColorBrush^ color) {
		switch (this->gvalves[vid]->get_status()) {
		case GateValveStatus::Open: {
			this->station->append_subtrack(vid, eid1, color);

			if (eid2 != FS::_) {
				this->station->append_subtrack(vid, eid2, color);
			}
		}
		}
	}

	void try_flow_water(FS vid, FS* path, unsigned int count, CanvasSolidColorBrush^ color) {
		switch (this->gvalves[vid]->get_status()) {
		case GateValveStatus::Open: {
			this->station->append_subtrack(vid, path[0], color);
			this->station->append_subtrack(path, count, color);
		}
		}
	}

	template<unsigned int N>
	void try_flow_water(FS vid, FS (&path)[N], CanvasSolidColorBrush^ color) {
		this->try_flow_water(vid, path, N, color);
	}
	
	void try_flow_water(FS vid, FS eid, CanvasSolidColorBrush^ color) {
		this->try_flow_water(vid, eid, FS::_, color);
	}

// never deletes these graphlets mannually
private:
	Tracklet<FS>* station;
	Tracklet<FS>* hopper_room;
	Tracklet<FS>* hopper_water;
	std::map<FS, Credit<Labellet, FS>*> captions;
	std::map<FS, Credit<Labellet, FS>*> labels;
	std::map<FSSCommand, Credit<Buttonlet, FSSCommand>*> shifts;
	std::map<FS, Credit<WaterPumplet, FS>*> pumps;
	std::map<FS, Credit<GateValvelet, FS>*> gvalves;
	std::map<FS, Credit<ManualValvelet, FS>*> mvalves;
	std::map<FS, Credit<UpperHopperDoorlet, FS>*> uhdoors;
	std::map<FS, Credit<Percentagelet, FS>*> progresses;
	std::map<FS, Credit<Dimensionlet, FS>*> pressures;
	std::map<FS, Credit<Dimensionlet, FS>*> powers;
	std::map<FS, Credit<Dimensionlet, FS>*> rpms;
	std::map<FS, Credit<Dimensionlet, FS>*> flows;
	std::map<FS, Omegalet*> nintercs;
	Segmentlet* ps_draghead;
	Segmentlet* sb_draghead;
	Hatchlet* ps_sea;
	Hatchlet* sb_sea;
	
private:
	CanvasTextFormat^ caption_font;
	CanvasTextFormat^ label_font;
	DimensionStyle percentage_style;
	DimensionStyle pump_style;
	DimensionStyle highlight_style;
	DimensionStyle plain_style;

private:
	FlushsPage* master;
};

FlushsPage::FlushsPage(PLCMaster* plc) : Planet(__MODULE__), device(plc) {
	Flush* dashboard = new Flush(this);

	this->dashboard = dashboard;
	this->gate_valve_op = make_menu<FSGVOperation, Credit<GateValvelet, FS>, PLCMaster*>(dashboard, plc);
	this->upper_door_op = make_menu<FSHDOperation, Credit<UpperHopperDoorlet, FS>, PLCMaster*>(dashboard, plc);
	this->ps_pump_op = make_menu<FSPSOperation, Credit<WaterPumplet, FS>, PLCMaster*>(dashboard, plc);
	this->sb_pump_op = make_menu<FSSBOperation, Credit<WaterPumplet, FS>, PLCMaster*>(dashboard, plc);
	this->grid = new GridDecorator();

	this->device->append_confirmation_receiver(dashboard);

	{ // load decorators
		this->append_decorator(new PageDecorator());

#ifdef _DEBUG
		this->append_decorator(this->grid);
#else
		this->grid->set_active_planet(this);
#endif
	}
}

FlushsPage::~FlushsPage() {
	if (this->dashboard != nullptr) {
		delete this->dashboard;
	}

#ifndef _DEBUG
	delete this->grid;
#endif
}

void FlushsPage::load(CanvasCreateResourcesReason reason, float width, float height) {
	auto dashboard = dynamic_cast<Flush*>(this->dashboard);
	
	if (dashboard != nullptr) {
		float vinset = statusbar_height();
		float gwidth = width / 64.0F;
		float gheight = (height - vinset - vinset) / 36.0F;

		this->grid->set_grid_width(gwidth);
		this->grid->set_grid_height(gheight, vinset);
		
		dashboard->construct(gwidth, gheight);

		{ // load graphlets
			this->change_mode(FSMode::Dashboard);
			dashboard->load(width, height, gwidth, gheight);
			
			this->change_mode(FSMode::WindowUI);
			this->statusbar = this->insert_one(new Statusbarlet(this->name(), this->device));
			this->statusline = this->insert_one(new Statuslinelet(default_logging_level));
		}

		{ // delayed initializing
			this->get_logger()->append_log_receiver(this->statusline);

			if (this->device != nullptr) {
				this->device->get_logger()->append_log_receiver(this->statusline);
			}
		}
	}
}

void FlushsPage::reflow(float width, float height) {
	auto dashboard = dynamic_cast<Flush*>(this->dashboard);
	
	if (dashboard != nullptr) {
		float vinset = statusbar_height();
		float gwidth = this->grid->get_grid_width();
		float gheight = this->grid->get_grid_height();

		this->change_mode(FSMode::WindowUI);
		this->move_to(this->statusline, 0.0F, height, GraphletAnchor::LB);

		this->change_mode(FSMode::Dashboard);
		dashboard->reflow(width, height, gwidth, gheight, vinset);
	}
}

bool FlushsPage::can_select(IGraphlet* g) {
	auto btn = dynamic_cast<Buttonlet*>(g);

	return ((dynamic_cast<GateValvelet*>(g) != nullptr)
		|| (dynamic_cast<UpperHopperDoorlet*>(g) != nullptr)
		|| (dynamic_cast<WaterPumplet*>(g) != nullptr)
		|| (dynamic_cast<ArrowHeadlet*>(g) != nullptr)
		|| ((btn != nullptr) && (btn->get_status() != ButtonStatus::Disabled)));
}

void FlushsPage::on_tap_selected(IGraphlet* g, float local_x, float local_y) {
	auto gvalve = dynamic_cast<GateValvelet*>(g);
	auto uhdoor = dynamic_cast<UpperHopperDoorlet*>(g);
	auto wpump = dynamic_cast<Credit<WaterPumplet, FS>*>(g);
	auto shift = dynamic_cast<Credit<Buttonlet, FSSCommand>*>(g);

	if (gvalve != nullptr) {
		menu_popup(this->gate_valve_op, g, local_x, local_y);
	} else if (uhdoor != nullptr) {
		menu_popup(this->upper_door_op, g, local_x, local_y);
	} else if (shift != nullptr) {
		this->device->send_command((shift->id == FSSCommand::LeftShift) ? left_shifting_command : right_shifting_command);
	} else if (wpump != nullptr) {
		switch (wpump->id) {
		case FS::PSPump: menu_popup(this->ps_pump_op, g, local_x, local_y); break;
		case FS::SBPump: menu_popup(this->sb_pump_op, g, local_x, local_y); break;
		}
	}
}

#pragma once

#include "planet.hpp"
#include "modbus.hpp"
#include "command.hpp"

#include "snip/togglet.hpp"
#include "snip/textlet.hpp"
#include "snip/statuslet.hpp"
#include "snip/storagelet.hpp"
#include "snip/motorlet.hpp"
#include "snip/gaugelet.hpp"
#include "snip/vibratorlet.hpp"
#include "snip/liquidlet.hpp"
#include "snip/serew/funnellet.hpp"
#include "snip/serew/sleevelet.hpp"
#include "snip/serew/gearboxlet.hpp"
#include "snip/serew/gluecleanerlet.hpp"

namespace WarGrey::SCADA {
	private class BWorkbench : public WarGrey::SCADA::Planet {
	public:
		~BWorkbench() noexcept;
		BWorkbench(Platform::String^ label, Platform::String^ plc);

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;

	public:
		void on_tap(ISnip* snip, float x, float y, bool shifted, bool ctrled) override;

	private:
		WarGrey::SCADA::IModbusConfirmation* console;
		WarGrey::SCADA::CommandMenu<WarGrey::SCADA::Menu>* cmdmenu;
		Platform::String^ caption;
		Platform::String^ device;

	// never deletes these snips mannually	
	private:
		WarGrey::SCADA::Statusbarlet* statusbar;
		WarGrey::SCADA::Statuslinelet* statusline;
		WarGrey::SCADA::Togglet* shift;
	};
}

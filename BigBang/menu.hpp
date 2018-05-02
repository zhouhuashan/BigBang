#pragma once

#include "object.hpp"
#include "graphlet/primitive.hpp"

namespace WarGrey::SCADA {
	WarGrey::SCADA::IGraphlet* menu_current_target_graphlet();

	void menu_append(
		Windows::UI::Xaml::Controls::MenuFlyout^ menu_background,
		Platform::String^ label,
		Windows::UI::Xaml::Input::ICommand^ cmd);

	void menu_show(
		Windows::UI::Xaml::Controls::MenuFlyout^ menu_background,
		WarGrey::SCADA::IGraphlet* g,
		float local_x, float local_y,
		float xoff, float yoff);

	template <typename Menu>
	private class IMenuCommand abstract {
	public:
		virtual void execute(Menu cmd, WarGrey::SCADA::IGraphlet* g) = 0;
	};

	template <typename Menu>
	private ref class MenuCommand sealed : public Windows::UI::Xaml::Input::ICommand {
		/** NOTE	
		 * Interface linguistically is not class,
		 * all the required methods therefore should be marked as `virtual` instead of `override`.
		 */
	internal:
		MenuCommand(IMenuCommand<Menu>* exe, Menu cmd) : executor(exe), command(cmd) {}
		
	public:
		virtual bool CanExecute(Platform::Object^ who_cares) {
			return true;
		}

		virtual void Execute(Platform::Object^ who_cares) {
			this->executor->execute(this->command, menu_current_target_graphlet());
		}

	public:
		event Windows::Foundation::EventHandler<Platform::Object^>^ CanExecuteChanged {
			// this event is useless but to satisfy the C++/CX compiler
			virtual Windows::Foundation::EventRegistrationToken add(Windows::Foundation::EventHandler<Platform::Object^>^ handler) {
				return Windows::Foundation::EventRegistrationToken{ 0L };
			}

			virtual void remove(Windows::Foundation::EventRegistrationToken token) {}
		}

	private:
		WarGrey::SCADA::IMenuCommand<Menu>* executor;
		Menu command;
	};

	template <typename Menu>
	private class CommandMenu {
	public:
		CommandMenu() { this->menu_background = ref new Windows::UI::Xaml::Controls::MenuFlyout(); }

	public:
		void append(Menu cmd, WarGrey::SCADA::IMenuCommand<Menu>* exe) {
			menu_append(this->menu_background, cmd.ToString(), ref new WarGrey::SCADA::MenuCommand<Menu>(exe, cmd));
		}

		void show_for(WarGrey::SCADA::IGraphlet* g, float local_x, float local_y, float xoff = 2.0F, float yoff = 2.0F) {
			menu_show(this->menu_background, g, local_x, local_y, xoff, yoff);
		}

	private:
		Windows::UI::Xaml::Controls::MenuFlyout^ menu_background;
	};
}

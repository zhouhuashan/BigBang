#pragma once

#include "planet.hpp"

namespace WarGrey::SCADA {
	private enum class RR { B1, B2, B3, B4, Count };

    [::Windows::Foundation::Metadata::WebHostHidden]
	public ref class Console sealed : public Windows::UI::Xaml::Controls::SplitView {
	public:
		Console();
		void initialize_component(Windows::Foundation::Size region);

	public:
		void suspend(Windows::ApplicationModel::SuspendingOperation^ op);

	private:
		void animate(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e);
		void animating(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs^ e);
		void animated(Platform::Object^ sender, Platform::Object^ e);

	private:
		float transformX; 
		WarGrey::SCADA::UniverseDisplay^ universe;
		Windows::Foundation::TimeSpan duration;
		Windows::UI::Xaml::Media::Animation::Storyboard^ pt_story;
		Windows::UI::Xaml::Media::Animation::DoubleAnimation^ pt_action;
		Windows::UI::Xaml::Media::Animation::Storyboard^ nt_story;
		Windows::UI::Xaml::Media::Animation::DoubleAnimation^ nt_action;
		Windows::UI::Xaml::Controls::TextBlock^ labels[static_cast<unsigned int>(RR::Count)];
	};
}

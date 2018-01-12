﻿#include "console.hxx"
#include "tongue.hpp"
#include "B.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media::Media3D;

/*************************************************************************************************/
Console::Console() : SplitView() {
	this->Margin = ThicknessHelper::FromUniformLength(4.0);
	this->PanePlacement = SplitViewPanePlacement::Left;

	this->DisplayMode = SplitViewDisplayMode::Overlay;
	this->OpenPaneLength = 48;
	this->IsPaneOpen = false;

	this->transform = ref new CompositeTransform3D();
	this->ManipulationMode = ManipulationModes::TranslateX;
	this->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &Console::animating);
	this->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &Console::animated);
}

Console::~Console() {
	for (size_t i = 0; i < static_cast<unsigned int>(RR::Count); i++) {
		if (this->universes[i] != nullptr) {
			delete this->universes[i];
		}
	}
}

void Console::initialize_component(Size region) {
	ListView^ navigator = ref new ListView();

	navigator->SelectionMode = ListViewSelectionMode::Single;

	for (size_t i = 0; i < static_cast<unsigned int>(RR::Count); i++) {
		auto label = ref new TextBlock();
		this->voids[i] = ref new StackPanel();

		label->Text = speak(speak(static_cast<RR>(i).ToString()));

		navigator->Items->Append(label);
		if (i == 0) {
			navigator->SelectedItem = label;
		}
	}

	// this->universes[0] = new BSegment(this->voids[0], RR::A.ToString(), "192.168.0.188");
	this->universes[0] = new BSegment(this->voids[0], RR::B1.ToString(), "192.168.1.114");
	this->universes[1] = new BSegment(this->voids[0], RR::B2.ToString(), "192.168.1.188");
	//this->universes[2] = new BSegment(this->voids[0], RR::B3.ToString(), "192.168.1.114");
	//this->universes[3] = new BSegment(this->voids[0], RR::B4.ToString(), "192.168.1.114");

	this->Content = this->voids[0];
	this->Pane = navigator;

	this->reflow(region.Width, region.Height);
}

void Console::reflow(float width, float height) {
	for (size_t i = 0; i < static_cast<unsigned int>(RR::Count); i++) {
		if (this->universes[i] != nullptr) {
			this->universes[i]->resize(width, height);
		}
	}
}

void Console::animating(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e) {
	this->transform->TranslateX += e->Delta.Translation.X;
	this->Content->Transform3D = this->transform;
}

void Console::animated(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs^ e) {
	this->Content->Transform3D = nullptr;
}

void Console::suspend(Windows::ApplicationModel::SuspendingOperation^ op) {
	// TODO: Save application state and stop any background activity.
	// Do not assume that the application will be terminated or resumed with the contents of memory still intact.
}

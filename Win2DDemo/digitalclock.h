#pragma once

#include "pasteboard.h"

namespace Win2D::UIElement {
    public ref class DigitalClock sealed : public Win2D::UIElement::Win2DPanel {
    public:
        DigitalClock(Windows::UI::Xaml::Controls::Panel^ parent);

    internal:
        void LoadResources(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesEventArgs^ args) override;
        void Draw(Microsoft::Graphics::Canvas::CanvasDrawingSession^ args) override;

    private:
        void UpdateTimeStamp();
        void OnTickUpdate(Platform::Object^ sender, Platform::Object^ e);

    private:
        Platform::String^ timestamp;
        Platform::String^ datestamp;
        Microsoft::Graphics::Canvas::Text::CanvasTextFormat^ fontInfo;

    private:
        Windows::UI::Xaml::DispatcherTimer^ timer;
        Windows::Globalization::Calendar^ datetime;
        Windows::Globalization::DateTimeFormatting::DateTimeFormatter^ longdate;
        Windows::Globalization::DateTimeFormatting::DateTimeFormatter^ longtime;
    };
}

#include <algorithm>
#include <WindowsNumerics.h>

#include "pasteboard.hpp"

using namespace std;
using namespace Platform;

using namespace Win2D::UIElement;

using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;

using namespace Microsoft::Graphics::Canvas;

struct SnipInfo {
    float x;
    float y;
};

static Thickness zero(0.0, 0.0, 0.0, 0.0);

IPasteboard::IPasteboard(Panel^ parent, String^ id) : IPasteboard(parent, id, zero) {}

IPasteboard::IPasteboard(Panel^ parent, String^ id, Thickness padding) : Win2DCanvas(parent, id) {
    this->inset = padding;
}

IPasteboard::~IPasteboard() {
    while (this->head_snip != nullptr) {
        Snip* snip = this->head_snip;
        this->head_snip = snip->next;

        delete snip;
    }
}

void IPasteboard::insert(Snip* snip, float x, float y) {
    this->before_insert(snip, x, y);
    
    if (this->tail_snip == nullptr) this->tail_snip = snip;
    snip->next = this->head_snip;
    if (this->head_snip != nullptr) this->head_snip->prev = snip;
    this->head_snip = snip;

    snip->info = (void *)new SnipInfo();
    this->move(snip, x, y);

    this->after_insert(snip, x, y);
}

void IPasteboard::move(Snip* snip, float x, float y) {
    SnipInfo* info = (SnipInfo*)snip->info;

    info->x = x;
    info->y = y;
    this->refresh();
}

void IPasteboard::draw(CanvasDrawingSession^ ds) {
    float Width = this->active_width;
    float Height = this->active_height;
    float tx = (float)this->inset.Left;
    float ty = (float)this->inset.Top;
    float width, height;

    // https://blogs.msdn.microsoft.com/win2d/2014/09/15/why-does-win2d-include-three-different-sets-of-vector-and-matrix-types/
    ds->Transform = make_float3x2_translation(tx, ty);
    auto activeRegion = ds->CreateLayer(1.0f, Rect(0.0f, 0.0f, Width, Height));
    for (Snip* child = this->tail_snip; child != nullptr; child = child->prev) {
        SnipInfo* info = (SnipInfo*)child->info;
        child->fill_extent(info->x, info->y, &width, &height);
        width = max(Width - info->x, width);
        height = max(Height - info->y, height);
        auto layer = ds->CreateLayer(1.0f, Rect(info->x, info->y, width, height));
        child->draw(ds, info->x, info->y);
        delete layer /* Must Close the Layer Explicitly */;
    }
    delete activeRegion;
    
    ds->DrawRectangle(0.0f, 0.0f, Width, Height, Colors::SkyBlue);
    ds->DrawRectangle(-tx, -ty, (float)canvas->ActualWidth, (float)canvas->ActualHeight, Colors::RoyalBlue);
}

/*************************************************************************************************/
Pasteboard::Pasteboard(Panel^ parent, String^ id) : Pasteboard(parent, id, zero) {}
Pasteboard::Pasteboard(Panel^ parent, String^ id, Thickness inset) : IPasteboard(parent, id, inset) {}

void Pasteboard::resize(double width, double height) {
    this->canvas->Width = width;
    this->canvas->Height = height;
}

VPasteboard::VPasteboard(Panel^ parent, String^ id) : VPasteboard(parent, id, 0.0f) {}
VPasteboard::VPasteboard(Panel^ parent, String^ id, float gapsize) : VPasteboard(parent, id, gapsize, zero) {}
VPasteboard::VPasteboard(Panel^ parent, String^ id, float gapsize, Thickness inset) : IPasteboard(parent, id, inset) {
    this->gapsize = gapsize;
    this->anchor = -gapsize;
}

void VPasteboard::before_insert(Snip* snip, float x, float y) {
    this->begin_edit_sequence();
}

void VPasteboard::after_insert(Snip* snip, float, float) {
    float Width = this->active_width;
    float x = 0.0f;
    float y = anchor + gapsize;
    float width = 0.0;
    float height = 0.0;
    
    snip->fill_extent(x, y, &width, &height);
    this->move(snip, x, y);
    this->anchor += (gapsize + height);
    this->min_width = max(Width, width);
    this->min_height = this->anchor;
    this->end_edit_sequence();
}

void VPasteboard::resize(double width, double height) {
    this->canvas->Height = height;
}

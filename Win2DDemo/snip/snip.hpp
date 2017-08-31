#pragma once

#include "canvas.hpp"

namespace Win2D::UIElement {
    private enum SnipTypes { Text, Icon };

    private class Snip {
    public:
        virtual SnipTypes GetType() = 0;
        virtual ~Snip() noexcept {};

    public:
        virtual void FillExtent(float x, float y,
            float* width =nullptr, float* height = nullptr,
            float* descent = nullptr, float* space = nullptr,
            float* lspace = nullptr, float* rspace = nullptr)
            = 0;

        virtual void Draw(Microsoft::Graphics::Canvas::CanvasDrawingSession^ ds, float x, float y) = 0;

    public:
        void* info;

    public:
        Snip* next;
        Snip* prev;
    };

    private class SnipIcon : public Snip {
    public:
        ~SnipIcon() noexcept;
        SnipIcon(float size);
        Win2D::UIElement::SnipTypes GetType() override;

    public:
        void FillExtent(float x, float y,
            float* w = nullptr, float* h = nullptr,
            float* d = nullptr, float* s = nullptr,
            float* l = nullptr, float* r = nullptr)
            override;

    protected:
        float size;
    };
}
#include <ppltasks.h>

#include "sprite.hpp"
#include "syslog.hpp"

using namespace WarGrey::SCADA;

using namespace Concurrency;

using namespace Windows::UI;
using namespace Microsoft::Graphics::Canvas;

CanvasRenderTarget^ ISprite::take_snapshot(float dpi) {
	CanvasDevice^ shared_dc = CanvasDevice::GetSharedDevice();
	float width, height;

	this->fill_extent(0.0F, 0.0F, &width, &height);

	{ // WARNING: there is no synchronous mechanism for graphlet.
		CanvasRenderTarget^ snapshot = ref new CanvasRenderTarget(shared_dc, width, height, dpi);
		CanvasDrawingSession^ ds = snapshot->CreateDrawingSession();

		ds->Clear(ColorHelper::FromArgb(0, 255, 255, 255));
		this->draw(ds, 0.0F, 0.0F, width, height);

		return snapshot;
	}
}

void ISprite::save(Platform::String^ path, float dpi) {
	CanvasRenderTarget^ snapshot = this->take_snapshot(dpi);

	create_task(snapshot->SaveAsync(path, CanvasBitmapFileFormat::Auto, 1.0F)).then([=](task<void> saving) {
		try {
			saving.get();
		} catch (Platform::Exception^ e) {
			syslog(Log::Alert, "failed to save graphlet as bitmap:" + e->Message);
		}
	});
}

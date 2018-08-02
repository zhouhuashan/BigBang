#pragma once

Platform::String^ file_name_from_path(Windows::Foundation::Uri^ uri);
Platform::String^ file_name_from_path(Platform::String^ path);

Platform::String^ file_extension_from_path(Windows::Foundation::Uri^ uri);
Platform::String^ file_extension_from_path(Platform::String^ path);

Windows::Foundation::Uri^ ms_appx_file(Platform::String^ file, Platform::String^ ext, Platform::String^ rootdir = "graphlet");

Windows::Foundation::Uri^ ms_appdata_folder(Platform::String^ folder);

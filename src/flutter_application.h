
#pragma once

#include <string>
#include <vector>

#include <flutter_embedder.h>
#include <gdk/gdk.h>
#include <xkbcommon/xkbcommon.h>

namespace flutter {

class Application;

class RenderDisplay {
public:
    virtual void vsync_callback(void *data, intptr_t baton) = 0;
    virtual FlutterRendererConfig renderEngineConfig() = 0;
    virtual void onEngineStarted() = 0;
    Application* application = nullptr;
};

class Application
{
public:
    Application(RenderDisplay* display) {
        display->application = this;
    }

    virtual bool sendWindowMetrics(int32_t physical_width_, int32_t physical_height_, int32_t screen_width_, int32_t screen_height_) = 0;
    virtual void keyboardKey(const GdkEventType type, const xkb_keycode_t hardware_keycode, const xkb_keysym_t keysym, guint state, const uint32_t utf32) = 0;
    virtual void onPointerEvent(const FlutterPointerPhase phase, uint32_t time, double x, double y) = 0;
    virtual FlutterEngineResult onVsync(const intptr_t baton, const uint64_t current_ns, const uint64_t finish_time_ns) = 0;
    virtual uint64_t getCurrentTime() = 0;
    virtual bool isStarted() const = 0;
};

class FlutterApplication : public Application {
public:
    FlutterApplication(RenderDisplay* display, const std::string &bundle_path, const std::vector<std::string> &command_line_args);
    ~FlutterApplication();

    bool sendWindowMetrics(int32_t physical_width_, int32_t physical_height_, int32_t screen_width_, int32_t screen_height_) override;
    void keyboardKey(const GdkEventType type, const xkb_keycode_t hardware_keycode, const xkb_keysym_t keysym, guint state, const uint32_t utf32) override;
    void onPointerEvent(const FlutterPointerPhase phase, uint32_t time, double x, double y) override;
    FlutterEngineResult onVsync(const intptr_t baton, const uint64_t current_ns, const uint64_t finish_time_ns) override;
    uint64_t getCurrentTime() override;
    bool isStarted() const override;
private:
    FlutterEngine engine_ = nullptr;
};

}

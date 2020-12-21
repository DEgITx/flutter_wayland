#include "flutter_application.h"
#include "macros.h"
#include "utils.h"
#include "elf.h"
#include "cify.h"

namespace flutter {

static double get_pixel_ratio(int32_t physical_width, int32_t physical_height, int32_t pixels_width, int32_t pixels_height) {

  if (pixels_width == 0 || physical_height == 0 || pixels_width == 0 || pixels_height == 0) {
    return 1.0;
  }

  // TODO
  return 1.0;
}

FlutterApplication::FlutterApplication(RenderDisplay* display, const std::string &bundle_path, const std::vector<std::string> &command_line_args) : Application(display) {
      FlutterRendererConfig config = display->renderEngineConfig();
      
      auto icu_data_path = GetICUDataPath();

      if (icu_data_path == "") {
        printf("Error: no icu_data_path\n");
        return;
      }

      std::vector<const char *> command_line_args_c;

      for (const auto &arg : command_line_args) {
        command_line_args_c.push_back(arg.c_str());
      }
      
      FlutterProjectArgs args = {
        .struct_size       = sizeof(FlutterProjectArgs),
        .assets_path       = bundle_path.c_str(),
        .icu_data_path     = icu_data_path.c_str(),
        .command_line_argc = static_cast<int>(command_line_args_c.size()),
        .command_line_argv = command_line_args_c.data(),
        .vsync_callback    = cify([display](void *data, intptr_t baton){ display->vsync_callback(data, baton); }),
        .compute_platform_resolved_locale_callback = [](const FlutterLocale **supported_locales, size_t number_of_locales) -> const FlutterLocale * {
          printf("compute_platform_resolved_locale_callback: number_of_locales: %zu\n", number_of_locales);

          if (number_of_locales > 0) {
            return supported_locales[0];
          }

          return nullptr;
        },
    };
    
    std::string libapp_aot_path = bundle_path + "/" + FlutterGetAppAotElfName(); // dw: TODO: There seems to be no convention name we could use, so let's temporary hardcode the path.

    if (FlutterEngineRunsAOTCompiledDartCode()) {
        FLWAY_LOG << "Using AOT precompiled runtime." << std::endl;

        if (std::ifstream(libapp_aot_path)) {
            FLWAY_LOG << "Loading AOT snapshot: " << libapp_aot_path << std::endl;

            const char *error;
            auto handle = Aot_LoadELF(libapp_aot_path.c_str(), 0, &error, &args.vm_snapshot_data, &args.vm_snapshot_instructions, &args.isolate_snapshot_data, &args.isolate_snapshot_instructions);

            if (!handle) {
                FLWAY_ERROR << "Could not load AOT library: " << libapp_aot_path << " (error: " << error << ")" << std::endl;
                return;
            }
        }
    }

    auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, &args, display /* userdata */, &engine_);

    if (result != kSuccess) {
        FLWAY_ERROR << "Could not run the Flutter engine" << std::endl;
        engine_ = nullptr;
        return;
    }

    display->onEngineStarted();
}

FlutterApplication::~FlutterApplication() {
    if (engine_) {
        auto result = FlutterEngineShutdown(engine_);
        if (result == kSuccess) {
            engine_ = nullptr;
        } else {
            FLWAY_ERROR << "Could not shutdown the Flutter engine." << std::endl;
        }
    }
}

bool FlutterApplication::isStarted() const
{
    return engine_ != nullptr;
}

bool FlutterApplication::sendWindowMetrics(int32_t physical_width, int32_t physical_height, int32_t screen_width, int32_t screen_height)
{
    FlutterWindowMetricsEvent event = {};
    event.struct_size               = sizeof(event);
    event.width                     = screen_width;
    event.height                    = screen_height;
    event.pixel_ratio               = get_pixel_ratio(physical_width, physical_height, screen_width, screen_height);

    auto success = FlutterEngineSendWindowMetricsEvent(engine_, &event) == kSuccess;

    FLWAY_LOG << "flutter window metric: " << event.width << "x" << event.height << " par: " << event.pixel_ratio << " status: " << (success ? "success" : "failed") << std::endl;

    return success;
}

void FlutterApplication::keyboardKey(const GdkEventType type, const xkb_keycode_t hardware_keycode, const xkb_keysym_t keysym, guint state, const uint32_t utf32)
{
  if (utf32) {
    if (utf32 >= 0x21 && utf32 <= 0x7E) {
      printf("the key %c was %s\n", (char)utf32, type == GDK_KEY_PRESS ? "pressed" : "released");
    } else {
      printf("the key U+%04X was %s\n", utf32, type == GDK_KEY_PRESS ? "pressed" : "released");
    }
  } else {
    char name[64];
    xkb_keysym_get_name(keysym, name, sizeof(name));

    printf("the key %s was %s\n", name, type == GDK_KEY_PRESS ? "pressed" : "released");
  }

  std::string message;

  // dw: if you do not like so many backslashes,
  // please consider to rerwite it using RapidJson.
  message += "{";
  message += " \"type\":" + std::string(type == GDK_KEY_PRESS ? "\"keydown\"" : "\"keyup\"");
  message += ",\"keymap\":" + std::string("\"linux\"");
  message += ",\"scanCode\":" + std::to_string(hardware_keycode);
  message += ",\"toolkit\":" + std::string("\"gtk\"");
  message += ",\"keyCode\":" + std::to_string(keysym);
  message += ",\"modifiers\":" + std::to_string(state);
  if (utf32) {
    message += ",\"unicodeScalarValues\":" + std::to_string(utf32);
  }

  message += "}";

  if (!message.empty()) {
    bool success = FlutterSendMessage(engine_, "flutter/keyevent", reinterpret_cast<const uint8_t *>(message.c_str()), message.size());

    if (!success) {
      FLWAY_ERROR << "Error sending PlatformMessage: " << message << std::endl;
    }
  }
}

void FlutterApplication::onPointerEvent(const FlutterPointerPhase phase, uint32_t time, double x, double y)
{
    FlutterPointerEvent event = {
        .struct_size    = sizeof(event),
        .phase          = phase,
        .timestamp      = time * 1000,
        .x              = x,
        .y              = y,
        .device         = 0,
        .signal_kind    = kFlutterPointerSignalKindNone,
        .scroll_delta_x = 0,
        .scroll_delta_y = 0,
        .device_kind    = static_cast<FlutterPointerDeviceKind>(0), // dw: TODO: Why kFlutterPointerDeviceKindMouse does not work?
        .buttons        = 0,
    };

    FlutterEngineSendPointerEvent(engine_, &event, 1);
}

FlutterEngineResult FlutterApplication::onVsync(const intptr_t baton, const uint64_t current_ns, const uint64_t finish_time_ns)
{
    return FlutterEngineOnVsync(engine_, baton, current_ns, finish_time_ns);
}

uint64_t FlutterApplication::getCurrentTime()
{
    return FlutterEngineGetCurrentTime();
}

}
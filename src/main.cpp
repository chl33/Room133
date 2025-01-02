// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for
// details.

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <cstring>

#if HAVE_OLED
#include "SSD1306Wire.h"
#endif

#include <og3/blink_led.h>
#include <og3/constants.h>
#include <og3/ha_app.h>
#include <og3/html_table.h>
#include <og3/kernel_filter.h>
#include <og3/oled_wifi_info.h>
#include <og3/shtc3.h>
#include <og3/units.h>
#include <og3/variable.h>

#if HAVE_MOTION_LIGHT || HAVE_MOTION
#include <og3/pir.h>
#endif
#if HAVE_MOTION_LIGHT || HAVE_LEAK
#include <og3/kernel_filter.h>
#include <og3/mapped_analog_sensor.h>
#include <og3/motion_detector.h>
#endif

#define VERSION "0.9.8"

namespace og3 {

static const char kManufacturer[] = "Chris Lee";
static const char kModel[] = "Room133";
static const char kSoftware[] = "Roomlee2 v" VERSION;

#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
constexpr App::LogType kLogType = App::LogType::kUdp;
#else
// constexpr App::LogType kLogType = App::LogType::kNone;  // kSerial
constexpr App::LogType kLogType = App::LogType::kSerial;
#endif

HAApp s_app(HAApp::Options(kManufacturer, kModel,
                           WifiApp::Options()
                               .withSoftwareName(kSoftware)
                               .withDefaultDeviceName("rooml33")
#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
                               .withUdpLogHost(IPAddress(LOG_UDP_ADDRESS))

#endif
                               .withOta(OtaManager::Options(OTA_PASSWORD))
                               .withApp(App::Options().withLogType(kLogType))));

// Hardware config
#if HAVE_MOTION_LIGHT
#if !OLD_PINS
constexpr uint8_t kPirPin1 = 25;
constexpr uint8_t kLightPin = 33;
#else
constexpr uint8_t kPirPin1 = 16;
constexpr uint8_t kLightPin = 32;
#endif
#endif
#if HAVE_MOTION
#if !OLD_PINS
constexpr uint8_t kPirPin2 = 23;
#else
constexpr uint8_t kPirPin2 = 17;
#endif
#endif
#if HAVE_LEAK
constexpr uint8_t kLeakPin = 32;
#endif

// Whoops -- pins 34 and 35 are input-only.
// constexpr uint8_t kRedLed = 34;
// constexpr uint8_t kYellowLed = 35;
constexpr uint8_t kBlueLed = 27;

// Names
static const char kTemperature[] = "temperature";
static const char kHumidity[] = "humidity";
#if HAVE_MOTION_LIGHT
static const char kLight[] = "light";
static const char kPirModule[] = "pir";
static const char kMotion[] = "motion";
#endif
#if HAVE_MOTION
static const char kPirModule2[] = "pir2";
static const char kMotion2[] = "motion2";
#endif
#if HAVE_LEAK
static const char kLeakSensor[] = "leak";
constexpr float kLeakKernelSec = 2 * kSecInMin;
constexpr int kLeakDecimals = 1;
#endif

// Global variable for html, so asyncwebserver can send data in the background
// (single client)
String s_html;

#if HAVE_OLED
// Delay between updates of the OLED.
constexpr unsigned kOledSwitchMsec = 5000;
OledDisplayRing s_oled(&s_app.module_system(), kSoftware, kOledSwitchMsec, Oled::kTenPt);
#endif

#if HAVE_LEAK
class LeakSensor {
 public:
  LeakSensor(const char* name, ModuleSystem* module_system_, VariableGroup& cfgvg,
             VariableGroup& vg)
      : m_leak_sensor(
            MappedAnalogSensor::Options{
                .name = name,
                .pin = kLeakPin,
                .units = units::kPercentage,
                .description = "leak %",
                .raw_var_flags = 0,
                .mapped_var_flags = 0,
                .config_flags = VariableBase::kConfig | VariableBase::Flags::kSettable,
                .default_in_min = 9,  // kAdcMax
                .default_in_max = 4095,
                .default_out_min = 0.0f,
                .default_out_max = 100.0f,
                .config_decimals = 0,
                .decimals = 0,
                .valid_in_min = 0,
                .valid_in_max = 4095,
            },
            module_system_, cfgvg, vg),
        m_filtered_name(String(name) + "_filtered"),
        m_filter(
            {
                .name = m_filtered_name.c_str(),
                .units = units::kPercentage,
                .description = "filtered moisture",
                .var_flags = 0,
                .sigma = kLeakKernelSec,
                .decimals = 1,
                .size = KernelFilter::kDefaultNumSamples,
            },
            module_system_, vg) {}

  void read() {
    const float val = m_leak_sensor.read();  // TODO(chrishl): check is reasonable value
    const float secs = 1e-3 * millis();
    m_filter.addSample(secs, val);
  }

  const FloatVariable& value() { return m_filter.valueVariable(); }

 private:
  MappedAnalogSensor m_leak_sensor;
  String m_filtered_name;
  KernelFilter m_filter;
};
#endif

class Monitor : public Module {
 public:
  Monitor(HAApp* app)
      : Module("monitor", &app->module_system()),
        m_app(app),
        // Every minute, read sensors and send readings via MQTT, starting in 10
        // seconds.
        m_mqtt_scheduler(
            10 * kMsecInSec, kMsecInMin, [this]() { sendMqtt(); }, &app->tasks()),
        m_cvg("room_cfg"),
        m_vg("room"),
#if HAVE_MOTION_LIGHT
        m_light_sensor(
            MappedAnalogSensor::Options{
                .name = kLight,
                .pin = kLightPin,
                .units = units::kPercentage,
                .description = "light %",
                .raw_var_flags = 0,
                .mapped_var_flags = 0,
                .config_flags = VariableBase::kConfig | VariableBase::Flags::kSettable,
                .default_in_min = 4095,  // kAdcMax
                .default_in_max = 600,
                .default_out_min = 0.0f,
                .default_out_max = 100.0f,
                .config_decimals = 0,
                .decimals = 1,
                .valid_in_min = 0,
                .valid_in_max = 4095,
            },
            &app->module_system(), m_cvg, m_vg),
#endif  // HAVE_MOTION_LIGHT
#if HAVE_LEAK
        m_leak_sensor(kLeakSensor, &app->module_system(), m_cvg, m_vg),
#endif
#if HAVE_MOTION_LIGHT
        m_pir1(kPirModule, kMotion, &app->module_system(), kPirPin1, kMotion, m_vg, true, true),
#endif
#if HAVE_MOTION
        m_pir2(kPirModule2, kMotion2, &app->module_system(), kPirPin2, kMotion2, m_vg, true, true),
#endif
#if HAVE_OLED
        m_wifi_oled(&app->tasks()),
#endif
        m_blu_blink("blu_blink", kBlueLed, app, 500, false),
        m_shtc3(kTemperature, kHumidity, &app->module_system(), "temperature", m_vg) {
    setDependencies(&m_dependencies);
    add_init_fn([this]() {
      if (m_dependencies.ok()) {
#if HAVE_MOTION_LIGHT
        m_dependencies.ha_discovery()->addDiscoveryCallback(
            [this](HADiscovery* had, JsonDocument* json) {
              return had->addMeas(json, m_light_sensor.mapped_value(), ha::device_type::kSensor,
                                  nullptr);
            });
#endif
#if HAVE_LEAK
        m_dependencies.ha_discovery()->addDiscoveryCallback(
            [this](HADiscovery* had, JsonDocument* json) {
              return had->addMeas(json, m_leak_sensor.value(), ha::device_type::kSensor, nullptr);
            });
#endif
      }
#if HAVE_MOTION_LIGHT
      m_pir1.callOnMotion([this]() { onMotion(); });
#endif
#if HAVE_MOTION
      m_pir2.callOnMotion([this]() { onMotion(); });
#endif
#if HAVE_OLED
      s_oled.addDisplayFn([this]() {
        char text[80];
        snprintf(text, sizeof(text), "%.1fC %.1fRH", m_shtc3.temperature(), m_shtc3.humidity());
        s_oled.display(text);
      });
#endif
      m_app->config().read_config(m_vg);
    });
  }
  const VariableGroup& vg() const { return m_vg; }

  void handleConfigRequest(AsyncWebServerRequest* request) {
    ::og3::read(*request, m_cvg);
    s_html.clear();
    html::writeFormTableInto(&s_html, m_cvg);
    s_html += F(HTML_BUTTON("/", "Back"));
    sendWrappedHTML(request, kSoftware, kSoftware, s_html.c_str());
    s_app.config().write_config(m_cvg);
  }

  void readSensors() {
    m_shtc3.read();
#if HAVE_MOTION_LIGHT
    m_pir1.read();
#endif
#if HAVE_MOTION
    m_pir2.read();
#endif
#if HAVE_MOTION_LIGHT
    m_light_sensor.read();
#endif
#if HAVE_LEAK
    m_leak_sensor.read();
#endif
  }

 private:
  Logger& log() { return m_app->log(); }
  void sendMqtt() {
    readSensors();
    m_app->mqttSend(m_vg);

#if HAVE_MOTION_LIGHT && HAVE_MOTION
    log().debugf("OK? %s t:%.1f h:%.1f%% M1:%s lr:%d, M2:%s l:%.1f", m_shtc3.ok() ? "T" : "F",
                 m_shtc3.temperature(), m_shtc3.humidity(), m_pir1.motion() ? "T" : "F",
                 m_light_sensor.raw_counts(), m_pir2.motion() ? "T" : "F", m_light_sensor.value());
#elif HAVE_MOTION_LIGHT
    log().debugf("OK? %s t:%.1f h:%.1f%% lr:%d, l:%.1f M:%s", m_shtc3.ok() ? "T" : "F",
                 m_shtc3.temperature(), m_shtc3.humidity(), m_light_sensor.raw_counts(),
                 m_light_sensor.value(), m_pir1.motion() ? "T" : "F");
#elif HAVE_MOTION
    log().debugf("OK? %s t:%.1f h:%.1f%% M:%s", m_shtc3.ok() ? "T" : "F", m_shtc3.temperature(),
                 m_shtc3.humidity(), m_pir2.motion() ? "T" : "F");
#else
    log().debugf("OK? %s t:%.1f h:%.1f%%", m_shtc3.ok() ? "T" : "F", m_shtc3.temperature(),
                 m_shtc3.humidity());
#endif
  }

  void onMotion() {
    sendMqtt();
#if HAVE_MOTION_LIGHT
    if (m_pir1.motion()) {
      m_blu_blink.blink();
      log().debug("Motion1!");
#if HAVE_OLED
      s_oled.display("Motion");
#endif
    }
#endif
#if HAVE_MOTION
    if (m_pir2.motion()) {
      m_blu_blink.blink(2);
      log().debug("Motion2!");
#if HAVE_OLED
      s_oled.display("Motion2");
#endif
    }
#endif
    // Run update in 10 seconds to allow motion-off status to get published.
    // m_mqtt_scheduler.runIn(10 * kMsecInSec);
  }

  HAApp* const m_app;
  HADependencies m_dependencies;
  // Send configuration every 5 minutes.
  PeriodicTaskScheduler m_mqtt_scheduler;
  VariableGroup m_cvg;
  VariableGroup m_vg;
#if HAVE_MOTION_LIGHT
  MappedAnalogSensor m_light_sensor;
#endif  // HAVE_MOTION_LIGHT
#if HAVE_LEAK
  LeakSensor m_leak_sensor;
#endif
#if HAVE_MOTION_LIGHT
  Pir m_pir1;
#endif
#if HAVE_MOTION
  Pir m_pir2;
#endif
#if HAVE_OLED
  OledWifiInfo m_wifi_oled;
#endif
  BlinkLed m_blu_blink;
  Shtc3 m_shtc3;
};

Monitor s_monitor(&s_app);

WebButton s_button_wifi_config = s_app.createWifiConfigButton();
WebButton s_button_mqtt_config = s_app.createMqttConfigButton();
WebButton s_button_app_status = s_app.createAppStatusButton();
WebButton s_button_restart = s_app.createRestartButton();

void handleWebRoot(AsyncWebServerRequest* request) {
  s_monitor.readSensors();
  s_html.clear();
  html::writeTableInto(&s_html, s_monitor.vg());
  html::writeTableInto(&s_html, s_app.wifi_manager().variables());
  html::writeTableInto(&s_html, s_app.mqtt_manager().variables());
  s_html += F(HTML_BUTTON("/config", "Config"));
  s_button_wifi_config.add_button(&s_html);
  s_button_mqtt_config.add_button(&s_html);
  s_button_app_status.add_button(&s_html);
  s_button_restart.add_button(&s_html);
  sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_html.c_str());
}

}  // namespace og3

////////////////////////////////////////////////////////////////////////////////

void setup() {
  og3::s_app.web_server().on("/", og3::handleWebRoot);
  og3::s_app.web_server().on("/config", [](AsyncWebServerRequest* request) {
    og3::s_monitor.handleConfigRequest(request);
  });
  og3::s_app.setup();
}

void loop() { og3::s_app.loop(); }

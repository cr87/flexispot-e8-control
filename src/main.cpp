#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

#include "webserver.h"
#include "deskcontrol.h"
#include "scheduler.h"
#include "ntp.h"
#include "log.h"

/*****************************************************************************/

const unsigned long SERIAL_BAUDRATE = 115200;
const unsigned int WEBSERVER_PORT = 8080;
const char *NTP_SERVER = "pool.ntp.org";
const int NTP_TIME_DIFF = 2 * 3600;
const char *wifi_mod_str = "WiFi";
const LOG_LEVEL LOGLEVEL = LOG_LEVEL::LOG_LEVEL_INFO;

/* Settings likely to be modified by a user */

const char* WIFI_SSID = "put-your-wifi-ssid-here";
const char* WIFI_PASS = "put-your-wifi-password-here";

/*****************************************************************************/

void init_wifi(const char *ssid, const char *password);
void set_default_config();
void set_test_config();
void led_loop();

/*****************************************************************************/

void setup(void)
{
  /* Setup serial communication to computer using Arduino-Log */
  Serial.begin(SERIAL_BAUDRATE);
  log_set_global_level(LOGLEVEL);

  /* Initialize wifi and NTP */
  init_wifi(WIFI_SSID, WIFI_PASS);
  ntp_init(NTP_SERVER, NTP_TIME_DIFF);

  /* Initialize modules */
  ws_init(WEBSERVER_PORT, dc_get_current_height, dc_send_cmd, ntp_get_current_time);
  dc_init();
  sc_init();
  sc_set_time_provider(ntp_get_current_time);
  sc_set_desk_command_receiver(dc_send_cmd);
  sc_set_desk_state_provider(dc_get_current_state);

  /* Set our default config for now */
  set_default_config();

  /* LED for status notification */
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop(void)
{
  ntp_loop();
  dc_loop();
  ws_loop();
  sc_loop();
  led_loop();
}

/*****************************************************************************/

void init_wifi(const char *ssid, const char *password)
{
  log_msg(LOG_LEVEL::LOG_LEVEL_INFO, wifi_mod_str, "Connecting to WiFi.");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
  }

  log_msg(LOG_LEVEL::LOG_LEVEL_INFO, wifi_mod_str, "Connected with IP %i.%i.%i.%i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  if (MDNS.begin("esp8266"))
  {
    log_msg(LOG_LEVEL_INFO, "WebServer", "mDNS responder started.");
  }
  else
  {
    log_msg(LOG_LEVEL_ERROR, "WebServer", "Error setting up MDNS responder.");
  }
}

void set_default_config()
{
  /* Day configs */
  const DAY_CONFIG weekday_config = {
      .start_time = {.hour_u8 = 9U, .minute_u8 = 0U, .second_u8 = 0U}, /* start at 9am */
      .end_time = {.hour_u8 = 17U, .minute_u8 = 0U, .second_u8 = 0U},  /* end at 5pm */
      .interval_u16 = 60U * 60U,                                       /* activate every hour on the hour */
      .duration_u16 = 15U * 60U,                                       /* for 15 minutes...*/
      .enabled = 1};
  const DAY_CONFIG weekend_config = {0}; /* just disabled */

  /* Sitting/standing positions - in millimeter! */
  dc_set_params(1150U, 750U, 50U);

  /* Set for all weekdays */
  sc_set_day_config(MONDAY, &weekday_config);
  sc_set_day_config(TUESDAY, &weekday_config);
  sc_set_day_config(WEDNESDAY, &weekday_config);
  sc_set_day_config(THURSDAY, &weekday_config);
  sc_set_day_config(FRIDAY, &weekday_config);

  /* Set disabled for weekend */
  sc_set_day_config(SATURDAY, &weekend_config);
  sc_set_day_config(SUNDAY, &weekend_config);
}

void set_test_config()
{
  /* Day configs */
  const DAY_CONFIG test_config = {
      .start_time = {.hour_u8 = 0U, .minute_u8 = 0U, .second_u8 = 0U},
      .end_time = {.hour_u8 = 23U, .minute_u8 = 59U, .second_u8 = 59U},
      .interval_u16 = 3U * 60U,
      .duration_u16 = 1 * 60U,
      .enabled = 1};

  /* Sitting/standing positions - in millimeter! */
  dc_set_params(1150U, 750U, 50U);

  /* Set same test config for every day */
  sc_set_day_config(MONDAY, &test_config);
  sc_set_day_config(TUESDAY, &test_config);
  sc_set_day_config(WEDNESDAY, &test_config);
  sc_set_day_config(THURSDAY, &test_config);
  sc_set_day_config(FRIDAY, &test_config);
  sc_set_day_config(SATURDAY, &test_config);
  sc_set_day_config(SUNDAY, &test_config);
}

void led_loop()
{
  static uint8 led_state_u8 = LOW;

  /* Toggle state every x milliseconds */
  ONCE_EVERY_MS(500);
  led_state_u8 = (led_state_u8 == HIGH) ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, led_state_u8);
}

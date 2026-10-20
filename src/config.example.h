// save this as config.h, and add your own settings

// Wi-Fi settings
const char *WIFI_SSID = "your wifi name goes here";
const char *WIFI_PASSWORD = "your wifi password goes here";

// time settings
const char *TIME_NTP_SERVER = "pool.ntp.org";
const char *TIME_ZONE = "GMT0BST,M3.5.0/1,M10.5.0"; // Europe/London

// MQTT settings
const char *MQTT_BROKER = "host name or IP address";
const char *MQTT_TOPIC = "hub75-clock/message";
const char *MQTT_CLIENT_ID = "hub75-clock";
const char *MQTT_USERNAME = "";
const char *MQTT_PASSWORD = "";
const uint16_t MQTT_PORT = 1883;

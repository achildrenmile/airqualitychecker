// Replace with your network credentials
const char* ssid = "<yourwifissid>";
const char* password = "<yourwifipassword>";

// Replace with your MQTT Server information
char* mqtt_server = "<ipaddress or hostname to your broker>";
char* mqtt_name = "Environment Sensor 1"; //mqtt device name
char* mqtt_user="<mqtt broker user>";
char* mqtt_pass="<mqtt broker pass>";

char* mqtt_maintopic = "homeassistant/environmentzone1/data";
char* mqtt_temperaturetopic = "homeassistant/environmentzone1/data/temperature";
char* mqtt_humiditytopic ="homeassistant/environmentzone1/data/humidity";
char* mqtt_pressuretopic ="homeassistant/environmentzone1/data/pressure";
char* mqtt_altitudetopic ="homeassistant/environmentzone1/data/altitude";
char* mqtt_aiqtopic = "homeassistant/environmentzone1/data/aiq";


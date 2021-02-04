// Replace with your network credentials
const char* ssid = "<yourwifissid>";
const char* password = "<yourwifipassword>";

// Replace with your MQTT Server information
char* mqtt_server = "<ipaddress or hostname to your broker>";
char* mqtt_name = "Environment Sensor 1"; //mqtt device name
char* mqtt_user="<mqtt broker user>";
char* mqtt_pass="<mqtt broker pass>";

char* mqtt_maintopic = "homeassistant/environmentzone1/data"; //define an mqtt topic

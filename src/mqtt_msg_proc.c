#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "mqtt.h"
#include "config.h"
#include "ftp-manager.h"

int mqtt_msg_proc(char *topic, char *payload, int payloadlen)
{
	ftp_download("./ZBox_IOT_Service","ftp://113.240.239.164/Service/zoomlion/ZBox_IOT_Service");
	return 0;
}

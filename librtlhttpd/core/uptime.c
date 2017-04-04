#include <FreeRTOS.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "httpd.h"
#include "httpd-platform.h"


/*void uptime_timer_init(void)
{

} */

void uptime_str(char *buf)
{
	uint32_t a = xTaskGetTickCount() / configTICK_RATE_HZ;
	uint32_t days = a / 86400;
	a -= days * 86400;

	uint32_t hours = a / 3600;
	a -= hours * 3600;

	uint32_t mins = a / 60;
	a -= mins * 60;

	uint32_t secs = a;

	if (days > 0) {
		sprintf(buf, "%ud %02u:%02u:%02u", days, hours, mins, secs);
	} else {
		sprintf(buf, "%02u:%02u:%02u", hours, mins, secs);
	}
}

void uptime_print(void)
{
	uint32_t a = xTaskGetTickCount() / configTICK_RATE_HZ;
	uint32_t days = a / 86400;
	a -= days * 86400;

	uint32_t hours = a / 3600;
	a -= hours * 3600;

	uint32_t mins = a / 60;
	a -= mins * 60;

	uint32_t secs = a;

	if (days > 0) {
		httpd_printf("%ud %02u:%02u:%02u", days, hours, mins, secs);
	} else {
		httpd_printf("%02u:%02u:%02u", hours, mins, secs);
	}
}


#ifndef __MAIN_H__
#define __MAIN_H__

#include "esp_event.h"


extern "C" void app_main(void);
esp_err_t event_handler(void *ctx, system_event_t *event);

#endif

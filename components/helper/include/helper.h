#include "esp_err.h"

#ifndef HELPERS_H
#define HELPERS_H

#define ESP_RETURN_ERROR(x) do {      \
    esp_err_t __err_rc = (x);         \
    if (__err_rc != ESP_OK) {         \
        return __err_rc;              \
    }                                 \
} while(0)

#endif
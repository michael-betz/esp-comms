#ifndef JSON_SETTINGS_H
#define JSON_SETTINGS_H

#include "cJSON.h"
#include <ArduinoWebsockets.h>

void settings_ws_handler(websockets::WebsocketsMessage msg);

// set the .json file with settings and a fall-back defaults_file
// which will be copied over if not NULL
void set_settings_file(const char *f_settings, const char *f_defaults);

// loads the SETTINGS_FILE (if not loaded yet) and returns the cJSON object
cJSON *getSettings();

// Reloads the SETTINGS_FILE
void reloadSettings();

// Get item "string" from .json object. Returns NULL on error
#define jGet cJSON_GetObjectItemCaseSensitive

// return string from .json or default-value on error
const char *jGetS(const cJSON *json, const char *name, const char *default_val);

// return integer from .json or default-value on error
int jGetI(cJSON *json, const char *name, int default_val);

// return double from .json or default-value on error
int jGetD(cJSON *json, const char *name, double default_val);

// return bool from .json or default-value on error
int jGetB(cJSON *json, const char *name, bool default_val);

#endif

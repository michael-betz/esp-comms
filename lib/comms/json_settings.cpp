#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include "cJSON.h"
#include "json_settings.h"
#include "comms.h"

static const char *settings_file;
static char *txtData = NULL;

void settings_ws_handler(websockets::WebsocketsMessage msg)
{
    if (msg.length() <= 0)
        return;
    if (msg.rawData()[0] == 'c') {
        if (msg.length() > 1) {
            FILE *dest = fopen(settings_file, "wb");
            if (dest) {
                fputs(&msg.c_str()[1], dest);
                fclose(dest);
                dest = NULL;
                log_i("re-wrote %s", settings_file);
                reloadSettings();
            } else {
                log_e("fopen(%s, wb) failed: %s", settings_file, strerror(errno));
            }
        }
        g_ws_client->send(String("c") + txtData);
    }
    if (msg.rawData()[0] == 'r') ESP.restart();
}

void set_settings_file(const char *f_settings, const char *f_defaults)
{
    settings_file = f_settings;

    if (f_defaults && getSettings() == NULL) {
        char buf[32];
        size_t size;
        log_w("writing default-settings to %s", settings_file);
        FILE* source = fopen(f_defaults, "rb");
        FILE* dest = fopen(settings_file, "wb");
        if (source && dest) {
            while ((size = fread(buf, 1, sizeof(buf), source))) {
                fwrite(buf, 1, size, dest);
            }
        } else {
            log_e("could not copy %s to %s: %s", f_defaults, settings_file, strerror(errno));
        }

        if (source) fclose(source);
        if (dest) fclose(dest);
    }
}

char *readFileDyn(const char* file_name, int *file_size) {
    // opens the file file_name and returns it as dynamically allocated char*
    // if file_size is not NULL, copies the number of bytes read there
    // dont forget to call free() on the char* result
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        log_e("fopen(%s, rb) failed: %s", file_name, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    log_d("loading %s, fsize: %d", file_name, fsize);
    char *string = (char*) malloc(fsize + 1);
    if (!string) {
        log_e("malloc(%d) failed: %s", fsize + 1, strerror(errno));
        assert(false);
    }
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    if (file_size)
        *file_size = fsize;
    return string;
}

cJSON *readJsonDyn(const char* file_name) {
    // opens the json file `file_name` and returns it as cJSON*
    // don't forget to call cJSON_Delete() on it
    cJSON *root;

    // try to read settings.json from SD card
    // we keep txtData around for the web-interface
    free(txtData);
    txtData = readFileDyn(file_name, NULL);
    if (!txtData)
        return NULL;

    // load txtData as .json
    if (!(root = cJSON_Parse(txtData))) {
        log_e("JSON Error in %s", file_name);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            log_e("Error before: %s", error_ptr);
        }
    }
    return root;
}

static cJSON *g_settings = NULL;
cJSON *getSettings() {
    if (!g_settings) {
        g_settings = readJsonDyn(settings_file);
    }
    return g_settings;
}

void reloadSettings() {
    cJSON_Delete(g_settings);
    g_settings = NULL;
    free(txtData);
    txtData = NULL;
    getSettings();
}

// return string from .json or default-value on error
const char *jGetS(const cJSON *json, const char *name, const char *default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if(!cJSON_IsString(j)) {
        log_e("%s is not a string, falling back to %s", name, default_val);
        return default_val;
    }
    return j->valuestring;
}

// return integer from .json or default-value on error
int jGetI(cJSON *json, const char *name, int default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsNumber(j)) {
        log_e("%s is not a number, falling back to %d", name, default_val);
        return default_val;
    }
    return j->valueint;
}

// return double from .json or default-value on error
int jGetD(cJSON *json, const char *name, double default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsNumber(j)) {
        log_e("%s is not a number, falling back to %f", name, default_val);
        return default_val;
    }
    return j->valuedouble;
}

// return bool from .json or default-value on error
int jGetB(cJSON *json, const char *name, bool default_val) {
    const cJSON *j = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsBool(j)) {
        log_e("%s is not a bool, falling back to %s", name, default_val ? "true" : "false");
        return default_val;
    }
    return cJSON_IsTrue(j);
}

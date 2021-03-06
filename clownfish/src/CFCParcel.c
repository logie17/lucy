/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifndef true
  #define true 1
  #define false 0
#endif

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCParcel.h"
#include "CFCVersion.h"
#include "CFCUtil.h"

struct CFCParcel {
    CFCBase base;
    char *name;
    char *cnick;
    CFCVersion *version;
    char *prefix;
    char *Prefix;
    char *PREFIX;
};

static CFCParcel *default_parcel = NULL;

#define JSON_STRING 1
#define JSON_HASH   2

typedef struct JSONNode {
    int type;
    char *string;
    struct JSONNode **kids;
    size_t num_kids;
} JSONNode;

static JSONNode*
S_parse_json_for_parcel(const char *json);

static JSONNode*
S_parse_json_hash(const char **json);

static JSONNode*
S_parse_json_string(const char **json);

static void
S_skip_whitespace(const char **json);

static void
S_destroy_json(JSONNode *node);

static CFCParcel **registry = NULL;
static size_t num_registered = 0;

CFCParcel*
CFCParcel_fetch(const char *name) {
    // Return the default parcel for either a blank name or a NULL name.
    if (!name || !strlen(name)) {
        return CFCParcel_default_parcel();
    }

    for (size_t i = 0; i < num_registered ; i++) {
        CFCParcel *existing = registry[i];
        if (strcmp(existing->name, name) == 0) {
            return existing;
        }
    }

    return NULL;
}

void
CFCParcel_register(CFCParcel *self) {
    CFCParcel *existing = CFCParcel_fetch(self->name);
    if (existing) {
        CFCUtil_die("Parcel '%s' already registered", self->name);
    }
    if (!num_registered) {
        // Init default parcel as first.
        registry = (CFCParcel**)CALLOCATE(3, sizeof(CFCParcel*));
        CFCParcel *def = CFCParcel_default_parcel();
        registry[0] = (CFCParcel*)CFCBase_incref((CFCBase*)def);
        num_registered++;
    }
    size_t size = (num_registered + 2) * sizeof(CFCParcel*);
    registry = (CFCParcel**)REALLOCATE(registry, size);
    registry[num_registered++] = (CFCParcel*)CFCBase_incref((CFCBase*)self);
    registry[num_registered]   = NULL;
}

void
CFCParcel_reap_singletons(void) {
    for (size_t i = 0; i < num_registered; i++) {
        CFCBase_decref((CFCBase*)registry[i]);
    }
    FREEMEM(registry);
    num_registered = 0;
    registry = NULL;
    CFCBase_decref((CFCBase*)default_parcel);
    default_parcel = NULL;
}

static int
S_validate_name_or_cnick(const char *orig) {
    const char *ptr = orig;
    for (; *ptr != 0; ptr++) {
        if (!isalpha(*ptr)) { return false; }
    }
    return true;
}

const static CFCMeta CFCPARCEL_META = {
    "Clownfish::CFC::Model::Parcel",
    sizeof(CFCParcel),
    (CFCBase_destroy_t)CFCParcel_destroy
};

CFCParcel*
CFCParcel_new(const char *name, const char *cnick, CFCVersion *version) {
    CFCParcel *self = (CFCParcel*)CFCBase_allocate(&CFCPARCEL_META);
    return CFCParcel_init(self, name, cnick, version);
}

CFCParcel*
CFCParcel_init(CFCParcel *self, const char *name, const char *cnick,
               CFCVersion *version) {
    // Validate name.
    if (!name || !S_validate_name_or_cnick(name)) {
        CFCUtil_die("Invalid name: '%s'", name ? name : "[NULL]");
    }
    self->name = CFCUtil_strdup(name);

    // Validate or derive cnick.
    if (cnick) {
        if (!S_validate_name_or_cnick(cnick)) {
            CFCUtil_die("Invalid cnick: '%s'", cnick);
        }
        self->cnick = CFCUtil_strdup(cnick);
    }
    else {
        // Default cnick to name.
        self->cnick = CFCUtil_strdup(name);
    }

    // Default to version v0.
    if (version) {
        self->version = (CFCVersion*)CFCBase_incref((CFCBase*)version);
    }
    else {
        self->version = CFCVersion_new("v0");
    }

    // Derive prefix, Prefix, PREFIX.
    size_t cnick_len  = strlen(self->cnick);
    size_t prefix_len = cnick_len ? cnick_len + 1 : 0;
    size_t amount     = prefix_len + 1;
    self->prefix = (char*)MALLOCATE(amount);
    self->Prefix = (char*)MALLOCATE(amount);
    self->PREFIX = (char*)MALLOCATE(amount);
    memcpy(self->Prefix, self->cnick, cnick_len);
    if (cnick_len) {
        self->Prefix[cnick_len]  = '_';
        self->Prefix[cnick_len + 1]  = '\0';
    }
    else {
        self->Prefix[cnick_len] = '\0';
    }
    for (size_t i = 0; i < amount; i++) {
        self->prefix[i] = tolower(self->Prefix[i]);
        self->PREFIX[i] = toupper(self->Prefix[i]);
    }
    self->prefix[prefix_len] = '\0';
    self->Prefix[prefix_len] = '\0';
    self->PREFIX[prefix_len] = '\0';

    return self;
}

static CFCParcel*
S_new_from_json(const char *json, const char *path) {
    JSONNode *parsed = S_parse_json_for_parcel(json);
    if (!parsed) {
        CFCUtil_die("Invalid JSON parcel definition in '%s'", path);
    }
    const char *name     = NULL;
    const char *nickname = NULL;
    CFCVersion *version  = NULL;
    for (size_t i = 0, max = parsed->num_kids; i < max; i += 2) {
        JSONNode *key   = parsed->kids[i];
        JSONNode *value = parsed->kids[i + 1];
        if (key->type != JSON_STRING) {
            CFCUtil_die("JSON parsing error (filepath '%s')", path);
        }
        if (strcmp(key->string, "name") == 0) {
            if (value->type != JSON_STRING) {
                CFCUtil_die("'name' must be a string (filepath %s)", path);
            }
            name = value->string;
        }
        else if (strcmp(key->string, "nickname") == 0) {
            if (value->type != JSON_STRING) {
                CFCUtil_die("'nickname' must be a string (filepath %s)",
                            path);
            }
            nickname = value->string;
        }
        else if (strcmp(key->string, "version") == 0) {
            if (value->type != JSON_STRING) {
                CFCUtil_die("'version' must be a string (filepath %s)",
                            path);
            }
            version = CFCVersion_new(value->string);
        }
    }
    if (!name) {
        CFCUtil_die("Missing required key 'name' (filepath '%s')", path);
    }
    if (!version) {
        CFCUtil_die("Missing required key 'version' (filepath '%s')", path);
    }
    CFCParcel *self = CFCParcel_new(name, nickname, version);
    CFCBase_decref((CFCBase*)version);

    for (size_t i = 0, max = parsed->num_kids; i < max; i += 2) {
        JSONNode *key   = parsed->kids[i];
        JSONNode *value = parsed->kids[i + 1];
        if (strcmp(key->string, "name") == 0
            || strcmp(key->string, "nickname") == 0
            || strcmp(key->string, "version") == 0
           ) {
            ;
        }
        else {
            CFCUtil_die("Unrecognized key: '%s' (filepath '%s')",
                        key->string, path);
        }
    }

    S_destroy_json(parsed);
    return self;
}

CFCParcel*
CFCParcel_new_from_json(const char *json) {
    return S_new_from_json(json, "[NULL]");
}

CFCParcel*
CFCParcel_new_from_file(const char *path) {
    size_t len;
    char *json = CFCUtil_slurp_text(path, &len);
    CFCParcel *self = S_new_from_json(json, path);
    FREEMEM(json);
    return self;
}

void
CFCParcel_destroy(CFCParcel *self) {
    FREEMEM(self->name);
    FREEMEM(self->cnick);
    CFCBase_decref((CFCBase*)self->version);
    FREEMEM(self->prefix);
    FREEMEM(self->Prefix);
    FREEMEM(self->PREFIX);
    CFCBase_destroy((CFCBase*)self);
}

CFCParcel*
CFCParcel_default_parcel(void) {
    if (default_parcel == NULL) {
        default_parcel = CFCParcel_new("", "", NULL);
    }
    return default_parcel;
}

CFCParcel*
CFCParcel_clownfish_parcel(void) {
    CFCParcel *parcel = CFCParcel_fetch("Lucy");
    if (!parcel) {
        CFCVersion *version = CFCVersion_new("v0.3.0");
        parcel = CFCParcel_new("Lucy", "Lucy", version);
        CFCParcel_register(parcel);
        CFCBase_decref((CFCBase*)version);
        CFCBase_decref((CFCBase*)parcel);
    }
    return parcel;
}

int
CFCParcel_equals(CFCParcel *self, CFCParcel *other) {
    if (strcmp(self->name, other->name)) { return false; }
    if (strcmp(self->cnick, other->cnick)) { return false; }
    if (CFCVersion_compare_to(self->version, other->version) != 0) {
        return false;
    }
    return true;
}

const char*
CFCParcel_get_name(CFCParcel *self) {
    return self->name;
}

const char*
CFCParcel_get_cnick(CFCParcel *self) {
    return self->cnick;
}

CFCVersion*
CFCParcel_get_version(CFCParcel *self) {
    return self->version;
}

const char*
CFCParcel_get_prefix(CFCParcel *self) {
    return self->prefix;
}

const char*
CFCParcel_get_Prefix(CFCParcel *self) {
    return self->Prefix;
}

const char*
CFCParcel_get_PREFIX(CFCParcel *self) {
    return self->PREFIX;
}

/*****************************************************************************
 * The hack JSON parser coded up below is only meant to parse Clownfish parcel
 * file content.  It is limited in its capabilities because so little is legal
 * in a .cfp file.
 */

static JSONNode*
S_parse_json_for_parcel(const char *json) {
    if (!json) {
        return NULL;
    }
    S_skip_whitespace(&json);
    if (*json != '{') {
        return NULL;
    }
    JSONNode *parsed = S_parse_json_hash(&json);
    S_skip_whitespace(&json);
    if (*json != '\0') {
        S_destroy_json(parsed);
        parsed = NULL;
    }
    return parsed;
}

static void
S_append_kid(JSONNode *node, JSONNode *child) {
    size_t size = (node->num_kids + 2) * sizeof(JSONNode*);
    node->kids = (JSONNode**)realloc(node->kids, size);
    node->kids[node->num_kids++] = child;
    node->kids[node->num_kids]   = NULL;
}

static JSONNode*
S_parse_json_hash(const char **json) {
    const char *text = *json;
    S_skip_whitespace(&text);
    if (*text != '{') {
        return NULL;
    }
    text++;
    JSONNode *node = (JSONNode*)calloc(1, sizeof(JSONNode));
    node->type = JSON_HASH;
    while (1) {
        // Parse key.
        S_skip_whitespace(&text);
        if (*text == '}') {
            text++;
            break;
        }
        else if (*text == '"') {
            JSONNode *key = S_parse_json_string(&text);
            S_skip_whitespace(&text);
            if (!key || *text != ':') {
                S_destroy_json(node);
                return NULL;
            }
            text++;
            S_append_kid(node, key);
        }
        else {
            S_destroy_json(node);
            return NULL;
        }

        // Parse value.
        S_skip_whitespace(&text);
        JSONNode *value = NULL;
        if (*text == '"') {
            value = S_parse_json_string(&text);
        }
        else if (*text == '{') {
            value = S_parse_json_hash(&text);
        }
        if (!value) {
            S_destroy_json(node);
            return NULL;
        }
        S_append_kid(node, value);

        // Parse comma.
        S_skip_whitespace(&text);
        if (*text == ',') {
            text++;
        }
        else if (*text == '}') {
            text++;
            break;
        }
        else {
            S_destroy_json(node);
            return NULL;
        }
    }

    // Move pointer.
    *json = text;

    return node;
}

// Parse a double quoted string.  Don't allow escapes.
static JSONNode*
S_parse_json_string(const char **json) {
    const char *text = *json; 
    if (*text != '\"') {
        return NULL;
    }
    text++;
    const char *start = text;
    while (*text != '"') {
        if (*text == '\\' || *text == '\0') {
            return NULL;
        }
        text++;
    }
    JSONNode *node = (JSONNode*)calloc(1, sizeof(JSONNode));
    node->type = JSON_STRING;
    node->string = CFCUtil_strndup(start, text - start);

    // Move pointer.
    text++;
    *json = text;

    return node;
}

static void
S_skip_whitespace(const char **json) {
    while (isspace(json[0][0])) { *json = *json + 1; }
}

static void
S_destroy_json(JSONNode *node) {
    if (!node) {
        return;
    }
    if (node->kids) {
        for (size_t i = 0; node->kids[i] != NULL; i++) {
            S_destroy_json(node->kids[i]);
        }
    }
    free(node->string);
    free(node->kids);
    free(node);
}


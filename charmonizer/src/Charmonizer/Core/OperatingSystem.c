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

#define CHAZ_USE_SHORT_NAMES

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/ConfWriter.h"
#include "Charmonizer/Core/OperatingSystem.h"

static char dev_null[20] = "";
static char exe_ext[5]   = "";
static char obj_ext[5]   = "";
static char local_command_start[3] = "";
static int  shell_type = 0;
#define SHELL_TYPE_POSIX    1
#define SHELL_TYPE_CMD_EXE  2

void
OS_init(void) {
    if (Util_verbosity) {
        printf("Initializing Charmonizer/Core/OperatingSystem...\n");
    }

    if (Util_verbosity) {
        printf("Trying to find a bit-bucket a la /dev/null...\n");
    }

    /* Detect shell based on whether the bitbucket is "/dev/null" or "nul". */
    if (Util_can_open_file("/dev/null")) {
        fprintf(stderr, "OOP!\n");
        strcpy(dev_null, "/dev/null");
        strcpy(exe_ext, "");
        strcpy(obj_ext, "");
        strcpy(local_command_start, "./");
        shell_type = SHELL_TYPE_POSIX;
    }
    else if (Util_can_open_file("nul")) {
        strcpy(dev_null, "nul");
        strcpy(exe_ext, ".exe");
        strcpy(obj_ext, ".obj");
        strcpy(local_command_start, ".\\");
        shell_type = SHELL_TYPE_CMD_EXE;
    }
    else {
        /* Bail out because we couldn't find anything like /dev/null. */
        Util_die("Couldn't find anything like /dev/null");
    }
}

const char*
OS_exe_ext(void) {
    return exe_ext;
}

const char*
OS_obj_ext(void) {
    return obj_ext;
}

const char*
OS_dev_null(void) {
    return dev_null;
}

void
OS_remove_exe(const char *name) {
    char *exe_name = (char*)malloc(strlen(name) + strlen(exe_ext) + 1);
    sprintf(exe_name, "%s%s", name, exe_ext);
    remove(exe_name);
    free(exe_name);
}

void
OS_remove_obj(const char *name) {
    char *obj_name = (char*)malloc(strlen(name) + strlen(obj_ext) + 1);
    sprintf(obj_name, "%s%s", name, obj_ext);
    remove(obj_name);
    free(obj_name);
}

int
OS_run_local(const char *arg1, ...) {
    va_list  args;
    size_t   len     = strlen(local_command_start) + strlen(arg1);
    char    *command = (char*)malloc(len + 1);
    int      retval;
    char    *arg;

    /* Append all supplied texts. */
    sprintf(command, "%s%s", local_command_start, arg1);
    va_start(args, arg1);
    while (NULL != (arg = va_arg(args, char*))) {
        len += strlen(arg);
        command = (char*)realloc(command, len + 1);
        strcat(command, arg);
    }
    va_end(args);

    /* Run the command. */
    retval = system(command);
    free(command);
    return retval;
}

int
OS_run_quietly(const char *command) {
    int retval = 1;
    char *quiet_command = NULL;
    if (shell_type == SHELL_TYPE_POSIX) {
        char pattern[] = "%s > %s 2>&1";
        size_t size
            = sizeof(pattern) + strlen(command) + strlen(dev_null) + 10;
        quiet_command = (char*)malloc(size);
        sprintf(quiet_command, pattern, command, dev_null);
    }
    else if (shell_type == SHELL_TYPE_CMD_EXE) {
        char pattern[] = "%s > NUL 2> NUL";
        size_t size = sizeof(pattern) + strlen(command) + 10;
        quiet_command = (char*)malloc(size);
        sprintf(quiet_command, pattern, command);
    }
    else {
        Util_die("Don't know the shell type");
    }
    retval = system(quiet_command);
    free(quiet_command);

    return retval;
}

void
OS_mkdir(const char *filepath) {
    char *command = NULL;
    if (shell_type == SHELL_TYPE_POSIX || shell_type == SHELL_TYPE_CMD_EXE) {
        unsigned size = sizeof("mkdir") + 1 + strlen(filepath) + 1;
        command = (char*)malloc(size);
        sprintf(command, "mkdir %s", filepath);
    }
    else {
        Util_die("Don't know the shell type");
    }
    OS_run_quietly(command);
    free(command);
}

void
OS_rmdir(const char *filepath) {
    char *command = NULL;
    if (shell_type == SHELL_TYPE_POSIX) {
        unsigned size = strlen("rmdir") + 1 + strlen(filepath) + 1;
        command = (char*)malloc(size);
        sprintf(command, "rmdir %s", filepath);
    }
    else if (shell_type == SHELL_TYPE_CMD_EXE) {
        unsigned size = strlen("rmdir /q") + 1 + strlen(filepath) + 1;
        command = (char*)malloc(size);
        sprintf(command, "rmdir /q %s", filepath);
    }
    else {
        Util_die("Don't know the shell type");
    }
    OS_run_quietly(command);
    free(command);
}


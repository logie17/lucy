#define CHAZ_USE_SHORT_NAMES

#include <string.h>
#include <stdlib.h>
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/CompilerSpec.h"
#include "Charmonizer/Core/OperSys.h"

extern chaz_bool_t chaz_ModHand_charm_run_available;

static void
S_destroy(Compiler *self);

static chaz_bool_t
S_compile_exe(Compiler *self, const char *source_path, const char *exe_path, 
              const char *code, size_t code_len);

static chaz_bool_t
S_compile_obj(Compiler *self, const char *source_path, const char *obj_path, 
              const char *code, size_t code_len);

static void
S_add_inc_dir(Compiler *self, const char *dir);

static void
S_do_test_compile(Compiler *self);

chaz_Compiler*
chaz_CC_new(OperSys *oper_sys, const char *cc_command, const char *cc_flags)
{
    CompilerSpec *compiler_spec = CCSpec_find_spec();
    Compiler *self = (Compiler*)malloc(sizeof(Compiler));

    if (verbosity)
        printf("Creating compiler object...\n");

    /* assign */
    self->os              = oper_sys;
    self->cc_command      = strdup(cc_command);
    self->cc_flags        = strdup(cc_flags);

    /* init */
    self->buf             = NULL;
    self->buf_len         = 0;
    self->compile_exe     = S_compile_exe;
    self->compile_obj     = S_compile_obj;
    self->add_inc_dir     = S_add_inc_dir;
    self->destroy         = S_destroy;
    self->inc_dirs        = (char**)calloc(sizeof(char*), 1);

    /* set compiler-specific vars */
    self->include_flag    = strdup(compiler_spec->include_flag);
    self->object_flag     = strdup(compiler_spec->object_flag);
    self->exe_flag        = strdup(compiler_spec->exe_flag);

    /* add the current directory as an include dir */
    self->add_inc_dir(self, ".");

    /* if we can't compile anything, game over */
    S_do_test_compile(self);

    return self;
}

static void
S_destroy(Compiler *self)
{
    char **inc_dirs;

    for (inc_dirs = self->inc_dirs; *inc_dirs != NULL; inc_dirs++) {
        free(*inc_dirs);
    }
    free(self->inc_dirs);

    free(self->buf);
    free(self->cc_command);
    free(self->cc_flags);
    free(self->include_flag);
    free(self->object_flag);
    free(self->exe_flag);
    free(self);
}

static chaz_bool_t
S_compile_exe(Compiler *self, const char *source_path, const char *exe_path, 
              const char *code, size_t code_len)
{
    chaz_bool_t successful;
    OperSys *os = self->os;
    char *exe_full_filepath = NULL;
    char **inc_dirs;

    /* tack the exe_ext onto the path */
    join_strings(&exe_full_filepath, 0, exe_path, os->exe_ext, NULL);

    /* write the source file */
    write_file(source_path, code, code_len);

    /* prepare the compiler command */
    if (verbosity < 2 && chaz_ModHand_charm_run_available) {
        self->buf_len = join_strings(&(self->buf), self->buf_len, 
            os->local_command_start, "_charm_run ", self->cc_command, " ",
            source_path, " ", self->exe_flag, exe_full_filepath, " ", NULL);
    }
    else {
        self->buf_len = join_strings(&(self->buf), self->buf_len, 
            self->cc_command, " ", source_path, " ", self->exe_flag,
            exe_full_filepath, " ", NULL);
    }
    for (inc_dirs = self->inc_dirs; *inc_dirs != NULL; inc_dirs++) {
        self->buf_len = append_strings(&(self->buf), self->buf_len, 
            self->include_flag, *inc_dirs, " ", NULL);
    }
    self->buf_len = append_strings(&(self->buf), self->buf_len, 
        self->cc_flags, " ", NULL);

    /* execute the compiler command and detect success/failure */
    system(self->buf);
    successful = can_open_file(exe_full_filepath);
    free(exe_full_filepath);
    
    return successful;
}

static chaz_bool_t
S_compile_obj(Compiler *self, const char *source_path, const char *obj_path, 
              const char *code, size_t code_len)
{
    chaz_bool_t successful;
    OperSys *os = self->os;
    char *obj_full_filepath = NULL;

    /* tack the obj_ext onto the path */
    join_strings(&obj_full_filepath, 0, obj_path, os->obj_ext, NULL);
    
    /* write the source file */
    write_file(source_path, code, code_len);

    /* compile the source */
    if (verbosity < 2 && chaz_ModHand_charm_run_available) {
        join_strings(&(self->buf), self->buf_len, 
            os->local_command_start, "_charm_run ", self->cc_command, " ",
            source_path, " ", self->object_flag, obj_full_filepath, " ",
            self->include_flag, ". ", self->cc_flags, NULL);
    }
    else {
        join_strings(&(self->buf), self->buf_len, 
            self->cc_command, " ", source_path, " ", self->object_flag,
            obj_full_filepath, " ", self->include_flag, ". ", self->cc_flags,
            NULL);
    }
    system(self->buf);

    /* see if compilation was successful */
    successful = can_open_file(obj_full_filepath);
    free(obj_full_filepath);
    
    return successful;
}

static void
S_do_test_compile(Compiler *self)
{
    char *code = "int main() { return 0; }\n";
    chaz_bool_t success;
    
    if (verbosity) 
        printf("Trying to compile a small test file...\n");

    /* attempt compilation */
    success = self->compile_exe(self, "_charm_try.c", 
        "_charm_try", code, strlen(code));
    if (!success)
        die("Failed to compile a small test file");
    
    /* clean up */
    remove("_charm_try.c");
    self->os->remove_exe(self->os, "_charm_try");
}

static void
S_add_inc_dir(Compiler *self, const char *dir)
{
    size_t num_dirs = 0; 
    char **dirs = self->inc_dirs;

    /* count up the present number of dirs, reallocate */
    while (*dirs++ != NULL) { num_dirs++; }
    num_dirs += 1; /* passed-in dir */
    self->inc_dirs = realloc(self->inc_dirs, (num_dirs + 1)*sizeof(char*));

    /* put the passed-in dir at the end of the list */
    self->inc_dirs[num_dirs - 1] = strdup(dir);
    self->inc_dirs[num_dirs] = NULL;
}

/**
 * Copyright 2006 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

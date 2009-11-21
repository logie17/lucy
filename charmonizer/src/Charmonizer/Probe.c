#define CHAZ_USE_SHORT_NAMES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Charmonizer/Probe.h"
#include "Charmonizer/Core/HeadCheck.h"
#include "Charmonizer/Core/ModHandler.h"
#include "Charmonizer/Core/Util.h"
#include "Charmonizer/Core/Compiler.h"
#include "Charmonizer/Core/OperSys.h"

void
chaz_Probe_init(const char *osname, const char *cc_command,
                const char *cc_flags, const char *charmony_start)
{
    /* create os and compiler objects */
    os       = OS_new(osname);
    compiler = CC_new(os, cc_command, cc_flags);

    /* dispatch other tasks */
    ModHand_init();
    HeadCheck_init();
    ModHand_open_charmony_h(charmony_start);

    if (verbosity)
        printf("Initialization complete.\n");
}

void
chaz_Probe_clean_up()
{
    if (verbosity)
        printf("Cleaning up...\n");

    /* dispatch ModHandler's clean up routines, destroy objects */
    ModHand_clean_up();
    os->destroy(os);
    compiler->destroy(compiler);

    if (verbosity)
        printf("Cleanup complete.\n");
}

void
chaz_Probe_set_verbosity(int level)
{
    verbosity = level;
}

char*
chaz_Probe_slurp_file(char* filepath, size_t *len_ptr) {
    return slurp_file(filepath, len_ptr);
}

FILE*
chaz_Probe_get_charmony_fh(void)
{
    return charmony_fh;
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

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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Charmonizer/Test.h";
#include "Charmonizer/Test.c";

static void
S_run_tests(void) {
    LONG_EQ(0, 0, "sanity check");
}

int main(int argc, char **argv) {
    Test_start(1);
    S_run_tests();
    return !Test_finish();
}


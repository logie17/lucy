#define C_LUCY_RAMFOLDER
#define C_LUCY_CHARBUF
#include "Lucy/Util/ToolSet.h"

#define CHAZ_USE_SHORT_NAMES
#include "Charmonizer/Test.h"

#include "Lucy/Test/Store/TestRAMDirHandle.h"
#include "Lucy/Store/FileHandle.h"
#include "Lucy/Store/RAMFolder.h"
#include "Lucy/Store/RAMDirHandle.h"

static CharBuf foo           = ZCB_LITERAL("foo");
static CharBuf boffo         = ZCB_LITERAL("boffo");
static CharBuf foo_boffo     = ZCB_LITERAL("foo/boffo");

static void
test_all(TestBatch *batch)
{
    RAMFolder    *folder = RAMFolder_new(NULL);
    FileHandle   *fh;
    RAMDirHandle *dh;
    CharBuf      *entry;
    bool_t        saw_foo       = false;
    bool_t        saw_boffo     = false;
    bool_t        foo_was_dir   = false;
    bool_t        boffo_was_dir = false; 
    int           count         = 0;

    RAMFolder_MkDir(folder, &foo);
    fh = RAMFolder_Open_FileHandle(folder, &boffo, FH_CREATE | FH_WRITE_ONLY);
    DECREF(fh);
    fh = RAMFolder_Open_FileHandle(folder, &foo_boffo, 
        FH_CREATE | FH_WRITE_ONLY );
    DECREF(fh);

    dh = RAMDH_new(folder);
    entry = RAMDH_Get_Entry(dh);
    while (RAMDH_Next(dh)) {
        count++;
        if (CB_Equals(entry, (Obj*)&foo)) { 
            saw_foo = true;
            foo_was_dir = RAMDH_Entry_Is_Dir(dh);
        }
        else if (CB_Equals(entry, (Obj*)&boffo)) {
            saw_boffo = true;
            boffo_was_dir = RAMDH_Entry_Is_Dir(dh);
        }
    }
    ASSERT_INT_EQ(batch, 2, count, "correct number of entries");
    ASSERT_TRUE(batch, saw_foo, "Directory was iterated over");
    ASSERT_TRUE(batch, foo_was_dir, 
        "Dir correctly identified by Entry_Is_Dir");
    ASSERT_TRUE(batch, saw_boffo, "File was iterated over");
    ASSERT_FALSE(batch, boffo_was_dir, 
        "File correctly identified by Entry_Is_Dir");

    {
        u32_t refcount = RAMFolder_Get_RefCount(folder);
        RAMDH_Close(dh);
        ASSERT_INT_EQ(batch, RAMFolder_Get_RefCount(folder), refcount - 1,
            "Folder reference released by Close()");
    }

    DECREF(dh);
    DECREF(folder);
}

void
TestRAMDH_run_tests()
{
    TestBatch *batch = Test_new_batch("TestRAMDirHandle", 6, NULL);

    PLAN(batch);
    test_all(batch);

    batch->destroy(batch);
}

/* Copyright 2009 The Apache Software Foundation
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

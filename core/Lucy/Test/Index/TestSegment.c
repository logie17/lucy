#define C_LUCY_TESTSEG
#include "Lucy/Util/ToolSet.h"

#include "Lucy/Test.h"
#include "Lucy/Test/Index/TestSegment.h"
#include "Lucy/Index/Segment.h"
#include "Lucy/Store/RAMFolder.h"

static void
test_fields(TestBatch *batch)
{
    Segment *segment = Seg_new(1);
    ZombieCharBuf foo = ZCB_LITERAL("foo");
    ZombieCharBuf bar = ZCB_LITERAL("bar");
    ZombieCharBuf baz = ZCB_LITERAL("baz");
    i32_t field_num; 
    
    field_num = Seg_Add_Field(segment, (CharBuf*)&foo);
    ASSERT_TRUE(batch, field_num == 1, 
        "Add_Field returns field number, and field numbers start at 1");
    field_num = Seg_Add_Field(segment, (CharBuf*)&bar);
    ASSERT_TRUE(batch, field_num == 2, "add a second field");
    field_num = Seg_Add_Field(segment, (CharBuf*)&foo);
    ASSERT_TRUE(batch, field_num == 1,
        "Add_Field returns existing field number if field is already known");

    ASSERT_TRUE(batch, ZCB_Equals(&bar, (Obj*)Seg_Field_Name(segment, 2)),
        "Field_Name");
    ASSERT_TRUE(batch, Seg_Field_Name(segment, 3) == NULL, 
        "Field_Name returns NULL for unknown field number");
    ASSERT_TRUE(batch, Seg_Field_Num(segment, (CharBuf*)&bar) == 2,
        "Field_Num");
    ASSERT_TRUE(batch, Seg_Field_Num(segment, (CharBuf*)&baz) == 0, 
        "Field_Num returns 0 for unknown field name");

    DECREF(segment);
}

static void
test_metadata_storage(TestBatch *batch)
{
    Segment *segment = Seg_new(1);
    Obj *got;

    Seg_Store_Metadata_Str(segment, "foo", 3, (Obj*)CB_newf("bar"));
    got = Seg_Fetch_Metadata_Str(segment, "foo", 3);
    ASSERT_TRUE(batch, 
                   got 
                && Obj_Is_A(got, CHARBUF) 
                && CB_Equals_Str(got, "bar", 3), 
                "metadata round trip");
    DECREF(segment);
}

static void
test_seg_name_and_num(TestBatch *batch)
{
    Segment *segment_z = Seg_new(35);
    CharBuf *seg_z_name = Seg_num_to_name(35);
    ASSERT_TRUE(batch, Seg_Get_Number(segment_z) == I64_C(35), "Get_Number");
    ASSERT_TRUE(batch, CB_Equals_Str(Seg_Get_Name(segment_z), "seg_z", 5), 
        "Get_Name");
    ASSERT_TRUE(batch, CB_Equals_Str(seg_z_name, "seg_z", 5), 
        "num_to_name");
    DECREF(seg_z_name);
    DECREF(segment_z);
}

static void
test_count(TestBatch *batch)
{
    Segment *segment = Seg_new(100);
    
    ASSERT_TRUE(batch, Seg_Get_Count(segment) == 0, "count starts off at 0");
    Seg_Set_Count(segment, 120);
    ASSERT_TRUE(batch, Seg_Get_Count(segment) == 120, "Set_Count");
    ASSERT_TRUE(batch, Seg_Increment_Count(segment, 10) == 130,
        "Increment_Count");

    DECREF(segment);
}

static void
test_Compare_To(TestBatch *batch)
{
    Segment *segment_1      = Seg_new(1);
    Segment *segment_2      = Seg_new(2);
    Segment *also_segment_2 = Seg_new(2);

    ASSERT_TRUE(batch, Seg_Compare_To(segment_1, (Obj*)segment_2) < 0, 
        "Compare_To 1 < 2");
    ASSERT_TRUE(batch, Seg_Compare_To(segment_2, (Obj*)segment_1) > 0, 
        "Compare_To 1 < 2");
    ASSERT_TRUE(batch, Seg_Compare_To(segment_1, (Obj*)segment_1) == 0, 
        "Compare_To identity");
    ASSERT_TRUE(batch, Seg_Compare_To(segment_2, (Obj*)also_segment_2) == 0, 
        "Compare_To 2 == 2");

    DECREF(segment_1);
    DECREF(segment_2);
    DECREF(also_segment_2);
}

static void
test_Write_File_and_Read_File(TestBatch *batch)
{
    RAMFolder *folder  = RAMFolder_new(NULL);
    Segment   *segment = Seg_new(100);
    Segment   *got     = Seg_new(100);
    Obj       *meta;
    ZombieCharBuf flotsam = ZCB_LITERAL("flotsam");
    ZombieCharBuf jetsam  = ZCB_LITERAL("jetsam");

    Seg_Set_Count(segment, 111);
    Seg_Store_Metadata_Str(segment, "foo", 3, (Obj*)CB_newf("bar"));
    Seg_Add_Field(segment, (CharBuf*)&flotsam);
    Seg_Add_Field(segment, (CharBuf*)&jetsam);
    
    RAMFolder_MkDir(folder, Seg_Get_Name(segment));
    Seg_Write_File(segment, (Folder*)folder);
    Seg_Read_File(got, (Folder*)folder);

    ASSERT_TRUE(batch, Seg_Get_Count(got) == Seg_Get_Count(segment), 
        "Round-trip count through file");
    ASSERT_TRUE(batch, 
           Seg_Field_Num(got, (CharBuf*)&jetsam) 
        == Seg_Field_Num(segment, (CharBuf*)&jetsam), 
        "Round trip field names through file");
    meta = Seg_Fetch_Metadata_Str(got, "foo", 3);
    ASSERT_TRUE(batch, meta && Obj_Is_A(meta, CHARBUF) 
        && CB_Equals_Str(meta, "bar", 3), "Round trip metadata through file");

    DECREF(got);
    DECREF(segment);
    DECREF(folder);
}

void
TestSeg_run_tests()
{
    TestBatch *batch = Test_new_batch("TestSegment", 21, NULL);

    PLAN(batch);
    test_fields(batch);
    test_metadata_storage(batch);
    test_seg_name_and_num(batch);
    test_count(batch);
    test_Compare_To(batch);
    test_Write_File_and_Read_File(batch);

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

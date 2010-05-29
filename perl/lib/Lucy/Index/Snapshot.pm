package Lucy::Index::Snapshot;
use Lucy;

1;

__END__

__BINDING__

my $synopsis = <<'END_SYNOPSIS';
    my $snapshot = Lucy::Index::Snapshot->new;
    $snapshot->read_file( folder => $folder );    # load most recent snapshot
    my $files = $snapshot->list;
    print "$_\n" for @$files;
END_SYNOPSIS

my $constructor = <<'END_CONSTRUCTOR';
    my $snapshot = Lucy::Index::Snapshot->new;
END_CONSTRUCTOR

Clownfish::Binding::Perl::Class->register(
    parcel       => "Lucy",
    class_name   => "Lucy::Index::Snapshot",
    bind_methods => [
        qw(
            List
            Num_Entries
            Add_Entry
            Delete_Entry
            Read_File
            Write_File
            Set_Path
            Get_Path
            )
    ],
    bind_constructors => ["new"],
    make_pod          => {
        synopsis    => $synopsis,
        constructor => { sample => $constructor },
        methods     => [
            qw(
                list
                num_entries
                add_entry
                delete_entry
                read_file
                write_file
                set_path
                get_path
                )
        ],
    },
);

__COPYRIGHT__

    /**
     * Copyright 2010 The Apache Software Foundation
     *
     * Licensed under the Apache License, Version 2.0 (the "License");
     * you may not use this file except in compliance with the License.
     * You may obtain a copy of the License at
     *
     *     http://www.apache.org/licenses/LICENSE-2.0
     *
     * Unless required by applicable law or agreed to in writing, software
     * distributed under the License is distributed on an "AS IS" BASIS,
     * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
     * implied.  See the License for the specific language governing
     * permissions and limitations under the License.
     */

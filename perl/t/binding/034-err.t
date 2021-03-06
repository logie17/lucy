# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use strict;
use warnings;
use Lucy::Test;

package Nirvana;

sub enter {
    die Lucy::Object::Err->new("blam");
}

package GloriousDeath;
use base qw( Lucy::Object::Err );

package main;
use Test::More tests => 10;

isa_ok( Lucy::Object::Err->new("Bad stuff happened"),
    'Lucy::Object::Err', "new" );

my $glorious = GloriousDeath->new("Banzai");
isa_ok( $glorious, 'GloriousDeath',     "subclass" );
isa_ok( $glorious, 'Lucy::Object::Err', "subclass" );

isa_ok( Lucy::Object::Err::trap( "bite_the_dust", undef ),
    'Lucy::Object::Err', "trap string call" );

isa_ok( Lucy::Object::Err::trap( "Nirvana::enter", undef ),
    'Lucy::Object::Err', "trap string call in another package" );

isa_ok( Lucy::Object::Err::trap( \&bite_the_dust, undef ),
    'Lucy::Object::Err', "trap sub ref" );

isa_ok( Lucy::Object::Err::trap( \&Nirvana::enter, undef ),
    'Lucy::Object::Err', "trap sub ref to another package" );

isa_ok( Lucy::Object::Err::trap( \&judge_gladiator, "down" ),
    'Lucy::Object::Err', "pass argument to 'trap'" );

my $last_words = sub { die "Rosebud" };
isa_ok( Lucy::Object::Err::trap( $last_words, undef ),
    'Lucy::Object::Err', "Wrap host exception in Err" );

my $succeed = sub { };
ok( !defined( Lucy::Object::Err::trap( $succeed, undef ) ),
    "nothing to trap" );

sub bite_the_dust { die Lucy::Object::Err->new("gasp") }

sub judge_gladiator {
    my $thumb = shift;
    if ( $thumb and $thumb eq 'down' ) {
        die GloriousDeath->new("gurgle");
    }
}


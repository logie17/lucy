package Lucy::Build;
use strict;
use warnings;
use base qw( Module::Build );

use File::Spec::Functions qw( catdir catfile curdir splitpath updir );
use File::Path qw( mkpath );
use File::Find qw( find );
use ExtUtils::CBuilder;
use Config;
use Env qw( @PATH );

unshift @PATH, curdir();

my $base_dir = -e 'charmonizer' ? curdir() : updir();

my $METAQUOTE_EXE_PATH     = 'metaquote';
my $CHARMONIZE_EXE_PATH    = 'charmonize';
my $CHARMONIZER_SOURCE_DIR = catdir( $base_dir, 'charmonizer', 'src' );

my $EXTRA_CCFLAGS
    = $ENV{DEBUG_CHARM} ? " -ansi -pedantic -Wall -Wextra -std=c89 " : "";
my $VALGRIND = $ENV{CHARM_VALGRIND} ? "valgrind " : "";

# Compile the metaquote source filter utility.
sub ACTION_metaquote {
    my $self = shift;
    my $source_path
        = catfile( $base_dir, qw( charmonizer metaquote_src metaquote.c ) );

    # don't compile if we're up to date
    return if $self->up_to_date( [$source_path], $METAQUOTE_EXE_PATH );

    # compile
    print "\nBuilding $METAQUOTE_EXE_PATH...\n\n";
    my $cbuilder = ExtUtils::CBuilder->new;
    my $o_file   = $cbuilder->compile(
        source               => $source_path,
        extra_compiler_flags => $EXTRA_CCFLAGS,
    );
    $cbuilder->link_executable(
        objects  => [$o_file],
        exe_file => $METAQUOTE_EXE_PATH,
    );

    # clean both the object file and the executable
    $self->add_to_cleanup( $o_file, $METAQUOTE_EXE_PATH );
}

# Build the charmonize executable.
sub ACTION_charmonizer {
    my $self         = shift;
    my $filtered_dir = catdir( $base_dir, qw( charmonizer filtered_src ) );
    if ( !-d $filtered_dir ) {
        mkpath($filtered_dir) or die "can't mkpath '$filtered_dir': $!";
    }

    $self->dispatch('metaquote');

    my $charm_source_files
        = $self->_find_charm_source_files($CHARMONIZER_SOURCE_DIR);
    my $filtered_files = $self->_metaquote_charm_files($charm_source_files);
    my $charmonize_c   = catfile($base_dir, qw( charmonizer charmonize.c ) );
    my @all_source     = ( $charmonize_c, @$filtered_files );

    # don't compile if we're up to date
    return if $self->up_to_date( \@all_source, $CHARMONIZE_EXE_PATH );

    print "Building $CHARMONIZE_EXE_PATH...\n\n";

    my $cbuilder = ExtUtils::CBuilder->new;

    my @o_files;
    for (@all_source) {
        next unless /\.c$/;
        my $o_file = $cbuilder->object_file($_);
        push @o_files, $o_file;

        next if $self->up_to_date( $_, $o_file );

        $cbuilder->compile(
            source               => $_,
            include_dirs         => [$filtered_dir],
            extra_compiler_flags => $EXTRA_CCFLAGS,
        );
    }

    my $exe_path = $cbuilder->link_executable(
        objects  => \@o_files,
        exe_file => $CHARMONIZE_EXE_PATH,
    );

    $self->add_to_cleanup( $filtered_dir, @$filtered_files, @o_files,
        $CHARMONIZE_EXE_PATH, );
}

sub _find_charm_source_files {
    my ( $self, $dir ) = @_;
    my @files;
    find(
        {   wanted => sub {
                if ( $File::Find::name =~ /\.c?harm$/ ) {
                    push @files, $File::Find::name;
                }
            },
            no_chdir => 1,
        },
        $dir,
    );
    return \@files;
}

sub _metaquote_charm_files {
    my ( $self, $charm_files ) = @_;
    my @filtered_files;

    for my $source_path (@$charm_files) {
        my $dest_path = $source_path;
        $dest_path =~ s#(.*)src#$1filtered_src#;
        $dest_path =~ s#\.charm#.c#;
        $dest_path =~ s#\.harm#.h#;

        push @filtered_files, $dest_path;

        next if ( $self->up_to_date( $source_path, $dest_path ) );

        # create directories if need be
        my ( undef, $dir, undef ) = splitpath($dest_path);
        if ( !-d $dir ) {
            $self->add_to_cleanup($dir);
            mkpath $dir or die "Couldn't mkpath $dir";
        }

        # run the metaquote filter
        system( $METAQUOTE_EXE_PATH, $source_path, $dest_path );
    }

    return \@filtered_files;
}

# Run the charmonizer executable, creating the lucyconf.h file.
sub ACTION_lucyconf {
    my $self          = shift;
    my $lucyconf_in   = 'lucyconf_in';
    my $lucyconf_path = "lucyconf.h";

    $self->dispatch('charmonizer');

    return if $self->up_to_date( $CHARMONIZE_EXE_PATH, $lucyconf_path );
    print "\nWriting $lucyconf_path...\n\n";

    # write the infile with which to communicate args to charmonize
    my $os_name = lc($Config{osname});
    open( my $infile_fh, '>', $lucyconf_in )
        or die "Can't open '$lucyconf_in': $!";
    print $infile_fh qq|
        <charm_outpath>$lucyconf_path</charm_outpath>
        <charm_os_name>$os_name</charm_os_name>
        <charm_cc_command>$Config{cc}</charm_cc_command>
        <charm_cc_flags>$Config{ccflags}</charm_cc_flags>
    |;
    close $infile_fh or die "Can't close '$lucyconf_in': $!";

    if ($VALGRIND) {
        system("$VALGRIND ./$CHARMONIZE_EXE_PATH $lucyconf_in");
    }
    else {
        system( $CHARMONIZE_EXE_PATH, $lucyconf_in );
    }

    unlink($lucyconf_in) or die "Can't unlink '$lucyconf_in': $!";

    $self->add_to_cleanup($lucyconf_path);
}

sub ACTION_code {
    my $self = shift;
    $self->dispatch('lucyconf');
    $self->SUPER::ACTION_code(@_);
}

1;

__END__


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
     * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
     * implied.  See the License for the specific language governing
     * permissions and limitations under the License.
     */

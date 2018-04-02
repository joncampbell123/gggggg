#!/usr/bin/perl
open(X,"cat /x | sed -e 's/[ \t]//g' | cut -d = -f 1 |") || die;
while (my $line = <X>) {
    chomp $line;
    next unless $line ne "";

    $new = $line;
    $new =~ s/^SDL_SCANCODE/Game_KC/;

    print substr("# define $new                                            ",0,32)."$line\n";
}
close(X);


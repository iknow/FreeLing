#! /usr/bin/perl

##################################################################
##
##    FreeLing - Open Source Language Analyzers
##
##    Copyright (C) 2004   TALP Research Center
##                         Universitat Politecnica de Catalunya
##
##    This library is free software; you can redistribute it and/or
##    modify it under the terms of the GNU General Public
##    License as published by the Free Software Foundation; either
##    version 2.1 of the License, or (at your option) any later version.
##
##    This library is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##    General Public License for more details.
##
##    You should have received a copy of the GNU General Public
##    License along with this library; if not, write to the Free Software
##    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
##    contact: Lluis Padro (padro@lsi.upc.es)
##             TALP Research Center
##             despatx C6.212 - Campus Nord UPC
##             08034 Barcelona.  SPAIN
##
################################################################

################################################################
# train-relax.perl
#
# Script used to read a disambiguated corpus and create a basic bigram 
# constraint grammar for relax tagger.
#
# The corpus is expected to be in FreeLing "tagged" inpf/outf format, that is:
# one word per line, each line with format:
#     form lemma tag
#
# Sentences are separated with a blank line.
#
# The output is sent to stodut, which may be straightforwadly used as a 
# constraint grammar for the relax tagger (command line option -R, or config file
# option "TaggerRelaxFile")
#
# a parameter is expected in command line, stating whether the used 
# tagset is parole (1) or not (0)
#
################################################################

use strict;

my ($f1,$l1,$t1,$f2,$l2,$t2);
my (@resta,%unig,%bigr,$nu,$nb,$nt,$b,$t,$im);

my $PAROLE=$ARGV[0];

 $t1="OUT_OF_BOUNDS";
 while (<STDIN>) {
     
     if ($_ =~ /^$/) { $t2="OUT_OF_BOUNDS"; }
     else { ($f2,$l2,$t2,@resta)=split("[ \n]",$_); }
     
     $unig{TallarParole($t1)}++;
     $bigr{TallarParole($t1)."#".TallarParole($t2)}++;

     ($f1,$l1,$t1)=($f2,$l2,$t2);
     
     $nb++;
 }

 $unig{TallarParole($t2)}++;
 $nu=$nb+1;

 for $b (keys %bigr) {
    ($t1,$t2) = split("#",$b);

    $im=log($bigr{$b}*$nb/($unig{$t1}*$unig{$t2}))/log(2);

    if ($PAROLE) {
	if (!($t1 =~ /^[FIWZ]/ || $t1 eq "OUT_OF_BOUNDS" )) {$t1 .= "*";}
	if (!($t2 =~ /^[FIWZ]/ || $t2 eq "OUT_OF_BOUNDS" )) {$t2 .= "*";}
    }

    ## restriccio A|B
    if ($t1 ne "OUT_OF_BOUNDS") {print "$im\t$t1\t(1 $t2);\n";}
    ## restriccio B|A
    if ($t2 ne "OUT_OF_BOUNDS") {print "$im\t$t2\t(-1 $t1);\n";}
}

#-----------------------------------------------------------
sub TallarParole {
    my ($parole) = @_;
    my ($tall);

    if (! $PAROLE) {return $parole;}  # not using parole, return tag untouched

    if ($parole =~ /^F/ || $parole eq "OUT_OF_BOUNDS") {$tall = substr ($parole,0);}
    elsif ($parole =~ /^V/) {$tall = substr ($parole,0,3);}
    else {$tall = substr ($parole,0,2);}

    return $tall;
}




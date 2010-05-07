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

###############
#
#  Generate probabilities.dat file used by probabilities module in FreeLing
#
#  The aim is to estimate the probability of each possible PoS tag for a word
#
#  Input (stdin):
#
#   Annotated corpus, one word per line, line format:
#
#    form (lemma pos)+
#
#   The first pair (lemma pos) is the right disambiguation for the word in that context.
#   The following pairs are the discarded possibilities
#
#   The output (stdout) is the file that can be given to FreeLing for the ProbabilitiesFile option
#
#   If your corpus has unconsistent taggings, you may get some warnings
#
#   Make sure a file named 'unk-tags' exists, with all tags for open categories (i.e. all tags an 
#   unknown word might take)

use strict;
use File::Spec::Functions qw(rel2abs);
use File::Basename;

my $path = dirname(rel2abs($0));

my %freqclasses1=();
my %probclasses=();
my %problex=();

my $MAX_SUF_LEN=8;
my $PAROLE=$ARGV[0];

my ($forma,@tags,$tagOK,%clas,$tag,$classe,%classeforma,$x,$y,@t,%seen,@uniq,$nt,%unk,%occ,%occT,$nocc);

if ($PAROLE) {open TAGS,$path."/unk-tags.parole";}
else {open TAGS,$path."/unk-tags";}

while (<TAGS>) {
    chomp;
    $unk{$_}=1;
}
close TAGS;


while (($forma,@tags)=split("[ \n]",<STDIN>)) {
 
  ## calcular classe d'ambiguitat
  $forma = tolower($forma);
  %clas=();
  shift @tags;  # saltar lema
  $tag = shift @tags;  # obtenir tag
  $clas{TallarParole($tag)}=1;
  
  $tagOK=TallarParole($tag);
  while (@tags) {
     shift @tags;  # saltar lema
     my $t = shift @tags;  # obtenir tag
     $clas{TallarParole($t)}=1;
  }
  $classe = join "-", (sort keys %clas);

  if ($classe) {
     # acumular frequencies
     $freqclasses1{$tagOK}++;
     $probclasses{$classe}{$tagOK}++;
     $problex{$forma}{$tagOK}++;
     if ($classeforma{$forma} && ($classeforma{$forma} ne $classe)) {
        print STDERR "ERROR - '$forma' apppears as $classeforma{$forma} and as $classe.\n";
     }
     else {
       $classeforma{$forma}=$classe; 
     }
  }

  # comptar si el tag es obert
  if ($unk{$tag}) {
    $unk{$tag}++;
    $nocc++; 
  }

  ## occurrencies de sufixos
  my $l=length($forma);
  my $mx=($l<$MAX_SUF_LEN ? $l : $MAX_SUF_LEN);
  for (my $i=1; $i<=$mx; $i++) {
      my $suf= substr($forma,$l-$i);
      $occ{$suf}++;
      $occT{$suf."#".$tag}++;
  }
  
}


print "<UnknownTags>\n";
my $sp=0; 
my $nt=0;
foreach my $tag (sort keys %unk) {
    print "$tag $unk{$tag}\n";
    $unk{$tag}=$unk{$tag}/$nocc;
    $sp += $unk{$tag};
    $nt++;
}
my $mp=$sp/$nt;
print "</UnknownTags>\n";

my $sp=0;
foreach my $tag (keys %unk) {
    $sp +=  ($unk{$tag}-$mp)*($unk{$tag}-$mp)
}
print "<Theeta>\n".$sp/($nt-1)."\n</Theeta>\n";

print "<Suffixes>\n";
foreach my $suf (keys %occ) {
    my $lin="$suf $occ{$suf}";
    my $b=0;
    foreach my $tag (keys %unk) {
	if ($occT{$suf."#".$tag}) {
	   $b=1;
	   $lin .= " $tag ".$occT{$suf."#".$tag};
	}
    }
    if ($b) {print "$lin\n";}
}
print "</Suffixes>\n";

 
print "<SingleTagFreq>\n";
for $x (sort keys %freqclasses1) {
   print "$x $freqclasses1{$x}\n";
}
print "</SingleTagFreq>\n";

print "<ClassTagFreq>\n";

for $x (sort keys %probclasses) {
   @t = split ("-",$x);
   if (@t>1) {

      # llista de tags de la classe o apareguts
      push @t, keys %{$probclasses{$x}};
      %seen = ();
      @uniq = grep { ! $seen{$_} ++ } @t;
      @t= sort @uniq;

      ## classe sense NP
      my $x2=join("-", grep { ($_ ne "NP") and ($_ ne "NNP") } @t);

      if ($x eq $x2) {
	  print $x;
	  for $y (@t) {
	      if (! $probclasses{$x}{$y}) {
		  $probclasses{$x}{$y} = 0;
	      }
	      print " $y $probclasses{$x}{$y}";
	  }
	  print "\n";
      }
      else { # hi ha NP

	  print $x;
          # tag with max occurrences in normal class (no NP)
	  my $mx=0;  
	  for $y (@t) { 
	      if ($probclasses{$x2}{$y}>$mx) { $mx=$probclasses{$x2}{$y}; } 
	  }
          
	  # all tags, including NP
	  my $prob;
	  for $y (@t) {
	      if ($y eq "NP" or $y eq "NNP") { 
		  $prob= int(0.7 * $mx);
	      }
	      elsif (! $probclasses{$x2}{$y}) {
		  $prob = 0;
	      }
	      else {
		  $prob = $probclasses{$x2}{$y};
	      }
	      
	      print " $y $prob";
	  }
	  print "\n";	  
      }
   }
}
print "</ClassTagFreq>\n";

print "<FormTagFreq>\n";
for $x (sort keys %problex) {

   @t = split ("-",$classeforma{$x});
   if (@t>1) {

      # llista de tags de la classe o apareguts
      push @t, keys %{$problex{$x}};
      %seen = ();
      @uniq = grep { ! $seen{$_} ++ } @t;
      @t= sort @uniq;

      print "$x $classeforma{$x}";
      for $y (@t) {
        if (! $problex{$x}{$y}) {
          $problex{$x}{$y} = 0;
        }
        print " $y $problex{$x}{$y}";
      }
      print "\n";
   }
}
print "</FormTagFreq>\n";





# -------------------------------------------
sub TallarParole {
    my ($parole) = @_;
    my ($tall);
  
    if (! $PAROLE) {return $parole;}  # not using parole, return tag untouched

    if ($parole =~ /^F/) {$tall = substr ($parole,0);}
    elsif ($parole =~ /^V/) {$tall = substr ($parole,0,3);}
    else {$tall = substr ($parole,0,2);}
 
    return $tall;
}
 
 
#-----------------------------------------------------------------
sub tolower {
   my $s = shift;
   
   $s =~ tr/A-Z—«¡…Õ”⁄œ‹¿»Ï“˘/a-zÒÁ·ÈÌÛ˙Ô¸‡ËÏÚ˘/;
   return $s;
}


#! /usr/bin/perl

use lib '.';
use libmorfo_perl;
use strict;

## Modify this line to be your FreeLing installation directory
my $FREELINGDIR = "/home/padro/test";

my $DATA = $FREELINGDIR."/share/FreeLing/";

# create options set for maco analyzer. Default values are Ok, except for data files.
my $op=new libmorfo_perl::maco_options("es");
$op->set_active_modules(1,1,1,1,1,1,1,1,1);
$op->set_data_files($DATA."es/locucions.dat", $DATA."es/quantities.dat", $DATA."es/sufixos.dat",
                    $DATA."es/probabilitats.dat", $DATA."es/maco.db", $DATA."es/np.dat",  
                    $DATA."common/punct.dat");

# create analyzers
my $tk=new libmorfo_perl::tokenizer($DATA."es/tokenizer.dat");
my $sp=new libmorfo_perl::splitter($DATA."es/splitter.dat");
my $mf=new libmorfo_perl::maco($op);

## exchange comments in two following lines to change the tagger type used
my $tg=new libmorfo_perl::hmm_tagger("es",$DATA."es/tagger.dat",1,2);
#my $tg=new libmorfo_perl::relax_tagger($DATA."es/constr_gram.dat", 500,670.0,0.001, 1,2);

## read input text and analyze it.
while (<STDIN>) {
    chomp;

    my $l = $tk->tokenize($_);  # tokenize
    my $ls = $sp->split($l,0);  # split sentences
    $ls = $mf->analyze($ls);    # morphological analysis
    $ls = $tg->analyze($ls);    # PoS tagging

    ## print results for complete senteces so far.
    for my $s (@$ls) {
       my $ws = $s->get_words();
       for my $w (@$ws) {
         print $w->get_form()." ".$w->get_lemma()." ".$w->get_parole()."\n";
       }
       print "\n";
    }

}



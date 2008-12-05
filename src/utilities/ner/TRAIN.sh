#! /bin/bash

##
##  Script to train a NE classifier
##


## You'll need a corpus with the right NEs detected. The
## format of this corpus must follow the B-I-O approach
##
## CANVIAR_HO
##
## a la TDFS0
## escritora escritor NCFS000
## Bego�a_Ameztoy bego�a_ameztoy NP00SP0
## ha haber VAIP3S0
## presentado presentar VMP00SM
## hoy 24/5/2000 W
## en en SPS00
## Barcelona barcelona NP00G00
## su su DP3CS00
## �ltima �ltimo AQ0FS00
## novela novela NCFS000
##  . . Fp
##

## You also need a RGF definition of which features are relevant for
## you. You can use ner.rgf in the Spanish data directory as a
## starting point

## Once you have the corpus and the RGF file, you have to adapt this
## script, and execute it. Read the comments below

## ADAPT these paths to your installation. The list must include
## the location where the required libraries (libfries, libomlet,
## libdb_cxx and libpcre) are installed. If they are in different
## directories, write them all, separated with a colon.
export LD_LIBRARY_PATH='/usr/local/myLIBS/lib:'$LD_LIBRARY_PATH

## REPLACE "corpus.txt" with the file name of your training corpus
mycorpus=corpus.txt
## REPLACE "modelname" with the basename of your .rgf file
mymodel=modelname


###### Build a lexicon file ######

## extract all features appearing in your train corpus. The 
## sample "lexicon" program produces three lexicons: one with all
## features, one with those appearing more than 3 times, and one with
## those apperaing more than 1% of the times.  You should modify the
## sample program to adjust this to your needs, and check which value
## suits your trainig data better.
./lexicon $mymodel <$mycorpus >/dev/null
## Output is redirected to /dev/null because we do not want to keep the
## features by name, but it may be intresting to have a look at it
## to check that our rules are giong fine.
## $mymodel has to be the basename (without extension) of your .rfg file.
## The generated lexicons will be named $mymodel-*.lex


###### train an AdaBoost model ########

## First, select the lexicon you want to use and copy it to $mymodel".lex"
## eg:
##     cp modelname-3abs.lex modelname.lex

## The "train" program reads mymodel.rgf and mymodel.lex and trains a
## mymodel.abm adaboost model.  The string specifies the labels of the
## NE classes. They must be the same appearing at the training corpus.
## NOTE that these three files must be used always toghether, changing 
## one of them (e.g. unsing a differnt lexicon) will NOT work.
cat $mycorpus | ./train $mymodel "0 B 1 I 2 O"


## When the script finishes, move "mymodel.*" files to your FreeLing
## configuration directory, adjust your NEC prefix file configuration
## option, and that's it!.


### MORE INFORMATION

## For further details on feature extraction and adaboost training
## see documentation and usage examples of Omlet & Fries libraries.

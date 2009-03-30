#! /bin/bash

##
##  Script to train a NE recognizer
##


## You'll need a corpus with the right NEs detected. The
## format of this corpus must follow the B-I-O approach:
## first column has the tag B, I, O that says if the word
## is at the beginning, inside or outside of a NE. Second
## column is the word and the following columns are each 
## possible lemma for this word with the corresponding PoS
## tag. Also, the probability of each lemma and word is 
## present in the corpus, but if not available, let it to -1.
##
## Example:
##
##  O El el DA0MS0 -1
##  B Tribunal tribunal NCMS000 -1
##  I Supremo supremo AQ0MS0 -1
##  O de de SPS00 -1
##  O el el DA0MS0 -1
##  O estado estado NCMS000 -1 estar VAP00SM -1
##  O de de NCFS000 -1 de SPS00 -1
##  B Victoria victoria I -1 victoria NCFS000 -1
##  ...
##
## To generate this kind of file from a corpus whith just the words
## and B-I-O tags, you can use FreeLing command:
##  cat corpus | analyze -f lang.cfg --inpf splitted --outf morfo --noprob --noquant > corpus.txt
##
## (assuming that "corpus" has a word per line where the first column
## is the word)
##
## Once you have corpus.txt, you should syncronize it with the B-I-O
## tags of the corpus. Pay special attention to words that may be
## joined by FreeLing that may correspond to more than one B-I-O tag.
##
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
##  in the format explained above
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
cp modelname-3abs.lex modelname.lex

## The "train" program reads mymodel.rgf and mymodel.lex and trains a
## mymodel.abm adaboost model.  The string specifies the labels of the
## NE classes. They must be the same appearing at the training corpus.
## NOTE that these three files must be used always toghether, changing 
## one of them (e.g. using a different lexicon) will NOT work.
cat $mycorpus | ./train $mymodel "0 B 1 I 2 O"


## When the script finishes, move "mymodel.*" files to your FreeLing
## configuration directory, adjust your NER prefix file configuration
## option, and that's it!.


### MORE INFORMATION

## For further details on feature extraction and adaboost training
## see documentation and usage examples of Omlet & Fries libraries.

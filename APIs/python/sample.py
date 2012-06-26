#! /usr/bin/python
# -*- coding: iso-8859-1 -*-

## Run as root ./sample.py
## Reads from stdin and writes to stdout
## For example:
##     ./sample.py <prueba.txt >prueba_out.txt

import libmorfo_python
import sys

## Modify this line to be your FreeLing installation directory
FREELINGDIR = "/usr/local";
DATA = FREELINGDIR+"/share/FreeLing/";
LANG="en";

# create options set for maco analyzer. Default values are Ok, except for data files.
op= libmorfo_python.maco_options("en");
op.set_active_modules(1,1,1,1,1,1,1,1,0,0);
op.set_data_files(DATA+LANG+"/locucions.dat", DATA+LANG+"/quantities.dat", DATA+LANG+"/afixos.dat",
                  DATA+LANG+"/probabilitats.dat", DATA+LANG+"/maco.db", DATA+LANG+"/np.dat",  
                  DATA+"common/punct.dat",DATA+LANG+"/corrector/corrector.dat");

# create analyzers
tk=libmorfo_python.tokenizer(DATA+LANG+"/tokenizer.dat");
sp=libmorfo_python.splitter(DATA+LANG+"/splitter.dat");
mf=libmorfo_python.maco(op);

tg=libmorfo_python.hmm_tagger("en",DATA+LANG+"/tagger.dat",1,2);
sen=libmorfo_python.senses(DATA+LANG+"/senses30.db",0);

lin=sys.stdin.readline();
while (lin) :
        
    l = tk.tokenize(lin);
    ls = sp.split(l,0);
    ls = mf.analyze(ls);
    ls = tg.analyze(ls);
    ls = sen.analyze(ls);

    for s in ls :
       ws = s.get_words();
       for w in ws :
            print w.get_form()+" "+w.get_lemma()+" "+w.get_parole();
       print

    lin=sys.stdin.readline();
    

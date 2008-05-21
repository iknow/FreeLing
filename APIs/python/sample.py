#! /usr/bin/python

from libmorfo_python import *

## Modify this line to be your FreeLing installation directory
FREELINGDIR = "/home/padro/test";
DATA = FREELINGDIR+"/share/FreeLing/";

# create options set for maco analyzer. Default values are Ok, except for data files.
op=maco_options("es");
op.set_active_modules(1,1,1,1,1,1,1,1,1);
op.set_data_files(DATA+"es/locucions.dat", DATA+"es/quantities.dat", DATA+"es/sufixos.dat",
                  DATA+"es/probabilitats.dat", DATA+"es/maco.db", DATA+"es/np.dat",  
                  DATA+"common/punct.dat");

# create analyzers
tk=tokenizer(DATA+"es/tokenizer.dat");
sp=splitter(DATA+"es/splitter.dat");
mf=maco(op);

## exchange comments in two following lines to change the tagger type used
tg=hmm_tagger("es",DATA+"es/tagger.dat",true);

lin=sys.stdin.readline();
while (lin) :
    
    l = tk.tokenize(lin);
    ls = sp.split(l,0);
    ls = mf.analyze(ls);
    ls = tg.analyze(ls);

    for s in ls :
       ws = s.get_words();
       for w in ws :
         print w.get_form()+" "+w.get_lemma()+" "+w.get_parole()+"\n";

       print "\n";

    lin=sys.stdin.readline();


import java.io.*;
import morfo.*;

public class analyzer {

    // Modify this line to be your FreeLing installation directory
    static final String FREELINGDIR = "/usr/local";
    static final String DATA = FREELINGDIR+"/share/FreeLing/";
    static final String LANG = "es";
    
    public static void main(String argv[]) throws IOException {
	System.loadLibrary("morfo_java");
	
	// create options set for maco analyzer. Default values are Ok, except for data files.
	maco_options op = new maco_options(LANG);
        op.set_active_modules(true,true,true,true,true,true,true,true,0,false);
	op.set_data_files(DATA+LANG+"/locucions.dat", DATA+LANG+"/quantities.dat", 
			  DATA+LANG+"/afixos.dat", DATA+LANG+"/probabilitats.dat", 
			  DATA+LANG+"/maco.db", DATA+LANG+"/np.dat",  
			  DATA+"common/punct.dat",DATA+LANG+"/corrector/corrector.dat");

	// create analyzers
        tokenizer tk=new tokenizer(DATA+LANG+"/tokenizer.dat");
	splitter sp=new splitter(DATA+LANG+"/splitter.dat");
	maco mf=new maco(op);

	hmm_tagger tg = new hmm_tagger(LANG,DATA+LANG+"/tagger.dat",true,2);
        chart_parser parser = new chart_parser(DATA+LANG+"/grammar-dep.dat");
        dep_txala dep = new dep_txala(DATA+LANG+"/dep/dependences.dat", parser.get_start_symbol());

	disambiguator dis = new disambiguator(DATA+"common/wn16-ukb.bin",DATA+LANG+"/senses16.ukb",0.03,10);
	// Instead of a "disambiguator", you can use a "senses" object, that simply
        // gives all possible WN senses, sorted by frequency.
	// senses sen = new senses(DATA+LANG+"/senses16.db",false);

	BufferedReader input = new BufferedReader(new InputStreamReader(System.in, "iso-8859-15"));
	// BufferedReader input = new BufferedReader(new InputStreamReader(System.in,"utf-8"));
        String line = input.readLine();
        while (line != null) {

	    ListWord l = tk.tokenize(line);      // tokenize
	    ListSentence ls = sp.split(l,false);  // split sentences
	    ls=mf.analyze(ls);                       // morphological analysis
	    tg.analyze(ls);                       // PoS tagging

	    // sen.analyze(ls);
            dis.analyze(ls);
            printResults(ls,"tagged");

	    // Chunk parser
            parser.analyze(ls); 
            printResults(ls,"parsed");

	    // Dependency parser
            dep.analyze(ls);
            printResults(ls,"dep");

	    line = input.readLine();
	}

    } // main


    static void print_senses(word w) {
	String ss = w.get_senses_string();
	// The senses for a FreeLing word are a list of
        // pair<string,double> (sense and page rank). From java, we
        // have to get them as a string with format
        // sense:rank/sense:rank/sense:rank 
	// which will have to be splitted to obtain the info.
	// Here, we just output it: 
	System.out.print(" " + ss);
    }

    static void printResults(ListSentence ls, String format) {

	if (format=="parsed") {
	    System.out.println("-------- CHUNKER results -----------");
	    for (int i=0; i<ls.size(); i++) {
		TreeNode tree = ls.get(i).get_parse_tree();
		printParseTree(0,tree);
	    }
	}
	else if (format == "dep") {
	    System.out.println("-------- DEPENDENCY PARSER results -----------");
	    for (int i=0; i<ls.size(); i++) {
		TreeDepnode deptree =  ls.get(i).get_dep_tree();
		printDepTree(0,deptree);
	    }
	}
	else {
	    System.out.println("-------- TAGGER results -----------");
	    // get the analyzed words out of ls.  
	    for (int i=0; i<ls.size(); i++) {
		sentence s=ls.get(i);
		for (int j=0; j<s.size(); j++) {
		    word w=s.get(j);
		    System.out.print(w.get_form()+" "+w.get_lemma()+" "+w.get_parole());
                    print_senses(w);
		    System.out.println();
		}
		System.out.println();
	    }
	} 

    } // printResults
    

    static void printParseTree ( int depth,  TreeNode tr ) {
	word w;
	TreeNode child;
	node nd;
	long nch;
	
        // indent
	for (int i=0; i <depth; i++) System.out.print("  ");

	nch = tr.num_children();
	if (nch == 0) {
	    // it's a leaf
	    if (tr.get_info().is_head()) System.out.print("+");
	    w = tr.get_info().get_word();
	    System.out.print("("+w.get_form()+" "+w.get_lemma()+" "+w.get_parole());
	    print_senses(w);
	    System.out.println(")");
	}
	else {
            // not a leaf
	    if (tr.get_info().is_head()) System.out.print("+");
	    System.out.println(tr.get_info().get_label() + "_[");
	    
	    for (int i=0; i<nch; i++) {
		child = tr.nth_child_ref(i);
		if (child != null) printParseTree(depth+1, child);
		else System.err.println("ERROR: Unexpected NULL child.");
	    }
	    for (int i=0; i<depth; i++)	System.out.print("  ");
	    System.out.println("]");
	}
    } // printParseTree


    static void printDepTree ( int depth,  TreeDepnode tr ) {
	TreeDepnode child = null;
	TreeDepnode fchild = null;
	depnode childnode;
	depnode nd;
	long nch;
	int last, min;
	Boolean trob;
	
	for (int i=0; i<depth; i++) System.out.print("  ");
	
	System.out.print(tr.get_info().get_link_ref().get_info().get_label() + "/" + tr.get_info().get_label() + "/");
	word w = tr.get_info().get_word();
	System.out.print("(" + w.get_form() + " " + w.get_lemma() + " " + w.get_parole());
	print_senses(w);
	System.out.print(")");
	
	nch = tr.num_children();
	if (nch>0) {
	    System.out.println(" [");
	    
	    for (int i=0; i<nch; i++) {
		child = tr.nth_child_ref(i);
		if (child != null) {
		    if (! child.get_info().is_chunk())
			printDepTree(depth+1, child);
		}
		else System.err.println("ERROR: Unexpected NULL child.");
	    }
	    // print CHUNKS (in order)
	    last=0;
	    trob=true;
	    //while an unprinted chunk is found look, for the one with lower chunk_ord value
	    while (trob) {
		trob=false; min=9999;
		for (int i=0; i<nch; i++) {
		    child = tr.nth_child_ref(i);
		    childnode = child.get_info();
		    if (childnode.is_chunk()) {
			if ((childnode.get_chunk_ord()>last) && (childnode.get_chunk_ord()<min)) {
			    min=childnode.get_chunk_ord();
			    fchild = child;
			    trob=true;
			}
		    }
		}
		if (trob && (child!=null)) printDepTree(depth+1, fchild);
		last=min;
	    }
	    
	    for (int i=0; i<depth; i++) System.out.print("  ");
	    System.out.print("]");
	} 
	System.out.println("");

    } // printDepTree
    

};  // class analyzer



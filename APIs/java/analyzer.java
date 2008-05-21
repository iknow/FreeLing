
import java.io.*;
import morfo.*;

public class analyzer {

    // Modify this line to be your FreeLing installation directory
    static final String FREELINGDIR = "/home/padro/test";
    static final String DATA = FREELINGDIR+"/share/FreeLing/";
    
    public static void main(String argv[]) throws IOException {
	System.loadLibrary("morfo_java");
	
	// create options set for maco analyzer. Default values are Ok, except for data files.
	maco_options op = new maco_options("es");
        op.set_active_modules(true,true,true,true,true,true,true,true,true);
	op.set_data_files(DATA+"es/locucions.dat", DATA+"es/quantities.dat", DATA+"es/sufixos.dat",
			  DATA+"es/probabilitats.dat", DATA+"es/maco.db", DATA+"es/np.dat",  
			  DATA+"common/punct.dat");
		
	// create analyzers
        tokenizer tk=new tokenizer(DATA+"es/tokenizer.dat");
	splitter sp=new splitter(DATA+"es/splitter.dat");
	maco mf=new maco(op);
	
	hmm_tagger tg = new hmm_tagger("es",DATA+"es/tagger.dat",true,2);
        chart_parser parser = new chart_parser(DATA+"es/grammar-dep.dat");
        dependencyMaker dep = new dependencyMaker(DATA+"es/dep/dependences.dat", parser.get_start_symbol());


	BufferedReader input = new BufferedReader(new InputStreamReader(System.in, "iso-8859-15"));
	// BufferedReader input = new BufferedReader(new InputStreamReader(System.in,"utf-8"));
        String line = input.readLine();
        while (line != null) {

	    ListWord l = tk.tokenize(line);      // tokenize
	    ListSentence ls = sp.split(l,false);  // split sentences
	    ls=mf.analyze(ls);                       // morphological analysis
	    ls=tg.analyze(ls);                       // PoS tagging
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
		    System.out.println(w.get_form()+" "+w.get_lemma()+" "+w.get_parole());
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
	    System.out.println("(" + w.get_form() + " " + w.get_lemma() + " " + w.get_parole() + ")" );
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
	System.out.print("(" + w.get_form() + " " + w.get_lemma() + " " + w.get_parole() + ")" );
	
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
	    System.out.println("]");
	} 
	System.out.println("");

    } // printDepTree
    

};  // class analyzer



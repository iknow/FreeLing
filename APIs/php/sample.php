<? 

 include("analyzer.php");

 $FL_DATA = "/usr/local/share/FreeLing";

 // launch a server at port 12345
 $a = new analyzer("12345", "-f $FL_DATA/config/es.cfg --utf");

 // analyze a text
 $output = $a->analyze_text("los niños comen pescado");
 print $output;

 // You can also analyze a text file (and send results to another file in this example)
 $output = $a->analyze_file("myfile.txt","myfile.out");

 //// -----------------------------------------------------
 //// Alternatively, you can connect to an existing server on any machine.
 ////    Note that this will only work if you previously used the
 ////    "analyze" script to launch a server on that machine:port
 // $b = new analyzer("myserver.home.org:12345");

 //// analyze text
 // $output = $b->analyze_text("los niños comen pescado");
 // print $output;

?>

<? 

 include("analyzer.php");

 $FL_DATA = "/usr/local/share/FreeLing";

 // launch a server at port 12345. 
 // Have a look to analyzer.php to find out available parameters.
 $a = new analyzer("12345", "-f $FL_DATA/config/es.cfg --utf");

 $output = $a->analyze_text("Lorem Ipsum ha sido el texto de relleno estándar de las industrias desde el año 1500, cuando un impresor (N. del T. persona que se dedica a la imprenta) desconocido usó una galería de textos y los mezcló de tal manera que logró hacer un");
 print $output;

 // analyze a text
 $output = $a->analyze_text("los niños comen pescado");
 print $output;

 // You can also analyze a text file (and send results to another file in this example)
 // $output = $a->analyze_file("myfile.txt","myfile.out");

 //// -----------------------------------------------------
 //// Alternatively, you can connect to an existing server on any machine.
 ////    Note that this will only work if you previously used the
 ////    "analyze" script to launch a server on that machine:port
 // $b = new analyzer("myserver.home.org:12345");

 //// analyze text
 // $output = $b->analyze_text("los niños comen pescado");
 // print $output;

?>

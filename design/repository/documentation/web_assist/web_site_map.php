<?php

site_map( "top", "index.php", "Home" );
site_map( "gip", "gip", "GIP" );
site_push( "gip/" );
site_map( "structure", "structure", "GIP structure" );
site_map( "rf", "rf", "RF" );
site_push( "rf/" );
site_pop();
site_map( "alu", "alu", "ALU and shifter" );
site_push( "alu/" );
site_map( "operations", "operations.php", "Operations" );
site_map( "dataflow", "dataflow.php", "Dataflow" );
site_map( "implementation", "implementation.php", "Implementation" );
site_pop();
site_map( "arm_emulation", "arm_emulation", "ARM emulation" );
site_map( "microkernel", "microkernel", "Microkernel" );
site_pop();
/*
site_map( "company", "company", "Company" );
site_push( "company/" );
site_map( "contacts", "contacts.php", "Contacts" );
site_map( "people", "people", "People" );
site_push( "people/" );
site_map( "gavin", "gavin_stark", "Gavin J Stark" );
site_map( "john", "john_croft", "John Croft" );
site_pop();
site_pop();
site_map( "technology", "technology", "Technology" );
site_push( "technology/" );
site_map( "", "http://cyclicity-cdl.sourceforge.net", "Cyclicity CDL" );
site_pop();
site_map( "white_papers", "white_papers", "White Papers" );
site_map( "background", "background", "Background" );
*/
site_done();

?>

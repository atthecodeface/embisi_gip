<?php

site_map( "top", "index.php", "Home" );
site_map( "gip", "gip", "GIP" );
site_push( "gip/" );
site_map( "structure", "structure", "GIP structure" );
site_map( "instruction_set", "instruction_set", "Internal instruction set" );
site_push( "instruction_set/" );
site_pop();
site_map( "native_instruction_set", "native_instruction_set", "Native instruction set" );
site_push( "native_instruction_set/" );
site_pop();
site_map( "thread", "thread", "Threads" );
site_push( "threads/" );
site_pop();
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
site_push( "arm_emulation/" );
site_map( "accumulator", "accumulator.php", "Accumulator" );
site_map( "prefetch", "prefetch.php", "Prefetch" );
site_map( "flushing", "flushing.php", "Pipeline flushing" );
site_map( "branch", "branch.php", "Branch" );
site_map( "data_processing", "data_processing.php", "Data processing" );
site_map( "multiply", "multiply.php", "Multiply" );
site_map( "single_data_transfer", "single_data_transfer.php", "Single data transfer" );
site_map( "block_data_transfer", "block_data_transfer.php", "Block data transfer" );
site_pop();
site_map( "microkernel", "microkernel", "Microkernel" );
site_push( "microkernel/" );
site_map( "outline_operation", "outline_operation.php", "Outline Operation" );
site_map( "detailed_operation", "detailed_operation.php", "Detailed Operation" );
site_map( "linux", "linux.php", "Linux operation" );
site_map( "outline_code", "outline_code.php", "Outline code" );
site_pop();
site_pop();
site_done();

?>

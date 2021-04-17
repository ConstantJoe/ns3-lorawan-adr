#!/bin/bash
  
    ./waf --run "lorawan-example-later -nNodes=100 -dr=0 --RngRun=1" &> output_thesis/later-DR0/adr-100-0-1.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=0 --RngRun=2" &> output_thesis/later-DR0/adr-100-0-2.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=0 --RngRun=3" &> output_thesis/later-DR0/adr-100-0-3.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=0 --RngRun=4" &> output_thesis/later-DR0/adr-100-0-4.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=0 --RngRun=5" &> output_thesis/later-DR0/adr-100-0-5.txt &

	./waf --run "lorawan-example-later -nNodes=100 -dr=3 --RngRun=1" &> output_thesis/later-DR3/adr-100-3-1.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=3 --RngRun=2" &> output_thesis/later-DR3/adr-100-3-2.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=3 --RngRun=3" &> output_thesis/later-DR3/adr-100-3-3.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=3 --RngRun=4" &> output_thesis/later-DR3/adr-100-3-4.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=3 --RngRun=5" &> output_thesis/later-DR3/adr-100-3-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=500 -dr=0 --RngRun=1" &> output_thesis/later-DR0/adr-500-0-1.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=0 --RngRun=2" &> output_thesis/later-DR0/adr-500-0-2.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=0 --RngRun=3" &> output_thesis/later-DR0/adr-500-0-3.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=0 --RngRun=4" &> output_thesis/later-DR0/adr-500-0-4.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=0 --RngRun=5" &> output_thesis/later-DR0/adr-500-0-5.txt &

	./waf --run "lorawan-example-later -nNodes=500 -dr=3 --RngRun=1" &> output_thesis/later-DR3/adr-500-3-1.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=3 --RngRun=2" &> output_thesis/later-DR3/adr-500-3-2.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=3 --RngRun=3" &> output_thesis/later-DR3/adr-500-3-3.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=3 --RngRun=4" &> output_thesis/later-DR3/adr-500-3-4.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=3 --RngRun=5" &> output_thesis/later-DR3/adr-500-3-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=1000 -dr=0 --RngRun=1" &> output_thesis/later-DR0/adr-1000-0-1.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=0 --RngRun=2" &> output_thesis/later-DR0/adr-1000-0-2.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=0 --RngRun=3" &> output_thesis/later-DR0/adr-1000-0-3.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=0 --RngRun=4" &> output_thesis/later-DR0/adr-1000-0-4.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=0 --RngRun=5" &> output_thesis/later-DR0/adr-1000-0-5.txt &

	./waf --run "lorawan-example-later -nNodes=1000 -dr=3 --RngRun=1" &> output_thesis/later-DR3/adr-1000-3-1.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=3 --RngRun=2" &> output_thesis/later-DR3/adr-1000-3-2.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=3 --RngRun=3" &> output_thesis/later-DR3/adr-1000-3-3.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=3 --RngRun=4" &> output_thesis/later-DR3/adr-1000-3-4.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=3 --RngRun=5" &> output_thesis/later-DR3/adr-1000-3-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=2500 -dr=0 --RngRun=1" &> output_thesis/later-DR0/adr-2500-0-1.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=0 --RngRun=2" &> output_thesis/later-DR0/adr-2500-0-2.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=0 --RngRun=3" &> output_thesis/later-DR0/adr-2500-0-3.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=0 --RngRun=4" &> output_thesis/later-DR0/adr-2500-0-4.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=0 --RngRun=5" &> output_thesis/later-DR0/adr-2500-0-5.txt &

	./waf --run "lorawan-example-later -nNodes=2500 -dr=3 --RngRun=1" &> output_thesis/later-DR3/adr-2500-3-1.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=3 --RngRun=2" &> output_thesis/later-DR3/adr-2500-3-2.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=3 --RngRun=3" &> output_thesis/later-DR3/adr-2500-3-3.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=3 --RngRun=4" &> output_thesis/later-DR3/adr-2500-3-4.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=3 --RngRun=5" &> output_thesis/later-DR3/adr-2500-3-5.txt 
	
	

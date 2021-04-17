#!/bin/bash
  
    ./waf --run "lorawan-example-later -nNodes=100 -dr=2 --RngRun=1" &> output_thesis/later-DR2/adr-100-2-1.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=2 --RngRun=2" &> output_thesis/later-DR2/adr-100-2-2.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=2 --RngRun=3" &> output_thesis/later-DR2/adr-100-2-3.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=2 --RngRun=4" &> output_thesis/later-DR2/adr-100-2-4.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=2 --RngRun=5" &> output_thesis/later-DR2/adr-100-2-5.txt &

	./waf --run "lorawan-example-later -nNodes=100 -dr=5 --RngRun=1" &> output_thesis/later-DR5/adr-100-5-1.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=5 --RngRun=2" &> output_thesis/later-DR5/adr-100-5-2.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=5 --RngRun=3" &> output_thesis/later-DR5/adr-100-5-3.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=5 --RngRun=4" &> output_thesis/later-DR5/adr-100-5-4.txt &
	./waf --run "lorawan-example-later -nNodes=100 -dr=5 --RngRun=5" &> output_thesis/later-DR5/adr-100-5-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=500 -dr=2 --RngRun=1" &> output_thesis/later-DR2/adr-500-2-1.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=2 --RngRun=2" &> output_thesis/later-DR2/adr-500-2-2.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=2 --RngRun=3" &> output_thesis/later-DR2/adr-500-2-3.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=2 --RngRun=4" &> output_thesis/later-DR2/adr-500-2-4.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=2 --RngRun=5" &> output_thesis/later-DR2/adr-500-2-5.txt &

	./waf --run "lorawan-example-later -nNodes=500 -dr=5 --RngRun=1" &> output_thesis/later-DR5/adr-500-5-1.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=5 --RngRun=2" &> output_thesis/later-DR5/adr-500-5-2.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=5 --RngRun=3" &> output_thesis/later-DR5/adr-500-5-3.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=5 --RngRun=4" &> output_thesis/later-DR5/adr-500-5-4.txt &
	./waf --run "lorawan-example-later -nNodes=500 -dr=5 --RngRun=5" &> output_thesis/later-DR5/adr-500-5-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=1000 -dr=2 --RngRun=1" &> output_thesis/later-DR2/adr-1000-2-1.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=2 --RngRun=2" &> output_thesis/later-DR2/adr-1000-2-2.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=2 --RngRun=3" &> output_thesis/later-DR2/adr-1000-2-3.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=2 --RngRun=4" &> output_thesis/later-DR2/adr-1000-2-4.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=2 --RngRun=5" &> output_thesis/later-DR2/adr-1000-2-5.txt &

	./waf --run "lorawan-example-later -nNodes=1000 -dr=5 --RngRun=1" &> output_thesis/later-DR5/adr-1000-5-1.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=5 --RngRun=2" &> output_thesis/later-DR5/adr-1000-5-2.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=5 --RngRun=3" &> output_thesis/later-DR5/adr-1000-5-3.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=5 --RngRun=4" &> output_thesis/later-DR5/adr-1000-5-4.txt &
	./waf --run "lorawan-example-later -nNodes=1000 -dr=5 --RngRun=5" &> output_thesis/later-DR5/adr-1000-5-5.txt 
	
	./waf --run "lorawan-example-later -nNodes=2500 -dr=2 --RngRun=1" &> output_thesis/later-DR2/adr-2500-2-1.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=2 --RngRun=2" &> output_thesis/later-DR2/adr-2500-2-2.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=2 --RngRun=3" &> output_thesis/later-DR2/adr-2500-2-3.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=2 --RngRun=4" &> output_thesis/later-DR2/adr-2500-2-4.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=2 --RngRun=5" &> output_thesis/later-DR2/adr-2500-2-5.txt &

	./waf --run "lorawan-example-later -nNodes=2500 -dr=5 --RngRun=1" &> output_thesis/later-DR5/adr-2500-5-1.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=5 --RngRun=2" &> output_thesis/later-DR5/adr-2500-5-2.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=5 --RngRun=3" &> output_thesis/later-DR5/adr-2500-5-3.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=5 --RngRun=4" &> output_thesis/later-DR5/adr-2500-5-4.txt &
	./waf --run "lorawan-example-later -nNodes=2500 -dr=5 --RngRun=5" &> output_thesis/later-DR5/adr-2500-5-5.txt 
	
	

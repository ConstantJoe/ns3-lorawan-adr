#!/bin/bash
  
    ./waf --run "lorawan-example -nNodes=100 -dr=DR1 --RngRun=1" &> output_thesis/DR1/adr-100-1-1.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR1 --RngRun=2" &> output_thesis/DR1/adr-100-1-2.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR1 --RngRun=3" &> output_thesis/DR1/adr-100-1-3.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR1 --RngRun=4" &> output_thesis/DR1/adr-100-1-4.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR1 --RngRun=5" &> output_thesis/DR1/adr-100-1-5.txt &

	./waf --run "lorawan-example -nNodes=100 -dr=DR4 --RngRun=1" &> output_thesis/DR4/adr-100-1-1.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR4 --RngRun=2" &> output_thesis/DR4/adr-100-1-2.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR4 --RngRun=3" &> output_thesis/DR4/adr-100-1-3.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR4 --RngRun=4" &> output_thesis/DR4/adr-100-1-4.txt &
	./waf --run "lorawan-example -nNodes=100 -dr=DR4 --RngRun=5" &> output_thesis/DR4/adr-100-1-5.txt 
	
	./waf --run "lorawan-example -nNodes=500 -dr=DR1 --RngRun=1" &> output_thesis/DR1/adr-500-1-1.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR1 --RngRun=2" &> output_thesis/DR1/adr-500-1-2.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR1 --RngRun=3" &> output_thesis/DR1/adr-500-1-3.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR1 --RngRun=4" &> output_thesis/DR1/adr-500-1-4.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR1 --RngRun=5" &> output_thesis/DR1/adr-500-1-5.txt &

	./waf --run "lorawan-example -nNodes=500 -dr=DR4 --RngRun=1" &> output_thesis/DR4/adr-500-1-1.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR4 --RngRun=2" &> output_thesis/DR4/adr-500-1-2.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR4 --RngRun=3" &> output_thesis/DR4/adr-500-1-3.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR4 --RngRun=4" &> output_thesis/DR4/adr-500-1-4.txt &
	./waf --run "lorawan-example -nNodes=500 -dr=DR4 --RngRun=5" &> output_thesis/DR4/adr-500-1-5.txt 
	
	./waf --run "lorawan-example -nNodes=1000 -dr=DR1 --RngRun=1" &> output_thesis/DR1/adr-1000-1-1.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR1 --RngRun=2" &> output_thesis/DR1/adr-1000-1-2.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR1 --RngRun=3" &> output_thesis/DR1/adr-1000-1-3.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR1 --RngRun=4" &> output_thesis/DR1/adr-1000-1-4.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR1 --RngRun=5" &> output_thesis/DR1/adr-1000-1-5.txt &

	./waf --run "lorawan-example -nNodes=1000 -dr=DR4 --RngRun=1" &> output_thesis/DR4/adr-1000-1-1.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR4 --RngRun=2" &> output_thesis/DR4/adr-1000-1-2.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR4 --RngRun=3" &> output_thesis/DR4/adr-1000-1-3.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR4 --RngRun=4" &> output_thesis/DR4/adr-1000-1-4.txt &
	./waf --run "lorawan-example -nNodes=1000 -dr=DR4 --RngRun=5" &> output_thesis/DR4/adr-1000-1-5.txt 
	
	./waf --run "lorawan-example -nNodes=2500 -dr=DR1 --RngRun=1" &> output_thesis/DR1/adr-2500-1-1.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR1 --RngRun=2" &> output_thesis/DR1/adr-2500-1-2.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR1 --RngRun=3" &> output_thesis/DR1/adr-2500-1-3.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR1 --RngRun=4" &> output_thesis/DR1/adr-2500-1-4.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR1 --RngRun=5" &> output_thesis/DR1/adr-2500-1-5.txt &

	./waf --run "lorawan-example -nNodes=2500 -dr=DR4 --RngRun=1" &> output_thesis/DR4/adr-2500-1-1.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR4 --RngRun=2" &> output_thesis/DR4/adr-2500-1-2.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR4 --RngRun=3" &> output_thesis/DR4/adr-2500-1-3.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR4 --RngRun=4" &> output_thesis/DR4/adr-2500-1-4.txt &
	./waf --run "lorawan-example -nNodes=2500 -dr=DR4 --RngRun=5" &> output_thesis/DR4/adr-2500-1-5.txt 
	
	

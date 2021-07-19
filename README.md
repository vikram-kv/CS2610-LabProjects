# CS2610-LabProjects

This repo contains two projects done as part of the lab course _CS2610 - 	Computer Organization and Architecture Lab_ under Dr. Madhu Mutyam in the Jan - May 2021 semester. Both the projects were done in groups of 3. My teammates were [Nabyendu](https://github.com/saha-nab2001) and [Anirudh](https://github.com/anikk7).

### Project 1: Cache Simulator

We designed a cache simulator that simulates a cache given its associativity and the replacement policy followed. The accepted replacement policies are Random Replacement Policy, Least Recently Used(_LRU_) Policy and Tree-based Pseudo-LRU Policy. The problem statement can be found in _CacheSimulator.pdf_ along with more details on our implementation. Given a memory trace detailing access address and access type(r/w) as input, our code produces various cache events caused by the input which includes # of misses of each type(compulsary/conflict/capacity,read/write), # of accesses(read/write) and # of dirty blocks evicted. Read _readme.txt_ before executing our code. The language used was C++.

###### Test Instance

The memory trace in _input.txt_ with the sample terminal inputs as given in _readme.txt_ produces the sample output given in _readme.txt_.


### Project 2: Pipelined(5 stage) MIPS Processor

We designed a virtual pipelined RISC processor using python. The data hazards(RAW) are addressed by having a flag for each register and control hazards cause stalls in the pipeline. The instruction set and other pertinent details can be found in _PipelinedProcessor.pdf_. The bash script file _compile.sh_ acts as an 'assembler' to produce machine instructions for the assembly code. Our code produces the program-modified _DCache.txt_ and _Output.txt_ which contains various characteristics of the program run(like # of stalls, # of insns of various types, avg CPI, etc). Both these files will be in the output folder. 

###### Test Instance

Here, the assembly code in _program.asm_ has been assembled to give the ICache.txt in _input_ folder. And the files given in the _input_ folder have been fed into the pipeline to produce the outputs in _output_ folder.

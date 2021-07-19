
Team : K V Vikram, Anirudh and Nabyendu

Implementation Details:-

	i) We have used the c++ language for designing the simulator. The code file is CacheSimulator.cpp.
	ii) We have also given the python file input_gen.py which we have used to generate several input files
	for testing purposes with "nearly" random memory traces. One of such files is input.txt using which we 
	will generate the sample output given below.
	iii) Our CacheBlock will not have a pointer to another CacheBlock because we have used STL containers.
	
Compilation And Execution Commands:-

g++ -std=c++17 -o CacheSimulator CacheSimulator.cpp
./CacheSimulator

Sample Input(from terminal):-

Enter the cache size(in bytes):8192
Enter the block size(in bytes):64
Enter the associativity code:8
Enter the replacement policy code:2
Enter the input file name:input.txt

Sample Output(to terminal):-

Cache Size: 8192
Block Size: 64
8 Way Set-Associative Cache
Pseudo-LRU Replacement Policy
Number of Cache Accesses: 65536
Number of Read Accesses: 32791
Number of Write Accesses: 32745
Number of Cache Misses: 65021
Number of Compulsory Misses: 16080
Number of Capacity Misses: 0
Number of Conflict Misses: 48941
Number of Read Misses: 32519
Number of Write Misses: 32502
Number of Dirty Blocks Evicted: 32568

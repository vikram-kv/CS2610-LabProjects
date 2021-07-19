/**********************************************
File: Cache Simulator - Lab 6 Group Assignment
Course: CS2610
Authors: K V Vikram(CS19B021), Anirudh Kulkarni(CS19B075) and Nabyendu Saha(CS19B076)
Inputs : Cache specifications from stdin and memory traces along with access type(read/write) from a file
Outputs: Cache stats to stdout
Assumptions: Write-back cache
**********************************************/

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_set>			
using namespace std;

/**********************************************************
*struct: CacheBlock
*desc: stores tag value(in long integer form), validity bit and dirty bit
**********************************************************/
struct CacheBlock
{
	long tag;
	bool validBit;
	bool dirtyBit;
};

/**********************************************************
*struct: CacheData
*desc: stores cache statistic parameters like # of accesses,misses,dirty blocks
*      evicted and few other subcategories of accesses/misses
**********************************************************/
struct CacheData
{
	long long accesses;
	long long readAccesses;
	long long writeAccesses;
	long long misses;
	long long compulsoryMisses;
	long long capacityMisses;
	long long conflictMisses;
	long long readMisses;
	long long writeMisses;
	long long dirtyBlocksEvicted;
	/***********************************************************
	*constructor: CacheData
	*desc:        sets all parameters to 0
	***********************************************************/
	CacheData()
	{
		accesses = readAccesses = writeAccesses = 0;
		misses = compulsoryMisses = capacityMisses = 0;
		conflictMisses = readMisses = writeMisses = 0;
		dirtyBlocksEvicted = 0;
	}
	//function to print all the parameter values of CacheData structure in stdout
	void PrintDetails()
	{
		cout << "Number of Cache Accesses: " << accesses << endl;
		cout << "Number of Read Accesses: " << readAccesses << endl;
		cout << "Number of Write Accesses: " << writeAccesses << endl;
		cout << "Number of Cache Misses: " << misses << endl;
		cout << "Number of Compulsory Misses: " << compulsoryMisses << endl;
		cout << "Number of Capacity Misses: " << capacityMisses << endl;
		cout << "Number of Conflict Misses: " << conflictMisses << endl;
		cout << "Number of Read Misses: " << readMisses << endl;
		cout << "Number of Write Misses: " << writeMisses << endl;
		cout << "Number of Dirty Blocks Evicted: " << dirtyBlocksEvicted << endl;
	}
};

/*************************************************************************************
*func: hexstringToInteger
*desc: converts hexadecimal number of the form 0x(8 digits) given as a string
*      to long integer. capital or small letters can be used for digits of value >=10
*************************************************************************************/
long hexstringToInteger(string str)
{
	long intValue = 0;
	for(int i = 2;i<10;++i)
	{
		intValue *= 16;
		if(str[i] < 'A')								//here, str[i] is in 0,1...,9
			intValue += (int)(str[i]-'0');
		else if(str[i] >= 'a')							//here, str[i] is in a,b...,f
			intValue += 10 + (int)(str[i]-'a');
		else											//here, str[i] is in A,B,..,F
			intValue += 10 + (int)(str[i]-'A');			
	}
	return intValue;									//equivalent long integer is returned
}

/*******************************************************************************
*func: longToBinary
*desc: converts the long integer "value" to binary string of length "length"
*******************************************************************************/
string longToBinary(long value,long length)
{
	string str;
	str.resize(length);									//stmts to resize str to size 'length' and make it
	for(int i=0;i<length;++i)							//all its entries '0'
	{
		str[i] = '0';
	}
	int j = length;
	while(value != 0)
	{
		str[--j] =(char)(value%2+48);					//setting str[--j] to '0'(ascii = 48)/'1' depending 
		value/=2;										//on value%2. loop update is value = value/2
	}
	return str;											//bitstring str is returned
}

class Cache
{
private:
	long cacheSize;										//cache size in bytes
	long blockSize;										//block size in bytes
	long noOfSets;										//# of sets in cache
	long setSize;										//# of ways in a set
	const int rpCode;									//replacement policy code
	CacheData data;										//cache statistic data storing variable
	vector<vector<CacheBlock>> vcache; 					//for usage only with PLRU policy
	vector<list<CacheBlock>> lcache;   					//for usage only with LRU and RANDOM policies
	//set for storing block addresses of blocks which have been accessed atleast once
	unordered_set<long> accessSet;
	//vector of boolean arrays used for representing perfect trees for each set. Used only with PLRU policy
	vector<bool*> treeList;

	/***********************************************************************
	*func: updateMisses
	*desc: if a cache miss occurs, total miss count is incremented
	       Depending on accessType(= 'r'/'w'), corresponding counter is incremented
	       Depending on type of miss, corrsponding counter is incremented
	       Select is for choosing between conflict and capacity miss
	*************************************************************************/
	void updateMisses(char accessType,long blockAddress,bool select)
	{
		++data.misses;               							//total miss count is incremented
		if(accessType == 'r')        							//read/write miss count is incremented depending on accessType
		{
			++data.readMisses;
		}
		else
		{
			++data.writeMisses;
		}
		if(accessSet.find(blockAddress) == accessSet.end())		//If the block(referenced by blockAddress) has not been 
		{														//accessed previously, we have a compulsory(cold) miss
			++ data.compulsoryMisses;
			accessSet.insert(blockAddress);						//we add blockAddress to accessSet
		}
		else
		{														//here, blockAddress has been accessed before
			if(select == 0)										//if select = 0, we dont have a FA cache.					
				++ data.conflictMisses;							//so, we increment conflict misses count.
			else												//otherwise, for a FA cache, we increment 
				++ data.capacityMisses;							//capacity miss count
		}
	}

	/***************************************************************************
	*func: FindInsertionPositionPLRU
	*desc: traverses the set tree to find appropriate position for inserting a new
	       block in the set at 'index' position. Also updates the tree bits
	       simultaneously. The insertion position is found by following the tree
	       starting with the root, moving left if we see a '0' and moving right if
	       we see a '1' till we reach an invalid node
	***************************************************************************/
	long FindInsertionPositionPLRU(long index)
	{
		long position = 0;					 //stores insertion position for new block
		long i = 1;							 //start from root at index = 1 
		while(i < setSize)					 //loop till we reach an invalid node
		{
			position *= 2;				     //stmts to update position at each level of the tree
			position += treeList[index][i]; 
			if(treeList[index][i] == 0)    	 //if current bit is 0, change it to 1 and go left
			{
				treeList[index][i] = 1;
				i *= 2;
			}
			else                          	 //otherwise, change current bit to 0 and go right
			{
				treeList[index][i] = 0;
				i = 2*i + 1;
			}
		}
		return position;					 //position is returned
	}

	/************************************************************************
	*func: UpdateTreeBitsPLRU
	*desc: updates the bits after an access has been made to the block at 'position'
		   of the set at 'index'. The tree bits in the path given by binary representation
		   of 'position' are made to point to the sub-tree not corresponding to the path
		   (i.e) each bit in the path is assigned the complement of the corr. bit in binary
		   representation of 'position'.
	*************************************************************************/
	void UpdateTreeBitsPLRU(long index,long position)
	{
		long binaryLength = (long)log2(setSize);
		string binaryRep = longToBinary(position,binaryLength);	 //stmt to convert position to its binary form
		long i = 1;
		long strPtr = 0;
		while(i < setSize)
		{
			//the current bit of path is assigned the complement of current value in binaryRep
			treeList[index][i] =(bool)('1' - binaryRep[strPtr]);
			if(binaryRep[strPtr] == '0')
				i = 2*i;
			else
				i = 2*i + 1;
			++strPtr;
		}
	}

	/*********************************************************************
	*func: Vsearch
	*desc: search for a block with tag = 'tag' in the set at 'index'.
		   used only for vcache(plru policy)
	*********************************************************************/
	long VSearch(long index,long tag)
	{
		for(long i=0;i<setSize;++i)
		{
			if(vcache[index][i].validBit == 1 && vcache[index][i].tag == tag)
			{
				return i;  //found at i
			}
		}
		return -1; //not found
	}

	/*********************************************************************
	*func: Lsearch
	*desc: search for a block with tag = 'tag' in the set at 'index'.
		   used only for lcache(random or lru policy)
	*********************************************************************/
	list<CacheBlock>::iterator LSearch(long index,long tag)
	{
		auto itr = lcache[index].begin();
		for(;itr!=lcache[index].end();++itr)
		{
			if((*itr).validBit == 1 && (*itr).tag == tag)
			{
				return itr; //found so return corr. itr pointer
			}
		}
		return lcache[index].end();  //not found
	}

public:
	//constructor
	//cSize = cache size, bSize = block size, aCode = associativity code and replacementPolicyCode = 
	//replacement policy code(both are defined as in assignment file)
	Cache(long cSize,long bSize,size_t aCode,size_t replacementPolicyCode):rpCode(replacementPolicyCode)
	{
		cacheSize = cSize; 
		blockSize = bSize;
		if(aCode == 0)
		{
			setSize = (cSize/bSize);  		//fully associative
		}
		else if(aCode == 1)
		{
			setSize = 1;              		//direct mapped so set size is 1
		}
		else
		{
			setSize = aCode;         		//set associative so set size is given
		}
		noOfSets = (cSize/bSize)/setSize;  	//= (cache size/ block size)/ set size
		if(rpCode == 0 || rpCode == 1)     	//if lru or random, we use vector<list> of CacheBlocks
		{
			//Stmts for initialization of lcache and accessSet
			lcache.clear();
			lcache.resize(noOfSets);
			for(auto itr = lcache.begin();itr!= lcache.end();++itr)
			{
				(*itr).clear();
			}
			accessSet.clear();
		}
		else                              //if plru, we use vector<vector> of CacheBlocks
		{
			//Stmts for initialization of vcache and accessSet
			vcache.clear();
			vcache.resize(noOfSets);
			for(auto itr = vcache.begin();itr!= vcache.end();++itr)
			{
				(*itr).clear();
				for(long i = 0;i < setSize;++i)
				{
					(*itr).push_back(CacheBlock{-1,0,0});	//invalid(valid = 0) blocks with tag = -1
				}
			}
			accessSet.clear();
			//stmts for initialization of treeList(vector of boolean arrays, 1 array for each set)
			//we start indexing at 1 for the root of a tree. So, left(node i) = node @ 2*i and 
			//right(node i) = node @ 2*i + 1
			treeList.clear();
			treeList.resize(noOfSets);
			for(int i = 0;i < noOfSets;++i)
			{
				treeList[i] = new bool[setSize];
				for(int j = 0;j < setSize; ++j)
				{
					treeList[i][j] = 0;       //initialize all entries of all trees to 0        
				} 
			}
		}
	}

	/**********************************************************************
	*func: access
	*desc: accesses the byte 'address'. Access Type = READ if 'readWrite' ='r'
	       and = WRITE if 'readWrite' ='w'. 'address' is in hex representation
	**********************************************************************/
	void access(string address,char readwrite)
	{
		long decAddrs = hexstringToInteger(address);	//convert address to long integer
		++data.accesses;                            	//increment to total access count
		if(readwrite == 'r')
		{
			++data.readAccesses;                   	 	//for READ, increment read access count
		}
		else
		{
			++data.writeAccesses;                   	//for WRITE, increment write access count
		}
		long blockAddress = decAddrs/blockSize;			//block address in integer form
		long index = blockAddress%noOfSets; 			//set index of block referenced by 'blockAddress'
		long tag = blockAddress/noOfSets;   			//tag of block referenced by 'blockAddress'
		if(rpCode == 0)                             	//if RP is RANDOM 
		{
			auto position = LSearch(index,tag);     	//search in list of set at 'index' for 'tag'
			if(position == lcache[index].end())     	//if block with tag = 'tag' is not found
			{
				//stmt to update miss parameters
				updateMisses(readwrite, blockAddress, setSize == (cacheSize/blockSize));
				if(lcache[index].size() == setSize) 	//if set at 'index' is full
				{
					long newPos = rand() % setSize;  	//get random location
					auto itr = lcache[index].begin();
					for(long i = 0; i < newPos; ++i)  	//reach newPos
						++itr;
					if((*itr).dirtyBit == 1)          	//if dirty, then increment counter for dirty blocks evicted
						++data.dirtyBlocksEvicted;
					if(readwrite == 'r')              	//if access type = READ, new block is not dirty, has tag = 'tag' and is valid
						*itr = CacheBlock{tag,1,0};
					else
						*itr = CacheBlock{tag,1,1};   	//here, new block is dirty, has tag = 'tag' and is valid
				}
				else 
				{										//here, set at 'index' is not full, so we add the new block at the end
					if(readwrite == 'r')
						lcache[index].push_back(CacheBlock{tag,1,0});
					else
						lcache[index].push_back(CacheBlock{tag,1,1});
				}
			}
			else 
			{											//here, block with tag = 'tag' has been found(cache hit)
				if(readwrite == 'w') 					//if access type = WRITE, then set dirty bit to 1
				{
					(*position).dirtyBit = 1;
				}
			}
		}
		else if(rpCode == 1)            							//if RP = LRU
		{
			auto position = LSearch(index,tag);						//search in list of set at 'index' for 'tag'
			if(position == lcache[index].end()) 					//if block with tag = 'tag' is not found
			{
				//stmt to update miss parameters
				updateMisses(readwrite, blockAddress, setSize == (cacheSize/blockSize));
				if(lcache[index].size() == setSize) 				//if set at 'index' is full
				{
					if(lcache[index].back().dirtyBit == 1) 			//increment dirty blocks evicted if lru block is dirty
						++data.dirtyBlocksEvicted;
					lcache[index].pop_back();						//delete lru block
				}
				//we push the most recently used block = new block to the front of the list of set at 'index'
				//with its fields appropriately set
				if(readwrite == 'r')
					lcache[index].push_front(CacheBlock{tag,1,0});
				else
					lcache[index].push_front(CacheBlock{tag,1,1});
			}
			else
			{	//here, block with tag = 'tag' has been found(cache hit) at 'position'. So we move it to the front after
				//making it dirty if access type = WRITE
				CacheBlock temp = *position;
				if(readwrite == 'w')
					temp.dirtyBit = 1;
				lcache[index].erase(position);
				lcache[index].push_front(temp);
			}
		}
		else  																	//if RP = PLRU
		{
			long position = VSearch(index,tag);									//search in vector of set at 'index' for 'tag'
			if(position == -1)													//if block with tag = 'tag' is not found
			{
				//stmt to update miss parameters
				updateMisses(readwrite, blockAddress, setSize == (cacheSize/blockSize));
				//find insertion position for the new block in set at 'index'
				long insertionPosition = FindInsertionPositionPLRU(index);
				if(vcache[index][insertionPosition].dirtyBit == 1)				//updating dirty blocks evicted counter if
				{																//block to be evicted is dirty
					++data.dirtyBlocksEvicted;
				}
				//we insert the new block at insertionPosition in the vector of set at 'index'
				//with its fields appropriately set
				if(readwrite == 'r')
					vcache[index][insertionPosition] = CacheBlock{tag,1,0};
				else
					vcache[index][insertionPosition] = CacheBlock{tag,1,1};
			}
			else
			{
				//here, block with tag = 'tag' has been found(cache hit) at 'position'.
				if(readwrite == 'w')
					vcache[index][position].dirtyBit = 1; //set dirty bit to 1 if access type = WRITE
				//update tree bits of set at 'index' after accessing the block at 'position'
				UpdateTreeBitsPLRU(index,position);
			}
		}
	}

	//function to print details of the cache as required by the problem statement
	void PrintDetails()
	{
		cout << "Cache Size: " << cacheSize << endl;
		cout << "Block Size: " << blockSize << endl;
		if(setSize == cacheSize/blockSize)
		{
			cout << "Fully Associative Cache" << endl;
		}
		else if(setSize == 1)
		{
			cout << "Direct-Mapped Cache" << endl;
		}
		else
		{
			cout << setSize << " Way Set-Associative Cache" << endl;			
		}
		if(rpCode == 0)
		{
			cout << "Random Replacement Policy" << endl;	
		}
		else if(rpCode == 1)
		{
			cout << "LRU Replacement Policy" << endl;	
		}
		else
		{
			cout << "Pseudo-LRU Replacement Policy" << endl;	
		}
		data.PrintDetails();
	}
};

int main()
{
	long cacheSize;											//cache size in bytes
	long blockSize;											//block size in bytes
	size_t associativity;									//=0/1/2/4/8/16/32 as in problem statement
	size_t replacementPolicy;								//=0/1/1 as in problem statement
	string inputFile;										//name of file with memory traces
	string address;											//var to store an 32 bit address in 0x representation
	char rw;												//='r' for READ and ='w' for WRITE
	//Stmts to get inputs from stdin
	cout << "Enter the cache size(in bytes):" ;
	cin >> cacheSize;
	cout << "Enter the block size(in bytes):" ;
	cin >> blockSize;
	cout << "Enter the associativity code:" ;
	cin >> associativity;
	cout << "Enter the replacement policy code:" ;
	cin >> replacementPolicy;
	cout << "Enter the input file name:" ;
	cin >> inputFile;
	Cache myCache(cacheSize,blockSize,associativity,replacementPolicy);	//myCache is initialized
	//Stmts to open inputFile and read it line by line and make access to myCache
	ifstream inFilePtr;
	inFilePtr.open(inputFile);
	inFilePtr >> address;
	inFilePtr >> rw;
	while(!inFilePtr.eof())
	{
		myCache.access(address,rw);
		inFilePtr >> address;
		inFilePtr >> rw;
	}
	inFilePtr.close();										//inputFile is closed
	myCache.PrintDetails();									//Required output is sent to stdout
	return 0;
}
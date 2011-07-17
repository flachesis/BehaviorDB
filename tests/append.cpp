#include "bdb.hpp"
#include "error.hpp"
#include <iostream>
#include <cstdio>

void usage()
{}


int main(int argc, char** argv)
{
	using namespace std;
	using namespace BDB;

	Config conf;
	conf.root_dir = "tmp/";
	conf.min_size = 32;

	BehaviorDB bdb(conf);

	char const *data = "yang";
	char const *data2 = "acer";
	error_code ec;

	AddrType addr = bdb.put(data, 4, &ec);
	if(ec){
		cerr<<"put failure"<<endl;	
		return 0;
	}
	printf("%08x\n", addr);

	addr = bdb.put(data2, 4, addr, npos, &ec);

	if(ec){
		cerr<<"append failure"<<endl;	
		return 0;
	}
	
	printf("%08x\n", addr);
	return 0;
}


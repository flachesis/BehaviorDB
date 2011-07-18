#include "bdb.hpp"
#include "error.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdio>

void usage()
{
	printf("put amount\n");	
}

int main(int argc, char** argv)
{
	using namespace std;
	using namespace BDB;
	using namespace boost::system;

	Config conf;
	conf.root_dir = "tmp/";

	conf.min_size = 32;
	BehaviorDB bdb(conf);
	
	char const* data = "acer";
	vector<AddrType> addrs;
	error_code ec;
	for(int i=0;i<10000;++i){
		addrs.push_back(bdb.put(data, 4, &ec));
		if(ec == bdb_errc::idpool_no_space){
			
			cerr<<"err_category: "<<ec.category().name()<<endl;
			cerr<<"err_msg: "<<ec.message()<<endl;
			break;
		}
		//printf("%08x\n", addrs[i]);
	}
	
	for(int i=0;i<10000;++i){
		bdb.del(addrs[i], &ec);
		if(ec){
			cerr<<"del failure"<<endl;
			break;
		}
	}

	return 0;
}

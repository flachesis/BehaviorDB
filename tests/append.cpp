#include "bdb.h"

void create(char const *prefix, char *data, size_t size)
{
	strcpy(data, prefix);
	strcpy(data + size - strlen("data end"), "data end");
}

void verify(char const *data, size_t size)
{
	printf("Head: %s::", data);
	printf("Tail: %s", data+size-strlen("data end"));
}

/** \include append.cpp
 */

int main()
{

	int dist[16] = {0};
	char data[129];
	char data_2k[2049];

	create("128 head", data, 128);
	create("2k head", data_2k, 2048);
	
	Config conf;
	conf.migrate_threshold = 0x8f;

	BehaviorDB bdb(conf);
	AddrType addr;
	SizeType size;

	addr = bdb.put(data, 128);
	size = bdb.get(data, 128, addr);
	printf("put\tAddr: %8x ", addr);
	verify(data, 128);
	printf("\n");
	
	while(addr < 0xf00000ff){
		addr = bdb.append(addr, data_2k, 2048);
		dist[addr>>28]++;
	}

	for(int i=0;i<16;++i)
		printf("Pool[%d]:%8d\n", i, dist[i]);	
	
	return 0;
}

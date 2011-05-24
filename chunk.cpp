#include "chunk.h"

#include <cstdlib>
#include <cstring>
#include <istream>
#include <sstream>
#include <ostream>
#include <iomanip>

//#include <iostream>

using std::hex;

void
read_header(FILE* fp, ChunkHeader &ch)
{
	static char buf[9];
	buf[8] = 0;
	// first byte is preserved
	if(8 != fread(buf, 1, 8, fp)){
		perror("read_header");
		exit(1);
	}
	ch.size = strtoul(&buf[1], 0, 16);
}

void
write_header(FILE* fp, ChunkHeader const& ch)
{
	if(0 >  fprintf(fp, "%08x", ch.size)){
		perror("write_header");
		exit(1);
	}
}

std::istream& 
operator>>(std::istream &is, ChunkHeader &ch)
{
	static char buf[9];
	buf[8] = 0;

	is.read(buf, 8);
	//ch.liveness = *buf;
	ch.size = strtoul(&buf[1], 0, 16);
	/*
	std::cout<<"cheader: "<<
			(unsigned short)ch.liveness<<"-"<<
			ch.size<<"\n";
		*/
	return is;	
}

std::ostream& 
operator<<(std::ostream &os, ChunkHeader const &ch)
{
	
	using std::ios;
	using std::setfill;
	using std::setw;
	
	//os.write((char*)&ch.liveness, 1);

	ios::fmtflags oldflag = os.flags();
	os.unsetf(oldflag);
	os<<setfill('0')<<setw(8)<<hex<<ch.size;
	os.flags(oldflag);
	
	return os;	
}


#ifndef _CHUNK_H_
#define _CHUNK_H_

#include <iosfwd>
#include <istream>
#include "bdb.h"

struct ChunkHeader
{
	unsigned char liveness;
	SizeType size;
	
	ChunkHeader()
	:liveness(0), size(0)
	{}
};

std::istream& 
operator>>(std::istream& is, ChunkHeader & ch);


std::ostream& 
operator<<(std::ostream& os, ChunkHeader const& ch);

void
write_header(FILE* fp, ChunkHeader const &ch);

void
read_header(FILE* fp, ChunkHeader &ch);

#endif


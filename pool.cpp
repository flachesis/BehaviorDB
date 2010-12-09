
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cassert>

#include "bdb.h"
#include "idPool.h"

#include <iostream>

/// Pool - A Chunk Manager
struct Pool
{
	Pool();
	~Pool();

	/** Create chunk file
	 *  @param size Chunk size of this pool.
	 *  @remark Error Number: none.
	 *  @remark Any failure happend in this method causes
	 *  system termination.
	 */
	void 
	create_chunk_file(SizeType size);
	
	/** Get chunk size
	 *  @return Chunk size of this pool.
	 *  @error None.
	 */
	SizeType 
	chunk_size() const;

	/** Put data to some chunk
	 *  @param data Data to be put into this pool.
	 *  @param size Size of the data.
	 *  @return Address for accesing data just put.
	 *  @remark Error Number: SYSTEM_ERROR, ADDRESS_OVERFLOW.
	 */
	AddrType 
	put(char const* data, SizeType size);
	
	/** Append data to a chunk
	 *  @param address Indicate which chunk to be appended.
	 *  @param data Data to be appended.
	 *  @param size Size of the data.
	 *  @param next_pool_idx Next pool index estimated by BehaviorDB.
	 *  @param next_pool Pointer to next pool refered by next_pool_idx.
	 *  @return Address for accessing the chunk that stores concatenated data.
	 *  @remark Error Number: SYSTEM_ERROR, ADDRESS_OVERFLOW, DATA_TOO_BIG.
	 */
	AddrType 
	append(AddrType address, char const* data, SizeType size, 
		AddrType next_pool_idx, Pool* next_pool);
	
	/** Get chunk
	 *  @param output Output buffer for placing retrieved data.
	 *  @param size Size of output buffer
	 *  @param address Indicate which chunk to be retrieved.
	 *  @return Size of the output buffer.
	 *  @remark Error Number: SYSTEM_ERROR, DATA_TOO_BIG.
	 *  @remark In order to enhance security of library. 
	 *  Client has to be responsible for ensuring the size of output buffer
	 *  being large enough.
	 */
	SizeType 
	get(char *output, SizeType const size, AddrType address);
	
	/** Delete chunk
	 *  @param address Address of chunk to be deleted.
	 *  @return Address of chunk just deleted.
	 *  @remark Error Number: None.
	 */
	AddrType
	del(AddrType address);

	/** Error number.
	 *  Not zero when any applicaton or system error happened.
	 */
	int error_num;

protected:
	
	/** Get size of data stored in a chunk
	 *  @param address Indicate which chunk.
	 *  @return Size of data.
	 *  @remark Error Number: SYSTEM_ERROR
	 */
	SizeType
	sizeOf(AddrType address);
	
	/** Move data from one chunk to another pool
	 *  @param src_file File stream of another pool which had seeked to source data.
	 *  @param orig_size Size of original data.
	 *  @param data Data to be appended to the original data.
	 *  @param size Size of appended data.
	 *  @return Address of the chunk that stores concatenated data.
	 *  @remark Error Number: SYSTEM_ERROR, ADDRESS_OVERFLOW
	 */
	AddrType
	migrate(std::fstream &src_file, SizeType orig_size, 
		char const *data, SizeType size); 

	void
	write_log(char const *operation, 
		AddrType const* address,
		AddrType tell,
		SizeType size = 0,
		char const *desc = 0);

	void clear_error();

private:
	Pool(Pool const &cp);
	Pool& operator=(Pool const &cp);
	

	SizeType chunk_size_;
	std::fstream file_;
	IDPool<AddrType> idPool_;
	std::ofstream wrtLog_;
	
};

// ---------------- Misc Functions --------------

inline SizeType 
estimate_max_size(AddrType address)
{
	SizeType s = address >> 28;
	if(s > 15)
		return -1;
	return 1<<s;
}

inline AddrType 
estimate_pool_index(SizeType size)
{
	// Determin which pool to put
	AddrType pIdx(0);
	SizeType bound(size); // +8 for size value
	
	while(bound > (1<<pIdx)<<10)
		++pIdx;

	if(pIdx > 15)
		return -1;

	return pIdx;
}

struct error_num_to_str
{
	char const *operator()(int error_num)
	{
		return buf[error_num];
	}
private:
	static char buf[5][40];
};

char error_num_to_str::buf[5][40] = {
	"No error",
	"Address Overflow",
	"System Error",
	"Data too big",
	"Allocation failure"
};

// ------------- BehaviorDB impl -----------------

error_num_to_str ETOS;

using std::setw;
using std::hex;
using std::setfill;
using std::endl;
using std::ios;

BehaviorDB::BehaviorDB()
: error_num(0), pools_(new Pool[16]), accLog_(new std::ofstream), errLog_(new std::ofstream)
{
	using std::ios;

	for(SizeType i=0;i<16;++i){
		pools_[i].create_chunk_file((1<<i)<<10);	
	}

	// open access log
	accLog_->open("access.log", ios::out | ios::app);
	if(!accLog_->is_open()){
		accLog_->open("access.log", ios::out | ios::trunc);
		if(!accLog_->is_open()){
			fprintf(stderr, "Open access.log failed; system msg - ");
			fprintf(stderr, strerror(errno));
		}
	}
	
	// open error log
	errLog_->open("error.log", ios::out | ios::app);
	if(!errLog_->is_open()){
		errLog_->open("error.log", ios::out | ios::trunc);
		if(!errLog_->is_open()){
			fprintf(stderr, "Open error.log failed; system msg - ");
			fprintf(stderr, strerror(errno));
		}
	}
}

BehaviorDB::~BehaviorDB()
{
	errLog_->close();
	accLog_->close();
	delete accLog_;
	delete [] pools_;
}

void
BehaviorDB::clear_error()
{
	// clear error bits except SYSTEM_ERROR
	error_num &= ~(ADDRESS_OVERFLOW | DATA_TOO_BIG | ALLOC_FAILURE);
}

bool
BehaviorDB::error_return()
{
	if(error_num != 0){
		log_access("error", 0, 0, "System error had not been recovered");
		return true;
	}
	return false;
}


void
BehaviorDB::log_access(char const *operation, AddrType address, SizeType size, char const *desc)
{
	accLog_->unsetf(ios::hex);
	*accLog_<<"["<<setw(6)<<setfill(' ')<<operation<<"]"
		<<"Size(B):"<<setw(8)<<size<<" "
		<<"Address(HEX):"<<setw(8)<<setfill('0')<<hex<<address<<endl;
}

AddrType
BehaviorDB::put(char const* data, SizeType size)
{
	clear_error();
	if(error_return())	return -1;

	AddrType pIdx = estimate_pool_index(size+8);
	
	if(pIdx > 15){ // exceed capacity
		error_num = DATA_TOO_BIG;
		*errLog_<<"[error]"<<ETOS(error_num)<<size<<endl;
		return -1;
	}
	
	AddrType rt = pools_[pIdx].put(data, size);
	
	error_num = pools_[pIdx].error_num;

	if(rt == -1 && 0 != error_num){
		*errLog_<<"[error]"<<ETOS(error_num)<<endl;
		return -1;
	}

	pIdx = pIdx<<28 | rt;

	// write access log
	log_access("put", pIdx, size);
	
	return pIdx;
}


AddrType
BehaviorDB::append(AddrType address, char const* data, SizeType size)
{
	clear_error();
	if(error_return())	return -1;
	
	AddrType pIdx = address >> 28;
	
	// check address roughly
	if(pIdx > 15){
		error_num = ADDRESS_OVERFLOW;
		*errLog_<<"[error]"<<ETOS(error_num)<<": "<<address<<endl;
		return -1;
	}

	AddrType rt, next_pIdx(0);
	
	// Estimate next pool ----------------
	// ! No preservation space for size value since
	// it had been counted as part of existed data
	next_pIdx = estimate_pool_index(size+((1<<pIdx)<<10));

	if(next_pIdx > 15)
		next_pIdx = estimate_pool_index(size);

	if(next_pIdx > 15){
		error_num = DATA_TOO_BIG;
		*errLog_<<"[error]"<<ETOS(error_num)<<": "<<size<<endl;
		return -1;
	}

	rt = pools_[pIdx].append(address, data, size, next_pIdx, &pools_[next_pIdx]);
	
	if(rt == -1 && pools_[pIdx].error_num != 0){
		error_num = pools_[pIdx].error_num;
		*errLog_<<"[error]"<<ETOS(error_num)<<endl;
		return -1;
	}

	// write access log
	log_access("append", rt, size);
	
	return rt;

}

SizeType
BehaviorDB::get(char *output, SizeType const size, AddrType address)
{
	clear_error();
	if(error_return())	return -1;
	
	AddrType pIdx = address >> 28;
	
	// check address roughly
	if(pIdx > 15){
		error_num = ADDRESS_OVERFLOW;
		*errLog_<<"[error]"<<ETOS(error_num)<<": "<<address<<endl;
		return -1;
	}
	

	SizeType rt = pools_[pIdx].get(output, size, address);
	
	if(rt == -1 || pools_[pIdx].error_num){
		error_num = pools_[pIdx].error_num;
		*errLog_<<"[error]"<<ETOS(error_num)<<endl;
		return rt;
	}
	
	// write access log
	log_access("get", address, rt);

	return rt;
}

AddrType
BehaviorDB::del(AddrType address)
{
	clear_error();
	if(error_return())	return -1;
	
	AddrType pIdx = address >> 28;
	
	// check address roughly
	if(pIdx > 15){
		error_num = ADDRESS_OVERFLOW;
		*errLog_<<"[error]"<<ETOS(error_num)<<": "<<address<<endl;
		return -1;
	}

	AddrType rt = pools_[pIdx].del(address);
	
	if(rt == -1 && pools_[pIdx].error_num){
		error_num = pools_[pIdx].error_num;
		*errLog_<<"[error]"<<ETOS(error_num)<<endl;
		return -1;
	}

	// write access log
	log_access("del", address, 0);

	return address;

}


// ------------- Pool implementation ------------

Pool::Pool()
: error_num(0), idPool_(0, 1<<28)
{}

Pool::~Pool()
{
	wrtLog_.close();
	file_.close();
}

void
Pool::create_chunk_file(SizeType chunk_size)
{ 
	using namespace std;

	chunk_size_ = chunk_size;

	if(file_.is_open())
		file_.close();
	
	stringstream cvt;
	
	cvt<<"pools/"
		<<setw(4)<<setfill('0')<<hex
		<< (chunk_size_>>10)
		<< ".pool";
	{
		// create chunk file
		char const* name(cvt.str().c_str());
		file_.open(name, ios_base::in | ios_base::out);
		if(!file_.is_open()){
			file_.open(name, ios_base::in | ios_base::out | ios_base::trunc);
			if(!file_.is_open()){
				fprintf(stderr, "Pools initial: %s\n", strerror(errno));
				exit(1);
			}
		}
	}
	
	cvt<<".log";
	{
		char const *name(cvt.str().c_str());
		// init write log
		wrtLog_.open(name, ios_base::out | ios_base::app);
		if(!wrtLog_.is_open()){
			wrtLog_.open(name, ios_base::out | ios_base::trunc);
			if(!file_.is_open()){
				fprintf(stderr, "Pool logs initial: %s\n", strerror(errno));
				exit(1);
			}
		}
		wrtLog_<<unitbuf;
	}

	cvt.clear();
	cvt.str("");
	cvt<<"transcations/"
		<<setw(4)<<setfill('0')<<hex
		<< (chunk_size_>>10)
		<<".trs";

	idPool_.replay_transcation(cvt.str().c_str());

	idPool_.init_transcation(cvt.str().c_str());

	return;
}

SizeType
Pool::chunk_size() const
{ return chunk_size_; }

SizeType
Pool::sizeOf(AddrType address)
{

	AddrType off = (address & 0x0fffffff);
	char size_val_ar[9] = {0};
	char *size_val(size_val_ar);

	file_.seekg(off * chunk_size_, ios::beg);
	file_.read(size_val, 8);
	
	if(file_.bad() || file_.fail()){
		write_log("sizeOf", &off, file_.tellg(), 0, strerror(errno));
		error_num = SYSTEM_ERROR;
		return -1;
	}

	while('0' == *size_val)
		++size_val;
	
	return (strtoul(size_val, 0, 10));
}

void
Pool::write_log(char const *operation, 
		AddrType const* address,
		AddrType tell,
		SizeType size,
		char const *desc)
{
	wrtLog_<<"["<<setw(10)<<setfill(' ')<<operation<<"]";
	
	if(address)
		wrtLog_<<" address: "<<setw(8)<<setfill('0')<<(*address&0x0fffffff);
	
	wrtLog_<<" tell(B): "<<setw(10)<<tell;

	if(size)
		wrtLog_<<" size(B): "<<setw(8)<<size;
	if(desc)
		wrtLog_<<" "<<desc;
	wrtLog_<<endl;
}

void
Pool::clear_error()
{
	error_num &= ~(ALLOC_FAILURE | DATA_TOO_BIG);	
}


AddrType
Pool::put(char const* data, SizeType size)
{
	// TODO: Partial buffering for big chunk
	
	clear_error();
	if(error_num)
		return -1;

	if(!idPool_.avail()){
		write_log("putErr", 0, file_.tellp(), size, "IDPool overflowed");
		error_num = ADDRESS_OVERFLOW;
		return -1; 
	}
	
	AddrType off = idPool_.Acquire();
	
	// clear() is required when previous read reach the file end
	file_.clear();
	file_.seekp(off * chunk_size_, ios::beg);
	
	
	// write 8 bytes size value ahead
	file_<<setw(8)<<setfill('0')<<(size);
	file_.write(data, (size));
	
	if(!file_.good()){
		idPool_.Release(off);
		write_log("putErr", &off, file_.tellp(), size, strerror(errno));
		error_num = SYSTEM_ERROR;
		return -1;
	}
	
	// write log
	write_log("put", &off, file_.tellp(), size);
	
	return off;
}

AddrType 
Pool::append(AddrType address, char const* data, SizeType size, 
	AddrType next_pool_idx, Pool* next_pool)
{
	using std::stringstream;
	
	clear_error();
	if(error_num)
		return -1;
	
	AddrType off = address & 0x0fffffff;
	SizeType used_size;

	if(!idPool_.isAcquired(off))
		used_size = 0;
	else
		used_size = sizeOf(address);
	
	if(used_size == -1 && SYSTEM_ERROR == error_num ){
		return -1;
	}

	if(used_size + size > chunk_size_){ // need to migration
		if( used_size + size > next_pool->chunk_size() ){ // no pool for migration
			write_log("appErr", &address, file_.tellg(), used_size + size, "Exceed supported chunk size");
			error_num = DATA_TOO_BIG;
			return -1;	
		}

		AddrType rt = next_pool_idx<<28 | next_pool->migrate(file_, used_size, data, size);

		if(-1 == rt && next_pool->error_num != 0){ // migration failed
			error_num = next_pool->error_num;
			return rt;
		}

		idPool_.Release(off);
		return rt;
	}
	
	// update new size
	stringstream cvt;
	cvt<<setw(8)<<setfill('0')<<(used_size + size);
	
	file_.clear();
	file_.seekp(-8, ios::cur);
	file_.write(cvt.str().c_str(), 8);

	// append data
	file_.seekp(used_size, ios::cur);
	file_.write(data, size);

	if(!file_.good()){ // write failed
		write_log("appErr", &address, file_.tellp(), size, strerror(errno));
		error_num = SYSTEM_ERROR;
		return -1;
	}

	// write log
	write_log("append", &address, file_.tellp(), used_size + size);

	return address;		
}

SizeType 
Pool::get(char *output, SizeType const size, AddrType address)
{
	using namespace std;
	
	clear_error();
	if(error_num)
		return -1;
	
	// TODO: check wheather the address has record
	// Following code assume size value > 0
	
	SizeType data_size(sizeOf(address));
	
	if(data_size == -1 && error_num == SYSTEM_ERROR){
		return -1;	
	}
	
	if(data_size > size){
		error_num = DATA_TOO_BIG;
		return data_size;
	}
	
	// TODO: assume sizeOf will seek to the data begin.
	// is that dangerous?
	file_.read(output, data_size);

	if(!file_.gcount()){ // read failure
		write_log("getErr", &address, file_.tellg(), data_size, strerror(errno));
		error_num = SYSTEM_ERROR;
		return -1;
	}
	
	// write log
	write_log("get", &address, file_.tellg(), data_size);
	
	return size;

}

AddrType
Pool::del(AddrType address)
{
	clear_error();
	if(error_num)
		return -1;

	idPool_.Release(address&0x0fffffff);

	// write log
	write_log("del", &address, file_.tellg());

	return address;
}

AddrType
Pool::migrate(std::fstream &src_file, SizeType orig_size, 
	char const *data, SizeType size)
{
	clear_error();
	if(error_num)
		return -1;

	// !! For eliminating seek operations
	// assume the pptr() of src_file is located 
	// in head of original data
	
	SizeType new_size = orig_size + size;

	if(!idPool_.avail()){
		write_log("migErr", 0, file_.tellp(), size, "IDPool overflowed");
		error_num = ADDRESS_OVERFLOW;
		return -1;
	}
	
	AddrType off = idPool_.Acquire();
	
	// clear() is required when previous read reach the file end
	file_.clear();
	file_.seekp(off * chunk_size_, ios::beg);
	
	
	// write 8 bytes size value ahead
	file_<<setw(8)<<setfill('0')<<new_size;
	
	SizeType toRead(orig_size);
	char buf[4096];
	while(toRead){
		if(toRead > 4096)
			src_file.read(buf, 4096);
		else
			src_file.read(buf, toRead);

		if(!src_file.gcount()){ // read failure
			write_log("migErr", &off, src_file.tellg(), orig_size, strerror(errno));
			error_num = SYSTEM_ERROR;
			return -1;
		}
		
		toRead -= src_file.gcount();
		file_.write(buf, src_file.gcount());

		if(!file_){ // write failure
			write_log("migErr", &off, file_.tellp(), orig_size, strerror(errno));
			error_num = SYSTEM_ERROR;
			return -1;
		}
	}

	file_.write(data, size);
	
	if(!file_){ // write failure
		write_log("migErr", &off, file_.tellp(), size, strerror(errno));
		error_num = SYSTEM_ERROR;
		return -1;
	}
	
	// write log
	write_log("migrate", &off, file_.tellp(), new_size);
	
	return off;
}



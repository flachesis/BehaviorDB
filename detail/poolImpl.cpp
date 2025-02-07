#include "error.hpp"
#include "poolImpl.hpp"
#include "idPool.hpp"
#include "v_iovec.hpp"
#include "boost/variant/apply_visitor.hpp"
#include <cassert>
#include <cstdio>
#include <stdexcept>

namespace BDB {
	

	
	pool::pool(pool::config const &conf, addr_eval<AddrType>& addrEval)
	: addrEval(addrEval),
	  dirID(conf.dirID), 
	  work_dir(conf.work_dir), trans_dir(conf.trans_dir), 
	  //addrEval(conf.addrEval), 
	  file_(0), idPool_(0), headerPool_(conf.dirID, conf.header_dir)
	{
		using namespace std;

		// create pool file
		char fname[256];
		if(work_dir.size() > 256) 
			throw length_error("pool: length of pool_dir string is too long");
		

		sprintf(fname, "%s%04x.pool", work_dir.c_str(), dirID);
		if(0 == (file_ = fopen(fname, "r+b"))){
			if(0 == (file_ = fopen(fname, "w+b"))){
				string msg("pool: Unable to create pool file ");
				msg += fname;
				throw invalid_argument(msg.c_str());
			}
		}
        
        mig_buf_ = new char[MIGBUF_SIZ];
        file_buf_ = new char[MIGBUF_SIZ];
        
		if(0 != setvbuf(file_, file_buf_, _IOFBF, MIGBUF_SIZ))
			throw runtime_error("pool: setvbuf to pool file failed");

		// setup idPool
		sprintf(fname, "%s%04x.tran", trans_dir.c_str(), dirID);
		idPool_ = new IDPool(fname, 0);

		
	}
	
	pool::~pool()
	{
		delete idPool_;
        delete file_buf_;
        delete mig_buf_;
		fclose(file_);
	}
	
	pool::operator void const*() const
	{ 
		if(!this || !addrEval.is_init() || !idPool_)
			return 0;
		return this;
	}
	
	AddrType
	pool::write(char const* data, size_t size)
	{
		assert(0 != *this && "pool is not proper initiated");
		

		AddrType loc_addr = idPool_->Acquire();
		ChunkHeader header;
		header.size = size;
		
		if(-1 == seek(loc_addr)){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		// allow data = 0 to act as allocation
		if(0 != data && 
			size != fwrite(data, 1, size, file_) && 
			0 != fflush(file_))
		{
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		if(-1 == headerPool_.write(header, loc_addr)){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		if(-1 == idPool_->Commit(loc_addr)){
			idPool_->Release(loc_addr);
			on_error(COMMIT_FAILURE, __LINE__);
			return -1;
		}
		return loc_addr;

	}
	
	// off == npos represents an append write
	AddrType
	pool::write(char const* data, size_t size, AddrType addr, size_t off, ChunkHeader const* header)
	{
		assert(0 != *this && "pool is not proper initiated");
		
		// TODO allow now!
		if(!idPool_->isAcquired(addr)){
			// error: do not support until bitmap idpool available
			on_error(NON_EXIST, __LINE__);
			return -1;
		}
		

		ChunkHeader loc_header;
		if(header)
			loc_header = *header;
		else if( -1 == headerPool_.read(&loc_header, addr)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		assert(size + loc_header.size <= addrEval.chunk_size_estimation(dirID) && 
			"data exceeds chunk size");
		
		assert((off == npos || off <= loc_header.size) && 
			"invalid offset for put");

		off = (npos == off) ? loc_header.size : off;
		
		// data need to be moved can not larger than move buffer
		size_t moved = loc_header.size - off;
		loc_header.size += size;
		
		if(moved > MIGBUF_SIZ)
			return merge_move(data, size, addr, off, this, &loc_header); 

		if(-1 == seek(addr, off)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		// read data to be moved into mig_buf
		if(moved && moved != fread(mig_buf_, 1, moved, file_)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;

		}
		
		if(moved && -1 == seek(addr, off)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		size_t partial(0);
		// write new data
		if(size != (partial = fwrite(data, 1, size, file_))){
			if(!partial){ // no data written
				// abort directly	
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}
			// rollback to previous state
			clearerr(file_);
			if(-1 == seek(addr, off)){
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}			
			if( partial != fwrite(mig_buf_, 1, partial, file_)){
				// rollback failed, leave broken data alone
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}
		}
		
		// write buffered data
		if(moved && moved != (partial = fwrite(mig_buf_, 1, moved, file_))){
			// rollback to previous state
			clearerr(file_);
			if(-1 == seek(addr, off)){
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}			
			if( partial != fwrite(mig_buf_, 1, moved, file_)){
				// rollback failed, leave broken data alone
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}
			
		}
		
		if(0 != fflush(file_)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		// update header 
		if(-1 == headerPool_.write(loc_header, addr)){
			if(-1 == seek(addr, off)){
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}			
			if( partial != fwrite(mig_buf_, 1, moved, file_)){
				// rollback failed, leave broken data alone
				on_error(ROLLBACK_FAILURE, __LINE__);
				return -1;
			}
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}


		return addr;
	}
	
	AddrType
	pool::write(viov* vv, size_t len)
	{
		assert(0 != *this && "pool is not proper initiated");

		size_t total(0);
		for(size_t i=0; i<len; ++i){
			total += vv[i].size;		
		}
		
		assert(total <= addrEval.chunk_size_estimation(dirID) && 
			"data exceeds chunk size");

		ChunkHeader header;
		header.size = total;
		

		AddrType loc_addr = idPool_->Acquire();
		
		if(-1 == headerPool_.write(header, loc_addr)){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		if( -1 == seek(loc_addr) ){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		write_viov wv;
		wv.dest = file_;
		wv.dest_pos = addr_off2tell(loc_addr, 0);
		wv.buf = mig_buf_;
		wv.bsize = MIGBUF_SIZ;
		off_t loopOff(0);
		for(size_t i=0; i<len; ++i){
			wv.size = vv[i].size;
			if( vv[i].size && 
			    0 == boost::apply_visitor(wv, vv[i].data)){
				// error handle
				idPool_->Release(loc_addr);
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}
			wv.dest_pos += vv[i].size;
		}
		
		if(0 != fflush(file_)){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		if( -1 == idPool_->Commit(loc_addr)){
			idPool_->Release(loc_addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		return loc_addr;
	}
	
	AddrType
	pool::replace(char const *data, size_t size, AddrType addr, ChunkHeader const *header)
	{
		assert(0 != *this && "pool is not proper initiated");

		if(!idPool_->isAcquired(addr)){
			on_error(NON_EXIST, __LINE__);
			return -1;
		}
		
		ChunkHeader loc_header, new_header;
		if(header)
			loc_header = *header;
		else if(-1 == headerPool_.read(&loc_header, addr)) {
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		assert(size <= addrEval.chunk_size_estimation(dirID));
		new_header.size = size;
		
		if(-1 == seek(addr, 0)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		if(size != fwrite(data, 1, size, file_) &&
			0 != fflush(file_)){
			idPool_->Release(addr);
			idPool_->Commit(addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		// update header 
		if(-1 == headerPool_.write(new_header, addr)){
			idPool_->Release(addr);
			idPool_->Commit(addr);
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		return addr;
	}


	size_t
	pool::read(char* buffer, size_t size, AddrType addr, size_t off, ChunkHeader const* header)
	{
		assert(0 != *this && "pool is not proper initiated");
		if(!idPool_->isAcquired(addr)){
			on_error(NON_EXIST, __LINE__);
			return -1;
		}

		ChunkHeader loc_header;
		if(header)
			loc_header = *header;
		else if(-1 == headerPool_.read(&loc_header, addr)) {
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		if(off > loc_header.size)
			return 0;

		if(-1 == seek(addr, off)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		
		size_t toRead = (size > loc_header.size - off) ? 
			loc_header.size - off 
			: size;

		if(toRead != fread(buffer, 1, toRead, file_)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		return toRead;

	}
	
	size_t
	pool::read(std::string *buffer, size_t max, AddrType addr, size_t off, ChunkHeader const* header)
	{
		assert(0 != *this && "pool is not proper initiated");
		if(!buffer) return 0;
		if(buffer->size()) buffer->clear();
		size_t readCnt(0), total(0);
		while(0 < (readCnt = read(mig_buf_, MIGBUF_SIZ, addr, off))){
			if(-1 == readCnt) return -1;
			if(total + readCnt > max) break;
			buffer->append(mig_buf_, readCnt);
			off += readCnt;
			total += readCnt;
		}
		return total;
	}
	
	AddrType
	pool::merge_copy(char const* data, size_t size, AddrType src_addr, size_t off, 
		pool *dest_pool, ChunkHeader const* header)
	{
		assert(0 != *this && "pool is not proper initiated");
		assert(0 != *dest_pool && "dest pool is not proper initiated");

		ChunkHeader loc_header;
		if(header)
			loc_header = *header;
		else if( -1 == headerPool_.read(&loc_header, src_addr) ){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
	
		viov vv[3];
		file_src fs;
		no_data_src nds;
		AddrType loc_addr;
		if(0 == off){ // prepend
			if(0 == data)
				vv[0].data = nds;
			else	
				vv[0].data = data;
			vv[0].size = size;
			fs.fp = file_;
			fs.off = addr_off2tell(src_addr, 0);
			vv[1].data = fs;
			vv[1].size = loc_header.size;
			loc_addr = dest_pool->write(vv, 2);
		}else if(loc_header.size == off){ // append
			fs.fp = file_;
			fs.off = addr_off2tell(src_addr, 0);
			vv[0].data = fs;
			vv[0].size = loc_header.size;
			if(0 == data)
				vv[1].data = nds;
			else
				vv[1].data = data;
			vv[1].size = size;
			loc_addr = dest_pool->write(vv, 2);
		}else{ // insert
			fs.fp = file_;
			fs.off = addr_off2tell(src_addr, 0);
			vv[0].data = fs;
			vv[0].size = off;
			if(0 == data)
				vv[1].data = nds;
			else
				vv[1].data = data;
			vv[1].size = size;
			fs.off += off;
			vv[2].data = fs;
			vv[2].size = loc_header.size - off;
			loc_addr = dest_pool->write(vv, 3);
		}

		if(-1 == loc_addr){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		return loc_addr;
	}

	AddrType
	pool::merge_move(char const*data, size_t size, AddrType src_addr, size_t off, 
		pool *dest_pool, ChunkHeader const* header)
	{
		assert(0 != *this && "pool is not proper initiated");
		assert(0 != *dest_pool && "dest pool is not proper initiated");

		AddrType loc_addr = 
			merge_copy(data, size, src_addr, off, dest_pool, header);

		idPool_->Release(src_addr);
		idPool_->Commit(src_addr);

		return loc_addr;
	}

	size_t
	pool::free(AddrType addr)
	{ 
		assert(0 != *this && "pool is not proper initiated");

		if(idPool_->isAcquired(addr)){
			idPool_->Release(addr);
			idPool_->Commit(addr);
		} else {
			on_error(NON_EXIST, __LINE__);
			return -1;
		}
		return 0;
	}

	size_t
	pool::erase(AddrType addr, size_t off, size_t size)
	{ 
		assert(0 != *this && "pool is not proper initiated");

		if(!idPool_->isAcquired(addr)){
			on_error(NON_EXIST, __LINE__);
			return -1;
		}

		ChunkHeader header;
		headerPool_.read(&header, addr);
		
		// TODO exception(error) ?
		if(off > header.size) return header.size;

		// overflow check
		size = (off + size > off) ?  
			(off + size > header.size) ? header.size - off : size
			: header.size - off;

		size_t toRead = header.size - size - off;
		
		header.size -= size;

		headerPool_.write(header, addr);
		
		size_t readCnt, loopOff(0);
		
		while(toRead > 0){
			readCnt = (toRead > MIGBUF_SIZ) ? MIGBUF_SIZ : toRead;
			if(-1 == seek(addr, off + size + loopOff)){
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}

			if(readCnt != fread(mig_buf_, 1, readCnt, file_)){
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}
			if(-1 == seek(addr, off + loopOff)){
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}
			if(readCnt != fwrite(mig_buf_, 1, readCnt, file_) &&
				0 != fflush(file_))
			{
				on_error(SYSTEM_ERROR, __LINE__);
				return -1;
			}
			loopOff += readCnt;
			toRead -= readCnt;
		}
		
		return header.size;
	}
	
	size_t
	pool::overwrite(char const* data, size_t size, AddrType addr, size_t off)
	{
		assert(true == idPool_->isAcquired(addr) && 
			"overwrite to invalid address");

		assert(off+size < addrEval.chunk_size_estimation(dirID)
			&& "exceed chunk size");

		if(-1 == seek(addr, off)){
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		if(size != fwrite(data, 1, size, file_) &&
			fflush(file_) )
		{
			on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}

		return size;
	}

	int
	pool::head(ChunkHeader *header, AddrType addr) const
	{ 
		assert(0 != *this && "pool is not proper initiated");

		if(-1 == headerPool_.read(header, addr)){
			const_cast<pool*>(this)->on_error(SYSTEM_ERROR, __LINE__);
			return -1;
		}
		return 0;
	}

	off_t
	pool::seek(AddrType addr, size_t off)
	{
		assert(0 != *this && "pool is not proper initiated");

		off_t pos = addr;
		pos *= addrEval.chunk_size_estimation(dirID);
		pos += off;
		if(-1 == fseeko(file_, pos, SEEK_SET)){
			return -1;
		}
		return pos;
	}


	off_t
	pool::addr_off2tell(AddrType addr, size_t off) const
	{
		assert(0 != *this && "pool is not proper initiated");

		off_t pos = addr;
		pos *= addrEval.chunk_size_estimation(dirID);
		pos += off;
		return pos;
	}
	
	void
	pool::on_error(int errcode, int line)
	{
		assert(0 != *this && "pool is not proper initiated");

		// TODO: lock for mutli proc/thread
		err_.push_back(std::make_pair<int, int>(errcode, line));	
		// unlock
	}
	
	std::pair<int, int>
	pool::get_error()
	{
		assert(0 != *this && "pool is not proper initiated");

		std::pair<int, int> rt(0,0);
		if(!err_.empty()){
			rt = err_.front();
			err_.pop_front();
		}
		return rt;
		
	}
	
	void
	pool::pine(AddrType addr)
	{ idPool_->Lock(addr); }

	void
	pool::unpine(AddrType addr)
	{ idPool_->Unlock(addr); }

	bool
	pool::is_pinned(AddrType addr)
	{ return idPool_->isLocked(addr); }

} // end of BDB namespace

#ifndef _BDBIMPL_HPP
#define _BDBIMPL_HPP

#include <cstdio>
#include <string>
#include "common.hpp"
#include "addr_eval.hpp"

namespace boost {
	namespace system {
		class error_code;
	}	
} // end of namespace boost


namespace BDB {
	
	template<typename B, typename V>
	class IDValPool;
	
	using boost::system::error_code;

	struct pool;
	struct AddrIterator;	
	
	struct BDBImpl 
	{
		friend struct bdbStater;
		friend struct AddrIterator;

	        BDBImpl(Config const & conf);
		~BDBImpl();
		
		operator void const*() const;
		
		void
		init_(Config const & conf);

		/** @brief Put data
		 *  @param data
		 *  @param size
		 *  @return Global address
		 */
		AddrType
		put(char const *data, size_t size, error_code *ec=0);

		AddrType
		put(char const *data, size_t size, AddrType addr, size_t off=npos);
		
		AddrType
		put(std::string const& data)
		{ return put(data.data(), data.size()); }
		
		AddrType
		put(std::string const& data, AddrType addr, size_t off=npos)
		{ return put(data.data(), data.size(), addr, off); }
		
		AddrType
		update(char const *data, size_t size, AddrType addr);
		
		AddrType
		update(std::string const& data, AddrType addr)
		{ return update(data.data(), data.size(), addr); }

		size_t
		get(char *output, size_t size, AddrType addr, size_t off=0);
		
		size_t
		get(std::string *output, size_t max, AddrType addr, size_t off=0);

		size_t
		del(AddrType addr);

		size_t
		del(AddrType addr, size_t off, size_t size);

		AddrIterator
		begin() const;

		AddrIterator
		end() const;
		
		void stat(Stat* s) const;

	private: // disable interfaces
		BDBImpl(BDBImpl const& cp);
		BDBImpl& operator=(BDBImpl const &cp);

	protected:
		// handle error triggered in pool(s)
		void
		error(unsigned int dir);

		// handle error triggered in BDBImpl
		void
		error(int errcode, int line);
	
	private:
		typedef addr_eval<AddrType> addrEval;

		pool* pools_;
		FILE* log_;
		char log_buf_[256];

		IDValPool<AddrType, AddrType> *global_id_;

	};

} // end of namespace BDB

#endif // end of header

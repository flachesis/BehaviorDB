#ifndef BDB_ERROR_HPP
#define BDB_ERROR_HPP

#include <string>
#include "boost/system/error_code.hpp"

namespace BDB  {

	using boost::system::error_code;

	//! Error numbers
	enum ERRORNUMBER
	{
		/// Pool can not address more chunks
		ADDRESS_OVERFLOW = 1,

		/// I/O operation failure
		SYSTEM_ERROR,

		/// Data is too big to be handled by BehaviorDB 
		DATA_TOO_BIG ,

		/// Pool is locked
		POOL_LOCKED,
		
		NON_EXIST
	};

	struct error_num_to_str
	{
		char const *operator()(int error_num)
		{
			return buf[error_num];
		}
	private:
		static char buf[6][40];
	};
	
	class idpool_error_category 
	: public boost::system::error_category
	{
	public:
		char const* name() const;
		std::string message(int ev) const;
	};
} // end of namespace BDB

#endif // end of header

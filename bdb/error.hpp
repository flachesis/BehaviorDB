#ifndef BDB_ERROR_HPP
#define BDB_ERROR_HPP

#include <string>
#include "boost/system/error_code.hpp"

namespace BDB  {

	using boost::system::error_code;
	using boost::system::error_category;
	using boost::system::error_condition;

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
	

	namespace bdb_errc {
		enum {
			idpool_no_space = 1,
			idpool_disk_failure,
			idpool_alloc_failure
		};
	} // end of namespace BDB_ERROR
	
	class bdb_category_impl
	: public error_category
	{
	public:
		virtual char const* 
		name() const;

		virtual std::string 
		message(int ev) const;

		virtual bool 
		equivalent(error_code const & ec, int cond) const; 
	};

	error_category const& 
	bdb_error_category();

	error_code 
	make_error_code(int e);

	error_condition
	make_error_condition(int e);


} // end of namespace BDB

#endif // end of header

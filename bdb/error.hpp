#ifndef BDB_ERROR_HPP
#define BDB_ERROR_HPP

#include <string>
#include "boost/system/error_code.hpp"

namespace BDB  {

	using boost::system::error_code;
	using boost::system::error_category;
	using boost::system::error_condition;

	namespace bdb_errc {
		enum errc_t{
			idpool_no_space = 1,
			idpool_disk_failure,
			idpool_alloc_failure,
			idpool_full
		};
	} // end of namespace BDB_ERROR
	
	error_category const& 
	bdb_error_category();

} // end of namespace BDB


namespace boost {
	namespace system {
		template<> struct is_error_condition_enum<BDB::bdb_errc::errc_t>
		{ enum{value = true}; };
		
		error_code 
		make_error_code(BDB::bdb_errc::errc_t e);

		error_condition
		make_error_condition(BDB::bdb_errc::errc_t e);

	}
}

#endif // end of header

#ifndef BDB_ERROR_HPP
#define BDB_ERROR_HPP

#include <string>
#include "boost/system/error_code.hpp"

namespace BDB  {
	// TODO Deprecate this
	//! Error numbers
	enum ERRORNUMBER
	{
		/// Pool can not address more chunks
		ADDRESS_OVERFLOW = 1,

		/// I/O operation failure
		SYSTEM_ERROR = 2,

		/// Data is too big to be handled by BehaviorDB 
		DATA_TOO_BIG = 3,

		/// Pool is locked
		POOL_LOCKED = 4,
		
		NON_EXIST = 5
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

	using boost::system::error_code;
	using boost::system::error_category;
	using boost::system::error_condition;

	namespace bdb_errc {
		namespace basic {
			enum errc_t {
				success = boost::system::errc::success,
				disk_full = boost::system::errc::no_space_on_device,
				again = boost::system::errc::resource_unavailable_try_again,
				bad_alloc = boost::system::errc::not_enough_memory,
				bad_address = boost::system::errc::bad_address,
				unrecoverable = boost::system::errc::state_not_recoverable,
				disk_failure,
				not_found,
			};
		} // end of namespace basic 
		
		namespace id_pool {
			enum errc_t {
				disk_full = basic::disk_full,
				disk_failure = basic::disk_failure,
				bitmap_full = basic::again,
				bitmap_resize_failure = basic::bad_alloc
			};
		} // end of namespace idpool
		
		namespace header_pool {
			enum errc_t {
				disk_failure = basic::disk_failure
			};
		} // end of namespace header_pool
		
		namespace pool {
			enum errc_t {
				disk_full = basic::disk_full,
				disk_failure = basic::disk_failure,
				not_found = basic::not_found,
				bad_address = basic::bad_address,
				data_broken = basic::unrecoverable
			};
		} // end of namespace pool

		namespace bdb {
			enum errc_t {
				global_id_full = id_pool::bitmap_full,
				not_found = basic::not_found,
			};	
		}

		enum errc_t {
			idpool_no_space = 1,
			idpool_disk_failure,
			idpool_alloc_failure,
			idpool_full
		};


	} // end of namespace BDB_ERROR
	
	error_category const& 
	basic_category();

	error_category const& 
	id_pool_category();

	error_category const& 
	header_pool_category();

	error_category const& 
	bdb_category();

	error_category const& 
	bdb_error_category();
	
	


} // end of namespace BDB


namespace boost {
namespace system {
		
	template<> struct is_error_condition_enum<BDB::bdb_errc::basic::errc_t>
	{ static const bool value = true; };

	template<> struct is_error_condition_enum<BDB::bdb_errc::id_pool::errc_t>
	{ static const bool value = true; };

	template<> struct is_error_condition_enum<BDB::bdb_errc::header_pool::errc_t>
	{ static const bool value = true; };
	
	template<> struct is_error_condition_enum<BDB::bdb_errc::pool::errc_t>
	{ static const bool value = true; };

	template<> struct is_error_condition_enum<BDB::bdb_errc::bdb::errc_t>
	{ static const bool value = true; };

	error_code 
	make_error_code(BDB::bdb_errc::basic::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::basic::errc_t e);


	error_code 
	make_error_code(BDB::bdb_errc::id_pool::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::id_pool::errc_t e);


	error_code 
	make_error_code(BDB::bdb_errc::header_pool::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::header_pool::errc_t e);

	error_code 
	make_error_code(BDB::bdb_errc::pool::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::pool::errc_t e);

	error_code 
	make_error_code(BDB::bdb_errc::bdb::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::bdb::errc_t e);

	// old

	template<> struct is_error_condition_enum<BDB::bdb_errc::errc_t>
	{ static const bool value = true; };

	error_code 
	make_error_code(BDB::bdb_errc::errc_t e);
	
	error_condition
	make_error_condition(BDB::bdb_errc::errc_t e);
		
} // namespace system 
} // namespace boost

#endif // end of header

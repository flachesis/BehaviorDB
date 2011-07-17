#include "error.hpp"

namespace BDB {


	char error_num_to_str::buf[6][40] = {
		"No error",
		"Address overflow",
		"System error",
		"Data too big",
		"Pool locked",
		"Non exist address"
	};
	
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
	
	char const *
	bdb_category_impl::name() const
	{ return "BehaviorDB"; }

	std::string 
	bdb_category_impl::message(int ev) const
	{

		switch(ev){
		case bdb_errc::idpool_no_space:
			return "(idpool) no space on device";
		case bdb_errc::idpool_alloc_failure: 
			return "(idpool) bitmap resize failure";
		case bdb_errc::idpool_disk_failure:
			return "(idpool) write transaction failure";
		case bdb_errc::idpool_full:
			return "(idpool) bitmap full";
		}
		return "unknow";
	}
	
	bool
	bdb_category_impl::equivalent(error_code const& ec, int cond) const
	{
		using namespace boost::system;

		switch(cond){
		case bdb_errc::idpool_no_space:
			return ec == errc::no_space_on_device;
		case bdb_errc::idpool_alloc_failure:
			return ec == errc::not_enough_memory;
		case bdb_errc::idpool_disk_failure:
			return ec == errc::io_error ||
				ec == errc::bad_address ||
				ec == errc::file_too_large;
		}
		return false;
	}
	
	error_category const& 
	bdb_error_category()
	{ 
		static bdb_category_impl inst;
		return inst;
	}

	


} // end of namespace BDB

namespace boost {
	
	namespace system { 
		error_code
		make_error_code(BDB::bdb_errc::errc_t e)
		{ return error_code((int)e, BDB::bdb_error_category()); }


		error_condition
		make_error_condition(BDB::bdb_errc::errc_t e)
		{ return error_condition((int)e, BDB::bdb_error_category()); }
	
	} // end of namespace system

} // end of namespace boost

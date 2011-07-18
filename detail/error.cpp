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
	
	class basic_category_impl
	: public error_category
	{
	public:
		virtual char const* 
		name() const
		{ return "BDB::basic"; }

		virtual std::string 
		message(int ev) const
		{ 
			using namespace bdb_errc;

			switch(ev){
			case basic::disk_full:
				return "disk is full";
			case basic::again:
				return "release entries and try again";
			case basic::bad_alloc:
				return "allocation failure";
			case basic::bad_address:
				return "invalid address/size passed by clients";
			case basic::unrecoverable:
				return "unrecoverable state";
			case basic::disk_failure:
				return "disk failure";
			case basic::not_found:
				return "not found";
			}
			return "unknow";
		}

		virtual bool 
		equivalent(error_code const & ec, int cond) const
		{
			using namespace boost::system;

			switch(cond){
			case bdb_errc::basic::disk_full:
				return ec == errc::no_space_on_device;
			case bdb_errc::basic::again:
				return ec == errc::resource_unavailable_try_again;
			case bdb_errc::basic::disk_failure:
				return ec == errc::io_error ||
					ec == errc::file_too_large;
			}
			return false;

		}

	};
	
	class id_pool_category_impl
	: public basic_category_impl
	{
		virtual char const* 
		name() const
		{ return "BDB::id_pool"; }
	};

	
	class header_pool_category_impl
	: public basic_category_impl
	{
		virtual char const* 
		name() const
		{ return "BDB::header_pool"; }
	};
	
	class pool_category_impl
	: public basic_category_impl
	{
		virtual char const* 
		name() const
		{ return "BDB::pool"; }
	};
	
	class bdb_category_impl
	: public basic_category_impl
	{
		virtual char const* 
		name() const
		{ return "BDB::bdb"; }
	};
	
	error_category const& 
	basic_category()
	{ 
		static basic_category_impl inst;
		return inst;
	}

	error_category const& 
	id_pool_category()
	{ 
		static id_pool_category_impl inst;
		return inst;
	}

	error_category const& 
	header_pool_category()
	{ 
		static header_pool_category_impl inst;
		return inst;
	}

	error_category const& 
	pool_category()
	{ 
		static pool_category_impl inst;
		return inst;
	}
	
	error_category const& 
	bdb_category()
	{ 
		static bdb_category_impl inst;
		return inst;
	}

	// old

	class _bdb_category_impl
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
	_bdb_category_impl::name() const
	{ return "BehaviorDB"; }

	std::string 
	_bdb_category_impl::message(int ev) const
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
	_bdb_category_impl::equivalent(error_code const& ec, int cond) const
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
		static _bdb_category_impl inst;
		return inst;
	}

	


} // end of namespace BDB

namespace boost {
namespace system { 
	
	error_code
	make_error_code(BDB::bdb_errc::basic::errc_t e)
	{ return error_code((int)e, BDB::basic_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::basic::errc_t e)
	{ return error_condition((int)e, BDB::basic_category()); }

	error_code
	make_error_code(BDB::bdb_errc::id_pool::errc_t e)
	{ return error_code((int)e, BDB::id_pool_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::id_pool::errc_t e)
	{ return error_condition((int)e, BDB::id_pool_category()); }

	error_code
	make_error_code(BDB::bdb_errc::header_pool::errc_t e)
	{ return error_code((int)e, BDB::header_pool_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::header_pool::errc_t e)
	{ return error_condition((int)e, BDB::header_pool_category()); }
	error_code
	make_error_code(BDB::bdb_errc::pool::errc_t e)
	{ return error_code((int)e, BDB::pool_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::pool::errc_t e)
	{ return error_condition((int)e, BDB::pool_category()); }
	error_code
	make_error_code(BDB::bdb_errc::bdb::errc_t e)
	{ return error_code((int)e, BDB::bdb_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::bdb::errc_t e)
	{ return error_condition((int)e, BDB::bdb_category()); }
	
	// old

	error_code
	make_error_code(BDB::bdb_errc::errc_t e)
	{ return error_code((int)e, BDB::bdb_error_category()); }

	error_condition
	make_error_condition(BDB::bdb_errc::errc_t e)
	{ return error_condition((int)e, BDB::bdb_error_category()); }

} // end of namespace system
} // end of namespace boost


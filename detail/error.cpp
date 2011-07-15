#include "error.hpp"

namespace BDB {

	using namespace boost::system;

	char error_num_to_str::buf[6][40] = {
		"No error",
		"Address overflow",
		"System error",
		"Data too big",
		"Pool locked",
		"Non exist address"
	};

	char const *
	idpool_error_category::name() const
	{ return "IDPool"; }

	std::string 
	idpool_error_category::message(int ev) const
	{
		switch(ev){
		case errc::not_enough_memory: 
			return "bitmap resize failure";
		case errc::no_space_on_device:
			return "write transaction failure";
		}
		return "unknow";
	}

} // end of namespace BDB

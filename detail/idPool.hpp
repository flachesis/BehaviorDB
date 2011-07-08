#ifndef _IDPOOL_HPP
#define _IDPOOL_HPP

#include <stdexcept>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cassert>
#include <sstream>
#include "boost/dynamic_bitset.hpp"
#include "common.hpp"

/// @todo TODO: Transaction file compression (snapshot).

/** @brief Integer ID manager within bitmap storage.
 */
template<typename BlockType = unsigned int>
class IDPool
{
	friend struct bdbStater;

protected:
	typedef boost::dynamic_bitset<> Bitmap;
public:

	/** Default constructor
	 * @desc Construct a IDPool that manages numerical ID.
	 * @post An IDPool without associated transaction file.
	 * Clients can invoke init_transaction menually. Legal
	 * ID range of this IDPool is
	 * (0,  numeric_limits<BlockType>::max() - 1]. 
	 * @throw std::bad_alloc
	 */
	IDPool();
	
	/** Constructor for being given begin 
	 * @desc Construct a IDPool that manages numerical ID.
	 * @param trans_file name of a transaction file
	 * @param beg user-defined ID begin number 
	 * @pre beg < numric_limits<BlockType>::max()
	 * @post A IDPool that its storage is a partially allocated 
	 * dynamic bitmap. Legal ID range of this IDPool is
	 * (beg,  numeric_limits<BlockType>::max() - 1]. 
	 * @throw std::bad_alloc
	 */
	IDPool(char const* trans_file, BlockType beg);

	/** Constructor for being given begin and end
	 * @desc Construct a IDPool that manages numerical ID.
	 * @param trans_file name of a transaction file
	 * @param beg user-defined ID begin number 
	 * @param end user-defined ID end number 
	 * @pre beg <= end
	 * @post A IDPool that its storage is a partially allocated 
	 * dynamic bitmap. Legal ID range of this IDPool is
	 * (beg, end]. 
	 * @throw std::bad_alloc
	 */	
	IDPool(char const* trans_file, BlockType beg, BlockType end);
	
	~IDPool();

	operator void const*() const;
	
	/** Test if an ID already exists in an IDPool
	 *  @param id
	 */
	bool 
	isAcquired(BlockType const& id) const;

	/** Acquire an ID from a IDPool.
	 * @throw A write transaction error of type 
	 * std::runtime_error.
	 */
	BlockType 
	Acquire();
	
	/** Release an ID
	 *  @throw A write transaction error of type
	 *  std::runtime_error
	 */
	int 
	Release(BlockType const &id);

	bool 
	avail() const;
	
	/** Find the first acquired ID from curID which is included
	 * @param curID Current ID
	 * @remark The curID will be tested also.
	 */
	BlockType 
	next_used(BlockType curID) const;

	size_t 
	size() const;
	
	void 
	replay_transaction(char const* transaction_file);
	
	void 
	init_transaction(char const* transaction_file);
	
	BlockType begin() const
	{ return beg_; }

	BlockType end() const
	{ return end_; }
	
	size_t
	num_blocks() const;

protected:
	
	void extend();
	IDPool(BlockType beg, BlockType end);

private:
	IDPool(IDPool const &cp);
	IDPool& operator=(IDPool const &cp);
protected:

	BlockType const beg_, end_;
	FILE*  file_;
	Bitmap bm_;
	bool full_alloc_;

};

/** @brief Extend IDPool<B> for associating a value with a ID.
 */
template<typename BlockType, typename ValueType>
class IDValPool : public IDPool<BlockType>
{
	friend struct bdbStater;
	typedef IDPool<BlockType> super;
public:
	IDValPool(char const* trans_file, BlockType beg, BlockType end);
	~IDValPool();
	
	/** Acquire an ID and associate a value with the ID
	 * @param val
	 * @return ID
	 */
	BlockType Acquire(ValueType const &val);
	
	/** Find value by ID
	 * @param id
	 * @pre id exists in an IDValPool (Test it by isAcquire())
	 * @return Associated value
	 */
	ValueType Find(BlockType const &id) const;
	
	/** Update value by ID
	 * @param id
	 * @param val
	 * @pre id exists in an IDValPool (Test it by isAcquire())
	 */
	void Update(BlockType const& id, ValueType const &val);

	void replay_transaction(char const* transaction_file);

	// size_t block_size() const;
private:
	ValueType* arr_;
};

#include "idPool.tcc"

#endif

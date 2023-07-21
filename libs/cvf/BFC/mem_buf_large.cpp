
#include"BFC/mem.h"
#include<stdlib.h>
#include<memory.h>
#include<assert.h>

_FF_BEG

static uint g_mem_buffer_unit_size=0;

void set_alloc_buffer_unit(uint unit_size)
{
	g_mem_buffer_unit_size= unit_size;
}

struct _mem_item
{
	enum{_MAGIC=0xFE27C98A}; //must be even

	int		m_magic;
	uint	m_size;
public:
	void* data_ptr() const
	{
		return ((char*)this)+sizeof(_mem_item);
	}
	static _mem_item* alloc(uint size, int is_buffered)
	{
		uint dsize=size+sizeof(_mem_item);
		_mem_item *m=(_mem_item*)malloc(dsize);
		m->m_magic=_MAGIC|is_buffered;
		m->m_size=size;
		return m;
	}
	static _mem_item* from_data_ptr(void *data_ptr)
	{
		return (_mem_item*)(((char*)data_ptr)-sizeof(_mem_item));
	}
	static void release(_mem_item *ptr)
	{
		free( ptr );
	}
};

struct _block
{
	//uint	   m_size;
	_mem_item *m_mem;
};

struct _block_array
{
	enum{SIZE=16};

	_block m_data[SIZE];
	int    m_count;
	_block_array *m_next;
public:
	static _block_array* create()
	{
		_block_array *obj=(_block_array*)malloc(sizeof(_block_array));
		memset(obj,0,sizeof(_block_array));
		return obj;
	}
	_mem_item* remove()
	{
		_block_array *blk=this;
		while(blk->m_next && blk->m_next->m_count>0)
			blk=blk->m_next;

		if(blk->m_count>0)
		{
			--blk->m_count;
			return blk->m_data[blk->m_count].m_mem;
		}
		return NULL;
	}
	void add(_mem_item *mi)
	{
		_block_array *blk=this;
		while(blk->m_next && blk->m_count==SIZE)
			blk=blk->m_next;

		if(blk->m_count==SIZE)
		{
			blk->m_next=_block_array::create();
			blk=blk->m_next;
		}

		blk->m_data[ blk->m_count ].m_mem=mi;
		++blk->m_count;
	}
};

typedef _block_array* _block_array_ptr;

enum{_MBUF_SIZE=256};
_block_array_ptr *g_mbuf=NULL;

void* ff_alloc(size_t size)
{
	uint index=0;
	if(g_mem_buffer_unit_size<=0 || size%g_mem_buffer_unit_size!=0 
		||(index=size/g_mem_buffer_unit_size)>=_MBUF_SIZE
		)
		return _mem_item::alloc(size, 0x0)->data_ptr();

	if(!g_mbuf)
	{
		const int msize=sizeof(_block_array_ptr)*_MBUF_SIZE;
		g_mbuf=(_block_array_ptr*)malloc(msize);
		memset(g_mbuf,0, msize);
	}

	_block_array_ptr vlist=g_mbuf[index];
	_mem_item *mi=vlist? vlist->remove() : NULL;
	
	if(!mi)
	{
		mi=_mem_item::alloc(size, 0x01);
		//vlist->add(mi);
	}

	return mi->data_ptr();
}

void ff_free(void *ptr)
{
	if(ptr)
	{
		_mem_item *mi=_mem_item::from_data_ptr(ptr);
		assert( (mi->m_magic&~1)==_mem_item::_MAGIC);
		bool _released=false;
		if(mi->m_magic&1)
		{
			uint index=(mi->m_size/g_mem_buffer_unit_size);
			if(index<_MBUF_SIZE)
			{
				_block_array_ptr vlist=g_mbuf[index];
				if(!vlist)
				{
					vlist=g_mbuf[index]=_block_array::create();
				}
			
				vlist->add(mi);
				_released=true;
			}
		}
		if(!_released)
		{
			_mem_item::release(mi);
		}
	}
}

_FF_END

void* operator new (size_t size)
{
	return ff::ff_alloc(size);
}
void* operator new[](size_t size)
{
	return ff::ff_alloc(size);
}

void operator delete (void *ptr)
{
	ff::ff_free(ptr);
}
void operator delete[](void *ptr)
{
	ff::ff_free(ptr);
}


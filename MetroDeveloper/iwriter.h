#pragma once

// TSNest 4A cls
#pragma pack(push, 1)
struct iwriter_vtbl {
	void* iwriter_dtor_0;

	void* valid;
	void* seek;
	void* tell;
	void* w;
	void* space;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct gtl__intrusive_base {
	void* _shared_part_mt;
	void* _shared_part_lock;
	void* _ref_count;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct size_align {
	unsigned char storage[4];
	//byte align;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct static_allocator {
	void* data;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct hybrid_allocator {
	static_allocator sallocator;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct hvector_base {
	hybrid_allocator baseclass_0;
	void* _array;
	short _capacity;
	short _size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct uvector_base {
	hvector_base baseclass_0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct u_vector_base {
	uvector_base baseclass_0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct u_hvector {
	u_vector_base baseclass_0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct u_stack {
	u_hvector c;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct str_shared {
	void* p_;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct vfs__writer_base_t {
	u_stack chunk_pos;
	str_shared f_name;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct iwriter {
	iwriter_vtbl* __vftable;

	gtl__intrusive_base baseclass_4;
	vfs__writer_base_t baseclass_10;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct iwriterObj {
	iwriter* _object;
};
#pragma pack(pop)

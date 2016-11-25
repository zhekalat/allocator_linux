// compile: gcc -shared -fpic ./mylloc.c -o ./mylloc.so 
// libc sub: export LD_PRELOAD=./mylloc.so

#include <unistd.h>

#define MEM_ALIGN(x) (x + (sizeof(size_t)-1)) & ~(sizeof(size_t)-1)

struct header {
	struct header *next, *last;
	int is_available;
	size_t size;
	void *memory;
};

typedef struct header *Header;

int init = 0;
Header start_point = NULL;

void merge(Header current)
{
	current->size = current->size + current->next->size + sizeof(struct header);
	current->next = current->next->next;
	if (current->next)
	{
		current->next->last = current;
	}
}

void split(Header current, size_t size)
{
	Header follow = (Header)((char*) current + size);
	follow->last = current;
	follow->next = current->next;
	follow->size = current->size - size;
	follow->is_available = 1;
	follow->memory = follow + 1;
	if (current->next)
	{
		current->next->last = follow;
	}
	current->next = follow;
	current->size = size - sizeof(struct header);
}

void *malloc(size_t size)
{
	if (!size) 
		return NULL;
	size_t total_size = MEM_ALIGN(size + sizeof(struct header));
	Header last = NULL;
	if (!init)
	{
		init = 1;
		start_point = sbrk(MEM_ALIGN(sizeof(struct header)));
		if (start_point == (void*) -1)
		{
			return NULL;
		}
		start_point->next = NULL;
		start_point->last = NULL;
		start_point->size = 0;
		start_point->is_available = 0;
		start_point->memory = NULL;
	}

	Header current = start_point;
	while (current && (!current->is_available || current->size < size))
	{
		last = current;
		current = current->next;
	}

	if (!current)
	{
		current = sbrk(total_size);
		if (current == (void*) -1)
		{
			return NULL;
		}
		current->next = NULL;
		current->last = last;
		current->size = total_size - sizeof(struct header);
		current->memory = current + 1;
		last->next = current;
	} 
	else if (total_size + sizeof(size_t) < current->size)
	{
		split(current, total_size);
	}
	current->is_available = 0;
	return current->memory;
}

void free(void *ptr)
{
	if (!ptr || (Header)ptr < start_point || ptr > sbrk(0)) 
		return;
	Header current = (Header) ptr - 1;
	if (current->memory != ptr)
		return;
	current->is_available = 1;
	if (current->next && current->next->is_available)
	{
		merge(current);
	}
	if (current->last->is_available)
	{
		merge(current = current->last);
	}
	if (!current->next)
	{
		current->last->next = NULL;
		sbrk(-current->size - sizeof(struct header));
	}
}

void *calloc(size_t count, size_t size)
{
	size_t total_size = count * size;
	void *current = malloc(total_size);
	return current;
}

void *realloc(void *ptr, size_t size)
{
	if (ptr && (Header)ptr >= start_point && ptr <= sbrk(0))
	{
		Header current = (Header) ptr - 1;
		if (current->memory == ptr)
		{
			write(2, "Hey\n", 4);
			if (current->size >= size + sizeof(struct header))
			{
				write(2, "Lal\n", 4);
				split(current, MEM_ALIGN(current->size + sizeof(struct header)));
				return current->memory;
			}
			else
			{
				write(2, "ale\n", 4);
				free(ptr);
				void *new_ptr = malloc(size);
				return new_ptr;
			}	
		}
		else
			return NULL;
	}
	else
	{
		void *new_ptr = malloc(size);
		return new_ptr;
	}
}

#include <unistd.h>

#define MEM_ALIGN(x) (x + (sizeof(size_t)-1)) & ~(sizeof(size_t)-1)

struct header 
{
    struct header *next, *last;
    size_t        size;
    int           is_available;
    void         *memory;
};

typedef struct header *Header;

int init = 0;
Header start_point = NULL;

Header find_memory(size_t size, Header *heap) 
{
    Header current = start_point;
	while (current && (!current->is_available || current->size < size))
	{
		*heap = current;
		current = current->next;
	}
    return current;
}

void merge(Header current) 
{
    current->size = current->size + current->next->size + sizeof(struct header);
    current->next = current->next->next;
    if (current->next) 
	{
        current->next->last = current;
    }
}

void *malloc(size_t size) 
{
	if (!init)
	{
		start_point = sbrk(MEM_ALIGN(sizeof(struct header)));
        if (start_point == (void*) -1)
            return NULL;
        start_point->next = NULL;
        start_point->last = NULL;
        start_point->size = 0;
        start_point->is_available = 0;
        start_point->memory = NULL;
		init = 1;
	}
    if (!size)
		return NULL;
    size_t length = MEM_ALIGN(size + sizeof(struct header));
    Header last = NULL;
    Header current = find_memory(size, &last);
    if (!current) 
	{
        Header new_mem = sbrk(length);
        if (new_mem == (void*) -1) 
		{
            return NULL;
        }
        new_mem->next = NULL;
        new_mem->last = last;
        new_mem->size = length - sizeof(struct header);
        new_mem->memory = new_mem + 1;
        last->next = new_mem;
        current = new_mem;
    }
    current->is_available = 0;
    return current->memory;
}

void free(void *ptr) {
    if (!ptr || ptr < (void *)start_point || ptr > sbrk(0))
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
        sbrk(- current->size - sizeof(struct header));
    }
}

void *calloc(size_t nmemb, size_t size) 
{
    size_t length = nmemb * size;
    void *ptr = malloc(length);
    return ptr;
}

void *realloc(void *ptr, size_t size) 
{
    void *new_ptr = malloc(size);
    if (new_ptr && ptr && ptr >= (void *)start_point && ptr <= sbrk(0)) 
	{
        Header current = (Header) ptr - 1;
        if (current->memory == ptr) 
		{
			size_t length;
			size_t i;
			if (current->size > size)
				length = size;
			else
				length = current->size;
            char *dst = new_ptr, *src = ptr;
            for (i = 0; i < length; *dst = *src, ++src, ++dst, ++i);
            free(ptr);
        }
    }
    return new_ptr;
}

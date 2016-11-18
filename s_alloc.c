// compile: gcc -shared -fpic ./s_alloc.c -o ./s_alloc.so 
// libc sub: export LD_PRELOAD=s_alloc.so

#include <unistd.h>

void *memory_start;
void *last_adress;
int init = 0;
int timer = 0;
struct Hdr
{
	int is_available;
	int size;
};

void allocator_init()
{
	last_adress = sbrk(0);
	memory_start = last_adress;
	init = 1;
}

void *malloc(long memory_size)
{
	void *current_location;
	struct Hdr *current_location_struct;
	void *memory_location; // return
	memory_location = 0;
	memory_size = memory_size + sizeof(struct Hdr);
	if (init == 0)
		allocator_init();
	current_location = memory_start;
	while(current_location != last_adress && memory_location == 0)
	{		
		current_location_struct = (struct Hdr *)current_location;
		if(current_location_struct->is_available)
		{
			if(current_location_struct->size >= memory_size)
			{
				current_location_struct->is_available = 0;	
				memory_location = current_location;
			}
		}
		current_location = current_location + current_location_struct->size;
	}
	if(memory_location == 0) // Если мы не нашли свободной памяти, запрашиваем у ОС.
	{
		sbrk(memory_size);	
		memory_location = last_adress;
		last_adress = last_adress + memory_size;
		current_location_struct = memory_location;
		current_location_struct->is_available = 0;
		current_location_struct->size = memory_size;
	} 	
	memory_location = memory_location + sizeof(struct Hdr); // Память получена, сдвигаем указатель в её начало.
	return memory_location;
}

void defrag()
{
	timer = 0;
	void *current_location;
	void *next_location;

	struct Hdr *current_location_struct;
	struct Hdr *next_location_struct;
	current_location = memory_start;
	while(current_location != last_adress)
	{		
		current_location_struct = (struct Hdr *)current_location;
		if(current_location_struct->is_available && current_location + current_location_struct->size != last_adress)
		{
			next_location = current_location + current_location_struct->size;
			next_location_struct = (struct Hdr *)next_location;
			if(next_location_struct->is_available)
				current_location_struct->size = current_location_struct->size + next_location_struct->size;
			else
				current_location = current_location + current_location_struct->size + next_location_struct->size;

		}
		else
			current_location = current_location + current_location_struct->size;
	}
}

void free(void *location)
{
	struct Hdr *mem_start;
	mem_start = location - sizeof(struct Hdr);
	mem_start->is_available = 1;
	timer = timer + 1;
	if (timer == 5)
		defrag();
}

void *calloc(long count, long memory_size)
{
	return malloc(count*memory_size);
}

void *realloc(void *old_memory_location, long memory_size)
{
	struct Hdr *old_mem_start;
	memory_size = memory_size + sizeof(struct Hdr);
	old_mem_start = old_memory_location - sizeof(struct Hdr);
	if (old_mem_start->size > memory_size + sizeof(struct Hdr)) // Если нужно меньше памяти чем было, освобождаем часть.
	{
		struct Hdr *new_mem_start;
		new_mem_start = old_mem_start + memory_size;
		new_mem_start->is_available = 1;
		new_mem_start->size = old_mem_start->size - memory_size - sizeof(struct Hdr);
		old_mem_start->size = memory_size;
		return old_memory_location;
	}
	else														// Иначе освобождаем всю и ищем новую.
	{
		free(old_memory_location);
		return malloc(memory_size);
	}
}

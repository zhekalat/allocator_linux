// compile: gcc -shared -fpic s_alloc.c -o s_alloc.so 
// libc sub: export LD_PRELOAD=filename

#include <unistd.h>

void *memory_start;
void *last_adress;

struct Hdr
{
	int is_available;
	int size;
};

void allocator_init()
{
	last_adress = sbrk(0);
	memory_start = last_adress;
	has_initialized = 1;
}

void *malloc(long memory_size)
{
	void *current_location;
	struct Hdr *current_location_struct;
	void *memory_location; // return
	memory_location = 0;
	memory_size = memory_size + sizeof(struct Hdr);
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

void free(void *location)
{
	struct Hdr *mem_start;
	mem_start = location - sizeof(struct Hdr);
	mem_start->is_available = 1;
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

void defrag ()
{
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

void *malloc(long numbytes) 
{ 	
/* Место откуда начинается поиск */ 	
void *current_location;  
	/* Представим что мы работаем с  	* memory_control_block */ 
	struct mem_control_block *current_location_mcb;  
	/* В этот указатель мы вернём найденную память.  На время поиска он должен быть 0 */ 	
	void *memory_location;  	/* Инициализируем, если мы этого не сделали */ 
	if(! has_initialized) 	
	{ 		
malloc_init(); 	
}  	
/* Память содержит в себе memory 	* control block, но пользователям функции mallocне нужно 	* об этом знать. Просто смещаем указатель на размер структуры */ 	
numbytes = numbytes + sizeof(struct mem_control_block);  
	/* Присваиваем memory_location 0 пока не найдем подходящий участок */ 
	memory_location = 0;  	/* Начинаем поиск с начала доступной (управляемой) памяти */ 
	current_location = managed_memory_start;  
	/* Ищем по всему доступному пространству  */ 
	while(current_location != last_valid_address) 
		{ 		
	/* По факту current_location и current_location_mcb 		* одинаковые адреса.  Но current_location_mcb 		* мы используем как структуру , а  		* current_location как указатель для перемещенияt */  	
	current_location_mcb = 			(struct mem_control_block *)current_location; 	
	if(current_location_mcb->is_available) 	
		{ 			
	if(current_location_mcb->size >= numbytes) 		
		{ 			
	/* Воооу! Мы нашли подходящий блок... */  				/* Кто первым встал, того и тапки - отмечаем участок как занятый */ 				current_location_mcb->is_available = 0;  	
	/* Мы оккупировали эту территорию */ 		
	memory_location = current_location;  				/* Прекращаем цикл */ 	
	break; 		
	} 	
	}  		
	/* Если мы оказались здесь, это потому что текущиё блок памяти нам не подошёл, сяпаем дальше */ 		current_location = current_location + 			current_location_mcb->size; 	}  	/* Если мы всё ещё не имеем подходящего адреса, то следует запросить память у ОС */ 	if(! memory_location) 	{ 		/* Move the program break numbytes further */ 		sbrk(numbytes);  		/* После выделения, last_valid_address должен обновится */ 		memory_location = last_valid_address;  		/* Перемещаемся от last valid address на 		* numbytes вперёд */ 		last_valid_address = last_valid_address + numbytes;  		/* И инициализируем mem_control_block */ 		current_location_mcb = memory_location; 		current_location_mcb->is_available = 0; 		current_location_mcb->size = numbytes;  	}  	/* Теперь мы получили память (если не получили ошибок).  	* И в memory_location также есть место под 	* mem_control_block */  	/* Перемещаем указатель в конец mem_control_block */ 	memory_location = memory_location + sizeof(struct mem_control_block);  	/* Возвращаем указатель */ 	return memory_location;   } 
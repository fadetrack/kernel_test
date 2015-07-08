/*
 * store the key and value  of hashmap. 
 */
#include <lcthw/keyvalue.h>

keyvalue* 
keyvalue_create()
{
	keyvalue *kv = calloc(1, sizeof(keyvalue));
	check_mem(kv);

	kv->key = calloc(SHANUM, SHA);	
	check_mem(kv->key);
	kv->index = 0;
	kv->capacity = (SHANUM*SHA);
	kv->expand_rate = (SHANUM*SHA);

	return kv;

	error:
		if (kv) {
			keyvalue_destroy(kv);
		}   

	return NULL;
}

void  
keyvalue_destroy(keyvalue *kv)
{
	if (kv)
	{
		free(kv->key);
		free(kv);
	}
}

/*
 * 0 : stands for not full capacity.
 * 1 : stands for full capacity.
 */
int 
keyvalue_full(keyvalue *kv)
{
	debug("the capacity is %lu and index id %lu", kv->capacity, kv->index);
	if (kv->index <= kv->capacity - 1)
		return 0;
	else
		return 1;
}

static inline int 
keyvalue_resize(keyvalue *kv, size_t newsize)
{
    kv->capacity = newsize;
    check(kv->capacity > 0, "The newsize must be > 0.");

    void *contents = realloc(kv->key, kv->capacity * SHA);
    // check contents and assume realloc doesn't harm the original on error
    check_mem(contents);

    kv->key = contents;
    return 0;
	
	error:
    	return -1; 
}

int 
keyvalue_expand(keyvalue *kv)
{
    size_t old_max = kv->capacity;
    check(keyvalue_resize(kv, kv->capacity + kv->expand_rate) == 0,
            "Failed to expand kv to new size: %lu",
            kv->capacity + kv->expand_rate);

    memset(kv->key + old_max, 0, kv->expand_rate);     
	debug("the capacity is %lu", kv->capacity);
 
    return 0;

	error:
    	return -1; 
}

void* 
keyvalue_push(keyvalue *kv, void *el)
{
	int old_index = kv->index;
	strncpy(kv->key + old_index, el, SHA);
	kv->index += SHA;

    if(keyvalue_full(kv)) {
		debug("the kv is full, so we should expand it");
        if (keyvalue_expand(kv) != 0) {
			return NULL;
		}
    } 
	return (kv->key + old_index);
}

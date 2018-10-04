/* @Abdelmoughite_Eljoaydi 

/* allocates a new thread-safe-resource id */
TSRM_API ts_rsrc_id ts_allocate_id(ts_rsrc_id *rsrc_id, size_t size, ts_allocate_ctor ctor, ts_allocate_dtor dtor)
{
    int i;

	TSRM_ERROR((TSRM_ERROR_LEVEL_CORE, "Obtaining a new resource id, %d bytes", size));

	tsrm_mutex_lock(tsmm_mutex);

	/* Obtain a resource id */
	*rsrc_id = TSRM_SHUFFLE_RSRC_ID(id_count++); // ((rsrc_id)+1) 
	TSRM_ERROR((TSRM_ERROR_LEVEL_CORE, "Obtained resource id %d", *rsrc_id));

	/* Store the new resource type in the resource sizes table */
	/* Global Resource table expansion, adding a new resource type */
	if (resource_types_table_size < id_count) { // check if the table exapansion is necessary.

		/*We reallocate a memory block = sizeof(tsrm_resource_type) x the new resource id */
		resource_types_table = (tsrm_resource_type *) realloc(resource_types_table, sizeof(tsrm_resource_type)*id_count);
		if (!resource_types_table) {
			tsrm_mutex_unlock(tsmm_mutex);      // if an error occured during allocation, we unlock "tsmm_mutex" for the calling thread.
			TSRM_ERROR((TSRM_ERROR_LEVEL_ERROR, "Unable to allocate storage for resource"));
			*rsrc_id = 0;
			return 0;
		}
		resource_types_table_size = id_count;  // Update resource_types_table_size value with the new id_count
	}

	resource_types_table[TSRM_UNSHUFFLE_RSRC_ID(*rsrc_id)].size = size;
	resource_types_table[TSRM_UNSHUFFLE_RSRC_ID(*rsrc_id)].ctor = ctor;
	resource_types_table[TSRM_UNSHUFFLE_RSRC_ID(*rsrc_id)].dtor = dtor;
	resource_types_table[TSRM_UNSHUFFLE_RSRC_ID(*rsrc_id)].done = 0;

	/* enlarge the arrays for the already active threads */ 
	for (i=0; i<tsrm_tls_table_size; i++) {
		tsrm_tls_entry *p = tsrm_tls_table[i];

		while (p) {
			if (p->count < id_count) {
				int j;

				p->storage = (void *) realloc(p->storage, sizeof(void *)*id_count);

				for (j=p->count; j<id_count; j++) {
					p->storage[j] = (void *) malloc(resource_types_table[j].size);
					if (resource_types_table[j].ctor) {
						resource_types_table[j].ctor(p->storage[j], &p->storage);
					}
				}
				p->count = id_count; // assign the new resources count to the global count. 
			}
			p = p->next; 
		}
	}
	tsrm_mutex_unlock(tsmm_mutex);

	TSRM_ERROR((TSRM_ERROR_LEVEL_CORE, "Successfully allocated new resource id %d", *rsrc_id));
	return *rsrc_id; 
}

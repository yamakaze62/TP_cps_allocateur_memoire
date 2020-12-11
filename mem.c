#include "mem.h"
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

// constante définie dans gcc seulement
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* La seule variable globale autorisée
 * On trouve à cette adresse la taille de la zone mémoire
 */
static void* memory_addr;

static inline void *get_system_memory_adr() {
	return memory_addr;
}

static inline size_t get_system_memory_size() {
	return *(size_t*)memory_addr;
}


struct fb {
	size_t size;
	struct fb* next;

};
struct global{
	size_t size;
	struct fb* first;

};
void mem_init(void* mem, size_t taille)
{
  memory_addr = mem;
  *(size_t*)memory_addr = taille;
	assert(mem == get_system_memory_adr());
	assert(taille == get_system_memory_size());
	struct global *g = (struct global *)(memory_addr);
	g->size = taille;
	g->first = (struct fb*)(mem+sizeof(struct global));
	g->first->size = taille - sizeof(struct global);
	g->first->next = NULL;
	mem_fit(mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {

	void* addr=memory_addr+sizeof(struct global);
	struct global *g = (struct global *)memory_addr;
	struct fb *nxtf=g->first;
	while (addr - memory_addr < g->size ){
			if(addr!=nxtf){
				print(addr+sizeof(size_t), *(size_t*)addr, 0);
			}
			else{
				print(nxtf, nxtf->size, 1);
				nxtf=nxtf->next;
			}
			addr=addr+*(size_t*)addr;
	}
}

static mem_fit_function_t *mem_fit_fn;
void mem_fit(mem_fit_function_t *f) {
	mem_fit_fn=f;
}

void *mem_alloc(size_t taille) {
	struct global *g =(struct global *)memory_addr;
	size_t taillezone=taille + sizeof(size_t);
	struct fb *fb=mem_fit_fn(g->first,taillezone);
	if (fb == NULL) {
		return NULL;
	}
	void *avlb;
	if (taille >= fb->size) {
		avlb = (void *)fb;
		if (g->first == fb) {
			struct fb *gfirst=g->first;
			gfirst = gfirst->next;
		} else {
			struct fb *cur = g->first;
			while (cur->next != fb) {
				cur = cur->next;
			}
			cur->next = cur->next->next;
		}
	} else {
		avlb = (void *)fb + fb->size - taille - sizeof(size_t);
		fb->size -= taille + sizeof(size_t);
	}
	*(size_t *)(avlb) = taille + sizeof(size_t);
	return (avlb + sizeof(size_t));
}
/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */

size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	return *(size_t*)(zone-sizeof(size_t));
}

void mem_free(void* mem) {
	size_t size = mem_get_size(mem);
	mem = mem - sizeof(size_t);
	struct global *g = (struct global *)memory_addr;
	struct fb *fb = g->first;
	struct fb *prev;
	while (fb != NULL && (void *)fb < mem) {
		prev = fb;
		fb = fb->next;
	}
	//printf("%p\n%p\n%p\n",(void *)prev+prev->size,mem,(void *)fb);
	if ((void *)prev + prev->size == mem) {
		prev->size = prev->size + size;
		if ((void *)fb == mem + size) {
			prev->size += fb->size;
			prev->next = fb->next;
		}
	} else {
		if ((void *)fb == mem + size) {
			size += fb->size;
			struct fb *n = fb->next;
			fb = (struct fb *)mem;
			prev->next = fb;
			fb->size = size;
			fb->next = n;
		} else {
			struct fb *new = (struct fb*)mem;
			prev->next = new;
			new->size = size;
			new->next = fb;
		}
	}
}


struct fb* mem_fit_first(struct fb *list, size_t size) {
	struct fb* p;
	p=list;
	while(p!= NULL && (p->size < size)){
		p=p->next;
	}
	if(p!=NULL){
		return(p);
	}
	return NULL;
}



/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	    list = mem_fit_first(list,size);
    if (list != NULL) {
        size_t ecartmin = list->size - size;
        struct fb *current = list->next;
        while (current != NULL) {
            if (current->size >= size && current->size-size < ecartmin) {
                ecartmin = current->size - size;
                list = current;
            }
            current = current->next;
        }
    }
    return list;
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
	    list = mem_fit_first(list,size);
    if (list != NULL) {
        size_t ecartmax = list->size - size;
        struct fb *current = list->next;
        while (current != NULL) {
            if (current->size >= size && current->size-size > ecartmax) {
                ecartmax = current->size - size;
                list = current;
            }
            current = current->next;
        }
    }
    return list;
}

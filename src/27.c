// ACCEPTED: 27/30
// MAX: 6.5 sec, 20MB


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LEN 20
#define STARTING_CHAR 47
#define MINHEAP_ORDINI_SIZE 256
#define MINHEAP_LOTTI_SIZE 16
#define HTC_MAG_ING_STARTING_SIZE 93133
#define HTC_MAG_RIC_STARTING_SIZE 93133
#define LOAD_FACTOR 3

// MINHEAP_SIZE: powers of 2
// HASH TABLE HTC SIZE: primes far from powers of 2

// OGGETTI TYPEDEF:

// nodo lotto di ingrediente
typedef struct lotto_s
{
    // TODO: remove nome_ingrediente - redundant
    char nome_ingrediente[LEN];
    int quantita_ingrediente;
    int scadenza;
} lotto_t;

// MINHEAP: LOTTI DI INGREDIENTI
// nodo minheap_lotti
typedef struct minheap_lotti_s
{
    // pointer to dynamic array
    // cell type: order pointer (lotto_t*)
    lotto_t **arr;
    int current_size;
    int max_size;
} minheap_lotti_t;

// HASH TABLE C: MAGAZZINO INGREDIENTI
typedef struct htc_mag_ing_item_s
{
    // key = nome_ingrediente
    char nome_ingrediente[LEN];
    // value = pointer alla minheap dei lotti ingredienti
    minheap_lotti_t *lotti_ingrediente;
    struct htc_mag_ing_item_s *next;
} htc_mag_ing_item_t;

// nodo ingrediente della ricetta
typedef struct ingrediente_ricetta_s
{
    // item stored in slots[h(key)]
    // key=pointer to ingredient in mag_ingredienti
    // TODO: current: change nome_ingrediente to p_ingrediente
    htc_mag_ing_item_t *p_ingrediente;
    // value=quantita
    int quantita_ingrediente;
} ingrediente_ricetta_t;

// EXCEPTION FOR CORRECT ORDER
// HASH TABLE OA: INGREDIENTI DELLA RICETTA
typedef struct ht_ing_ric_s
{
    // array of pointers to hash table items
    ingrediente_ricetta_t **slots;
    int current_size;
    int max_size;
    // always full (current_size=max_size)
} ht_ing_ric_t;

// STRUTTURE DATI: OGGETTI

// QUEUE: ATTESA_ORDINI
// nodo linked list per ordini
typedef struct queue_node_ordini_s
{
    // punta ad una nodo/struct di tipo ordine_s, cosi come *next
    // p_ordine and next are not initialized
    struct ordine_s *p_ordine;
    struct queue_node_ordini_s *next;
} queue_node_ordini_t;

// nodo queue attesa_ordini (linked list)
typedef struct queue_s
{
    queue_node_ordini_t *head, *tail;
} queue_t;

// EXCEPTION FOR CORRECT ORDER
// HASH TABLE C: MAGAZZINO RICETTE
typedef struct htc_mag_ric_item_s
{
    // key = nome_ricetta
    // all other nome_ricetta pointers to this item
    char nome_ricetta[LEN];
    int peso_ricetta;
    // pointer to ht oa ingredienti della ricetta
    ht_ing_ric_t *ingredienti_ricetta;
    struct htc_mag_ric_item_s *next;
} htc_mag_ric_item_t;

typedef struct htc_mag_ric_s
{
    htc_mag_ric_item_t **slots;
    int current_size;
    int max_size;
    float load_factor;
    // load_factor=1.0*#elementi/#slots
} htc_mag_ric_t;
// ORDINI
// nodo ordine
typedef struct ordine_s
{
    // pointer to ricetta
    htc_mag_ric_item_t *p_ricetta;
    int quantita_ordine;
    int peso_ordine;
    // peso_ordine = peso_ricetta * quantita_ordine
    int arrivo_ordine;
} ordine_t;

// MINHEAP: PRONTI_ORDINI
// nodo minheap_ordini
typedef struct minheap_ordini_s
{
    // pointer to dynamic array
    // cell type: order pointer (ordine_t*)
    ordine_t **arr;
    int current_size;
    int max_size;
} minheap_ordini_t;

typedef struct htc_mag_ing_s
{
    htc_mag_ing_item_t **slots;
    int current_size;
    int max_size;
    float load_factor;
    // load_factor=1.0*#elementi/#slots
} htc_mag_ing_t;

// LINKED LIST: INPUT AGGIUNGI_RICETTA()
// node of input linked list
typedef struct ll_node_input_ar_s
{
    char ingrediente[LEN];
    int quantita;
    struct ll_node_input_ar_s *next;
} ll_node_input_ar_t;

typedef struct ll_input_ar_s
{
    ll_node_input_ar_t *head;
    int size;
    // size must be known to allocate hash table open addressing for ingredients
} ll_input_ar_t;

// STRUTTURE DATI: FUNZIONI

// EXCEPTION FOR CORRECT ORDER
// HASH TABLE C: MAGAZZINO RICETTE
// calcola h(key), no probing, no cycle
int hash_htc_mag_ric(char *key, int size)
{
    // sum char values of string * position of char
    int i, len, sum = 0;
    len = strlen(key);
    for (i = 0; i < len; i++)
        sum += ((key[i] - STARTING_CHAR) * (i + 1)) % size;
    return sum % size;
}
// return node of searched ricetta from mag_ricette, or NULL if not found
htc_mag_ric_item_t *search_htc_mag_ric(htc_mag_ric_t *h, char *s_key)
{
    int index;
    index = hash_htc_mag_ric(s_key, h->max_size);
    htc_mag_ric_item_t *current = NULL;
    // start from slot head until slot linked list end
    current = h->slots[index];
    // if slot empty
    if (current == NULL)
        return current;
    // if slot not empty, look for s_key
    while (current != NULL)
    {
        if (strcmp(current->nome_ricetta, s_key) == 0)
            return current;
        current = current->next;
    }
    return current;
}

// QUEUE: ATTESA_ORDINI
// ATTESA_ORDINI e PRONTI_ORDINI
// create ordine node (malloc) of existing ricetta and return its pointer
// delete with simple free(ordine_t*)
ordine_t *create_node_ordine(htc_mag_ric_item_t *p_ricetta, int quantita, int peso, int arrivo)
{
    ordine_t *tmp = malloc(sizeof(ordine_t));
    if (tmp != NULL)
    {
        // find node of ricetta
        tmp->p_ricetta = p_ricetta;
        tmp->quantita_ordine = quantita;
        tmp->peso_ordine = peso;
        tmp->arrivo_ordine = arrivo;
    }
    return tmp;
}

// search ricetta in queue (attesa_ordini), return 1 if found, -1 if not found
int search_queue(queue_t *q, htc_mag_ric_item_t *p_ricetta)
{
    queue_node_ordini_t *current = q->head;
    while (current != NULL)
    {
        if (current->p_ordine->p_ricetta == p_ricetta)
            return 1;
        current = current->next;
    }
    return -1;
}

// return pointer first order in the queue or NULL if empty
ordine_t *peek_queue(queue_t *q)
{
    if (q->head == NULL)
        return NULL;
    return q->head->p_ordine;
}

// aggiunge nuovo nodo ordine alla tail della queue
void enqueue(queue_t *q, ordine_t *ordine_nuovo)
{
    // creo nuovo nodo da aggiungere alla queue
    queue_node_ordini_t *tmp = malloc(sizeof(queue_node_ordini_t));
    if (tmp != NULL)
    {
        tmp->p_ordine = ordine_nuovo;
        tmp->next = NULL;
        // se queue vuota
        if (q->tail == NULL)
        {
            q->head = tmp;
            q->tail = tmp;
            return;
        }
        // else, cambio next dell'ultimo nodo
        q->tail->next = tmp;
        // cambio tail
        q->tail = tmp;
    }
}

// remove and return head queue_node_ordini or NULL if empty
// does not free *p_ordine or removed node
queue_node_ordini_t *dequeue(queue_t *q)
{
    // queue vuota
    if (q->head == NULL && q->tail == NULL)
        return NULL;
    // queue non vuota - rimouve head della queue
    queue_node_ordini_t *tmp = q->head;
    q->head = q->head->next;
    // se ho rimosso l'unico elemento, diventa vuota
    if (q->head == NULL)
        q->tail = NULL;
    // copia il pointer all'ordine da tmp a ordine
    return tmp;
}

// crea nodo queue dinamico
queue_t *create_queue()
{
    queue_t *q = malloc(sizeof(queue_t));
    if (q != NULL)
    {
        q->head = NULL;
        q->tail = NULL;
    }
    return q;
}

// queue di ordini
// libera nodo queue dinamico, i nodi della queue e  i nodi ordini puntati
void deallocate_queue(queue_t *q)
{
    queue_node_ordini_t *returned = NULL;

    // libera tutte le celle della queue
    while (q->head != NULL || q->tail != NULL)
    {
        // libera anche l'ordine puntato!!!
        returned = dequeue(q);
        free(returned->p_ordine);
        free(returned);
    }
    free(q);
}

// MINHEAP: PRONTI_ORDINI
int parent_heap(int i) { return i / 2; }
int left_heap(int i) { return i * 2; }
int right_heap(int i) { return i * 2 + 1; }

// create_node_ordine() at ATTESA_ORDINI

// riorganizza la heap in una minheap_ordini dopo un inserimento
void insert_minheap_ordini_fixup(minheap_ordini_t *h, int i)
{
    // minheap_ordini: parent <= child
    int parent = parent_heap(i);
    // sort based on arrivo_ordine
    if (h->arr[parent]->arrivo_ordine > h->arr[i]->arrivo_ordine)
    {
        // if parent > child, swap them
        ordine_t *tmp = h->arr[i];
        h->arr[i] = h->arr[parent];
        h->arr[parent] = tmp;
        // check parent
        insert_minheap_ordini_fixup(h, parent);
    }
}

// inserisce elemento nuovo nella minheap_ordini
void insert_minheap_ordini(minheap_ordini_t *h, ordine_t *new)
{
    // if max size reached, double array size
    if (h->current_size == h->max_size)
    {
        h->max_size = h->max_size * 2;
        ordine_t **tmp = realloc(h->arr, sizeof(ordine_t *) * h->max_size);
        // realloc() frees old memory
        // original pointer unchanged if realloc fails
        if (tmp != NULL)
            h->arr = tmp;
        else
            printf("Error: realloc() failed\n");
    }
    // indexes start from 0, current_size is index of next free cell
    h->arr[h->current_size] = new;
    // SEGMENTATION FAULT
    insert_minheap_ordini_fixup(h, h->current_size);
    h->current_size++;
}

// riorganizza la heap in una minheap_ordini dopo una rimozione
void minheapify_ordine(minheap_ordini_t *h, int i)
{
    int l, r, min;
    ordine_t *tmp;
    l = left_heap(i);
    r = right_heap(i);
    min = i;
    // sort based on arrivo_ordine
    if (l <= h->current_size &&
        h->arr[l]->arrivo_ordine < h->arr[min]->arrivo_ordine)
        min = l;
    if (r <= h->current_size &&
        h->arr[r]->arrivo_ordine < h->arr[min]->arrivo_ordine)
        min = r;
    // if i non era l'elemento minimo nel sottoalbero
    if (min != i)
    {
        // swap value in i con value in min, controllo sottoalbero di indice min
        tmp = h->arr[i];
        h->arr[i] = h->arr[min];
        h->arr[min] = tmp;
        minheapify_ordine(h, min);
    }
}

// rimuove l'elemento minimo e lo ritorna, o NULL if empty
ordine_t *pop_minheap_ordini(minheap_ordini_t *h)
{
    // tmp=returned node, tmp2=realloc() aux
    ordine_t *tmp;
    if (h->current_size > 0)
    {
        tmp = h->arr[0];
        h->current_size--;
        // replace removed element with last element of array
        h->arr[0] = h->arr[h->current_size];
        minheapify_ordine(h, 0);
        // shrink array:
        // avoid shrinking below 2 cells and
        // repeated shrinking-extendning, by leaving 1 free cell
        if (h->max_size > 2 && h->current_size < h->max_size / 2)
        {
            h->max_size = h->max_size / 2;
            // array of pointers
            ordine_t **tmp2 = realloc(h->arr, sizeof(ordine_t *) * h->max_size);
            // realloc() frees old memory
            // original pointer unchanged if realloc fails
            if (tmp2 != NULL)
                h->arr = tmp2;
            else
                printf("Error: realloc() failed\n");
        }
        return tmp;
    }
    return NULL;
}

// ritorna elemento min senza rimuoverlo dalla heap
ordine_t *peek_minheap_ordini(minheap_ordini_t *h)
{
    if (h->current_size > 0)
        return h->arr[0];
    return NULL;
}

// return 1 if ricetta found, -1 if not
int search_minheap_ordini(minheap_ordini_t *h, htc_mag_ric_item_t *p_ricetta)
{
    int i = 0;
    while (i < h->current_size)
    {
        if (h->arr[i]->p_ricetta == p_ricetta)
            return 1;
        i++;
    }
    return -1;
}

// crea ed inizializza un nodo minheap_ordini
minheap_ordini_t *create_minheap_ordini(int size)
{
    minheap_ordini_t *h = malloc(sizeof(minheap_ordini_t));
    if (h != NULL)
    {
        h->current_size = 0;
        h->max_size = size;
        // allocate array with defined size
        h->arr = malloc(sizeof(ordine_t *) * size);
        if (h->arr == NULL)
            printf("Error in malloc of array\n");
    }
    return h;
}

// dealloca la minheap_ordini, la sua array e i pointer nell'array
void deallocate_minheap_ordini(minheap_ordini_t *h)
{
    int i;
    for (i = 0; i < h->current_size; i++)
        free(h->arr[i]);
    free(h->arr);
    free(h);
}

// MINHEAP: LOTTI DI INGREDIENTI
// alloca ed inizializza nodo lotto di ingrediente
// deallocate with simple free()
lotto_t *create_node_lotto(char *ingrediente, int quantita, int scadenza)

{
    lotto_t *tmp = malloc(sizeof(lotto_t));
    if (tmp != NULL)
    {
        strcpy(tmp->nome_ingrediente, ingrediente);
        tmp->quantita_ingrediente = quantita;
        tmp->scadenza = scadenza;
    }
    return tmp;
}

// riorganizza la heap in una minheap_lotti dopo un inserimento
void insert_minheap_lotti_fixup(minheap_lotti_t *h, int i)
{
    // minheap_lotti: parent <= child
    int parent = parent_heap(i);
    // sort based on scadenza
    if (h->arr[parent]->scadenza > h->arr[i]->scadenza)
    {
        // if parent > child, swap them
        lotto_t *tmp = h->arr[i];
        h->arr[i] = h->arr[parent];
        h->arr[parent] = tmp;
        // check parent
        insert_minheap_lotti_fixup(h, parent);
    }
}

// inserisce elemento nuovo nella minheap_lotti
void insert_minheap_lotti(minheap_lotti_t *h, lotto_t *new)
{
    // if max size reached, double array size
    if (h->current_size == h->max_size)
    {
        h->max_size = h->max_size * 2;
        lotto_t **tmp = realloc(h->arr, sizeof(lotto_t *) * h->max_size);
        // realloc() frees old memory
        // original pointer unchanged if realloc fails
        if (tmp != NULL)
            h->arr = tmp;
        else
            printf("Error: realloc() failed\n");
    }
    // indexes start from 0, current_size is index of next free cell
    h->arr[h->current_size] = new;
    insert_minheap_lotti_fixup(h, h->current_size);
    h->current_size++;
}

// riorganizza la heap in una minheap_lotti dopo una rimozione
void minheapify_lotti(minheap_lotti_t *h, int i)
{
    int l, r, min;
    lotto_t *tmp;
    l = left_heap(i);
    r = right_heap(i);
    min = i;
    // sort based on scadenza
    if (l <= h->current_size &&
        h->arr[l]->scadenza < h->arr[min]->scadenza)
        min = l;
    if (r <= h->current_size &&
        h->arr[r]->scadenza < h->arr[min]->scadenza)
        min = r;
    // if i non era l'elemento minimo nel sottoalbero
    if (min != i)
    {
        // swap value in i con value in min, controllo sottoalbero di indice min
        tmp = h->arr[i];
        h->arr[i] = h->arr[min];
        h->arr[min] = tmp;
        minheapify_lotti(h, min);
    }
}

// rimuove l'elemento e lo dealloca
void pop_minheap_lotti(minheap_lotti_t *h)
{
    // tmp=returned node, tmp2=realloc() aux
    lotto_t *tmp;
    if (h->current_size > 0)
    {
        tmp = h->arr[0];
        h->current_size--;
        // replace removed element with last element of array
        h->arr[0] = h->arr[h->current_size];
        minheapify_lotti(h, 0);
        // shrink array:
        // avoid shrinking below 2 cells and
        // repeated shrinking-extendning, by leaving 1 free cell
        if (h->max_size > 2 && h->current_size < h->max_size / 2)
        {
            h->max_size = h->max_size / 2;
            // array of pointers
            lotto_t **tmp2 = realloc(h->arr, sizeof(lotto_t *) * h->max_size);
            // realloc() frees old memory
            // original pointer unchanged if realloc fails
            if (tmp2 != NULL)
                h->arr = tmp2;
            else
                printf("Error: realloc() failed\n");
        }
        free(tmp);
    }
}

// ritorna elemento min senza rimuoverlo dalla heap
lotto_t *peek_minheap_lotti(minheap_lotti_t *h)
{
    if (h->current_size > 0)
        return h->arr[0];
    return NULL;
}

// crea ed inizializza un nodo minheap_lotti
minheap_lotti_t *create_minheap_lotti(int size)
{
    minheap_lotti_t *h = malloc(sizeof(minheap_lotti_t));
    if (h != NULL)
    {
        h->current_size = 0;
        h->max_size = size;
        // allocate array with defined size
        h->arr = malloc(sizeof(lotto_t *) * size);
        if (h->arr == NULL)
            printf("Error in malloc of array\n");
    }
    return h;
}

// dealloca la minheap_lotti, la sua array e i pointer nell'array
void deallocate_minheap_lotti(minheap_lotti_t *h)
{
    int i;
    for (i = 0; i < h->current_size; i++)
        free(h->arr[i]);
    free(h->arr);
    free(h);
}

// HASH TABLE C: MAGAZZINO INGREDIENTI
// calcola h(key), no probing, no cycle
int hash_htc_mag_ing(char *key, int size)
{
    // sum char values of string * position of char
    int i, len, sum = 0;
    len = strlen(key);
    for (i = 0; i < len; i += 2)
        sum += ((key[i] - STARTING_CHAR) * (i + 1)) % size;
    return sum % size;
}

// crea nuova hash table
htc_mag_ing_t *create_htc_mag_ing(int size)
{
    // alloca hash table
    htc_mag_ing_t *h = malloc(sizeof(htc_mag_ing_t));
    if (h != NULL)
    {
        h->current_size = 0;
        h->max_size = size;
        h->load_factor = 0;
        // allocate slots array and initialize cells to NULL
        h->slots = malloc(sizeof(htc_mag_ing_item_t *) * h->max_size);
        if (h == NULL)
            printf("Error in malloc of array\n");
        else
        {
            int i = 0;
            for (i = 0; i < h->max_size; i++)
                h->slots[i] = NULL;
        }
    }
    return h;
}

// return node of searched ingredient, or NULL if not found
htc_mag_ing_item_t *search_htc_mag_ing(htc_mag_ing_t *h, char *s_ing)
{
    int index;
    index = hash_htc_mag_ing(s_ing, h->max_size);
    htc_mag_ing_item_t *current = NULL;
    // start from slot head until slot linked list end
    current = h->slots[index];
    // if slot empty
    if (current == NULL)
        return current;
    // if slot not empty, look for s_ing
    else
    {
        while (current != NULL)
        {
            if (strcmp(current->nome_ingrediente, s_ing) == 0)
                return current;
            else
                current = current->next;
        }
    }
    return current;
}

// HASH TABLE OA: INGREDIENTI DELLA RICETTA
// hash function: calcola index in base a stringa key,
// size di hash table e cycle (probing)
int hash_ht_ing_ric(char *key, int size, int cycle)
{
    // sum char values of string * position of char
    int i, len, sum = 0;
    len = strlen(key);
    for (i = 0; i < len; i++)
        sum += ((key[i] - STARTING_CHAR) * (i + 1)) % size;
    return (sum + cycle) % size;
}

// search item with key==s_key, return index or -1 if not found
int search_ht_ing_ric(ht_ing_ric_t *h, char *s_key)
{
    int i = 0, try_index;
    while (i < h->max_size)
    {
        try_index = hash_ht_ing_ric(s_key, h->max_size, i);
        // if item==NULL, la key non é presente
        if (h->slots[try_index] == NULL)
            return -1;
        // compare keys
        if (strcmp(s_key, h->slots[try_index]->p_ingrediente->nome_ingrediente) == 0)
            return try_index;
        i++;
    }
    return -1;
}

// aggiunge nuovo nodo ingrediente alla hash table ingredienti della ricetta
void insert_ht_ing_ric(htc_mag_ing_t *mag_ingredienti, ht_ing_ric_t *h, htc_mag_ing_item_t *p_ingrediente, int quantita)
{
    if (h->current_size == h->max_size)
    {
        printf("Error: hash table full\n");
        return;
    }
    // allocate new array item
    ingrediente_ricetta_t *tmp = malloc(sizeof(ingrediente_ricetta_t));
    if (tmp == NULL)
        printf("Error in malloc()\n");
    else
    {
        // initialize new item
        tmp->p_ingrediente = p_ingrediente;
        tmp->quantita_ingrediente = quantita;
        // add new item to the hash table
        int i = 0, try;
        while (i < h->max_size)
        {
            try = hash_ht_ing_ric(tmp->p_ingrediente->nome_ingrediente, h->max_size, i);
            // if position unoccupied, insert item
            if (h->slots[try] == NULL)
            {
                h->slots[try] = tmp;
                h->current_size++;
                // end function
                return;
            }
            // if occupied, look for another position
            i++;
        }
    }
}

// crea nuova hash table ingredienti della ricetta
ht_ing_ric_t *create_ht_ing_ric(int size)
{
    // allocate hash table
    ht_ing_ric_t *h = malloc(sizeof(ht_ing_ric_t));
    if (h != NULL)
    {
        h->current_size = 0;
        h->max_size = size;
        // allocate array of ingredients and initialize cells to NULL
        h->slots = malloc(sizeof(ingrediente_ricetta_t *) * h->max_size);
        if (h->slots == NULL)
            printf("Error in malloc of array\n");
        else
        {
            int i = 0;
            for (i = 0; i < h->max_size; i++)
                h->slots[i] = NULL;
        }
    }
    return h;
}

// dealloca ingredienti nell'array ingredienti, array e hash table ing ric
void deallocate_ht_ing_ric(ht_ing_ric_t *h)
{
    // #malloc==#free
    int i;
    for (i = 0; i < h->max_size; i++)
    {
        if (h->slots[i] != NULL)
            free(h->slots[i]);
    }
    free(h->slots);
    free(h);
}

// HASH TABLE C: MAGAZZINO INGREDIENTI

// only extend
// AVOID REHASHING - BLOCKS
// double array size and remap already inserted items
void rehash_htc_mag_ing(htc_mag_ing_t *h)
{
    printf("Rehashing magazzino ingredienti\n");
    // double new array size
    htc_mag_ing_t *new = NULL;
    htc_mag_ing_item_t *tmp = NULL;
    new = create_htc_mag_ing(h->max_size * 2);
    // copy elements from old hash table to new hash table
    int i, new_index;
    for (i = 0; i < h->max_size; i++)
    {
        // remap linked elements from old slot to new slots (not the same new slot)
        while (h->slots[i] != NULL)
        {
            new_index = hash_htc_mag_ing(h->slots[i]->nome_ingrediente, new->max_size);
            // if new slot empty
            if (new->slots[new_index] == NULL)
                new->slots[new_index] = h->slots[i];
            // if new slot has elements
            else
            {
                tmp = h->slots[i];
                tmp->next = new->slots[new_index];
                new->slots[new_index] = tmp;
            }
            new->current_size++;
            h->slots[i] = h->slots[i]->next;
        }
    }
    if (new->current_size != h->current_size)
        printf("Error in rehashing\n");
    free(h->slots);
    h->slots = new->slots;
    h->max_size = new->max_size;
    h->load_factor = 1.0 * h->current_size / h->max_size;
    free(new);
    printf("Finished rehashing\n");
}

// insert new item to head of slot, rehash if load_factor exceeded
void insert_htc_mag_ing(htc_mag_ing_t *h, char *new_ing, minheap_lotti_t *minheap_lotti)
{
    h->load_factor = 1.0 * (h->current_size + 1) / h->max_size;
    if (h->load_factor > LOAD_FACTOR)
        rehash_htc_mag_ing(h);
    // allocate new item
    int index = hash_htc_mag_ing(new_ing, h->max_size);
    htc_mag_ing_item_t *tmp = malloc(sizeof(htc_mag_ing_item_t));
    if (tmp == NULL)
        printf("Error in malloc()\n");
    else
    {
        // initialize and insert new item
        strcpy(tmp->nome_ingrediente, new_ing);
        tmp->lotti_ingrediente = minheap_lotti;
        tmp->next = h->slots[index];
        h->slots[index] = tmp;
        h->current_size++;
    }
}

// deallocate magazzino ingredienti
// deallocates ht items, ht slots and hash table, minheap lotti pointed by items
void deallocate_htc_mag_ing(htc_mag_ing_t *h)
{
    int i;
    htc_mag_ing_item_t *tmp = NULL;
    for (i = 0; i < h->max_size; i++)
    {
        while (h->slots[i] != NULL)
        {
            tmp = h->slots[i];
            h->slots[i] = h->slots[i]->next;
            deallocate_minheap_lotti(tmp->lotti_ingrediente);
            free(tmp);
        }
    }
    free(h->slots);
    free(h);
}

// HASH TABLE C: MAGAZZINO RICETTE
// crea nuova hash table
htc_mag_ric_t *create_htc_mag_ric(int size)
{
    // alloca hash table
    htc_mag_ric_t *h = malloc(sizeof(htc_mag_ric_t));
    if (h != NULL)
    {
        h->current_size = 0;
        h->max_size = size;
        h->load_factor = 0;
        // allocate slots array and initialize cells to NULL
        h->slots = malloc(sizeof(htc_mag_ric_item_t *) * h->max_size);
        if (h == NULL)
            printf("Error in malloc of array\n");
        else
        {
            int i = 0;
            for (i = 0; i < h->max_size; i++)
                h->slots[i] = NULL;
        }
    }
    return h;
}

// only extend
// AVOID REHASHING - BLOCKS
// double array size and remap already inserted items
void rehash_htc_mag_ric(htc_mag_ric_t *h)
{
    printf("Rehashing magazzino ricette\n");
    // double new array size
    htc_mag_ric_t *new = NULL;
    htc_mag_ric_item_t *tmp = NULL;
    new = create_htc_mag_ric(h->max_size * 2);
    // copy elements from old hash table to new hash table
    int i, new_index;
    for (i = 0; i < h->max_size; i++)
    {
        // remap linked elements from old slot to new slots (not the same new slot)
        while (h->slots[i] != NULL)
        {
            new_index = hash_htc_mag_ric(h->slots[i]->nome_ricetta, new->max_size);
            // TO CHECK: if new slot empty or not
            // if new slot empty
            if (new->slots[new_index] == NULL)
                new->slots[new_index] = h->slots[i];
            // if new slot has elements
            else
            {
                tmp = h->slots[i];
                tmp->next = new->slots[new_index];
                new->slots[new_index] = tmp;
            }
            new->current_size++;
            h->slots[i] = h->slots[i]->next;
        }
    }
    if (new->current_size != h->current_size)
        printf("Error in rehashing\n");
    free(h->slots);
    h->slots = new->slots;
    h->max_size = new->max_size;
    h->load_factor = 1.0 * h->current_size / h->max_size;
    free(new);
    printf("Finished rehashing\n");
}

// insert new item to head of slot, rehash if load_factor exceeded
void insert_htc_mag_ric(htc_mag_ric_t *h, char *new_key, int peso_ricetta, ht_ing_ric_t *ht_ingredienti)
{
    h->load_factor = 1.0 * (h->current_size + 1) / h->max_size;
    if (h->load_factor > LOAD_FACTOR)
        rehash_htc_mag_ric(h);
    // allocate new item
    int index = hash_htc_mag_ric(new_key, h->max_size);
    htc_mag_ric_item_t *tmp = malloc(sizeof(htc_mag_ric_item_t));
    if (tmp == NULL)
        printf("Error in malloc()\n");
    else
    {
        // initialize and insert new item
        strcpy(tmp->nome_ricetta, new_key);
        tmp->peso_ricetta = peso_ricetta;
        tmp->ingredienti_ricetta = ht_ingredienti;
        tmp->next = h->slots[index];
        h->slots[index] = tmp;
        h->current_size++;
    }
}

// delete recipe with searched key if found
void delete_htc_mag_ric(htc_mag_ric_t *h, char *key)
{
    int index;
    index = hash_htc_mag_ric(key, h->max_size);
    htc_mag_ric_item_t *prev = NULL, *current = NULL;
    // start from slot head until slot linked list end
    current = h->slots[index];
    while (current != NULL)
    {
        // if node with matching key found, delete node
        if (strcmp(current->nome_ricetta, key) == 0)
        {
            // if head node
            if (prev == NULL)
                h->slots[index] = current->next;
            // else: node inside linked list (not head)
            else
                prev->next = current->next;
            // free current recipe node,and its pointed ingredient hash table
            deallocate_ht_ing_ric(current->ingredienti_ricetta);
            free(current);
            h->current_size--;
            h->load_factor = 1.0 * (h->current_size) / h->max_size;
            // risposta aggiungi_ricetta()
            printf("rimossa\n");
            return;
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }
}

// deallocate magazzino ricette
// deallocates ht items, ht slots, hash table, ing_ric nodes pointed by recipes
void deallocate_htc_mag_ric(htc_mag_ric_t *h)
{
    int i;
    htc_mag_ric_item_t *tmp = NULL;
    for (i = 0; i < h->max_size; i++)
    {
        while (h->slots[i] != NULL)
        {
            tmp = h->slots[i];
            h->slots[i] = h->slots[i]->next;
            deallocate_ht_ing_ric(tmp->ingredienti_ricetta);
            free(tmp);
        }
    }
    free(h->slots);
    free(h);
}

// MERGE SORTED PRIORITY QUEUE: CORRIERE
// Queue (linked list) already added at ATTESA_ORDINI

// Sort: decreasing order based on peso_ordine, increasing time order arrivo_ordine
// merge 2 sorted sublists, return pointer to first node of merged list
queue_node_ordini_t *merge_ll(queue_node_ordini_t *a, queue_node_ordini_t *b)
// nodes already malloced
// a and b heads of 2 sublists
{
    queue_node_ordini_t *returned_head = NULL;
    if (a == NULL)
        return b;
    else if (b == NULL)
        return a;
    if (a->p_ordine->peso_ordine == b->p_ordine->peso_ordine)
    {
        if (a->p_ordine->arrivo_ordine < b->p_ordine->arrivo_ordine)
        {
            returned_head = a;
            returned_head->next = merge_ll(a->next, b);
        }
        else
        {
            returned_head = b;
            returned_head->next = merge_ll(a, b->next);
        }
    }
    else if (a->p_ordine->peso_ordine > b->p_ordine->peso_ordine)
    {
        returned_head = a;
        returned_head->next = merge_ll(a->next, b);
    }
    else
    {
        returned_head = b;
        returned_head->next = merge_ll(a, b->next);
    }
    // return merged linked list head node pointer
    return returned_head;
}

// divide linked list in mezzo in 2 sublists
void divide_ll(queue_node_ordini_t *head_node, queue_node_ordini_t **low_head_p,
               queue_node_ordini_t **high_head_p)
// nome=pointer to node, nome_p=pointer to pointer of node
{
    queue_node_ordini_t *single_step = NULL, *double_step = NULL;
    // find middle-1 node
    single_step = head_node;
    double_step = single_step->next;
    while (double_step != NULL)
    {
        double_step = double_step->next;
        // if didn't reach end, advance both
        if (double_step != NULL)
        {
            single_step = single_step->next;
            double_step = double_step->next;
        }
    }
    *low_head_p = head_node;
    *high_head_p = single_step->next;
    // separate lists
    single_step->next = NULL;
}

// merge sort linked list recursively, forgets tail pointer
void merge_sort_ll(queue_node_ordini_t **head_node_p)
// nome=pointer to node, nome_p=pointer to pointer of node
{
    if (*head_node_p == NULL || (*head_node_p)->next == NULL)
        return;
    // if sublist has > 1 elements
    // head node pointers of 2 new sublists
    queue_node_ordini_t *low_head, *high_head;
    // pass 1 pointer, 2 pointer to pointers
    divide_ll(*head_node_p, &low_head, &high_head);
    merge_sort_ll(&low_head);
    merge_sort_ll(&high_head);
    // merge 2 sorted sublists and return merged head node pointer
    *head_node_p = merge_ll(low_head, high_head);
}

// merge sort queue and find new tail
void merge_sort_queue_ll(queue_t *q)
{
    if (q->head == NULL || q->head->next == NULL)
        return;
    merge_sort_ll(&(q->head));
    // find new tail if >1 elements in queue
    queue_node_ordini_t *before_tail = NULL;
    before_tail = q->head;
    while (before_tail->next->next != NULL)
        before_tail = before_tail->next;
    q->tail = before_tail->next;
}

// LINKED LIST: INPUT AGGIUNGI_RICETTA()
// create reusable linked list input, free at the end of main()
ll_input_ar_t *create_ll_ar()
{
    ll_input_ar_t *list = malloc(sizeof(ll_input_ar_t));
    if (list != NULL)
    {
        list->head = NULL;
        list->size = 0;
    }
    return list;
}

// push new input node to input linked list
void push_input_ar(ll_input_ar_t *list, char *ingrediente, int quantita)
{
    ll_node_input_ar_t *new = malloc(sizeof(ll_node_input_ar_t));
    if (new != NULL)
    {
        strcpy(new->ingrediente, ingrediente);
        new->quantita = quantita;
        new->next = list->head;
        list->head = new;
        list->size++;
    }
}

// pop and return node from list, without freeing it
ll_node_input_ar_t *pop_input_ar(ll_input_ar_t *list)
{
    ll_node_input_ar_t *tmp = NULL;
    if (list->head != NULL)
    {
        tmp = list->head;
        list->head = list->head->next;
        list->size--;
    }
    return tmp;
}

// WALK AUXILIARY FUNCTIONS

void walk_htc_mag_ric(htc_mag_ric_t *h)
{
    int i;
    htc_mag_ric_item_t *tmp = NULL;
    printf("Load_factor: %f\n", h->load_factor);
    for (i = 0; i < h->max_size; i++)
    {
        tmp = h->slots[i];
        while (tmp != NULL)
        {
            printf("Ricetta: %s, peso:%d, pointer to ingredients: %p \n",
                   tmp->nome_ricetta, tmp->peso_ricetta, tmp->ingredienti_ricetta);
            tmp = tmp->next;
        }
    }
}

void walk_minheap_lotti(minheap_lotti_t *h)
{
    printf("Current size: %d, max size: %d\n", h->current_size, h->max_size);
    int i;
    for (i = 0; i < h->current_size; i++)
    {
        printf("scadenza: %d, quantity: %d\n", h->arr[i]->scadenza, h->arr[i]->quantita_ingrediente);
    }
    printf("\n");
}

void walk_htc_mag_ing(htc_mag_ing_t *h)
{
    int i;
    htc_mag_ing_item_t *tmp = NULL;
    printf("Load_factor: %f\n", h->load_factor);
    for (i = 0; i < h->max_size; i++)
    {
        tmp = h->slots[i];
        while (tmp != NULL)
        {
            printf("Ingrediente. %s", tmp->nome_ingrediente);
            walk_minheap_lotti(tmp->lotti_ingrediente);
            tmp = tmp->next;
        }
    }
}

// prints: {arrivo_ordine} {ricetta_ordine}
void walk_queue(queue_t *q)
{
    queue_node_ordini_t *current = q->head;
    while (current != NULL)
    {
        printf("%d %s ", current->p_ordine->arrivo_ordine, current->p_ordine->p_ricetta->nome_ricetta);
        current = current->next;
    }
}

// prints: {arrivo_ordine} {ricetta_ordine}
void walk_minheap_ordini(minheap_ordini_t *h)
{
    // printf("Current size: %d, max size: %d\n", h->current_size, h->max_size);
    int i;
    for (i = 0; i < h->current_size; i++)
        printf("%d %s ", h->arr[i]->arrivo_ordine, h->arr[i]->p_ricetta->nome_ricetta);
    printf("\n");
}

// PREPARAZIONE ORDINI

// remove lotti di ingrediente con scadenza scaduta
void controlla_scadenza_ingrediente(int tempo, minheap_lotti_t *lotti_current_ing)
{
    lotto_t *peeked = peek_minheap_lotti(lotti_current_ing);
    while (peeked != NULL && tempo >= peeked->scadenza)
    {
        pop_minheap_lotti(lotti_current_ing);
        peeked = peek_minheap_lotti(lotti_current_ing);
    }
}

// FLUSH_LOTTI_SCADUTI - to free memory
// elimina lotti scaduti ad ogni periodo
void flush_tutti_lotti_scaduti(int tempo, int periodo, htc_mag_ing_t *mag_ingredienti)
{
    int i;
    // check every period
    if (tempo % periodo == 0)
    {
        for (i = 0; i < mag_ingredienti->max_size; i++)
        {
            if (mag_ingredienti->slots[i] != NULL)
            {
                controlla_scadenza_ingrediente(tempo, mag_ingredienti->slots[i]->lotti_ingrediente);
            }
        }
    }
}

// return 1 if enough quantity of current ing, -1 otherwise
int controlla_quantita_ingrediente(int tempo, minheap_lotti_t *lotti_current_ing, int quantita_richiesta)
{
    int i, sum = 0;
    if (lotti_current_ing->current_size > 0)
    {
        // delete lotti scaduti
        controlla_scadenza_ingrediente(tempo, lotti_current_ing);
        // check if deleting scaduti emptied minheap
        if (lotti_current_ing->current_size > 0)
        {
            // printf("Quantita richiesta: %d\n", quantita_richiesta);
            // walk_minheap_lotti(lotti_current_ing);
            // iterate through minheap_lotti di un ingrediente, if needed quantity reached, return flag = 1
            for (i = 0; i < lotti_current_ing->current_size; i++)
            {
                sum += lotti_current_ing->arr[i]->quantita_ingrediente;
                if (sum >= quantita_richiesta)
                    return 1;
            }
        }
    }
    return -1;
}

// return 1 if all ingredients available for ordine, -1 if missing ingredient(s)
int controlla_ingredienti_ordine(int tempo, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti, ordine_t *ordine)
{
    int i, quantita_richiesta;
    // cycle through ingredients (hash table oa, slots always full) of ricetta
    for (i = 0; i < ordine->p_ricetta->ingredienti_ricetta->max_size; i++)
    {
        // find current_ingredient
        ingrediente_ricetta_t *current_ing_ricetta = ordine->p_ricetta->ingredienti_ricetta->slots[i];
        quantita_richiesta = current_ing_ricetta->quantita_ingrediente * ordine->quantita_ordine;
        // for readability
        minheap_lotti_t *lotti_current_ing = current_ing_ricetta->p_ingrediente->lotti_ingrediente;
        //  check availability of current ingredient, return -1 if current not available
        if (controlla_quantita_ingrediente(tempo, lotti_current_ing, quantita_richiesta) == -1)
            return -1;
    }
    // printf("Enough of all ingredients\n");
    return 1;
}

// consume ingredients used up by ordine
// precond: ricetta, all ingredients available:
// search_htc_mag_ric()!= NULL && controlla_ingredienti_ordine() == 1
void consuma_ingredienti_ordine(htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti, ordine_t *ordine)
{
    int i, quantita_richiesta, quantita_lotto;
    // cycle through ingredients (hash table oa) of ricetta
    for (i = 0; i < ordine->p_ricetta->ingredienti_ricetta->max_size; i++)
    {
        // find current_ingredient
        ingrediente_ricetta_t *current_ing_ricetta = ordine->p_ricetta->ingredienti_ricetta->slots[i];
        quantita_richiesta = current_ing_ricetta->quantita_ingrediente * ordine->quantita_ordine;
        // for readability
        minheap_lotti_t *lotti_current_ing = current_ing_ricetta->p_ingrediente->lotti_ingrediente;
        //  printf("Consume: ingrediente: %s, quantita richiesta: %d\n", current_ing, quantita_richiesta);
        //  walk_minheap_lotti(lotti_current_ing);
        //  decrease quantita_ingrediente in lot(s)
        while (quantita_richiesta > 0)
        {
            // quantita ingrediente in first lot
            lotto_t *first_lot = peek_minheap_lotti(lotti_current_ing);
            quantita_lotto = first_lot->quantita_ingrediente;
            // if quantita_richiesta satisfied in first lost
            if (quantita_richiesta < quantita_lotto)
            {
                first_lot->quantita_ingrediente -= quantita_richiesta;
                quantita_richiesta = 0;
            }
            // if quantita_richiesta >= quantita_lotto, consume whole lot
            else
            {
                quantita_richiesta -= quantita_lotto;
                pop_minheap_lotti(lotti_current_ing);
            }
            // printf("Ingrediente consumato: %s", current_ing);
        }
        // printf("Ingredienti dell'ordine consumati\n");
    }
}

// CORRIERE
// 1 per riga: {arrivo_ordine} {nome_ricetta} {quantia_ordine} (not peso!)
void walk_corriere(queue_t *corriere)
{
    queue_node_ordini_t *current = corriere->head;
    while (current != NULL)
    {
        printf("%d %s %d\n", current->p_ordine->arrivo_ordine,
               current->p_ordine->p_ricetta->nome_ricetta,
               current->p_ordine->quantita_ordine);
        current = current->next;
    }
}

// print ordini in CORRIERE o "camioncino vuoto"
void print_corriere(queue_t *q)
{
    if (q->head == NULL)
        printf("camioncino vuoto\n");
    else
        walk_corriere(q);
}

// queue di ordini
// libera nodo queue dinamico, i nodi della queue e  i nodi ordini puntati
void flush_corriere(queue_t *q)
{
    queue_node_ordini_t *returned = NULL;

    // libera tutte le celle della queue
    while (q->head != NULL || q->tail != NULL)
    {
        // libera anche l'ordine puntato!!!
        returned = dequeue(q);
        free(returned->p_ordine);
        free(returned);
    }
}

// load, sort, print, flush ordini in corriere
void funzione_corriere(queue_t *corriere, int max_cap, minheap_ordini_t *pronti_ordini)
{
    int load = 0, flag = 0;
    ordine_t *ordine = peek_minheap_ordini(pronti_ordini);
    // until pronti_ordini emptied
    while (flag == 0 && pronti_ordini->current_size > 0)
    {
        // check weight of next order
        load += ordine->peso_ordine;
        // if max_cap surpassed, stop order loading
        if (load > max_cap)
            flag = 1;
        else
        {
            // load order in corriere and check next order
            enqueue(corriere, pop_minheap_ordini(pronti_ordini));
            ordine = peek_minheap_ordini(pronti_ordini);
        }
    }
    merge_sort_queue_ll(corriere);
    print_corriere(corriere);
    // prints: {arrivo_ordine} {nome_ricetta} {quantia_ordine}
    flush_corriere(corriere);
}

// COMANDI

// AGGIUNGI RICETTA
// add ricetta to mag_ricette and calculate peso_ricetta
// add ingredients to mag_ingredienti for ingredient pointers
void aggiungi_ricetta(ll_input_ar_t *list, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti)
{
    char whitespace = ' ', ricetta[LEN], ingrediente[LEN];
    int quantita, peso_ricetta;
    ll_node_input_ar_t *returned = NULL;
    if (scanf("%s%*c", ricetta) > 0)
    {
        // if recipe present: ignorato
        // still scan input
        if (search_htc_mag_ric(mag_ricette, ricetta) != NULL)
        {
            while (whitespace != '\n')
            {
                // do nothing with the input
                if (scanf("%s %d%c", ingrediente, &quantita, &whitespace) > 0)
                    ;
            }
            printf("ignorato\n");
            return;
        }
        // if not present, add: aggiunta
        peso_ricetta = 0;
        while (whitespace != '\n')
        {
            if (scanf("%s %d%c", ingrediente, &quantita, &whitespace) > 0)
                push_input_ar(list, ingrediente, quantita);
        }
        // allocate hash table oa for recipe ingredients, of size list->size
        ht_ing_ric_t *ht_ingredienti = create_ht_ing_ric(list->size);
        if (ht_ingredienti == NULL)
        {
            printf("Error in malloc of hash table ingredienti\n");
            return;
        }
        while (list->head != NULL)
        {
            returned = pop_input_ar(list);
            // check if ingredient already in mag_ingredienti
            htc_mag_ing_item_t *p_ingrediente = search_htc_mag_ing(mag_ingredienti, returned->ingrediente);
            if (p_ingrediente == NULL)
            {
                // add ingrediens to mag_ingredienti for p_ingrediente
                minheap_lotti_t *new_minheap = create_minheap_lotti(MINHEAP_LOTTI_SIZE);
                insert_htc_mag_ing(mag_ingredienti, returned->ingrediente, new_minheap);
                p_ingrediente = search_htc_mag_ing(mag_ingredienti, returned->ingrediente);
            }
            // put ingrediente_ricetta into ht_ing_ric ingredienti ricetta
            peso_ricetta += returned->quantita;
            insert_ht_ing_ric(mag_ingredienti, ht_ingredienti, p_ingrediente, returned->quantita);
            free(returned);
        }
        // insert ricetta to mag_ricette
        insert_htc_mag_ric(mag_ricette, ricetta, peso_ricetta, ht_ingredienti);
        printf("aggiunta\n");
    }
}

// RIMUOVI RICETTA
void rimuovi_ricetta(htc_mag_ric_t *mag_ricette, queue_t *attesa_ordini, minheap_ordini_t *pronti_ordini)
{
    char ricetta[LEN];
    if (scanf("%s%*c", ricetta) > 0)
    {
        htc_mag_ric_item_t *p_ricetta = search_htc_mag_ric(mag_ricette, ricetta);
        if (p_ricetta == NULL)
        {
            printf("non presente\n");
            return;
        }
        if (search_queue(attesa_ordini, p_ricetta) == 1 ||
            search_minheap_ordini(pronti_ordini, p_ricetta) == 1)
        {
            printf("ordini in sospeso\n");
            return;
        }
        delete_htc_mag_ric(mag_ricette, ricetta);
    }
}

// RIFORNIMENTO
// read and add new lots to mag_ingredienti
void rifornimento_add_new_lots(int tempo, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti)
{
    char whitespace = ' ', ingrediente[LEN];
    int quantita, scadenza;
    // add new lots
    while (whitespace != '\n')
    {

        if (scanf("%s %d %d%c", ingrediente, &quantita, &scadenza, &whitespace) > 0)
        {
            // find node of ingredient in magazzino ingredienti
            htc_mag_ing_item_t *searched = search_htc_mag_ing(mag_ingredienti, ingrediente);
            // if not present add ingredient to magazzino ingredienti
            if (searched == NULL)
            {
                // initialize empty lotti minheap of ingredient
                minheap_lotti_t *new_minheap = create_minheap_lotti(MINHEAP_LOTTI_SIZE);
                // initialize ingredient item in mag_ingredienti
                insert_htc_mag_ing(mag_ingredienti, ingrediente, new_minheap);
                // find node of ingredient in magazzino ingredienti
                searched = search_htc_mag_ing(mag_ingredienti, ingrediente);
            }
            // add new lotto node to minheap lotti of ingrediente in magazzino ingredienti
            lotto_t *new_lotto = create_node_lotto(ingrediente, quantita, scadenza);
            insert_minheap_lotti(searched->lotti_ingrediente, new_lotto);
        }
    }
}

// RIFORNIMENTO
// iterate through waitlist, remove if preparable
void rifornimento_controlla_attesa(int tempo, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti,
                                   minheap_ordini_t *pronti_ordini, queue_t *attesa_ordini)
{
    queue_node_ordini_t *prev = NULL, *current = NULL;
    int flag = 0;
    // repeat check head until head not preparable or list emptied
    while (attesa_ordini->head != NULL && flag == 0)
    {
        if (controlla_ingredienti_ordine(tempo, mag_ricette, mag_ingredienti, attesa_ordini->head->p_ordine) == 1)
        {
            consuma_ingredienti_ordine(mag_ricette, mag_ingredienti, attesa_ordini->head->p_ordine);
            insert_minheap_ordini(pronti_ordini, attesa_ordini->head->p_ordine);
            free(dequeue(attesa_ordini));
        }
        else
        {
            flag = 1;
        }
    }
    // if emptied attesa_ordini, end
    if (attesa_ordini->head == NULL)
        return;
    // if 1 element in attesa_ordini
    if (attesa_ordini->head->next == NULL)
    {
        // if preparable
        if (controlla_ingredienti_ordine(tempo, mag_ricette, mag_ingredienti, attesa_ordini->head->p_ordine) == 1)
        {
            consuma_ingredienti_ordine(mag_ricette, mag_ingredienti, attesa_ordini->head->p_ordine);
            insert_minheap_ordini(pronti_ordini, attesa_ordini->head->p_ordine);
            free(dequeue(attesa_ordini));
        }
        return;
    }
    // if >= 2 elements in attesa_ordini
    // cycle through nodes (excluding head, already checked)
    prev = attesa_ordini->head;
    current = prev->next;
    while (current != NULL)
    {
        // if current order preparable, prepare it, if last element, change tail
        if (controlla_ingredienti_ordine(tempo, mag_ricette, mag_ingredienti, current->p_ordine) == 1)
        {
            consuma_ingredienti_ordine(mag_ricette, mag_ingredienti, current->p_ordine);
            insert_minheap_ordini(pronti_ordini, current->p_ordine);
            // if current is last element, change tail
            if (current->next == NULL)
                attesa_ordini->tail = prev;
            // remove current from attesa_ordini
            prev->next = current->next;
            free(current);
            //  check node after removed current
            current = prev->next;
        }
        // if order not preparable, no removal, check next
        else
        {
            prev = current;
            current = current->next;
        }
    }
}

void rifornimento(int tempo, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti,
                  minheap_ordini_t *pronti_ordini, queue_t *attesa_ordini)
{
    rifornimento_add_new_lots(tempo, mag_ricette, mag_ingredienti);
    rifornimento_controlla_attesa(tempo, mag_ricette, mag_ingredienti, pronti_ordini, attesa_ordini);
    printf("rifornito\n");
}

// ORDINE
// evaluate new order - refuse, prepare and add to pronti_ordini or put in attesa_ordini
void ordine(int tempo, htc_mag_ric_t *mag_ricette, htc_mag_ing_t *mag_ingredienti,
            minheap_ordini_t *pronti_ordini, queue_t *attesa_ordini)
{
    char ricetta_ordine[LEN];
    int quantita_ordine, peso_ordine;
    if (scanf("%s %d", ricetta_ordine, &quantita_ordine) > 0)
    {
        htc_mag_ric_item_t *p_ricetta = search_htc_mag_ric(mag_ricette, ricetta_ordine);
        // check if recipe exists
        if (p_ricetta == NULL)
        {
            printf("rifiutato\n");
            return;
        }
        printf("accettato\n");
        // calcola peso ordine
        peso_ordine = p_ricetta->peso_ricetta * quantita_ordine;
        // create order node
        ordine_t *ordine = create_node_ordine(p_ricetta, quantita_ordine, peso_ordine, tempo);
        if (ordine == NULL)
        {
            printf("Error in malloc ordine\n");
            return;
        }
        // if enough ingredients, prepare order and put in PRONTI_ORDINI
        if (controlla_ingredienti_ordine(tempo, mag_ricette, mag_ingredienti, ordine) == 1)
        {
            consuma_ingredienti_ordine(mag_ricette, mag_ingredienti, ordine);
            insert_minheap_ordini(pronti_ordini, ordine);
            return;
        }
        // else, put in ATTESA_ORDINI
        enqueue(attesa_ordini, ordine);
    }
    else
        printf("Error reading\n");
}

// MAIN
int main()
{
    // VARIABILI main
    int tempo, periodo, max_cap, flag_eof;
    char comando[LEN];

    // INIZIALIZZAZIONE STRUTTURE DATI
    htc_mag_ric_t *mag_ricette = create_htc_mag_ric(HTC_MAG_RIC_STARTING_SIZE);
    htc_mag_ing_t *mag_ingredienti = create_htc_mag_ing(HTC_MAG_ING_STARTING_SIZE);
    queue_t *attesa_ordini = create_queue();
    queue_t *corriere = create_queue();
    minheap_ordini_t *pronti_ordini = create_minheap_ordini(MINHEAP_ORDINI_SIZE);
    ll_input_ar_t *list_input_ar = create_ll_ar();

    // INIZIO PROGRAMMA
    flag_eof = 0;
    tempo = 0;
    // read periodo, max_cap
    if (scanf("%d %d", &periodo, &max_cap) != 2)
    {
        printf("Error reading input\n");
        flag_eof = 1;
    }
    // read rest of stdin input line by line:
    while (!flag_eof)
    {
        // printf("--TEMPO: %d---\n", tempo);
        //    CORRIERE
        if (tempo > 0 && tempo % periodo == 0)
        {
            funzione_corriere(corriere, max_cap, pronti_ordini);
            flush_tutti_lotti_scaduti(tempo, periodo, mag_ingredienti);
            // prints: {arrivo_ordine} {nome_ricetta} {quantia_ordine}
            // printf("Attesa ordini: ");
            // walk_queue(attesa_ordini);
            // prints: {arrivo_ordine} {ricetta_ordine}
            // printf("\nPronti ordini: ");
            // walk_minheap_ordini(pronti_ordini);
            // prints: {arrivo_ordine} {ricetta_ordine}
        }
        if (scanf("%s%*c", comando) > 0)
        // always flush whitespace when reading with %*c - remains in stdin
        {
            // printf("%s\n", comando);
            //   continua in base al comando
            //   AGGIUNGI RICETTA
            if (strcmp(comando, "aggiungi_ricetta") == 0)
            {
                aggiungi_ricetta(list_input_ar, mag_ricette, mag_ingredienti);
                // walk_htc_mag_ric(mag_ricette);
            }

            // RIMUOVI RICETTA
            else if (strcmp(comando, "rimuovi_ricetta") == 0)
            {
                rimuovi_ricetta(mag_ricette, attesa_ordini, pronti_ordini);
            }

            // RIFORNIMENTO
            else if (strcmp(comando, "rifornimento") == 0)
            {
                rifornimento(tempo, mag_ricette, mag_ingredienti, pronti_ordini, attesa_ordini);
                // for adjusting LOTTI_SIZE:
                // walk_htc_mag_ing(mag_ingredienti);
            }

            // ORDINE
            else if (strcmp(comando, "ordine") == 0)
            {
                ordine(tempo, mag_ricette, mag_ingredienti, pronti_ordini, attesa_ordini);
                // printf("Attesa ordini:\n");
                // walk_queue(attesa_ordini);
                // printf("\nPronti ordini:\n");
                // walk_minheap_ordini(pronti_ordini);
                // printf("\n");
            }
            else
            {
                printf("Error: unknown command\n");
            }
            tempo++;
        }
        else
            flag_eof = 1;
    }
    // DEALLOCAZIONE STRUTTURE DATI
    deallocate_htc_mag_ric(mag_ricette);
    deallocate_htc_mag_ing(mag_ingredienti);
    deallocate_queue(attesa_ordini);
    deallocate_queue(corriere);
    deallocate_minheap_ordini(pronti_ordini);
    free(list_input_ar);
    // printf("Finished\n");
    return 0;
}
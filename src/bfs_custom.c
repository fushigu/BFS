//Stub for custom BFS implementations

#include "common.h"
#include "aml.h"
#include "csr_reference.h"
#include "bitmap_reference.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>

// two arrays holding visited VERTEX_LOCALs for current and next level
// we swap pointers each time
// int *q1, *q2;
int q_cnt, q2_cnt;

//VISITED bitmap parameters
unsigned long *visited;
int64_t visited_size;

unsigned long *visited1, *visited2;

// global variables of CSR graph to be used inside of AM-handlers
int64_t *pred_glob,*column;
// global variables of CSR graph to be used inside of AM-handlers
int *rowstarts;
// global variables of CSR graph to be used inside of AM-handlers
oned_csr_graph g;

// typedef struct visitmsg {
// 	int vloc;
// 	int vfrom;
// } visitmsg;

// AM-handler for check&visit
// void visithndl(int from, void* data, int sz) {
// 	visitmsg *m = data;
// 	if (!TEST_VISITEDLOC(m->vloc)) {
// 		SET_VISITEDLOC(m->vloc);
// 		q2[q2c++] = m->vloc;
// 		pred_glob[m->vloc] = VERTEX_TO_GLOBAL(from, m->vfrom);
	
// 	}
// }

// inline void send_visit(int64_t glob, int from) {
// 	visitmsg m = {VERTEX_LOCAL(glob), from};
// 	aml_send(&m, 1, sizeof(visitmsg), VERTEX_OWNER(glob));
// }

// localのvertex 番号を格納
typedef struct visitmsg {
	int vloc;
	int vfrom;
} visitmsg;

void send_visit(visitmsg *m, int send_id) {
	visitmsg n = {m->vfrom, m->vloc};
	aml_send(&n, 2, sizeof(visitmsg), send_id);
}

void check_visit_hndl(int from, void* data, int sz) {
	visitmsg *m = data;
	if (TEST_VISITEDLOC(m->vloc)) {
		// printf("%ld is visited, checked from %ld\n", VERTEX_TO_GLOBAL(my_pe(), m->vloc), VERTEX_TO_GLOBAL(from, m->vfrom));
		send_visit(m, from);
	}
}

void send_check_visit(int64_t glob, int from) {
	visitmsg m = {VERTEX_LOCAL(glob), from};
	// printf("check %ld is visited, from %ld\n", glob, VERTEX_TO_GLOBAL(my_pe(), from));
	aml_send(&m, 1, sizeof(visitmsg), VERTEX_OWNER(glob));
}



void in_queue_hndl(int from, void* data, int sz) {
	visitmsg *m = data;
	SET_VISITED2LOC(m->vloc);
	pred_glob[m->vloc] = VERTEX_TO_GLOBAL(from, m->vfrom);
	q2_cnt++;
	// printf("node %d has recieved from %d that %ld visited already and set visit %ld\n", rank, from, VERTEX_TO_GLOBAL(from, m->vfrom), VERTEX_TO_GLOBAL(my_pe(), m->vloc));
}

//user should provide this function which would be called once to do kernel 1: graph convert
void make_graph_data_structure(const tuple_graph* const tg) {
	// //graph conversion, can be changed by user by replacing oned_csr.{c,h} with new graph format 
	// convert_graph_to_oned_csr(tg, &g);

	// column=g.column;
	// visited_size = (g.nlocalverts + ulong_bits - 1) / ulong_bits;
	// visited = xmalloc(visited_size*sizeof(unsigned long));
	// //user code to allocate other buffers for bfs
	convert_graph_to_oned_csr(tg, &g);
	column = g.column;
	rowstarts = g.rowstarts;

	visited_size = (g.nlocalverts + ulong_bits - 1) / ulong_bits;
	// aml_register_handler(visithndl, 1);
	visited = xmalloc(visited_size*sizeof(unsigned long));
	visited2 = xmalloc(visited_size*sizeof(unsigned long));
	// for(int i = 0; i < g.nlocalverts;i++) q1[i]=0,q2[i]=0;
}

//user should provide this function which would be called several times to do kernel 2: breadth first search
//pred[] should be root for root, -1 for unrechable vertices
//prior to calling run_bfs pred is set to -1 by calling clean_pred
// void run_bfs(int64_t root, int64_t* pred) {
// 	pred_glob=pred;
// 	//user code to do bfs
// }
void run_bfs(int64_t root, int64_t* pred) {
	long sum;
	pred_glob=pred;
	aml_register_handler(check_visit_hndl, 1);
	aml_register_handler(in_queue_hndl, 2);

	CLEAN_VISITED();
	CLEAN_VISITED2();

	q_cnt = 0; sum=1; q2_cnt = 0;


	if (VERTEX_OWNER(root) == rank) {
		pred[VERTEX_LOCAL(root)]=root;
		SET_VISITED(root);
		SET_VISITED2(root);
		// q_cnt = 1;
	}

	while(sum) {
		aml_barrier();
		// forall vertex in this process
		for (size_t i = 0; i < g.nlocalverts; i++) {
			if(!TEST_VISITEDLOC(i)) {
				for (int j = rowstarts[i];j<rowstarts[i+1];j++) {
					send_check_visit(COLUMN(j), i);
				}
			} 
			// else {
			// 	printf("%ld has already visited\n", VERTEX_TO_GLOBAL(my_pe(), i));
			// }
		}
		aml_barrier();
		aml_barrier();
		// printf("%d rank, q2_cnt: %d\n", rank, q2_cnt);

		// gather to q1
		sum = q2_cnt; q2_cnt = 0;
		aml_long_allsum(&sum);

		// copy visited1, 2
		memcpy(visited, visited2, visited_size*sizeof(unsigned long));
	}
	aml_barrier();
}

//we need edge count to calculate teps. Validation will check if this count is correct
//user should change this function if another format (not standart CRS) used
void get_edge_count_for_teps(int64_t* edge_visit_count) {
	long i,j;
	long edge_count=0;
	for(i=0;i<g.nlocalverts;i++)
		if(pred_glob[i]!=-1) {
			for(j=g.rowstarts[i];j<g.rowstarts[i+1];j++)
				if(COLUMN(j)<=VERTEX_TO_GLOBAL(my_pe(),i))
					edge_count++;
		}
	aml_long_allsum(&edge_count);
	*edge_visit_count=edge_count;
}

//user provided function to initialize predecessor array to whatevere value user needs
void clean_pred(int64_t* pred) {
	int i;
	for(i=0;i<g.nlocalverts;i++) pred[i]=-1;
}

//user provided function to be called once graph is no longer needed
void free_graph_data_structure(void) {
	free_oned_csr_graph(&g);
	// free(q1); free(q2);
	free(visited);free(visited2);
}

//user should change is function if distribution(and counts) of vertices is changed
size_t get_nlocalverts_for_pred(void) {
	return g.nlocalverts;
}

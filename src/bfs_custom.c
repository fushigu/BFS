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
int *q1, *q2;
int qc, q2c;

//VISITED bitmap parameters
unsigned long *visited;
int64_t visited_size;

// globa variables of CSR graph to be used inside of AM-handlers
int64_t *pred_glob,*column;
int *rowstarts;
oned_csr_graph g;

typedef struct visitmsg {
	int vloc;
	int vfrom;
} visitmsg;

// AM-handler for check&visit
void visithndl(int from, void* data, int sz) {
	visitmsg *m = data;
	if (!TEST_VISITEDLOC(m->vloc)) {
		SET_VISITEDLOC(m->vloc) {
			q2[q2c++] = m->vloc;
			pred_glob[m->vloc] = VERTEX_TO_GLOBAL(from, m->vfrom);
		}
	}
}

inline void send_visit(int64_t glob, int from) {
	visitmsg m = {VERTEX_LOCAL(glob), from};
	aml_send(&m, 1, sizeof(visitmsg), VERTEX_OWNER(glob));
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
	aml_register_handler(visithndl, 1);
	visited = xmalloc(visited_size*sizeof(unsigned long));
	q1 = xmalloc(g.nlocalverts*sizeof(int));
	q2 = xmalloc(g.nlocalverts*sizeof(int));
	for(int i = 0; i < g.nlocalverts;i++) q1[i]=0,q2[i]=0;
}

//user should provide this function which would be called several times to do kernel 2: breadth first search
//pred[] should be root for root, -1 for unrechable vertices
//prior to calling run_bfs pred is set to -1 by calling clean_pred
// void run_bfs(int64_t root, int64_t* pred) {
// 	pred_glob=pred;
// 	//user code to do bfs
// }
void run_bfs(int64_t root, int64_t* pred) {
	int64_t nvisited;
	long sum;
	unsigned int lvl = 1;
	pred_glob=pred;
	aml_register_handler(visithndl,1);

	CLEAN_VISITED();

	qc = 0; sum=1; q2c = 0;

	nvisited = 1;
	// if owned root vertex
	if(VERTEX_OWNER(root) == rank) {
		pred[VERTEX_LOCAL(root)]=root;
		SET_VISITED(root);
		q1[0] = VERTEX_LOCAL(root);
		qc = 1;
	}

	while(sum) {
		// for all vertices in current level send visit AMs to all neighbours;
		for(int i = 0; i < qc; i++) {
			for (int j = rowstarts[q1[i]];j<rowstarts[q1[i]+1];j++) send_visit(COLUMN(j),q1[i]);
			aml_barrier();
		}
	}
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
	free(visited);
}

//user should change is function if distribution(and counts) of vertices is changed
size_t get_nlocalverts_for_pred(void) {
	return g.nlocalverts;
}

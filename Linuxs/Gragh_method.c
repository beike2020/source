/******************************************************************************
 * Function:	A example of min path gragh operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall Gragh_method.c -o Gragh_method
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define  MAXVEXA 	20
#define  MAXVEXB 	30

typedef int PatharcsD[MAXVEXA];
typedef int SPTableD[MAXVEXA];
typedef int PatharcsF[MAXVEXA][MAXVEXA];
typedef int SPTableF[MAXVEXA][MAXVEXA];

typedef struct MGraphA {
	int vexs[MAXVEXA];
	int arcs[MAXVEXA][MAXVEXA];
	int numVertexes;
	int numEdges;
} MGraphA;

typedef struct MGraphB {
	int vexs[MAXVEXB];
	int arcs[MAXVEXB][MAXVEXB];
	int numVertexes;
	int numEdges;
} MGraphB;

typedef struct EdgeNodeA {
	int adjvex;
	int weight;
	struct EdgeNodeA *next;
} EdgeNodeA;

typedef struct EdgeNodeB {
	int adjvex;
	int weight;
	struct EdgeNodeB *next;
} EdgeNodeB;

typedef struct VertexNodeA {
	int in;
	int data;
	EdgeNodeA *firstedge;
} VertexNodeA, AdjListA[MAXVEXA];

typedef struct VertexNodeB {
	int in;
	int data;
	EdgeNodeB *firstedge;
} VertexNodeB, AdjListB[MAXVEXB];

typedef struct {
	AdjListA adjList;
	int numVertexes;
	int numEdges;
} graphAdjListA, *GraphAdjListA;

typedef struct {
	AdjListB adjList;
	int numVertexes;
	int numEdges;
} graphAdjListB, *GraphAdjListB;

int *etv, *ltv;
int *stack2;
int top2;

void create_MGraphM(MGraphA * G)
{
	int i, j;

	G->numEdges = 16;
	G->numVertexes = 9;

	for (i = 0; i < G->numVertexes; i++)
		G->vexs[i] = i;

	for (i = 0; i < G->numVertexes; i++) {
		for (j = 0; j < G->numVertexes; j++) {
			if (i == j)
				G->arcs[i][j] = 0;
			else
				G->arcs[i][j] = G->arcs[j][i] = 65535;
		}
	}

	G->arcs[0][1] = 1;
	G->arcs[0][2] = 5;
	G->arcs[1][2] = 3;
	G->arcs[1][3] = 7;
	G->arcs[1][4] = 5;
	G->arcs[2][4] = 1;
	G->arcs[2][5] = 7;
	G->arcs[3][4] = 2;
	G->arcs[3][6] = 3;
	G->arcs[4][5] = 3;
	G->arcs[4][6] = 6;
	G->arcs[4][7] = 9;
	G->arcs[5][7] = 5;
	G->arcs[6][7] = 2;
	G->arcs[6][8] = 7;
	G->arcs[7][8] = 4;

	for (i = 0; i < G->numVertexes; i++) {
		for (j = i; j < G->numVertexes; j++)
			G->arcs[j][i] = G->arcs[i][j];
	}
}

void create_MGraphT(MGraphA * G)
{
	int i, j;

	G->numEdges = 20;
	G->numVertexes = 14;

	for (i = 0; i < G->numVertexes; i++)
		G->vexs[i] = i;

	for (i = 0; i < G->numVertexes; i++) {
		for (j = 0; j < G->numVertexes; j++)
			G->arcs[i][j] = 0;
	}

	G->arcs[0][4] = 1;
	G->arcs[0][5] = 1;
	G->arcs[0][11] = 1;
	G->arcs[1][2] = 1;
	G->arcs[1][4] = 1;
	G->arcs[1][8] = 1;
	G->arcs[2][5] = 1;
	G->arcs[2][6] = 1;
	G->arcs[2][9] = 1;
	G->arcs[3][2] = 1;
	G->arcs[3][13] = 1;
	G->arcs[4][7] = 1;
	G->arcs[5][8] = 1;
	G->arcs[5][12] = 1;
	G->arcs[6][5] = 1;
	G->arcs[8][7] = 1;
	G->arcs[9][10] = 1;
	G->arcs[9][11] = 1;
	G->arcs[10][13] = 1;
	G->arcs[12][9] = 1;
}

void create_MGraphP(MGraphB * G)
{
	int i, j;

	G->numEdges = 13;
	G->numVertexes = 10;
	for (i = 0; i < G->numVertexes; i++)
		G->vexs[i] = i;

	for (i = 0; i < G->numVertexes; i++) {
		for (j = 0; j < G->numVertexes; j++) {
			if (i == j)
				G->arcs[i][j] = 0;
			else
				G->arcs[i][j] = 65535;
		}
	}

	G->arcs[0][1] = 3;
	G->arcs[0][2] = 4;
	G->arcs[1][3] = 5;
	G->arcs[1][4] = 6;
	G->arcs[2][3] = 8;
	G->arcs[2][5] = 7;
	G->arcs[3][4] = 3;
	G->arcs[4][6] = 9;
	G->arcs[4][7] = 4;
	G->arcs[5][7] = 6;
	G->arcs[6][9] = 2;
	G->arcs[7][8] = 5;
	G->arcs[8][9] = 3;
}

void create_ALGraphT(MGraphA G, GraphAdjListA * GL)
{
	int i, j;
	EdgeNodeA *e;

	*GL = (GraphAdjListA) malloc(sizeof(graphAdjListA));
	(*GL)->numVertexes = G.numVertexes;
	(*GL)->numEdges = G.numEdges;

	for (i = 0; i < G.numVertexes; i++) {
		(*GL)->adjList[i].in = 0;
		(*GL)->adjList[i].data = G.vexs[i];
		(*GL)->adjList[i].firstedge = NULL;
	}

	for (i = 0; i < G.numVertexes; i++) {
		for (j = 0; j < G.numVertexes; j++) {
			if (G.arcs[i][j] == 1) {
				e = (EdgeNodeA *) malloc(sizeof(EdgeNodeA));
				e->adjvex = j;
				e->next = (*GL)->adjList[i].firstedge;
				(*GL)->adjList[i].firstedge = e;
				(*GL)->adjList[j].in++;

			}
		}
	}
}

void create_ALGraphP(MGraphB G, GraphAdjListB * GL)
{
	int i, j;
	EdgeNodeB *e;

	*GL = (GraphAdjListB) malloc(sizeof(graphAdjListB));
	(*GL)->numVertexes = G.numVertexes;
	(*GL)->numEdges = G.numEdges;
	for (i = 0; i < G.numVertexes; i++) {
		(*GL)->adjList[i].in = 0;
		(*GL)->adjList[i].data = G.vexs[i];
		(*GL)->adjList[i].firstedge = NULL;
	}

	for (i = 0; i < G.numVertexes; i++) {
		for (j = 0; j < G.numVertexes; j++) {
			if (G.arcs[i][j] != 0 && G.arcs[i][j] < 65535) {
				e = (EdgeNodeB *) malloc(sizeof(EdgeNodeB));
				e->adjvex = j;
				e->weight = G.arcs[i][j];
				e->next = (*GL)->adjList[i].firstedge;
				(*GL)->adjList[i].firstedge = e;
				(*GL)->adjList[j].in++;
			}
		}
	}
}

void shortPath_Dijkstra(MGraphA G, int v0, PatharcsD * P, SPTableD * D)
{
	int v, w, k, min;
	int final[MAXVEXA];

	for (v = 0; v < G.numVertexes; v++) {
		final[v] = 0;
		(*D)[v] = G.arcs[v0][v];
		(*P)[v] = -1;
	}

	(*D)[v0] = 0;
	final[v0] = 1;
	for (v = 1; v < G.numVertexes; v++) {
		min = 65535;
		for (w = 0; w < G.numVertexes; w++) {
			if (!final[w] && (*D)[w] < min) {
				k = w;
				min = (*D)[w];
			}
		}

		final[k] = 1;
		for (w = 0; w < G.numVertexes; w++) {
			if (!final[w] && (min + G.arcs[k][w] < (*D)[w])) {
				(*D)[w] = min + G.arcs[k][w];
				(*P)[w] = k;
			}
		}
	}
}

void shortPath_Floyd(MGraphA G, PatharcsF * P, SPTableF * D)
{
	int v, w, k;

	for (v = 0; v < G.numVertexes; ++v) {
		for (w = 0; w < G.numVertexes; ++w) {
			(*D)[v][w] = G.arcs[v][w];
			(*P)[v][w] = w;
		}
	}

	for (k = 0; k < G.numVertexes; ++k) {
		for (v = 0; v < G.numVertexes; ++v) {
			for (w = 0; w < G.numVertexes; ++w) {
				if ((*D)[v][w] > (*D)[v][k] + (*D)[k][w]) {
					(*D)[v][w] = (*D)[v][k] + (*D)[k][w];
					(*P)[v][w] = (*P)[v][k];
				}
			}
		}
	}
}

int topoLogical_SortT(GraphAdjListA GL)
{
	EdgeNodeA *e;
	int i, k, gettop;
	int top = 0;
	int count = 0;
	int *stack;

	stack = (int *)malloc(GL->numVertexes * sizeof(int));
	for (i = 0; i < GL->numVertexes; i++) {
		if (0 == GL->adjList[i].in)
			stack[++top] = i;
	}

	while (top != 0) {
		gettop = stack[top--];
		printf("%d -> ", GL->adjList[gettop].data);
		count++;
		for (e = GL->adjList[gettop].firstedge; e; e = e->next) {
			k = e->adjvex;
			if (!(--GL->adjList[k].in))
				stack[++top] = k;
		}
	}
	printf("\n");

	if (count < GL->numVertexes)
		return 0;
	else
		return 1;
}

int topoLogical_SortP(GraphAdjListB GL)
{
	EdgeNodeB *e;
	int i, k, gettop;
	int top = 0;
	int count = 0;
	int *stack;

	stack = (int *)malloc(GL->numVertexes * sizeof(int));
	for (i = 0; i < GL->numVertexes; i++) {
		if (0 == GL->adjList[i].in)
			stack[++top] = i;
	}

	top2 = 0;
	etv = (int *)malloc(GL->numVertexes * sizeof(int));
	for (i = 0; i < GL->numVertexes; i++)
		etv[i] = 0;
	stack2 = (int *)malloc(GL->numVertexes * sizeof(int));

	while (top != 0) {
		gettop = stack[top--];
		printf("%2d -> ", GL->adjList[gettop].data);
		count++;

		stack2[++top2] = gettop;

		for (e = GL->adjList[gettop].firstedge; e; e = e->next) {
			k = e->adjvex;
			if (!(--GL->adjList[k].in))
				stack[++top] = k;

			if ((etv[gettop] + e->weight) > etv[k])
				etv[k] = etv[gettop] + e->weight;
		}
	}
	printf("\n");

	if (count < GL->numVertexes)
		return 0;
	else
		return 1;
}

void criticalPath_SortP(GraphAdjListB GL)
{
	EdgeNodeB *e;
	int i, gettop, k, j;
	int ete, lte;

	topoLogical_SortP(GL);
	ltv = (int *)malloc(GL->numVertexes * sizeof(int));
	for (i = 0; i < GL->numVertexes; i++)
		ltv[i] = etv[GL->numVertexes - 1];

	printf("etv:\t");
	for (i = 0; i < GL->numVertexes; i++)
		printf("%2d -> ", etv[i]);
	printf("\n");

	while (top2 != 0) {
		gettop = stack2[top2--];
		for (e = GL->adjList[gettop].firstedge; e; e = e->next) {
			k = e->adjvex;
			if (ltv[k] - e->weight < ltv[gettop])
				ltv[gettop] = ltv[k] - e->weight;
		}
	}

	printf("ltv:\t");
	for (i = 0; i < GL->numVertexes; i++)
		printf("%2d -> ", ltv[i]);
	printf("\n");

	for (j = 0; j < GL->numVertexes; j++) {
		for (e = GL->adjList[j].firstedge; e; e = e->next) {
			k = e->adjvex;
			ete = etv[j];
			lte = ltv[k] - e->weight;
			if (ete == lte)
				printf("<v%d - v%d> length: %d \n",
				       GL->adjList[j].data, GL->adjList[k].data,
				       e->weight);
		}
	}
}

int min_path_Dijkstra()
{
	int i, j, f;
	MGraphA GD;
	SPTableD DD;
	PatharcsD PD;

	f = 0;
	create_MGraphM(&GD);
	shortPath_Dijkstra(GD, f, &PD, &DD);

	printf("\nThe min path by Dijkstra asc: \n");
	for (i = 1; i < GD.numVertexes; ++i) {
		printf("[v%d - v%d], weight: %2d, ", f, i, DD[i]);
		j = i;
		printf("path: %2d", i);
		while (PD[j] != -1) {
			printf(" -> %2d ", PD[j]);
			j = PD[j];
		}
		printf("\n");
	}

	printf("\nThe distance of points as follow:\n");
	for (i = 1; i < GD.numVertexes; ++i)
		printf("[v%d - v%d]: %d \n", GD.vexs[0], GD.vexs[i], DD[i]);

	return 0;
}

int min_path_Floyd()
{
	int v, w, k;
	MGraphA GF;
	SPTableF DF;
	PatharcsF PF;

	create_MGraphM(&GF);
	shortPath_Floyd(GF, &PF, &DF);

	printf("\nThe distance of points by Floyd as follow:\n");
	for (v = 0; v < GF.numVertexes; ++v) {
		for (w = v + 1; w < GF.numVertexes; w++) {
			printf("[v%d - v%d], weight: %2d, ", v, w, DF[v][w]);
			k = PF[v][w];
			printf("path: %2d", v);
			while (k != w) {
				printf(" -> %2d", k);
				k = PF[k][w];
			}
			printf(" -> %2d\n", w);
		}
		printf("\n");
	}

	printf("The min path D: \n");
	for (v = 0; v < GF.numVertexes; ++v) {
		for (w = 0; w < GF.numVertexes; ++w)
			printf("%2d\t", DF[v][w]);
		printf("\n");
	}

	printf("\nThe min path P:\n");
	for (v = 0; v < GF.numVertexes; ++v) {
		for (w = 0; w < GF.numVertexes; ++w)
			printf("%2d\t", PF[v][w]);
		printf("\n");
	}

	return 0;
}

int topo_sort_MGT()
{
	MGraphA G;
	GraphAdjListA GL;
	int result;

	create_MGraphT(&G);
	create_ALGraphT(G, &GL);
	result = topoLogical_SortT(GL);
	printf("result:%d\n", result);

	return 0;
}

int critical_sort_MGP()
{
	MGraphB G;
	GraphAdjListB GL;

	create_MGraphP(&G);
	create_ALGraphP(G, &GL);
	criticalPath_SortP(GL);

	return 0;
}

int main(void)
{
	printf("\nTest min path by Dijkstra:\n");
	min_path_Dijkstra();

	printf("\nTest min path by Floyd:\n");
	min_path_Floyd();

	printf("\nTest sort path by topo logical:\n");
	topo_sort_MGT();

	printf("\nTest sort path by critical:\ninc:\t");
	critical_sort_MGP();

	return 0;
}

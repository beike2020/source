/******************************************************************************
 * Function:	A example of minspan tree operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall MinTree_span.c -o MinTree_span
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  MAXEDGE 	20
#define  MAXVEX 	20
#define  INFINITY 	65535

typedef struct {
	int arc[MAXVEX][MAXVEX];
	int numVertexes, numEdges;
} MGraph;

typedef struct {
	int begin;
	int end;
	int weight;
} Edge;

void create_MGraph(MGraph * G)
{
	int i, j;

	G->numEdges = 15;
	G->numVertexes = 9;
	for (i = 0; i < G->numVertexes; i++) {
		for (j = 0; j < G->numVertexes; j++) {
			if (i == j)
				G->arc[i][j] = 0;
			else
				G->arc[i][j] = G->arc[j][i] = INFINITY;
		}
	}

	G->arc[0][1] = 10;
	G->arc[0][5] = 11;
	G->arc[1][2] = 18;
	G->arc[1][8] = 12;
	G->arc[1][6] = 16;
	G->arc[2][8] = 8;
	G->arc[2][3] = 22;
	G->arc[3][8] = 21;
	G->arc[3][6] = 24;
	G->arc[3][7] = 16;
	G->arc[3][4] = 20;
	G->arc[4][7] = 7;
	G->arc[4][5] = 26;
	G->arc[5][6] = 17;
	G->arc[6][7] = 19;

	for (i = 0; i < G->numVertexes; i++) {
		for (j = i; j < G->numVertexes; j++)
			G->arc[j][i] = G->arc[i][j];
	}
}

void swap_Edge(Edge * edges, int i, int j)
{
	int temp;

	temp = edges[i].begin;
	edges[i].begin = edges[j].begin;
	edges[j].begin = temp;

	temp = edges[i].end;
	edges[i].end = edges[j].end;
	edges[j].end = temp;

	temp = edges[i].weight;
	edges[i].weight = edges[j].weight;
	edges[j].weight = temp;
}

void sort_Weight(Edge edges[], MGraph * G)
{
	int i, j;

	for (i = 0; i < G->numEdges; i++) {
		for (j = i + 1; j < G->numEdges; j++) {
			if (edges[i].weight > edges[j].weight)
				swap_Edge(edges, i, j);
		}
	}

	printf("\nSort MiniSpanTree by heigh:\n");
	for (i = 0; i < G->numEdges; i++) {
		printf("(%d, %d) %d\n", edges[i].begin, edges[i].end,
		       edges[i].weight);
	}
}

int find_Parent(int *parent, int f)
{
	while (parent[f] > 0)
		f = parent[f];

	return f;
}

void MSTree_Kruskal(MGraph G)
{
	int i, j, n, m, k = 0;
	int parent[MAXVEX];
	Edge edges[MAXEDGE];

	for (i = 0; i < G.numVertexes - 1; i++) {
		for (j = i + 1; j < G.numVertexes; j++) {
			if (G.arc[i][j] < INFINITY) {
				edges[k].begin = i;
				edges[k].end = j;
				edges[k].weight = G.arc[i][j];
				k++;
			}
		}
	}

	sort_Weight(edges, &G);
	for (i = 0; i < G.numVertexes; i++)
		parent[i] = 0;

	printf("\nShow MiniSpanTree by Kruskal:\n");
	for (i = 0; i < G.numEdges; i++) {
		n = find_Parent(parent, edges[i].begin);
		m = find_Parent(parent, edges[i].end);
		if (n != m) {
			parent[n] = m;
			printf("(%d, %d) %d\n", edges[i].begin, edges[i].end,
			       edges[i].weight);
		}
	}
}

void MSTree_Prim(MGraph G)
{
	int min, i, j, k;
	int adjvex[MAXVEX], lowcost[MAXVEX];

	lowcost[0] = 0;
	adjvex[0] = 0;
	for (i = 1; i < G.numVertexes; i++) {
		lowcost[i] = G.arc[0][i];
		adjvex[i] = 0;
	}

	printf("\nShow MiniSpanTree by Prim:\n");
	for (i = 1; i < G.numVertexes; i++) {
		min = INFINITY;
		j = 1;
		k = 0;
		while (j < G.numVertexes) {
			if (lowcost[j] != 0 && lowcost[j] < min) {
				min = lowcost[j];
				k = j;
			}
			j++;
		}

		printf("(%d, %d) %d\n", adjvex[k], k, lowcost[k]);
		lowcost[k] = 0;
		for (j = 1; j < G.numVertexes; j++) {
			if (lowcost[j] != 0 && G.arc[k][j] < lowcost[j]) {
				lowcost[j] = G.arc[k][j];
				adjvex[j] = k;
			}
		}
	}
}

int main(void)
{
	MGraph G;

	create_MGraph(&G);
	MSTree_Kruskal(G);
	MSTree_Prim(G);

	return 0;
}

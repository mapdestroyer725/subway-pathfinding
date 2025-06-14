#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#pragma warning(disable:6031)
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0

int lineList[99] = { 1 };	//���������� ������ ��� 0�ʱ�ȭ
#define TRANSFER_W 44

#define CONSOLE_BUFFER 9999
char console_buffer[CONSOLE_BUFFER];
int console_buffer_idx = 0;
void print_console_buffer()
{
	printf("%s\n", console_buffer);
	console_buffer_idx = 0;
}
void load_console_buffer(char input[])
{
	strcat(console_buffer, input);
}



/*
* �׷���, ���, ������ ��� ���
* ��ȣ���� �� �׷����� ���� ȯ�� ó���� �����ϵ��� ��
* ȯ�� ���� 0ȣ���� ���� ���� �߽����� ����ġ 0���� �����.
*/


#define MAX_VERTICES 1009	//��(���) ��ȣ ����;�� �׷����� �迭���� ��. �Ҽ� 1009.
//���� ũ�� = ���̺� ũ��

//����. �� �� ����ġ ����
typedef struct edgeType
{
	int weight1, weight2;	//1)����ġ��
	int dest;				//2)������ ����� ������ȣ
	struct edgeType* link;	//3)��������Ʈ ������ ���� ��ũ.

} edgeType;

//�� ���. ���� ���� ������ ����Ǿ� ������ ������ ����� �� ��ȣ�� ã���� ��
typedef struct statiNode
{
	int lineId;
	int stationId;			//�� ���� ��ȣ. �� ��ȣ��� �׷����� �ε����� �����. 
	char statiName[99];
	struct edgeType* head;	//���� ����Ʈ�� ������ ��� ��(����)�� ����.

} statiNode;

//�� ���� ���� ���. ȣ������ �������� �ʰ� ��� ���� �ϳ��� ���� ��.
typedef struct GraphType
{
	int n;							//���� �� ����
	statiNode* statiList[MAX_VERTICES];	//�ε��� = ������ȣ.

} GraphType;


/*
*
* statiList[] -> statiNode.head -> edgeType.link -> link -> ... -> NULL
*
* Station :: 0 <-> 1 <-> 2
* Graph g ::
*
*	statiList[0] -> statiNode id=0,head -> edgeType dest=1,link -> NULL
*			 [1] -> statiNode id=1,head -> edgeType dest=0,link -> edgeType dest=2,link -> NULL
*			 [2] -> statiNode id=2,head -> edgeType dest=1,link -> NULL
*
*/

void init_graph(GraphType* g)
{
	g->n = 0;
	for (int i = 0; i < MAX_VERTICES; i++)
	{
		g->statiList[i] = NULL;
	}
}


/*
*  TDL: ��� ���� �� �ڵ� db���� �� �ߺ��߰� ����
*/


/*
* �ؽø��� �̿��ؼ� ������ ��
* �� �̸��� �ؽø� �Է����� �޾� ���̺� ����, �ش� ���̺�
* ��� ������ȣ�� �����Ͽ� �����ϵ��� ��
*
*
*/


/*
* ����
* hash_key -> hashedId -> g->statiList[hashedId] ...
*
* ����: �ؽ� ü�̴� �߰�
* hash_key -> hashedId -> hashTable[hashedId] -> hashChain next ~~ index -> g->statiList[index] ...
*
*/

typedef struct hashChain
{
	char name[99];	//
	int line;		//�� �̸�+����: Ž���� ���� �ּ� ����
	int index;		//������ ���� �ε���: �� ��Ͽ� �� ���� �����͵鿡 �����ϵ��� ��
	struct hashChain* chain;
} hashChain;

hashChain* hashTable[MAX_VERTICES];

// Copilot
void print_hashTable()
{
	printf(":: �ؽ� ���̺� ���� ::\n");
	for (int i = 0; i < MAX_VERTICES; i++)
	{
		hashChain* p = hashTable[i];
		if (p != NULL)
		{
			printf("[%4d]: ", i);
			while (p != NULL)
			{
				printf("(index=%3d, %2d, %s ) -> ", p->index, p->line, p->name);
				p = p->chain;
			}
			printf("NULL\n");
		}
	}
}



//
int hash(const char* name, int statiLine)
{
	int h = 0;	//Ȥ�ø��� unsigned ����
	while (*name)
		h = ((h * 31 + *name++) + MAX_VERTICES) % MAX_VERTICES;		//x31 ���� ���� �ѱ��� �����ڵ带 ���� �ؽ�

	h = ((h * 31 + statiLine) + MAX_VERTICES) % MAX_VERTICES;		//�뼱 ��ȣ�� �ؽ̿� ���
	if (h < 0)
	{
		printf("�ؽð� ���� %d\n", h);
		exit(1);
	}
	return h;
}
int hashing_get_index(const char* name, int statiLine)
{
	int h = hash(name, statiLine);
	printf("\nh=%d\nŽ���Ϸ��°�\t%s %d\nDB\t\t", h, name, statiLine);

	for (hashChain* p = hashTable[h]; p != NULL; p = p->chain)
	{
		printf("%s %d->", p->name, p->line);
		if (strcmp(p->name, name) == 0 && p->line == statiLine)
		{
			printf("\n```hashing success %d\n", h);
			return p->index;	//Ž�� ����, �����Ϳ� ���� ������ �ε����� ��ȯ.
		}
	}
	printf(";\n```search failed %d\n\n", -1 * h);
	return -1 * h;				//Ž���� ����, �ؽ����̺��� �ε����� ���ǰ��� ��ȯ
}


//
int remove_from_hashTable(const char* name, int statiLine)
{
	int h = hash(name, statiLine);
	if (hashTable[h] == NULL)
		return FALSE;

	hashChain* p = hashTable[h];
	hashChain* pre = NULL;

	while (p != NULL)
	{
		pre = p;
		p = p->chain;
		free(pre);
	}
	hashTable[h] = NULL;
	return TRUE;
}

void insert_edge(GraphType* g, int id_fst, int id_snd, int w_fst, int w_snd);

int add_node(GraphType* g, char name[], int statiLine)
{
	printf("()add node called\n");///dbg
	int listIndex;
	listIndex = hashing_get_index(name, statiLine);	//����: �ߺ��Ǵ� ��� ����.
	printf("```addnode listindex %d\n", listIndex);
	//�뼱��ȣ, ���̸��� �����ϴٸ� �ߺ�
	if (listIndex >= 0)
	{
		printf("��� �ߺ� �߻�(%s) \n", name);
		exit(1);
	}

	int hashIndex = -1 * listIndex;		// ������
	/* list DB�� ��� �߰� */
	statiNode* newNode = (statiNode*)malloc(sizeof(statiNode));
	if (newNode == NULL)
	{
		perror("�޸� ����");
		exit(1);
	}
	newNode->lineId = statiLine;
	newNode->stationId = g->n;
	strcpy(newNode->statiName, name);
	newNode->head = NULL;
	g->statiList[g->n] = newNode;		// �� ����� ���� �ε����� ����Ʈ�� ũ�� ��:g->n.

	/* �ؽ� ���̺��� ������Ʈ���� */
	hashChain* newChain = (hashChain*)malloc(sizeof(hashChain));
	if (newChain == NULL)
	{
		perror("�޸� ����");
		exit(1);
	}
	// �� ü�ο� ������ ����
	newChain->line = statiLine;
	strcpy(newChain->name, name);
	newChain->index = g->n;			// ����Ʈ�� ��� �ε��� g->n�� ����Ŵ.
	newChain->chain = NULL;

	/*
	* Table[x] = NULL
	*
	*			 ^ pre v
	* Table[x] = chain next -> NULL
	*
	*			 ^ pre -------- v
	* Table[x] = chain next -> chain next -> NULL
	*
	*			 ^ pre ---------------------- v
	* Table[x] = chain next -> chain next -> chain next -> NULL
	*/
	// �ؽ����̺� ü�� �� �� ����.
	hashChain* p = hashTable[hashIndex];
	if (p == NULL)
	{
		hashTable[hashIndex] = newChain;
		printf("ü�� �����.\n");
	}
	else
	{
		//p = p->chain; // Ʋ��.
		hashTable[hashIndex] = newChain;
		newChain->chain = p;
		printf("ü�� �� �տ� ����.\n");
	}

	printf("��� �߰� ���� list=%d table=%d index=%d\n", g->n, hashIndex, hashTable[hashIndex]->index);///dbg
	g->n++;	// �Ϸ� ���� ��ġ�ؾ� ��.
	// ��� �߰� ��.


	if (statiLine != 0)	//0ȣ���̸� �߰��� �ؾ� ��.
	{
		/*
		* �������ʹ� �̸��� ���� ��尡 �ִ��� Ȯ��
		*
		*CASE 1
		* (A1) -> (A2)
		*
		*CASE 2
		* (A1) -> (A2)--(A0)--(A3)
		*
		*ELSE
		* (A)(B)(C)
		*
		*/

		// �ణ�� ���İ�Ƽ. ������� �԰�ʹ�

		// 1: 0ȣ������ Ž��
		int zeroi = -1;
		zeroi = hashing_get_index(name, 0);

		if (zeroi >= 0)
			goto multi;
		// 2: ���θ���Ʈ�� ���鼭 ��ġ�� ��尡 �ִ��� Ȯ��. �ٸ� ���� ���ΰ��� �޶�� ��.
		for (int i = 0; i < 99 && lineList[i] != 0; i++)
		{
			if (lineList[i] != statiLine && hashing_get_index(name, lineList[i]) >= 0)
			{
				zeroi = hashing_get_index(name, lineList[i]);
				goto multi;
			}
		}
		goto single;

	multi:
		printf("\n\n0ȣ�� ���� <l%d h%d %s %d>, <l%d h%d %s %d>\n", g->n - 1, hashIndex, name, statiLine, zeroi, hash(g->statiList[zeroi]->statiName, g->statiList[zeroi]->lineId), g->statiList[zeroi]->statiName, g->statiList[zeroi]->lineId);
		//print_hashTable();
		insert_edge(g, g->n - 1, zeroi, 0, TRANSFER_W); //int id_fst, int id_snd, int w_fst, int w_snd

	single:;
	}
	return newNode->stationId;
}



/*
* delete_node()
* list[] -> node -> link -> ... -> link -> NULL
*
* ������ ����� ����� �ϴ� �ϴ� ��� ��ũ�� Ÿ�� �� ������(�ֺ�����)����
* �ű⿡ �ִ� ��ũ �� �����Ϸ��� ���� ��ġ�ϴ� ���� �����ϰ�
* ��ũ�� ��� ��� �Ŀ� ���� ��带 �����ϵ��� �Ѵ�
*
* node -> edge-
*
*/
int delete_node(GraphType* g, char name[], int statiLine)
{
	int listIndex;
	listIndex = hashing_get_index(name, statiLine);	//����: �ߺ��Ǵ� ��� ����.
	if (listIndex < 0)
		return FALSE;

	/* ���� ����� �ֺ� ���κ����� ���� ���� */
	edgeType* this = g->statiList[listIndex]->head;
	edgeType* p = NULL;
	edgeType* tofree = NULL;
	while (this != NULL)
	{
		if (this)
			printf("new this\n");
		printf("@this%d ", this->dest);
		p = g->statiList[this->dest]->head;		// p�� �ֺ� ���.
		while (p != NULL)
		{
			printf("@dest%d ", p->dest);
			// �ֺ� ��忡�� ����� ������ �����Ϸ��� �����
			if (p->dest == listIndex)
			{
				tofree = p;
				p = p->link;
				free(tofree);
				printf("found,free!\n");
				break;
			}
			//�ƴ϶�� �ֺ� ����� ���� ��ȯ
			p = p->link;
		}
		/*
		* �ֺ� ��� �ϳ��� ���� �Ϸ�
		* ���� �ٸ� �ֺ� ��带 ã��
		*/

		tofree = this;
		this = this->link;
		//free(tofree);	//�����Ϸ��� ����� ���� ����.
		//if (this == NULL)
		//	printf("this is NULL!\n");
		//else
		//	printf("this is not NULL!\n");
	}
	/*
	* �ֺ� ����� ��� ����� ���� ���� �Ϸ�
	* �����Ϸ��� ��忡 ����� ��� ���� ���� �Ϸ�.
	*
	* ���� �� ���� �����Ϸ��� ��带 �����ϴ� ��
	* �� ����Ʈ������ �ؽ� ���̺��� ����
	*/

	free(g->statiList[listIndex]);					// ��� ����
	g->statiList[listIndex] = NULL;					// �� ����Ʈ���� ����
	return remove_from_hashTable(name, statiLine);	// �ؽ� ���̺��� ����
}


void insert_edge(GraphType* g, int id_fst, int id_snd, int w_fst, int w_snd)
{
	printf("\n()insert edge called\n");
	if (id_fst == id_snd)
	{
		printf("�߸��� ���� �߰� �߻�(�� ��尡 ����): (%d, %d, %d, %d)\n", id_fst, id_snd, w_fst, w_snd);
		return;
	}

	printf("fst = %d, snd = %d\n", id_fst, id_snd);

	edgeType* newEdge = NULL;	//�� ���� ������. Ȥ�ø� �ߺ� ������ ���� �̸� ����.

	/* ���� ���� �۾� */
	//ȣ���� ���ٸ� �״�� ���� ����

	if (g->statiList[id_fst]->lineId == g->statiList[id_snd]->lineId)
	{
		printf("ȣ���� ����.\n");
		//1�� ����. �ܹ��� ���� fst->snd
		newEdge = (edgeType*)malloc(sizeof(edgeType));
		if (newEdge == NULL)
		{
			perror("�޸� ����");
			exit(1);
		}
		newEdge->dest = id_snd;
		newEdge->weight1 = w_fst;
		newEdge->weight2 = w_snd;
		newEdge->link = NULL;

		if (g->statiList[id_fst]->head == NULL)
			g->statiList[id_fst]->head = newEdge;
		else
		{
			edgeType* p = g->statiList[id_fst]->head;
			g->statiList[id_fst]->head = newEdge;
			newEdge->link = p;
		}

		//2�� ����. ������ ���� snd->fst
		newEdge = (edgeType*)malloc(sizeof(edgeType));
		if (newEdge == NULL)
		{
			perror("�޸� ����");
			exit(1);
		}
		newEdge->dest = id_fst;
		newEdge->weight1 = w_fst;
		newEdge->weight2 = w_snd;
		newEdge->link = NULL;

		if (g->statiList[id_snd]->head == NULL)
			g->statiList[id_snd]->head = newEdge;
		else
		{
			edgeType* p = g->statiList[id_snd]->head;
			g->statiList[id_snd]->head = newEdge;
			newEdge->link = p;
		}
		/* fst <-> snd */
	}
	//ȣ���� �ٸ���
	else
	{
		printf("ȣ���� �ٸ���.\n");
		// �� ���� �̸��� ���ƾ� ��
		if (strcmp(g->statiList[id_fst]->statiName, g->statiList[id_snd]->statiName) == 0)
		{
			printf("�� ���� �̸��� ����.\n");
			// �� ���� �̸��� ���� �� ��° ȣ�� ��ȣ�� 0���̸� �߾�ȯ�¿��� �ڵ� ���� ���̽���.
			if (g->statiList[id_snd]->stationId == 0)
			{
				//1�� ����. �ܹ��� ���� fst->snd
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = id_snd;
				newEdge->weight1 = w_fst;
				newEdge->weight2 = w_snd;
				newEdge->link = NULL;

				if (g->statiList[id_fst]->head == NULL)
					g->statiList[id_fst]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[id_fst]->head;
					g->statiList[id_fst]->head = newEdge;
					newEdge->link = p;
				}

				//2�� ����. ������ ���� snd->fst
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = id_fst;
				newEdge->weight1 = w_fst;
				newEdge->weight2 = w_snd;
				newEdge->link = NULL;

				if (g->statiList[id_snd]->head == NULL)
					g->statiList[id_snd]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[id_snd]->head;
					g->statiList[id_snd]->head = newEdge;
					newEdge->link = p;
				}
				/* fst <-> snd */
			}
			else
			{
				//
				/*
				*  �߾�ȯ�¿��� �̹� �����ߴ°�?
				*  �� ��ȣ�� �̸��� 0ȣ���� �ؽ� Ű�� �ؽ� ����� �ε��� ����
				*  ȣ���� �������� �ʴ´ٸ� �߾�ȯ�¿��� �߰��Ѵ�.
				*/
				printf("�߾�ȯ�¿��� �����Ǿ��ִ��� Ȯ���� �� ����.\n");
				//printf("%d", hashing_get_index(g->statiList[id_fst]->statiName, 0));
				int trans = -1;
				if (hashing_get_index(g->statiList[id_fst]->statiName, 0) < 0)
					trans = add_node(g, g->statiList[id_fst]->statiName, 0);	//�߾�ȯ�¿� �߰�.
				else
					trans = g->statiList[hashing_get_index(g->statiList[id_fst]->statiName, 0)]->stationId;	//���⼭ ������ ����=�����ε�����=�ؽ����̺� ����� �Ǿ����� �ʴ�.0�߾�ȯ�¿��� �ؽ����̺� ������� �ʰ� �Լ��� ����Ǵ� ���� �߻�.

				/* �߾�ȯ�¿��� �� ���� ���� */
				//�߾� --> ù°��
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = id_fst;						//������: ù°��
				newEdge->weight1 = 0;						//
				newEdge->weight2 = 0;						//ȯ�¿� ����ġ�� 0.
				newEdge->link = NULL;

				if (g->statiList[trans]->head == NULL)
					g->statiList[trans]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[trans]->head;	//
					g->statiList[trans]->head = newEdge;		//
					newEdge->link = p;							//�����: �߾�ȯ�¿�
				}

				//�߾� <-> ù°��
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = trans;
				newEdge->weight1 = 0;
				newEdge->weight2 = 0;
				newEdge->link = NULL;

				if (g->statiList[id_fst]->head == NULL)
					g->statiList[id_fst]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[id_fst]->head;
					g->statiList[id_fst]->head = newEdge;
					newEdge->link = p;
				}

				//�߾� --> ��°��
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = id_snd;
				newEdge->weight1 = 0;
				newEdge->weight2 = 0;
				newEdge->link = NULL;

				if (g->statiList[trans]->head == NULL)
					g->statiList[trans]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[trans]->head;
					g->statiList[trans]->head = newEdge;
					newEdge->link = p;
				}

				//�߾� <-> ��°��
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("�޸� ����");
					exit(1);
				}
				newEdge->dest = trans;
				newEdge->weight1 = 0;
				newEdge->weight2 = 0;
				newEdge->link = NULL;

				if (g->statiList[id_snd]->head == NULL)
					g->statiList[id_snd]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[id_snd]->head;
					g->statiList[id_snd]->head = newEdge;
					newEdge->link = p;
				}
			}

		}
		//�� ���� �̸��� �ٸ��� �߸��� ���̽���
		else
			printf("\n:: [!] �߸��� ���� ����: ȣ���� �ٸ��� �� ���� �̸��� �ٸ��ϴ�. ::\n");

	}
	printf("���� ���� ����\n");
}




/* ���� ���, ������ ���� �� ���� */

/* dijkstra �˰��� */



#define INF 9999999

int distance[MAX_VERTICES];
int visited[MAX_VERTICES];
int prev[MAX_VERTICES];

int choose(int distance[], int n, int found[])
{
	int i, min, minpos;
	min = INF;
	minpos = -1;
	for (i = 0; i < n; i++)
	{
		if (distance[i] < min && !found[i])
		{
			min = distance[i];
			minpos = i;
		}
	}
	printf("choose = %d\n", minpos);
	return minpos;
}

void print_status(GraphType* g)
{
	static int step = 1;
	printf("\n\nSTEP %d: \n", step++);
	printf("distance: \n");
	for (int i = 0; i < g->n; i++)
	{
		if (distance[i] == INF)
			printf(" * ");
		else
			printf("%2d ", distance[i]);
	}
	printf("\n");
	printf("\nfound: \n");
	for (int i = 0; i < g->n; i++)
		printf("%2d ", visited[i]);
	printf("\n\n");

}

int dijkstra(GraphType* g, int startId, int endId, int weightType)
{
	if (!(weightType == 1 || weightType == 2))
	{
		printf("����ġ ���� ����\n");
		exit(1);
	}

	int i, u, w;
	/* �ʱ�ȭ */
	for (i = 0; i < g->n; i++)
	{
		distance[i] = INF;	// �ֺ� �� ���������� ��� ���� �ʱ�ȭ
		visited[i] = FALSE;
		prev[i] = -1; // Copolot
	}
	for (edgeType* p = g->statiList[startId]->head; p != NULL; p = p->link)
	{
		if (weightType == 1)
			distance[p->dest] = p->weight1;
		else if (weightType == 2)
			distance[p->dest] = p->weight2;
		else
			exit(1);
		prev[p->dest] = startId; // Copilot

		//printf("@");
	}


	/* Ž�� ���� */
	distance[startId] = 0;
	visited[startId] = TRUE;
	//visited[endId] = 2;


	printf(" --------------------------------%d\n", endId);
	for (i = 0; i < g->n - 1; i++)
	{
		print_status(g); //STEP
		u = choose(distance, g->n, visited);
		if (u == -1)
		{
			printf("u = -1\n");
			exit(1);
		}
		if (g->statiList[u] == NULL)
		{
			printf("u�� ��\n");
			break;
		}
		else
			printf("u�� ���� �ƴ�\n");

		visited[u] = TRUE;
		for (edgeType* p = g->statiList[u]->head; p != NULL; p = p->link)
		{
			printf("startId: %d -> dest: %d | weight1: %d | weight2: %d\n",
				startId, p->dest, p->weight1, p->weight2);
			if (!visited[p->dest])
			{
				printf("pdest=%d %s", p->dest, g->statiList[p->dest]->statiName);
				int weight = (weightType == 1) ? p->weight1 : p->weight2;
				if (distance[u] + weight < distance[p->dest])
				{
					printf("dist=%d~~", distance[p->dest] = distance[u] + weight);
					prev[p->dest] = u; // Copilot
				}
			}
		}

		if (visited[endId] == TRUE)
			return distance[endId];		//Ž���� ����. �ִ� �Ÿ�
	}

	return FALSE;				//Ž���� ����
}

// ��� ��� �Լ� Copilot
int print_path(int prev[], int start, int end, GraphType* g, int last)
{
	if (end == -1) return;
	if (end != start) print_path(prev, start, prev[end], g, g->statiList[end]->lineId);
	if (g->statiList[end]->lineId == 0)
		printf("-%%-");
	else
		printf("%s_%d", g->statiList[end]->statiName, g->statiList[end]->lineId);

	if (last != 0 && g->statiList[end]->lineId != 0)
		printf(")---(");

}
int skiptodata(const char string[], int index)
{
	while (string[index] != ',' && string[index] != '(' && string[index] != ')' && string[index] != '=' && index < strlen(string))
		index++;
	return ++index;
}

#define MAX_READ 256

typedef struct inputData
{
	int weight1, weight2;
	int line1, line2;
	char name1[MAX_READ], name2[MAX_READ];
} inputData;

void init_inputData(inputData* d)
{
	d->weight1 = d->weight2 = d->line1 = d->line2 = 0;
}

void inputError(int fileline, const char readingStr[], int strIndex, char expected[])
{
	printf("\n[!] ������ �Է� ����(��: %d):\n%s", fileline, readingStr);
	for (int i = 0; i < strIndex; i++)
		printf(" ");
	printf("��...����, ��밪: %s\n\n", expected);
	exit(1);//dbg
}

int readNum(const char readingStr[], int strIndex, int fileline)
{
	int temp = 0;
	int _strIndex = strIndex;
	int _fileline = fileline;
	if (!(readingStr[strIndex] >= 48 && readingStr[strIndex] <= 57))
		inputError(_fileline, readingStr, _strIndex, "<ȣ��: �ڿ�����.>");
	while (readingStr[strIndex] >= 48 && readingStr[strIndex] <= 57 && strIndex < MAX_READ)
		temp = 10 * temp + (int)readingStr[strIndex++] - 48;
	return temp;
}

void readString(const char readingStr[], int strIndex, int fileline, char* name)
{
	int index = 0;
	char buffer[MAX_READ] = { '\0' };
	while (readingStr[strIndex] != ',' && readingStr[strIndex] != '\0' && readingStr[strIndex] != '\n' && strIndex < MAX_READ)
	{
		if (readingStr[strIndex] <= 48 && readingStr[strIndex] >= 64)
			inputError(fileline, readingStr, strIndex, "string");
		buffer[index] = readingStr[strIndex];
		index++; strIndex++;
	}
	buffer[index] = '\0';
	//printf("readingstr = %s\n", readingStr);///dbg
	//printf("finalbuffer = %s\n", buffer);///dbg
	strcpy(name, buffer);
}

void update_linedata(int line)
{
	int i = 1;
	for (i; i < 99 && lineList[i] != 0; i++)
	{
		if (lineList[i] == line)
			return;
	}
	lineList[i] = line;
}

int file(GraphType* g)
{
	FILE* readfile;
	readfile = fopen("stationData.txt", "r");

	if (readfile == NULL)
	{
		/* ���ø� ���� ���� */
		FILE* writefile = fopen("stationData.txt", "w");
		if (writefile == NULL)
		{
			perror("������ ������ �� �����ϴ�.\n");
			exit(1);
		}

		const char* text =
			"#\n"
			"# ������ ���� ���� �߰��� �� �ֽ��ϴ�.\n"
			"#\n"
			"#line=<ȣ��>\n"
			"#<�� �̸�>\n"
			"#<����ġ1>, <����ġ2>\n"
			"#<�� �̸�>\n"
			"#<����ġ1>, <����ġ2>\n"
			"#<�� �̸�>"
			"#\n"
			"#<�� �̸�>\n"
			"#line=<ȣ��>\n"
			"#<�� �̸�>\n"
			"#\n"
			"#<�� �̸�>\n"
			"#<�� �̸�>\n"
			"#/connect(<ȣ��1>, <�� �̸�1>, <ȣ��2>, <�� �̸�2>, <����ġ1>, <����ġ2>)\n"
			"#\n"
			"# ��Ģ1.�� ���̿� ����ġ�� �θ� �� ���� �ڵ� ����˴ϴ�.\n"
			"# ��Ģ2.�� �̸��� �����ϰ� ȣ���� �ٸ� ���� �ڵ����� ȯ�¿����ν� ����˴ϴ�.\n"
			"# ��Ģ3.�߰������� �����ϰ� ���� ���� /connect() ��ɾ ����ϼ���.\n"
			"# ��Ģ4.ȣ�� ���� �⺻���� 1ȣ���̸� �ڿ����� �Է� �����մϴ�.\n"
			"# Notice: �� �̸��� ����� ��� ������ϴ�.\n"
			"#\n";

		fputs(text, writefile);
		fclose(writefile);
		printf("������ ã�� ���߽��ϴ�. ���ο� ���ø��� ���� stationData.txt�� �����Ͽ����ϴ�. ���� ������ ANSI���� Ȯ�����ּ���.\n");
		return FALSE;
	}

	int fileline = 1;
	int failCount = 0;
	char readingStr[MAX_READ] = { '\0' };
	int strIndex;
	char buffer[MAX_READ] = { '\0' };
	int bufferindex;
	int prevNode = -1;

	int prevInput = 0;
	/*
	* �� �Է�	0 = ����.
	*			0 = line.
	*			0 = ���ڿ�.
	*			1 = ����.		--> ���� ���ڿ�(���̸�) **��. ����, line, /connect�̸� ����.
	*			0 = /connect.
	*/

	inputData* d = (inputData*)malloc(sizeof(inputData));
	if (d == NULL)
	{
		perror("�޸� ����");
		exit(1);
	}
	init_inputData(d);
	int settedLine = 1;

	/* buffer���� �ּ� �ٰ� ��� ���⸦ ������ */
	while (fgets(buffer, sizeof(readingStr), readfile))
	{
		/*
		* TDL: �ڵ尡 �ּ� ó���� �������� ���ϰ� ����.
		*		-> ����ó���ϰ� ����. �ٸ� ���Ḧ ����� �� �ϰ� ����.
		*			�� ���� \n, \0������ ����� �����ϴ°� ����.
		*/

		/* ���� ���� -> readingStr�� ����
		*  �ٹٲ��� ������ ���� ���־� �� */
		bufferindex = strIndex = 0;
		while (buffer[bufferindex] != '\0')	// �ٹٲ��� ����
		{
			if (buffer[bufferindex] != ' ' && buffer[bufferindex] != '\t')
				readingStr[strIndex++] = buffer[bufferindex];
			bufferindex++;
		}
		readingStr[strIndex] = '\0';		//������ �����ڵ�.

		strIndex = 0;

		/* ��ɾ� �о���̱� ���� */
		// �ּ� ���� �ƴ϶��
		printf("```readingstr = %s\n", readingStr);///dbg
		if (readingStr[0] != '#')
		{
			//printf("%c != #\n", readingStr[0]);///dbg
			/* �Է��� �ϴ� ��Ȯ�� �����ϰ� ���� */
			// ���� �Է�
			if (readingStr[0] == '\n' || readingStr[0] == '\0')
			{
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<�� �̸�>");
				prevInput = 0;
				/* ������ �ൿ ������ */
			}
			// connect ��ɾ� �Է�
			else if (readingStr[0] == '/')
			{
				//printf("%c == /\n", readingStr[0]);///dbg
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<�� �̸�>");
				prevInput = 0;

				/* /connect(<ȣ��1>, <���̸�1>,  <ȣ��2>, <���̸�2>, <����ġ1>, <����ġ2>) */
				// <ȣ��1> - int
				strIndex = skiptodata(readingStr, strIndex);	// ..connect(.. ������.
				d->line1 = readNum(readingStr, strIndex, fileline);

				// <���̸�1> - char
				strIndex = skiptodata(readingStr, strIndex);
				readString(readingStr, strIndex, fileline, d->name1);

				// <ȣ��2> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->line2 = readNum(readingStr, strIndex, fileline);

				// <���̸�2> - char
				strIndex = skiptodata(readingStr, strIndex);
				readString(readingStr, strIndex, fileline, d->name2);

				// <����ġ1> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->weight1 = readNum(readingStr, strIndex, fileline);

				// <����ġ2> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->weight2 = readNum(readingStr, strIndex, fileline);


				//insert_edge(g, add_node(g, d->name1, d->line1), add_node(g, d->name2, d->line2), d->weight1, d->weight2);
				insert_edge(g, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
				/* done. */
			}
			// ���� �Է�
			else if (readingStr[0] >= 48 && readingStr[0] <= 57)
			{
				//printf("%c == 0123456789\n", readingStr[0]);///dbg
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<�� �̸�>");
				prevInput = 1;

				// <����ġ1, 2> - int
				strIndex = 0;
				d->weight1 = readNum(readingStr, strIndex, fileline);
				strIndex = skiptodata(readingStr, strIndex);
				d->weight2 = readNum(readingStr, strIndex, fileline);
				/* done. */
			}
			// �ؽ�Ʈ �Է�
			else
			{
				//printf("%c == ����\n", readingStr[0]);///dbg
				/* line ���� ���� */
				if (strncmp(readingStr, "line=", 5) == 0)
				{
					strIndex = skiptodata(readingStr, 0);
					//printf("```line����\n\n");///dbg
					if (!(readingStr[5] >= 48 && readingStr[5] <= 57))
					{
						inputError(fileline, readingStr, strIndex, "<ȣ��>");
						printf("strindex = %d\n", strIndex);//dbg
					}
					else
					{
						printf("strindex = %d, strchar = \'%c\'\n", strIndex, readingStr[strIndex]);//dbg
						printf("����\n");//dbg
						update_linedata(settedLine);
						settedLine = readNum(readingStr, strIndex, fileline);
						/* done. */
					}
				}
				/* �� ��� �߰� */
				else
				{
					//printf("```���߰�\n\n");///dbg
					if (prevInput == 1)
					{
						/* ������ ���� �Է����� �ڵ� �����ϴ� ���̽� */
						readString(readingStr, strIndex, fileline, d->name2);
						add_node(g, d->name2, settedLine);
						d->line2 = settedLine;
						printf("\n\n%s %d %s %d %d %d�� edge insert �õ�.\n", d->name1, d->line1, d->name2, d->line2, d->weight1, d->weight2);///dbg
						printf("hasing return %d %d\n", hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2));///dbg
						insert_edge(g, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
						printf("\n�ڵ� ����. \n");

						/* ���� �����͸� ����� ����. */
						strcpy(d->name1, d->name2);
						d->line1 = d->line2;

					}
					else
					{
						/* ��常 �߰��ϴ� ���̽� */
						readString(readingStr, strIndex, fileline, d->name1);
						d->line1 = settedLine;
						update_linedata(settedLine);
						add_node(g, d->name1, d->line1);	//done.
					}
					prevInput = 0;
				}
			}
		}
		fileline++;
	}

	fclose(readfile);
	return failCount;
}

/*
int main()
{
	char str = '1';
	printf("%d\n", (int)str - 48);	//expect 1, confirmed.

	printf("alpha\n");
	goto test;
	printf("brava\n");
test:

	printf("chalri\n");
}

*/


void print_all_nodes(GraphType* g)
{
	int count = 0;
	int trans = 0;
	printf(":: ����� �� ��� ::\n");
	for (int i = 0; i < g->n; i++)
	{
		if (g->statiList[i] != NULL)//&& g->statiList[i]->stationId != 0
		{
			if (g->statiList[i]->lineId == 0)
			{
				printf("[ ȯ�¿� %s ]\n", g->statiList[i]->statiName);
				trans++;
			}
			else
			{
				printf("<%dȣ��, %s>\n", g->statiList[i]->lineId, g->statiList[i]->statiName);
				count++;
			}
		}
	}
	if (count > 0)
		printf(":: %d���� ���� Ž���� (ȯ�¿�: %d��) ::\n", count, trans);
	else
		printf("����� ���� �����ϴ�.\n");
}

void print_nearby(GraphType* g, char name[], int line)
{
	int index = hashing_get_index(name, line);
	if (index < 0)
	{
		printf(":: ���� ã�� �� �����ϴ� ::\n(%s %d)\n\n", name, line);
		return;
	}
	printf("\n\n:: %s �ֺ� ��� ::\n{ ", g->statiList[index]->statiName);
	for (edgeType* p = g->statiList[index]->head; p != NULL; p = p->link)
		printf("%s(%d), ", g->statiList[p->dest]->statiName, g->statiList[p->dest]->lineId);
	printf("}\n\n");
}

int main()
{
	printf("stationData.txt(ANSI) ������ �о���̴� ���Դϴ�...\n");
	GraphType* subway = (GraphType*)malloc(sizeof(GraphType));
	init_graph(subway);
	if (subway == NULL)
	{
		perror("�޸� ����");
		exit(1);
	}
	inputData* d = (inputData*)malloc(sizeof(inputData));
	if (d == NULL)
	{
		perror("�޸� ����");
		exit(1);
	}
	init_inputData(d);

	int failCount = file(subway);
	if (failCount == 0)
		printf("\n\n:: ���� �о���̱� �Ϸ�! ::\n");
	else
		printf("������ ��� ���������� �о������ ���߽��ϴ�. ���� Ƚ��: %d��\n\n", failCount);

	char buffer[MAX_READ] = { '\0' };
	int temp = -1;
	int seledtWeight = 0;


	char todo[99];
	while (1)
	{
		printf("\n������ �ұ��?\n- �� �߰�:\t(a)\n- �� ����:\t(d)\n- �� �� ����:\t(c)\n\n- ���� �� ã��:\t(q)\n- �ؽ����̺�:\t(x)\n\n- ��� ã��:\t(s)\n- ��� �� ����:\t(v)\n\n- �����ϱ�:\t(e)\n");

	try_again:
		printf("\n>>> ");
		scanf(" %s", &todo);

		/* ��� �߰� */
		if (todo[0] == 'a')
		{
			printf(":: �� �߰� ::\n");
			printf("�� �̸��� �����ΰ���? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("�� ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			if (temp > 0)
			{
				d->line1 = temp;
				add_node(subway, d->name1, d->line1);
				printf("\n(%s, %dȣ��)�� �߰��մϴ�.\n", d->name1, d->line1);
			}
			else
				printf("ȣ�� �Է��� �߸��Ǿ����ϴ�.\n");
		}
		/* ��� ���� */
		else if (todo[0] == 'd')
		{
			printf(":: ��� ���� ::\n");
			printf("������ �� �̸��� �Է��ϼ��� : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("�� ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			d->line1 = temp;

			if (delete_node(subway, d->name1, d->line1) == TRUE)
				printf("\n(%s, %dȣ��)�� �����߽��ϴ�.\n", d->name1, d->line1);
			else
				printf("\n(%s, %dȣ��)�� ã�� �� �����ϴ�.\n", d->name1, d->line1);

		}
		/* ���� ���� */
		else if (todo[0] == 'c')
		{
			printf(":: ���� ���� ::\n");
			printf("ù��° �� �̸��� �����ΰ���? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("ù��° ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			d->line1 = temp;
			printf("�ι�° �� �̸��� �����ΰ���? : ");
			scanf(" %s", &buffer);
			strcpy(d->name2, buffer);
			printf("�ι�° ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			d->line2 = temp;
			printf("�� ���� ���� ù ��° ����ġ�� ���ΰ���? : ");
			scanf(" %d", &temp);
			d->weight1 = temp;
			printf("�� ���� ���� �� ��° ����ġ�� ���ΰ���? : ");
			scanf(" %d", &temp);
			d->weight2 = temp;
			if (d->line1 > 0 && d->line2 > 0)
				insert_edge(subway, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
			else
				printf("ȣ�� �Է��� �߸��Ǿ����ϴ�.\n");

		}
		/* �ִ� ��� ã�� */
		else if (todo[0] == 's')
		{
			printf(":: �ִ� ��� ã�� ::\n");
			printf("ù��° �� �̸��� �����ΰ���? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("ù��° ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			d->line1 = temp;
			printf("�ι�° �� �̸��� �����ΰ���? : ");
			scanf(" %s", &buffer);
			strcpy(d->name2, buffer);
			printf("�ι�° ���� ��ȣ���ΰ���? : ");
			scanf(" %d", &temp);
			d->line2 = temp;
			printf("����ġ�� ������ �ּ���(1 �Ǵ� 2) : ");
			scanf(" %d", &temp);
			d->weight1 = temp;

			if (strcmp(subway->statiList[hashing_get_index(d->name1, d->line1)]->statiName, d->name1) == 0 &&
				strcmp(subway->statiList[hashing_get_index(d->name2, d->line2)]->statiName, d->name2) == 0 &&
				d->line1 > 0 && d->line2 > 0)
			{
				int tmp1 = hashing_get_index(d->name1, d->line1); int tmp2 = hashing_get_index(d->name2, d->line2);
				int distance = INF;
				distance = dijkstra(subway, tmp1, tmp2, d->weight1);
				printf("\n\n\n:: Ž�� �Ϸ�! ::\n<%s>, <%s> �� �Ÿ��� %d�Դϴ�.\n", d->name1, d->name2, distance);
				printf("[���)---(");
				print_path(prev, tmp1, tmp2, subway, 1); // Copilot
				printf("����]\n\n");
			}
			else
				printf(":: ������ �߻��߽��ϴ�. ���� ã�� �� ���ų� ȣ�� �Է��� �߸��Ǿ����ϴ�. ::\n");

		}
		/* ��� �� ���� */
		else if (todo[0] == 'v')
		{
			printf(":: ��� �� ���� ::\n");
			print_all_nodes(subway);
		}
		/* ���� */
		else if (todo[0] == 'e')
		{
			printf("���α׷��� �����մϴ�.\n");
			return TRUE;
		}
		else if (todo[0] == 'q')
		{
			printf(":: �ֺ�Ȯ�� ::\n");
			printf("�� �̸�: ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("�� ȣ��: ");
			scanf(" %d", &temp);
			d->line1 = temp;
			print_nearby(subway, d->name1, d->line1);
		}
		else if (todo[0] == 'x')
		{
			print_hashTable();
		}
		else
		{
			printf("�� �� ���� ��ɾ��Դϴ�. �ٽ� �Է��� �ּ���.\n");
			goto try_again;
		}

	}

	return;
}



/*
* TDL: 0ȣ�� �ڵ����� �ý����� ���� ���ߴ� �̰͸��ϸ� �ϼ�.
* insert edge�Լ����� �̹� �ý����� �뷫������ �����س����Ƿ�
* ��带 �߰��� �� ��� �����ؾ� �� �� �����ϱ⸸ �ϸ� ��.
*
* �̹� 2�� �̻��� ��尡 �־ 0ȣ�� �߾�ȯ�¿��� �����ߴٸ� �̸�+0ȣ������ �ؽ� �� ���� �� ���� �Ǵ°ǵ�
* �� ó���� �� ��° ��尡 �߰��� �� ��� ������ �ؾ� �ϴ� ���ΰ�?
*
* �̰� �� �� ��尡 �߰��� ������ n����ŭ ���� Ž���� �ϴ� �� ������ �� �ƴѰ�?
* O(n2)�� �Ǿ�����µ�?
*
* [1] ȣ�� ������ �̸� �������� ��������� ����� �����Ƿ�, ���ݱ��� �Էµ� ȣ���� ���� �����ϴ°� ���
* ������ �� ������
* ȣ�� ���� ��� ��¦ ��ٷο� �� ���ٴ� ����?
*
* [2] ��� ��忡 ���� 0ȣ���� ����������.
* ��- ��������Ʈ�� Ž�� Ƚ���� �þ�� ����? ����
* bfs�� ���ɻ� �̽��� ���� ��
*
* >>1
* ����ü �ϳ����� ���� �����ϰ� line�Է½� ���Ÿ� �ϰ� ���δ� �Ȱǵ帮���
*

*/

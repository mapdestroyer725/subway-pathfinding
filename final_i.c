#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#pragma warning(disable:6031)
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0

int lineList[99] = { 1 };	//전역변수는 나머지 모두 0초기화
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
* 그래프, 노드, 간선을 모두 사용
* 다호선을 한 그래프에 묶어 환승 처리에 유용하도록 함
* 환승 역은 0호선의 가상 역을 중심으로 가중치 0으로 연결됨.
*/


#define MAX_VERTICES 1009	//역(노드) 번호 저장;은 그래프의 배열에다 함. 소수 1009.
//정점 크기 = 테이블 크기

//간선. 역 간 가중치 저장
typedef struct edgeType
{
	int weight1, weight2;	//1)가중치들
	int dest;				//2)목적지 노드의 고유번호
	struct edgeType* link;	//3)인접리스트 형성을 위한 링크.

} edgeType;

//역 노드. 인접 역은 간선에 저장되어 있으며 간선에 저장된 역 번호를 찾으면 됨
typedef struct statiNode
{
	int lineId;
	int stationId;			//역 고유 번호. 이 번호대로 그래프의 인덱스에 저장됨. 
	char statiName[99];
	struct edgeType* head;	//연결 리스트로 인접한 모든 역(간선)을 저장.

} statiNode;

//역 정보 저장 담당. 호선별로 구분하지 않고 모든 역을 하나에 담기로 함.
typedef struct GraphType
{
	int n;							//역의 총 개수
	statiNode* statiList[MAX_VERTICES];	//인덱스 = 고유번호.

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
*  TDL: 노드 삽입 시 자동 db정렬 및 중복추가 방지
*/


/*
* 해시맵을 이용해서 정렬을 함
* 역 이름을 해시맵 입력으로 받아 테이블에 저장, 해당 테이블에
* 노드 고유번호를 저장하여 접근하도록 함
*
*
*/


/*
* 현재
* hash_key -> hashedId -> g->statiList[hashedId] ...
*
* 변경: 해시 체이닝 추가
* hash_key -> hashedId -> hashTable[hashedId] -> hashChain next ~~ index -> g->statiList[index] ...
*
*/

typedef struct hashChain
{
	char name[99];	//
	int line;		//역 이름+라인: 탐색을 위한 최소 정보
	int index;		//데이터 접근 인덱스: 역 목록에 들어가 세부 데이터들에 접근하도록 함
	struct hashChain* chain;
} hashChain;

hashChain* hashTable[MAX_VERTICES];

// Copilot
void print_hashTable()
{
	printf(":: 해시 테이블 내용 ::\n");
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
	int h = 0;	//혹시몰라 unsigned 삭제
	while (*name)
		h = ((h * 31 + *name++) + MAX_VERTICES) % MAX_VERTICES;		//x31 곱과 현재 한글의 유니코드를 더해 해싱

	h = ((h * 31 + statiLine) + MAX_VERTICES) % MAX_VERTICES;		//노선 번호도 해싱에 사용
	if (h < 0)
	{
		printf("해시값 오류 %d\n", h);
		exit(1);
	}
	return h;
}
int hashing_get_index(const char* name, int statiLine)
{
	int h = hash(name, statiLine);
	printf("\nh=%d\n탐색하려는거\t%s %d\nDB\t\t", h, name, statiLine);

	for (hashChain* p = hashTable[h]; p != NULL; p = p->chain)
	{
		printf("%s %d->", p->name, p->line);
		if (strcmp(p->name, name) == 0 && p->line == statiLine)
		{
			printf("\n```hashing success %d\n", h);
			return p->index;	//탐색 성공, 데이터에 접근 가능한 인덱스를 반환.
		}
	}
	printf(";\n```search failed %d\n\n", -1 * h);
	return -1 * h;				//탐색에 실패, 해시테이블의 인덱스의 음의곱을 반환
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
	listIndex = hashing_get_index(name, statiLine);	//음수: 중복되는 노드 없음.
	printf("```addnode listindex %d\n", listIndex);
	//노선번호, 역이름이 동일하다면 중복
	if (listIndex >= 0)
	{
		printf("노드 중복 발생(%s) \n", name);
		exit(1);
	}

	int hashIndex = -1 * listIndex;		// 음수곱
	/* list DB에 노드 추가 */
	statiNode* newNode = (statiNode*)malloc(sizeof(statiNode));
	if (newNode == NULL)
	{
		perror("메모리 부족");
		exit(1);
	}
	newNode->lineId = statiLine;
	newNode->stationId = g->n;
	strcpy(newNode->statiName, name);
	newNode->head = NULL;
	g->statiList[g->n] = newNode;		// 새 노드의 저장 인덱스는 리스트의 크기 값:g->n.

	/* 해시 테이블에도 업데이트해줌 */
	hashChain* newChain = (hashChain*)malloc(sizeof(hashChain));
	if (newChain == NULL)
	{
		perror("메모리 부족");
		exit(1);
	}
	// 새 체인에 데이터 저장
	newChain->line = statiLine;
	strcpy(newChain->name, name);
	newChain->index = g->n;			// 리스트의 노드 인덱스 g->n을 가리킴.
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
	// 해시테이블 체인 맨 앞 삽입.
	hashChain* p = hashTable[hashIndex];
	if (p == NULL)
	{
		hashTable[hashIndex] = newChain;
		printf("체인 덮어쓰기.\n");
	}
	else
	{
		//p = p->chain; // 틀림.
		hashTable[hashIndex] = newChain;
		newChain->chain = p;
		printf("체인 맨 앞에 삽입.\n");
	}

	printf("노드 추가 성공 list=%d table=%d index=%d\n", g->n, hashIndex, hashTable[hashIndex]->index);///dbg
	g->n++;	// 완료 전에 위치해야 함.
	// 노드 추가 완.


	if (statiLine != 0)	//0호선이면 추가만 해야 함.
	{
		/*
		* 이제부터는 이름이 같은 노드가 있는지 확인
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

		// 약간의 스파게티. 까르보나라 먹고싶다

		// 1: 0호선으로 탐색
		int zeroi = -1;
		zeroi = hashing_get_index(name, 0);

		if (zeroi >= 0)
			goto multi;
		// 2: 라인리스트를 돌면서 겹치는 노드가 있는지 확인. 다만 현재 라인과는 달라야 함.
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
		printf("\n\n0호선 연결 <l%d h%d %s %d>, <l%d h%d %s %d>\n", g->n - 1, hashIndex, name, statiLine, zeroi, hash(g->statiList[zeroi]->statiName, g->statiList[zeroi]->lineId), g->statiList[zeroi]->statiName, g->statiList[zeroi]->lineId);
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
* 간선을 양방향 끊어야 하니 일단 노드 링크를 타고 가 도착지(주변지점)에서
* 거기에 있는 링크 중 삭제하려는 노드와 일치하는 곳을 제거하고
* 링크를 모두 끊어낸 후에 현재 노드를 삭제하도록 한다
*
* node -> edge-
*
*/
int delete_node(GraphType* g, char name[], int statiLine)
{
	int listIndex;
	listIndex = hashing_get_index(name, statiLine);	//음수: 중복되는 노드 없음.
	if (listIndex < 0)
		return FALSE;

	/* 노드와 연결된 주변 노드로부터의 간선 제거 */
	edgeType* this = g->statiList[listIndex]->head;
	edgeType* p = NULL;
	edgeType* tofree = NULL;
	while (this != NULL)
	{
		if (this)
			printf("new this\n");
		printf("@this%d ", this->dest);
		p = g->statiList[this->dest]->head;		// p는 주변 노드.
		while (p != NULL)
		{
			printf("@dest%d ", p->dest);
			// 주변 노드에서 연결된 간선이 삭제하려는 노드라면
			if (p->dest == listIndex)
			{
				tofree = p;
				p = p->link;
				free(tofree);
				printf("found,free!\n");
				break;
			}
			//아니라면 주변 노드의 간선 순환
			p = p->link;
		}
		/*
		* 주변 노드 하나에 대해 완료
		* 이제 다른 주변 노드를 찾자
		*/

		tofree = this;
		this = this->link;
		//free(tofree);	//삭제하려는 노드의 간선 제거.
		//if (this == NULL)
		//	printf("this is NULL!\n");
		//else
		//	printf("this is not NULL!\n");
	}
	/*
	* 주변 노드의 모든 연결된 간선 제거 완료
	* 삭제하려는 노드에 연결된 모든 간선 제거 완료.
	*
	* 이제 할 일은 삭제하려는 노드를 삭제하는 것
	* 역 리스트에서와 해시 테이블에서 제거
	*/

	free(g->statiList[listIndex]);					// 노드 해제
	g->statiList[listIndex] = NULL;					// 역 리스트에서 제거
	return remove_from_hashTable(name, statiLine);	// 해시 테이블에서 제거
}


void insert_edge(GraphType* g, int id_fst, int id_snd, int w_fst, int w_snd)
{
	printf("\n()insert edge called\n");
	if (id_fst == id_snd)
	{
		printf("잘못된 간선 추가 발생(두 노드가 동일): (%d, %d, %d, %d)\n", id_fst, id_snd, w_fst, w_snd);
		return;
	}

	printf("fst = %d, snd = %d\n", id_fst, id_snd);

	edgeType* newEdge = NULL;	//새 간선 포인터. 혹시모를 중복 방지를 위해 미리 선언.

	/* 간선 연결 작업 */
	//호선이 같다면 그대로 서로 연결

	if (g->statiList[id_fst]->lineId == g->statiList[id_snd]->lineId)
	{
		printf("호선이 같다.\n");
		//1차 연결. 단방향 연결 fst->snd
		newEdge = (edgeType*)malloc(sizeof(edgeType));
		if (newEdge == NULL)
		{
			perror("메모리 부족");
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

		//2차 연결. 무방향 연결 snd->fst
		newEdge = (edgeType*)malloc(sizeof(edgeType));
		if (newEdge == NULL)
		{
			perror("메모리 부족");
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
	//호선이 다르면
	else
	{
		printf("호선이 다르다.\n");
		// 두 역의 이름이 같아야 함
		if (strcmp(g->statiList[id_fst]->statiName, g->statiList[id_snd]->statiName) == 0)
		{
			printf("두 역의 이름이 같다.\n");
			// 두 역의 이름이 같고 두 번째 호선 번호가 0번이면 중앙환승역과 자동 연결 케이스임.
			if (g->statiList[id_snd]->stationId == 0)
			{
				//1차 연결. 단방향 연결 fst->snd
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
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

				//2차 연결. 무방향 연결 snd->fst
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
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
				*  중앙환승역을 이미 생성했는가?
				*  역 번호의 이름과 0호선을 해시 키로 해싱 결과의 인덱스 역의
				*  호선이 존재하지 않는다면 중앙환승역을 추가한다.
				*/
				printf("중앙환승역이 생성되어있는지 확인을 해 보자.\n");
				//printf("%d", hashing_get_index(g->statiList[id_fst]->statiName, 0));
				int trans = -1;
				if (hashing_get_index(g->statiList[id_fst]->statiName, 0) < 0)
					trans = add_node(g, g->statiList[id_fst]->statiName, 0);	//중앙환승역 추가.
				else
					trans = g->statiList[hashing_get_index(g->statiList[id_fst]->statiName, 0)]->stationId;	//여기서 오류가 난다=음수인덱스다=해시테이블에 등록이 되어있지 않다.0중앙환승역이 해시테이블에 저장되지 않고 함수가 실행되는 문제 발생.

				/* 중앙환승역과 두 역을 연결 */
				//중앙 --> 첫째역
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
					exit(1);
				}
				newEdge->dest = id_fst;						//목적지: 첫째역
				newEdge->weight1 = 0;						//
				newEdge->weight2 = 0;						//환승역 가중치는 0.
				newEdge->link = NULL;

				if (g->statiList[trans]->head == NULL)
					g->statiList[trans]->head = newEdge;
				else
				{
					edgeType* p = g->statiList[trans]->head;	//
					g->statiList[trans]->head = newEdge;		//
					newEdge->link = p;							//출발지: 중앙환승역
				}

				//중앙 <-> 첫째역
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
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

				//중앙 --> 둘째역
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
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

				//중앙 <-> 둘째역
				newEdge = (edgeType*)malloc(sizeof(edgeType));
				if (newEdge == NULL)
				{
					perror("메모리 부족");
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
		//두 역의 이름이 다르면 잘못된 케이스임
		else
			printf("\n:: [!] 잘못된 간선 연결: 호선이 다른데 두 역의 이름이 다릅니다. ::\n");

	}
	printf("간선 연결 종료\n");
}




/* 대충 노드, 간선은 끝난 것 같음 */

/* dijkstra 알고리즘 */



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
		printf("가중치 선택 오류\n");
		exit(1);
	}

	int i, u, w;
	/* 초기화 */
	for (i = 0; i < g->n; i++)
	{
		distance[i] = INF;	// 주변 외 나머지까지 모두 먼저 초기화
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


	/* 탐색 시작 */
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
			printf("u가 널\n");
			break;
		}
		else
			printf("u가 널이 아님\n");

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
			return distance[endId];		//탐색에 성공. 최단 거리
	}

	return FALSE;				//탐색에 실패
}

// 경로 출력 함수 Copilot
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
	printf("\n[!] 데이터 입력 오류(줄: %d):\n%s", fileline, readingStr);
	for (int i = 0; i < strIndex; i++)
		printf(" ");
	printf("↑...여기, 기대값: %s\n\n", expected);
	exit(1);//dbg
}

int readNum(const char readingStr[], int strIndex, int fileline)
{
	int temp = 0;
	int _strIndex = strIndex;
	int _fileline = fileline;
	if (!(readingStr[strIndex] >= 48 && readingStr[strIndex] <= 57))
		inputError(_fileline, readingStr, _strIndex, "<호선: 자연수값.>");
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
		/* 템플릿 파일 생성 */
		FILE* writefile = fopen("stationData.txt", "w");
		if (writefile == NULL)
		{
			perror("파일을 생성할 수 없습니다.\n");
			exit(1);
		}

		const char* text =
			"#\n"
			"# 다음과 같이 역을 추가할 수 있습니다.\n"
			"#\n"
			"#line=<호선>\n"
			"#<역 이름>\n"
			"#<가중치1>, <가중치2>\n"
			"#<역 이름>\n"
			"#<가중치1>, <가중치2>\n"
			"#<역 이름>"
			"#\n"
			"#<역 이름>\n"
			"#line=<호선>\n"
			"#<역 이름>\n"
			"#\n"
			"#<역 이름>\n"
			"#<역 이름>\n"
			"#/connect(<호선1>, <역 이름1>, <호선2>, <역 이름2>, <가중치1>, <가중치2>)\n"
			"#\n"
			"# 규칙1.역 사이에 가중치를 두면 두 역은 자동 연결됩니다.\n"
			"# 규칙2.역 이름이 동일하고 호선이 다른 역은 자동으로 환승역으로써 연결됩니다.\n"
			"# 규칙3.추가적으로 연결하고 싶은 역은 /connect() 명령어를 사용하세요.\n"
			"# 규칙4.호선 설정 기본값은 1호선이며 자연수만 입력 가능합니다.\n"
			"# Notice: 역 이름에 띄어쓰기는 모두 사라집니다.\n"
			"#\n";

		fputs(text, writefile);
		fclose(writefile);
		printf("파일을 찾지 못했습니다. 새로운 템플릿의 파일 stationData.txt를 생성하였습니다. 파일 형식이 ANSI인지 확인해주세요.\n");
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
	* 전 입력	0 = 공백.
	*			0 = line.
	*			0 = 문자열.
	*			1 = 정수.		--> 다음 문자열(역이름) **필. 공백, line, /connect이면 에러.
	*			0 = /connect.
	*/

	inputData* d = (inputData*)malloc(sizeof(inputData));
	if (d == NULL)
	{
		perror("메모리 부족");
		exit(1);
	}
	init_inputData(d);
	int settedLine = 1;

	/* buffer에서 주석 줄과 모든 띄어쓰기를 제거함 */
	while (fgets(buffer, sizeof(readingStr), readfile))
	{
		/*
		* TDL: 코드가 주석 처리를 무시하지 못하고 있음.
		*		-> 정상처리하고 있음. 다만 종료를 제대로 못 하고 있음.
		*			줄 끝에 \n, \0감지로 제대로 종료하는걸 구축.
		*/

		/* 띄어쓰기 제거 -> readingStr에 저장
		*  줄바꿈은 구분을 위해 냅둬야 함 */
		bufferindex = strIndex = 0;
		while (buffer[bufferindex] != '\0')	// 줄바꿈은 냅둠
		{
			if (buffer[bufferindex] != ' ' && buffer[bufferindex] != '\t')
				readingStr[strIndex++] = buffer[bufferindex];
			bufferindex++;
		}
		readingStr[strIndex] = '\0';		//마지막 종료코드.

		strIndex = 0;

		/* 명령어 읽어들이기 시작 */
		// 주석 줄이 아니라면
		printf("```readingstr = %s\n", readingStr);///dbg
		if (readingStr[0] != '#')
		{
			//printf("%c != #\n", readingStr[0]);///dbg
			/* 입력을 일단 정확히 구분하고 본다 */
			// 공백 입력
			if (readingStr[0] == '\n' || readingStr[0] == '\0')
			{
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<역 이름>");
				prevInput = 0;
				/* ☆취할 행동 없음☆ */
			}
			// connect 명령어 입력
			else if (readingStr[0] == '/')
			{
				//printf("%c == /\n", readingStr[0]);///dbg
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<역 이름>");
				prevInput = 0;

				/* /connect(<호선1>, <역이름1>,  <호선2>, <역이름2>, <가중치1>, <가중치2>) */
				// <호선1> - int
				strIndex = skiptodata(readingStr, strIndex);	// ..connect(.. 날림용.
				d->line1 = readNum(readingStr, strIndex, fileline);

				// <역이름1> - char
				strIndex = skiptodata(readingStr, strIndex);
				readString(readingStr, strIndex, fileline, d->name1);

				// <호선2> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->line2 = readNum(readingStr, strIndex, fileline);

				// <역이름2> - char
				strIndex = skiptodata(readingStr, strIndex);
				readString(readingStr, strIndex, fileline, d->name2);

				// <가중치1> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->weight1 = readNum(readingStr, strIndex, fileline);

				// <가중치2> - int
				strIndex = skiptodata(readingStr, strIndex);
				d->weight2 = readNum(readingStr, strIndex, fileline);


				//insert_edge(g, add_node(g, d->name1, d->line1), add_node(g, d->name2, d->line2), d->weight1, d->weight2);
				insert_edge(g, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
				/* done. */
			}
			// 숫자 입력
			else if (readingStr[0] >= 48 && readingStr[0] <= 57)
			{
				//printf("%c == 0123456789\n", readingStr[0]);///dbg
				if (prevInput == 1)
					inputError(fileline, readingStr, 0, "<역 이름>");
				prevInput = 1;

				// <가중치1, 2> - int
				strIndex = 0;
				d->weight1 = readNum(readingStr, strIndex, fileline);
				strIndex = skiptodata(readingStr, strIndex);
				d->weight2 = readNum(readingStr, strIndex, fileline);
				/* done. */
			}
			// 텍스트 입력
			else
			{
				//printf("%c == 문자\n", readingStr[0]);///dbg
				/* line 변경 명렁어 */
				if (strncmp(readingStr, "line=", 5) == 0)
				{
					strIndex = skiptodata(readingStr, 0);
					//printf("```line변경\n\n");///dbg
					if (!(readingStr[5] >= 48 && readingStr[5] <= 57))
					{
						inputError(fileline, readingStr, strIndex, "<호선>");
						printf("strindex = %d\n", strIndex);//dbg
					}
					else
					{
						printf("strindex = %d, strchar = \'%c\'\n", strIndex, readingStr[strIndex]);//dbg
						printf("정상\n");//dbg
						update_linedata(settedLine);
						settedLine = readNum(readingStr, strIndex, fileline);
						/* done. */
					}
				}
				/* 역 노드 추가 */
				else
				{
					//printf("```역추가\n\n");///dbg
					if (prevInput == 1)
					{
						/* 이전에 간선 입력으로 자동 연결하는 케이스 */
						readString(readingStr, strIndex, fileline, d->name2);
						add_node(g, d->name2, settedLine);
						d->line2 = settedLine;
						printf("\n\n%s %d %s %d %d %d로 edge insert 시도.\n", d->name1, d->line1, d->name2, d->line2, d->weight1, d->weight2);///dbg
						printf("hasing return %d %d\n", hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2));///dbg
						insert_edge(g, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
						printf("\n자동 연결. \n");

						/* 이전 데이터를 현재로 갱신. */
						strcpy(d->name1, d->name2);
						d->line1 = d->line2;

					}
					else
					{
						/* 노드만 추가하는 케이스 */
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
	printf(":: 저장된 역 목록 ::\n");
	for (int i = 0; i < g->n; i++)
	{
		if (g->statiList[i] != NULL)//&& g->statiList[i]->stationId != 0
		{
			if (g->statiList[i]->lineId == 0)
			{
				printf("[ 환승역 %s ]\n", g->statiList[i]->statiName);
				trans++;
			}
			else
			{
				printf("<%d호선, %s>\n", g->statiList[i]->lineId, g->statiList[i]->statiName);
				count++;
			}
		}
	}
	if (count > 0)
		printf(":: %d개의 역이 탐색됨 (환승역: %d개) ::\n", count, trans);
	else
		printf("저장된 역이 없습니다.\n");
}

void print_nearby(GraphType* g, char name[], int line)
{
	int index = hashing_get_index(name, line);
	if (index < 0)
	{
		printf(":: 역을 찾을 수 없습니다 ::\n(%s %d)\n\n", name, line);
		return;
	}
	printf("\n\n:: %s 주변 노드 ::\n{ ", g->statiList[index]->statiName);
	for (edgeType* p = g->statiList[index]->head; p != NULL; p = p->link)
		printf("%s(%d), ", g->statiList[p->dest]->statiName, g->statiList[p->dest]->lineId);
	printf("}\n\n");
}

int main()
{
	printf("stationData.txt(ANSI) 파일을 읽어들이는 중입니다...\n");
	GraphType* subway = (GraphType*)malloc(sizeof(GraphType));
	init_graph(subway);
	if (subway == NULL)
	{
		perror("메모리 부족");
		exit(1);
	}
	inputData* d = (inputData*)malloc(sizeof(inputData));
	if (d == NULL)
	{
		perror("메모리 부족");
		exit(1);
	}
	init_inputData(d);

	int failCount = file(subway);
	if (failCount == 0)
		printf("\n\n:: 파일 읽어들이기 완료! ::\n");
	else
		printf("파일을 모두 정상적으로 읽어들이지 못했습니다. 실패 횟수: %d번\n\n", failCount);

	char buffer[MAX_READ] = { '\0' };
	int temp = -1;
	int seledtWeight = 0;


	char todo[99];
	while (1)
	{
		printf("\n무엇을 할까요?\n- 역 추가:\t(a)\n- 역 삭제:\t(d)\n- 역 간 연결:\t(c)\n\n- 인접 역 찾기:\t(q)\n- 해시테이블:\t(x)\n\n- 경로 찾기:\t(s)\n- 모든 역 보기:\t(v)\n\n- 종료하기:\t(e)\n");

	try_again:
		printf("\n>>> ");
		scanf(" %s", &todo);

		/* 노드 추가 */
		if (todo[0] == 'a')
		{
			printf(":: 역 추가 ::\n");
			printf("역 이름이 무엇인가요? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("이 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			if (temp > 0)
			{
				d->line1 = temp;
				add_node(subway, d->name1, d->line1);
				printf("\n(%s, %d호선)을 추가합니다.\n", d->name1, d->line1);
			}
			else
				printf("호선 입력이 잘못되었습니다.\n");
		}
		/* 노드 삭제 */
		else if (todo[0] == 'd')
		{
			printf(":: 노드 삭제 ::\n");
			printf("삭제할 역 이름을 입력하세요 : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("이 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			d->line1 = temp;

			if (delete_node(subway, d->name1, d->line1) == TRUE)
				printf("\n(%s, %d호선)을 삭제했습니다.\n", d->name1, d->line1);
			else
				printf("\n(%s, %d호선)을 찾을 수 없습니다.\n", d->name1, d->line1);

		}
		/* 간선 연결 */
		else if (todo[0] == 'c')
		{
			printf(":: 간선 연결 ::\n");
			printf("첫번째 역 이름이 무엇인가요? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("첫번째 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			d->line1 = temp;
			printf("두번째 역 이름이 무엇인가요? : ");
			scanf(" %s", &buffer);
			strcpy(d->name2, buffer);
			printf("두번째 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			d->line2 = temp;
			printf("두 역에 대한 첫 번째 가중치는 얼마인가요? : ");
			scanf(" %d", &temp);
			d->weight1 = temp;
			printf("두 역에 대한 두 번째 가중치는 얼마인가요? : ");
			scanf(" %d", &temp);
			d->weight2 = temp;
			if (d->line1 > 0 && d->line2 > 0)
				insert_edge(subway, hashing_get_index(d->name1, d->line1), hashing_get_index(d->name2, d->line2), d->weight1, d->weight2);
			else
				printf("호선 입력이 잘못되었습니다.\n");

		}
		/* 최단 경로 찾기 */
		else if (todo[0] == 's')
		{
			printf(":: 최단 경로 찾기 ::\n");
			printf("첫번째 역 이름이 무엇인가요? : ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("첫번째 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			d->line1 = temp;
			printf("두번째 역 이름이 무엇인가요? : ");
			scanf(" %s", &buffer);
			strcpy(d->name2, buffer);
			printf("두번째 역은 몇호선인가요? : ");
			scanf(" %d", &temp);
			d->line2 = temp;
			printf("가중치를 선택해 주세요(1 또는 2) : ");
			scanf(" %d", &temp);
			d->weight1 = temp;

			if (strcmp(subway->statiList[hashing_get_index(d->name1, d->line1)]->statiName, d->name1) == 0 &&
				strcmp(subway->statiList[hashing_get_index(d->name2, d->line2)]->statiName, d->name2) == 0 &&
				d->line1 > 0 && d->line2 > 0)
			{
				int tmp1 = hashing_get_index(d->name1, d->line1); int tmp2 = hashing_get_index(d->name2, d->line2);
				int distance = INF;
				distance = dijkstra(subway, tmp1, tmp2, d->weight1);
				printf("\n\n\n:: 탐색 완료! ::\n<%s>, <%s> 간 거리는 %d입니다.\n", d->name1, d->name2, distance);
				printf("[출발)---(");
				print_path(prev, tmp1, tmp2, subway, 1); // Copilot
				printf("도착]\n\n");
			}
			else
				printf(":: 오류가 발생했습니다. 역을 찾을 수 없거나 호선 입력이 잘못되었습니다. ::\n");

		}
		/* 모든 역 보기 */
		else if (todo[0] == 'v')
		{
			printf(":: 모든 역 보기 ::\n");
			print_all_nodes(subway);
		}
		/* 종료 */
		else if (todo[0] == 'e')
		{
			printf("프로그램을 종료합니다.\n");
			return TRUE;
		}
		else if (todo[0] == 'q')
		{
			printf(":: 주변확인 ::\n");
			printf("역 이름: ");
			scanf(" %s", &buffer);
			strcpy(d->name1, buffer);
			printf("역 호선: ");
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
			printf("알 수 없는 명령어입니다. 다시 입력해 주세요.\n");
			goto try_again;
		}

	}

	return;
}



/*
* TDL: 0호선 자동연결 시스템을 구축 안했다 이것만하면 완성.
* insert edge함수에서 이미 시스템을 대략적으로 구축해놨으므로
* 노드를 추가할 때 어떻게 감지해야 할 지 생각하기만 하면 됨.
*
* 이미 2개 이상의 노드가 있어서 0호선 중앙환승역을 생성했다면 이름+0호선으로 해싱 한 번만 해 보면 되는건데
* 맨 처음에 두 번째 노드가 추가될 때 어떻게 감지를 해야 하는 것인가?
*
* 이게 매 번 노드가 추가될 때마다 n번만큼 순차 탐색을 하는 건 무리일 거 아닌가?
* O(n2)이 되어버리는데?
*
* [1] 호선 정보는 이름 정보보다 상대적으로 상당히 작으므로, 지금까지 입력된 호선을 따로 저장하는건 어떤가
* 괜찮은 거 같은데
* 호선 정보 제어가 살짝 까다로울 거 같다는 정도?
*
* [2] 모든 노드에 대해 0호선을 만들어버릴까.
* 단- 다이직스트라 탐색 횟수가 늘어난다 정도? 많이
* bfs라 성능상 이슈가 생길 듯
*
* >>1
* 구조체 하나에다 대충 심플하게 line입력시 갱신만 하고 따로는 안건드리기로
*

*/

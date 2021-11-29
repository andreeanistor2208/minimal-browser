// Nistor Andreea Iuliana - 313CB

#include "utils.h"

#define MAX_URL_LENGTH 22 
//googleadservices.com la test4 are 20 + terminator altfel am eroare de memorie
//googleusercontent.com la test6 are 21 + terminator altfel am eroare valgrind

struct webPage
{
	char url[MAX_URL_LENGTH];
	int num_res;
	Resource *resurse;
};


struct nodGeneric
{
	void* continut;
	struct nodGeneric *pnext;
};

struct nodPrioGeneric
{
	void* continut;
	unsigned long prio;
	struct nodPrioGeneric *pnext;
};

struct queue
{
	size_t dime;
	struct nodGeneric *pfirst;
	struct nodGeneric *plast;
};

struct priority_queue
{
	size_t dime;
	struct nodPrioGeneric *pfirst;
	struct nodPrioGeneric *plast;
};

struct stiva
{
	size_t dime;
	struct nodGeneric *vf;
};

struct tab
{
	struct webPage *current_page;
	struct stiva back_stack;
	struct stiva forward_stack;
};

struct nodtab
{
	struct tab Tab;
	struct nodtab* next;
	struct nodtab* prev;
};

typedef struct listatab
{
	struct nodtab* listaTab;
	struct nodtab* tabCurrent;
	struct queue CoadaHistory;
	//coada de prioritati
	struct priority_queue CoadaDownloads;
	struct nodGeneric* CompletedDownloads;
} Browser;

void wait(Browser* explorer, int nr, int bandwidth); //foward declaration

void initQ(struct queue *q, size_t dime)
{
	q->dime = dime;
	q->pfirst = NULL;
	q->plast = NULL;
}

void initPQ(struct priority_queue *q, size_t dime)
{
	q->dime = dime;
	q->pfirst = NULL;
	q->plast = NULL;
}

void initS(struct stiva *s, size_t dime)
{
	s->dime = dime;
	s->vf = NULL;
}

void pushCoada(struct queue *q, struct nodGeneric *nou)
{
	if(q == NULL)
		return;
	if(nou == NULL)
		return;

	if(q->pfirst == NULL && q->plast == NULL)
	{
		q->pfirst = nou;
		q->plast = nou;
		nou->pnext = NULL;
	}
	else
	{
		nou->pnext = q->plast;
		q->plast = nou;

	}
}

void pushPrioCoada(struct priority_queue *pq, struct nodPrioGeneric *nou)
{
	if(pq == NULL)
		return;
	if(nou == NULL)
		return;

	//daca este primul nod
	if(pq->pfirst == NULL && pq->plast == NULL)
	{
		pq->pfirst = nou;
		pq->plast = nou;
		nou->pnext = NULL;
	}
	else
	{
		//daca are prioritate mai mica deccat primul nod se baga primul
		if(nou->prio < pq->pfirst->prio)
		{
			nou->pnext = pq->pfirst;
			pq->pfirst = nou;
		}
		else
		{
			struct nodPrioGeneric *p = pq->pfirst;
			struct nodPrioGeneric *aux = p;
			while(p != NULL && p->prio < nou->prio)
			{
				aux = p;
				p = p->pnext;
			}
			//a ajuns la capat
			if(p == NULL)
			{
				aux->pnext = nou;
				nou->pnext = NULL;
				pq->plast = nou;
			}
			//este intre elemente
			else if(p->prio == nou->prio && p == pq->pfirst)
			{
				aux = p->pnext;
				p->pnext = nou;
				nou->pnext = aux;
			}
			else
			{
				aux->pnext = nou;
				nou->pnext = p;
			}
		}
	}
}

void pushStiva(struct stiva *s, struct nodGeneric *nou)
{
	if(s == NULL)
		return;
	if(nou == NULL)
		return;

	if(s->vf == NULL )
	{
		s->vf = nou;
		nou->pnext = NULL;
	}
	else
	{
		struct nodGeneric *p = s->vf;
		s->vf = nou;
		nou->pnext = p;
	}
}

struct nodGeneric* popCoada(struct queue *q)
{
	if(q == NULL)
		return NULL;
	

	struct nodGeneric *ret;
	ret = q->pfirst;
	//daca am un singur element
	if(q->plast == q->pfirst)
		q->pfirst = q->plast = NULL;
	else
	{
		struct nodGeneric *p = q->plast;
		while(p->pnext != q->pfirst)
			p = p->pnext;

		q->pfirst = p;
		q->pfirst->pnext = NULL;
	}

	return ret;
}

struct nodPrioGeneric* popPrioCoada(struct priority_queue *q)
{
	if(q == NULL)
		return NULL;
	

	struct nodPrioGeneric *ret;
	ret = q->pfirst;
	//daca am un singur element
	if(q->plast == q->pfirst)
		q->pfirst = q->plast = NULL;
	else
	{
		q->pfirst = q->pfirst->pnext;
	}

	return ret;
}

struct nodGeneric* popStiva(struct stiva *s)
{
	if(s == NULL)
		return NULL;
	

	struct nodGeneric *ret;
	ret = s->vf;
	if(s->vf != NULL)
		s->vf = s->vf->pnext;
	
	return ret;
}

void addList(struct nodGeneric **rad, struct nodGeneric *nou)
{
	if(*rad == NULL)
	{
		*rad = nou;
		return;
	}
	else
	{
		struct nodGeneric *p = *rad;
		while(p->pnext != NULL)
			p = p->pnext;

		p->pnext = nou;
	}
}

void elibereazaPagina(struct webPage* p)
{
	if(p  != NULL)
	{
		if(p->resurse == NULL)
		{
			printf("Are resursele nule\n");
		}
		free(p->resurse);
	}
	
	free(p);
}

void elibereazaTab(struct tab TAB)
{
	elibereazaPagina(TAB.current_page);
	
	//curat si cele 2 stive la eliberare memorie
	struct nodGeneric* ret;
	while( (ret = popStiva(&TAB.forward_stack) ) != NULL)
	{
		elibereazaPagina(ret->continut);
		free(ret);
	}
	while( (ret = popStiva(&TAB.back_stack) ) != NULL)
	{
		elibereazaPagina(ret->continut);
		free(ret);
	}
}

void elibereazanodTab(struct nodtab *p)
{
	elibereazaTab(p->Tab);
	free(p);
}

void eliminaUltimNod(Browser* explorer, struct nodtab* lista)
{
	struct nodtab* p = lista;
	//duc p la ultimul nod
	if(p == NULL)
		printf("Intra pentru NULL\n");
	while(p->next != NULL)
		p = p->next;

	struct nodtab* aux = p->prev;

	aux->next = NULL;
	if(explorer->tabCurrent == p)
		explorer->tabCurrent = aux;
	elibereazanodTab(p);
}

void deltab(Browser *explorer)
{
	//daca este singurul tab deschis doar ii curat adresa
	if(explorer->tabCurrent == explorer->listaTab && 
		explorer->listaTab->next == NULL)
	{
		elibereazaPagina(explorer->tabCurrent->Tab.current_page);
		explorer->tabCurrent->Tab.current_page = NULL;
		//resetez/curat cele 2 stackuri
	}
	else
	{
		eliminaUltimNod(explorer, explorer->listaTab);
	}
}

void createTab(Browser *explorer)
{
	//cream un tab nou
	struct nodtab *nou = (struct nodtab*)malloc(sizeof(struct nodtab));
	nou->next = NULL;
	nou->prev = NULL;
	nou->Tab.current_page = NULL;
	initS(&nou->Tab.back_stack, sizeof(struct webPage) );
	initS(&nou->Tab.forward_stack, sizeof(struct webPage) );

	//daca este primul tab
	if(explorer->listaTab == NULL)
	{
		explorer->listaTab = nou;
	}
	//daca am deja taburi deschise
	else
	{
		struct nodtab* p = explorer->listaTab;
		//p trebuie sa arate catre ultimul element
		while(p->next != NULL)
			p = p->next;
		p->next = nou;
		nou->prev = p;
	}
	explorer->tabCurrent = nou;
}

void change_tab(Browser *explorer, int nr)
{
	struct nodtab *p = explorer->listaTab;

	while(nr > 0)
	{
		p = p->next;
		nr--;
	}
	explorer->tabCurrent = p;
	if(p == NULL)
		printf("Seteaza tabul curent la null");
	
}

int set_band(int *bw, int val)
{
	if(bw == NULL)
		return -1;
	*bw = val;
	return *bw;
}

void print_open_tabs(Browser *explorer, FILE *fout)
{
	struct nodtab *p = explorer->listaTab;
	int index = 0;
	while(p != NULL)
	{
		fprintf(fout, "(%d: ", index);
		index++;
		if(p->Tab.current_page == NULL)
			fprintf(fout, "empty)\n");
		else
			fprintf(fout, "%s)\n", p->Tab.current_page->url);

		p = p->next;
	}

}

void gotoPage(Browser *explorer, const char* url, FILE *fout,
										long unsigned bandwidth)
{
	if(explorer == NULL)
		return;

	wait(explorer, 1, bandwidth);

	struct nodGeneric *nou =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
	nou->continut = (char*)malloc(explorer->CoadaHistory.dime);
	strcpy(nou->continut, url);

	pushCoada(&explorer->CoadaHistory, nou);

	struct webPage* pagina_nou =(struct webPage*)malloc(sizeof(struct webPage));
	strcpy(pagina_nou->url, url);

	pagina_nou->resurse = get_page_resources(url, &pagina_nou->num_res);

	//daca este primul tab
	if(explorer->tabCurrent == NULL)	
		createTab(explorer);
	
	if(explorer->tabCurrent->Tab.current_page == NULL)
	{
		explorer->tabCurrent->Tab.current_page = pagina_nou;
	}
	//daca aveam deja o pagina deschisa in acel tab
	else
	{
		struct nodGeneric* nou =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
		nou->continut = explorer->tabCurrent->Tab.current_page;
		nou->pnext = NULL;
		pushStiva(&explorer->tabCurrent->Tab.back_stack, nou);
		//golim stiva de forward
		struct nodGeneric* ret;
		while((ret = popStiva(&explorer->tabCurrent->Tab.forward_stack)) != NULL)
		{
			elibereazaPagina(ret->continut);
			free(ret);
		}

		explorer->tabCurrent->Tab.current_page = pagina_nou;
	}
}

void back(Browser *explorer, FILE *fout)
{
	struct nodGeneric* former =popStiva(&explorer->tabCurrent->Tab.back_stack);
	if(former == NULL)
		fprintf(fout, "can't go back, no pages in the stack\n");
	else
	{
		struct nodGeneric *cur =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
		cur->continut = explorer->tabCurrent->Tab.current_page;
		cur->pnext = NULL;
		pushStiva(&explorer->tabCurrent->Tab.forward_stack, cur);
		explorer->tabCurrent->Tab.current_page = former->continut;
	}
	free(former);
}

void forward(Browser *explorer, FILE *fout)
{
	struct nodGeneric* pre =popStiva(&explorer->tabCurrent->Tab.forward_stack);
	if(pre == NULL)
		fprintf(fout, "can't go back, no pages in the stack\n");
	else
	{
		struct nodGeneric *cur =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
		cur->continut = explorer->tabCurrent->Tab.current_page;
		cur->pnext = NULL;
		pushStiva(&explorer->tabCurrent->Tab.back_stack, cur);
		explorer->tabCurrent->Tab.current_page = pre->continut;
	}
	free(pre);
}

void history(Browser* explorer, FILE *fout)
{
	struct queue temp;
	initQ(&temp,explorer->CoadaHistory.dime);
	//scot din coada de history, bag in a doua coada apoi fac copia la loc
	struct nodGeneric* ret;
	while( (ret = popCoada(&explorer->CoadaHistory)) !=NULL )
	{
		fprintf(fout, "%s\n", (char*)ret->continut);
		pushCoada(&temp, ret);
	}
	explorer->CoadaHistory = temp;
}

void del_history(Browser* explorer, int nr)
{
	struct nodGeneric *ret;
	if(nr == 0)
	{
		//SA VAD CAND ELIBEREZ MEMORIA
		while( (ret = popCoada(&explorer->CoadaHistory) ) != NULL)
		{
			free(ret->continut);
			free(ret);
		}
	}
	else
		while(nr > 0)
		{
			//SA VAD CAND ELIBEREZ MEMORIA
			ret = popCoada(&explorer->CoadaHistory);
			if(ret != NULL)
			{
				free(ret->continut);
				free(ret);
			}
			nr--;
		}
}

void lista_dl(Browser* explorer, FILE* fout)
{
	if(explorer->tabCurrent->Tab.current_page == NULL)
		return;
	for(int i = 0; i < explorer->tabCurrent->Tab.current_page->num_res;i++)
	{
		fprintf(fout, "[%d - \"%s\" : %ld]\n", 
				i, explorer->tabCurrent->Tab.current_page->resurse[i].name,
				explorer->tabCurrent->Tab.current_page->resurse[i].dimension);
	}
}

void download(Browser *explorer, int index)
{
	//verificam daca tabul curent are o pagina deschisa
	if(explorer->tabCurrent->Tab.current_page == NULL)
	{
		return;
	}
	if(explorer->tabCurrent->Tab.current_page->resurse == NULL)
	{
		
		return;
	}
	Resource *res =(Resource*)malloc(sizeof(Resource));
	strcpy(res->name, explorer->tabCurrent->Tab.current_page->resurse[index].name);
	res->dimension = explorer->tabCurrent->Tab.current_page->resurse[index].dimension;
	res->currently_downloaded = explorer->tabCurrent->Tab.current_page->resurse[index].currently_downloaded;

	struct nodPrioGeneric *nou =(struct nodPrioGeneric*)malloc(sizeof(struct nodPrioGeneric));
	nou->continut = res;
	nou->prio = ((Resource*)nou->continut)->dimension - ((Resource*)nou->continut)->currently_downloaded;
	
	nou->pnext = NULL;
	pushPrioCoada(&explorer->CoadaDownloads,nou);
}

void print_downloads(Browser* explorer, FILE *fout)
{
	struct nodPrioGeneric *p = explorer->CoadaDownloads.pfirst;
	
	while(p != NULL)
	{
		fprintf(fout, "[\"%s\" : %ld/%ld]\n", ((Resource*)p->continut)->name, 
			((Resource*)p->continut)->dimension - ((Resource*)p->continut)->currently_downloaded,
								((Resource*)p->continut)->dimension);
		p = p->pnext;
	}
	

	struct nodGeneric *p2 = explorer->CompletedDownloads;
	
	while(p2 != NULL)
	{
		fprintf(fout, "[\"%s\" : completed]\n", ((Resource*)p2->continut)->name);
		p2 = p2->pnext;
	} 
}

void wait(Browser* explorer, int nr, int bandwidth)
{
	

	if(explorer->CoadaDownloads.pfirst != NULL)
		while(nr > 0)
		{
			int dif = ((Resource*)explorer->CoadaDownloads.pfirst->continut)->dimension -
						((Resource*)explorer->CoadaDownloads.pfirst->continut)->currently_downloaded;
			if(dif > 0)
			{
				//vedem daca va termina sau nu downloadul current in urmatoarea
				//secunda si daca va descarca si din downloadul urmator
				if(bandwidth > dif)
				{
					
					int dif_ramas;
					//pentru cazul in care intr-o secunda pot face mai multe
					//downloads mici
					while(bandwidth > dif)
					{
						
						//calculam ce ramane dupa ce umplem vechiul
						dif_ramas = bandwidth - dif;

						struct nodPrioGeneric *p = popPrioCoada(&explorer->CoadaDownloads);
						struct nodGeneric *nou =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
						nou->continut = p->continut;
						nou->pnext = NULL;
						addList(&explorer->CompletedDownloads, nou);
						
						//bandwidth -= dif;

						dif = ((Resource*)explorer->CoadaDownloads.pfirst->continut)->dimension -
							((Resource*)explorer->CoadaDownloads.pfirst->continut)->currently_downloaded;

						free(p);
					}

					((Resource*)explorer->CoadaDownloads.pfirst->continut)->currently_downloaded += dif_ramas;
					
				}
				//daca termina fix urmatorul download
				else if(bandwidth == dif)
				{
					struct nodPrioGeneric *p = popPrioCoada(&explorer->CoadaDownloads);
					struct nodGeneric *nou =(struct nodGeneric*)malloc(sizeof(struct nodGeneric));
					nou->continut = p->continut;
					nou->pnext = NULL;
					addList(&explorer->CompletedDownloads, nou);
					free(p);
				}
				//daca doar avanseaza in downloadul curent
				else
				{
					((Resource*)explorer->CoadaDownloads.pfirst->continut)->currently_downloaded += bandwidth;
					
					
					//updatat prioritatea
					explorer->CoadaDownloads.pfirst->prio = ((Resource*)explorer->CoadaDownloads.pfirst->continut)->dimension
							- ((Resource*)explorer->CoadaDownloads.pfirst->continut)->currently_downloaded;
				}
			}

		nr--;
		}//while nr > 0
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("Fromatul corect: ./tema2 fisier_intrare fisier_iesire\n");
		return 0;
	}

	FILE *fin;
	fin = fopen(argv[1], "r");
	if(fin == NULL)
	{
		printf("Could not open input file\n");
		return 1;
	}

	FILE *fout;
	fout = fopen(argv[2], "w");
	if(fout == NULL)
	{
		printf("Could not open/create output file\n");
		return 2;
	}


	Browser explorer;
	explorer.listaTab = NULL;
	explorer.tabCurrent = NULL;
	//initializam coada de history
	initQ(&explorer.CoadaHistory, sizeof(char) * MAX_URL_LENGTH);
	initPQ(&explorer.CoadaDownloads, sizeof(struct nodPrioGeneric));
	explorer.CompletedDownloads = NULL;
	//cream un tab ptr a avea minim unul
	char * line = NULL;
    size_t len = 0;
    ssize_t read;

    int bandwidth = 1024;
    char *cmd;
    
    while ((read = getline(&line, &len, fin)) != -1) 
    {
    	cmd = strtok(line, " \n");
    	
    	if(strcmp(cmd, "set_band") == 0 )
    	{
    		char *number = strtok(NULL, "\n");
			if(number == NULL)
				printf("Incorrect format. Please give a number for the bandwidth\n");
			else
			{
				int nr = atoi(number);
				set_band(&bandwidth, nr);
			}
    	}

    	if(strcmp(cmd, "newtab") == 0)
    		createTab(&explorer);

    	if(strcmp(cmd, "deltab") == 0)
    		deltab(&explorer);

    	if(strcmp(cmd, "change_tab") == 0)
    	{
    		char *number = strtok(NULL, "\n");
			if(number == NULL)
				printf("Incorrect format. Please give a number for the target tab\n");
			else
			{
				int nr = atoi(number);
				change_tab(&explorer, nr);
			}
    	}

    	if(strcmp(cmd, "print_open_tabs") == 0)
    		print_open_tabs(&explorer, fout);

    	if(strcmp(cmd, "goto") == 0)
    	{
    		char *URL = strtok(NULL, "\n");
    		if(URL == NULL)
    			printf("Incorrect format. Please give the address\n");
    		else
    			gotoPage(&explorer, URL, fout, bandwidth);
    	}

    	if(strcmp(cmd, "back") == 0)
    		back(&explorer, fout);

    	if(strcmp(cmd, "forward") == 0)
    		forward(&explorer, fout);

    	if(strcmp(cmd, "history") == 0)
    		history(&explorer, fout);
    	

    	if(strcmp(cmd, "del_history") == 0)
    	{
    		char *number = strtok(NULL, "\n");
			if(number == NULL)
				printf("Incorrect format. Please give a number for the entries to delete\n");
			else
			{
				int nr = atoi(number);
				del_history(&explorer, nr);
			}
    	}

    	if(strcmp(cmd, "list_dl") == 0)
    		lista_dl(&explorer, fout);

    	if(strcmp(cmd, "download") == 0)
    	{
    		char *number = strtok(NULL, "\n");
			if(number == NULL)
				printf("Incorrect format. Please give a number for the entries to delete\n");
			else
			{
				int nr = atoi(number);
				download(&explorer, nr);
			}
    	}

    	if(strcmp(cmd, "downloads") == 0)
    		print_downloads(&explorer, fout);

    	if(strcmp(cmd, "wait") == 0)
    	{
    		char *number = strtok(NULL, "\n");
			if(number == NULL)
				printf("Incorrect format. Please give a number for the seconds to wait\n");
			else
			{
				int nr = atoi(number);
				wait(&explorer, nr,bandwidth);
			}
    	}
    	
    }

    if (line != NULL)
        free(line);

    //eliberam memoria ramasa
    struct nodtab* p = explorer.listaTab;
    while(p != NULL)
    {
    	struct nodtab* aux = p;
    	p = p->next;;
    	//eliberam aux
    	elibereazanodTab(aux);
    }
    struct nodGeneric *ret;
    while( (ret = popCoada(&explorer.CoadaHistory) ) != NULL)
	{
		free(ret->continut);
		free(ret);
	}

	//elibera coada de downloads
	struct nodPrioGeneric *p2;
	while( (p2 = popPrioCoada(&explorer.CoadaDownloads) ) != NULL)
	{
		free(p2->continut);
		free(p2);
	}
	//eliberam lista de dl terminate
	struct nodGeneric *p3 = explorer.CompletedDownloads;
	while(p3 != NULL)
	{
		struct nodGeneric *aux;
		aux = p3;
		p3 = p3->pnext;
		free(aux->continut);
		free(aux);
	}


	fclose(fin);
	fclose(fout);

	return 0;
}

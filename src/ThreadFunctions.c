//3h Ergasia Syspro - Panagiotis Mavrommatis
//ThreadFunctions.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include "structs.h"
//#include "skipListFunctions.h"
#include "StartersAndHelpFunctions.h"
#include "monitor_main_questions.h"

extern pthread_mutex_t virus_country_root_mutex;//Mutex gia na perimenoun ta ypoloipa threads ektos apo to prwto, na arxikopoihthei to virus kai to country dentro
extern pthread_mutex_t round_buffer_mutex;//mutex gia to round buffer 
extern pthread_mutex_t virus_mutex;//mutex gia otan elegxetai an yparxei hdh o ios sto dentro, kai an den yparxei na eisxwreite
extern pthread_mutex_t country_mutex;//mutex gia otan elegxetai an yparxei hdh to country sto dentro, kai an den yparxei na eisxwreite
extern pthread_mutex_t test_mutex;//mutex gia to flag poy leei sta threads na termatisoun
extern pthread_mutex_t error_checking_mutex;//Mutex gia to error checking sta records

extern pthread_mutex_t addVaccinationRecordsMutex;

struct RoundBuffer* Create_Circular_Buffer(int cyclicbufferSize){//first in first out, allagmenos kwdikas apo diafanies kai kwdika tou k ntoula.
	struct RoundBuffer* round_pointer;
	round_pointer=(struct RoundBuffer*)malloc(sizeof(struct RoundBuffer));
	
	
	round_pointer->file_name=(char**)malloc(cyclicbufferSize*sizeof(char*));//arxikopoihsh toy pinaka me ta strings
	for(int i=0;i<cyclicbufferSize;i++){
		round_pointer->file_name[i]=NULL;
	}
	round_pointer->total_size_of_buffer=cyclicbufferSize;//to megethos tou round buffer
	round_pointer->current_head=0;//h thesh ston pinaka pou tha eiswrhsei
	round_pointer->current_size=0;//to plithos twn file_name pou periexei to cyclic buffer
	round_pointer->current_tail=-1;// Thesh ston pinaka pou tha diavasei. -1=Den yparxei 

	return round_pointer;

}

int RB_Is_Full(struct RoundBuffer* round_pointer){//synarthsh pou elegexei an to cyclic buffer einai gemato
	if(round_pointer==NULL){
		printf("Error! round_pointer given is NULL\n");
		exit(1);
	}
	else if(round_pointer->total_size_of_buffer != round_pointer->current_size)//an to current size einai iso me to megethos to cyclic buffer tote einai gemato
		return 0;//Den einai gemato to buffer
	else 
		return 1;//einai gemato to buffer
}

int RB_Is_Empty(struct RoundBuffer* round_pointer){//synarthsh pou elegxei an to cyclic buffer einai adeio
	if(round_pointer==NULL){
		printf("Error! round_pointer given is NULL\n");
		exit(1);
	}else if(round_pointer->current_size>0)//an to current size einai >0 tote den einai adeio
		return 0;//den einai adio
	else
		return 1;//einai adeio
}

int add_to_RB(struct RoundBuffer* round_pointer,char* string){//prosthiki string sto cyclic buffer
	if(round_pointer==NULL){
		printf("Error! Given round_pointer is NULL\n");
		exit(1);
	}
	if(RB_Is_Full(round_pointer)==1){//an to round buffer einai gemato
		//printf("Gemato buffer den mporw na prosthesw\n");
		return 1;
	}
	round_pointer->current_tail=(round_pointer->current_tail+1);//ayksanoume kata mia thesh to pou isxwroume
	round_pointer->current_tail=round_pointer->current_tail % round_pointer->total_size_of_buffer;//Gia na mhn kseperasoume to size tou buffer, pairnoume to ypoloipo. Paromoia logikh me ton tropo pou spaw ta strings gia to bufferSize.
	round_pointer->current_size++;//Afksanw to plithos twn arxeiwn pou exoume sto round buffer kata 1
	//printf("aaaaaaaa\n");
	round_pointer->file_name[round_pointer->current_tail]=strdup(string);//eisxwrw sthn thesh pou upologisame prin to filename
	//printf("bbbbbbb\n");
	return 0;

}

char* get_from_RB(struct RoundBuffer* round_pointer){//synarthsh pou aferei string apo to cyclic buffer
	if(round_pointer==NULL){
		printf("Error! Given round_pointer is NULL\n");
		exit(1);
	}
	if(RB_Is_Empty(round_pointer)==1){//an to round buffer einai adeio
		//printf("Adeio buffer den mporw na afairesw\n");
		char* string=strdup("");
		return string;
	}
	char* string =strdup(round_pointer->file_name[round_pointer->current_head]);//pairnoume to string
	free(round_pointer->file_name[round_pointer->current_head]);
	

	round_pointer->current_head=(round_pointer->current_head+1);//ayksanoume kata mia thesh to pou diavazoume
	round_pointer->current_head=round_pointer->current_head % round_pointer->total_size_of_buffer;//Gia na mhn kseperasoume to size tou buffer, pairnoume to ypoloipo. Paromoia logikh me ton tropo pou spaw ta strings gia to bufferSize.
	round_pointer->current_size--;//meiwnw to plithos twn arxeiwn pou exoume sto round buffer kata 1
	
	return string;//epistrefw to sting

}

void *Thread_function(void* arg){
	struct Arguments_for_threads* thr_args;
	thr_args=(struct Arguments_for_threads*)arg;//pairnw to struct me ta arguments(koino gia ola ta threads)
	//printf("kollaw\n");
	pthread_mutex_lock(&virus_country_root_mutex);//an einai to prwto thread synexizei kai ta ypoloipa thread kleidwnoun mexri na ginei h prwth eisxwrhsh sta virus kai country Dentra
	if((thr_args->rootV!=NULL) && (thr_args->rootC!=NULL))
		pthread_mutex_unlock(&virus_country_root_mutex);//ama den einai to prwto thread, dhladh exoun parei timh oi rizes twn dentron, synexizei
	//printf("ksekolaw\n");
	int* noOfVirs=thr_args->noOfVirs;
	int* noOfCountries=thr_args->noOfCountries;
	int* no_more_records_flag=thr_args->no_more_records_flag;
	struct Country **rootC=&(thr_args->rootC);
    struct Virus **rootV=&(thr_args->rootV);
    struct RoundBuffer* round_pointer=thr_args->round_pointer;

    
	int rb_return=0;
	struct Citizen *headCitizen=NULL; //single linked list gia tous Citizens
	//while((*no_more_records_flag==0)){
	while(1){
		pthread_mutex_lock(&round_buffer_mutex);
		rb_return=RB_Is_Empty(round_pointer);//elegxoume an to cyclic buffer einai adeio
		pthread_mutex_unlock(&round_buffer_mutex);
		pthread_mutex_lock(&test_mutex);
		int temp1=*no_more_records_flag;//diavazoume thn metavlhth apo to struct
		pthread_mutex_unlock(&test_mutex);

		if(rb_return && (temp1==1))//ama to round buffer einai adeio kai den yparxoun alla arxei gia diavasma termatise
			break;

		//printf("no_more_records_flag%d\n", *no_more_records_flag);
		//pthread_mutex_unlock(&round_buffer_mutex);
	    

	    	int counter=0;
	    	int size=0,size2=0;
	    	char*buffer_string;
	    	
	    	//diavaw to string apo to round/cyclic buffer 
	    	pthread_mutex_lock(&round_buffer_mutex);
	    	buffer_string=get_from_RB(round_pointer);
	    	//printf("buffer_string:::%s\n",buffer_string );
	    	pthread_mutex_unlock(&round_buffer_mutex);


			//kai me tis synarthseis apo thn prwth ergasia to anoigoume to diavazoume kai dhmiourgoume thn arxikh lista headcitizen, gemizontas taytxrona kai ta antistoixa dentra gia country virus
			if(headCitizen==NULL){//ama einai to prwto txt poy diavazoume
		    	headCitizen = processFile(buffer_string,rootC, rootV, noOfVirs,noOfCountries); //process citizens input file
				//tempV=rootV;
			}
		    else{//an den eina to prwto txt pou diavazoume sto thread
		    	struct Citizen *headCitizenPointer=headCitizen;
		    	while(headCitizenPointer->next!=NULL)
		    		headCitizenPointer=headCitizenPointer->next;
		    	headCitizenPointer->next = processFile(buffer_string,rootC, rootV, noOfVirs,noOfCountries); //process citizens input file
		    }
		    free(buffer_string);
	    
	   
	}


     if (headCitizen == NULL)
	    {
	        return(void*) NULL;//ama ola ta arxei htan adeia tote epistrefoyme null
	    }else
	    //printList(headCitizen);
	    return (void*)headCitizen;//diaforetika epistrefoume ton arxiko komvo ths listas toy thread
	//printf("thread%d\n",*noOfVirs );
	

}

void *add_vaccination_thread_function(void* arg){
	struct Arguments_for_threads_addvaccination* new_args;
	new_args=(struct Arguments_for_threads_addvaccination*)arg;//pairnw to struct me ta arguments(koino gia ola ta threads)
	
	int* noOfVirs=new_args->noOfVirs;
	int* noOfCountries=new_args->noOfCountries;
	int* no_more_records_flag=new_args->no_more_records_flag;
	struct Country **rootC=&(new_args->rootC);
    struct Virus **rootV=&(new_args->rootV);
    struct RoundBuffer* round_pointer=new_args->round_pointer;
    struct SkipListHead ** Table_of_Heads=new_args->Table_of_Heads;
    struct Citizen *head=new_args->head;
    int ***bArrays=new_args->bArrays;
    long int bloomSize=new_args->bloomSize;

    int rb_return=0;

    while(1){
    	pthread_mutex_lock(&addVaccinationRecordsMutex);
    	Table_of_Heads=new_args->Table_of_Heads;
		rb_return=RB_Is_Empty(round_pointer);//elegxoume an to cyclic buffer einai adeio
		
		int temp1=*no_more_records_flag;//diavazoume thn metavlhth apo to struct
	

		if(rb_return && (temp1==1))//ama to round buffer einai adeio kai den yparxoun alla arxei gia diavasma termatise
			break;

			char*buffer_string;
	    	
	    	//diavaw to string apo to round/cyclic buffer 
	    	
	    	buffer_string=get_from_RB(round_pointer);
	    	//printf("buffer_string:::%s\n",buffer_string );
	    	struct SkipListHead ** Table_of_Heads2;

	    	Table_of_Heads2=NewFileProcess(buffer_string, *rootC, *rootV, noOfVirs,noOfCountries,Table_of_Heads,head,bArrays,bloomSize);//kai kaloume thn synarthsh gia na to anoiksei na to diavasei kai na eisxwrhsei ta nea records sthn skiplist bloomfilters
	    	new_args->Table_of_Heads=Table_of_Heads2;







	    	pthread_mutex_unlock(&addVaccinationRecordsMutex);


    	//printf("kollhsa\n");




    }
   // printf("Spaw\n");
    pthread_mutex_unlock(&addVaccinationRecordsMutex);

	return (void*)NULL;


}
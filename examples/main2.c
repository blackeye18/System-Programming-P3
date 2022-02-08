// 3h Ergasia SysPro - MAVROMMATIS PANAGIOTIS - sdi1800115 //
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <fcntl.h> 
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include<time.h>
// #include<signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>

#include "structs.h"
#include "skipListFunctions.h"
#include "StartersAndHelpFunctions.h"
#include "bloomfilter.h"
#include "bloomFunctions.h"
#include "BST.h"
#include "monitornewfunc.h"
#include "monitor_main_questions.h"
#include "ThreadFunctions.h"

//Tha mporousa na to perasw sto struct me ta args gia apofygh twn global metavlhtwn alla den prolavainw
pthread_mutex_t virus_country_root_mutex;//Mutex gia na perimenoun ta ypoloipa threads ektos apo to prwto, na arxikopoihthei to virus kai to country dentro
pthread_mutex_t round_buffer_mutex;//mutex gia to round buffer 
pthread_mutex_t virus_mutex;//mutex gia otan elegxetai an yparxei hdh o ios sto dentro, kai an den yparxei na eisxwreite
pthread_mutex_t country_mutex;//mutex gia otan elegxetai an yparxei hdh to country sto dentro, kai an den yparxei na eisxwreite
pthread_mutex_t test_mutex;//mutex gia to flag poy leei sta threads na termatisoun
pthread_mutex_t error_checking_mutex;//Mutex gia to error checking sta records

extern pthread_mutex_t addVaccinationRecordsMutex;

int main(int argc, char *argv[] ){
	if (!argsOK(argc, argv)) return 0;//elegxos tou plhthous twn arguments
	int port = atoi(argv[2]);
	int numThreads=atoi(argv[4]);
	long int bufferSize=atol(argv[6]);
	int cyclicbufferSize=atoi(argv[8]);
	long int bloomSize=atol(argv[10]);
	char* sub_directories=argv[11];

	//printf("port: %d\n",port );
	//printf("numThreads: %d\n",numThreads );
	//printf("bufferSize: %ld\n",bufferSize );
	//printf("cyclicbufferSize: %d\n",cyclicbufferSize );
	//printf("bloomSize: %ld\n",bloomSize );
	//printf("sub_directories: %s\n",sub_directories );

	//kwdikas pou grafitke me thn voithia twn diafaniwn kai tou piazza
	char hostname[HOST_NAME_MAX+1];
	gethostname(hostname,HOST_NAME_MAX+1);
	//printf("child: hostname %s\n\n",hostname );
	struct hostent *ghb; 
	struct in_addr address;
	ghb=gethostbyname(hostname);
	memcpy(&address,*ghb->h_addr_list,sizeof(address));
	char * caddress;
	caddress=strdup(inet_ntoa(address));
    //printf("Child: address: %s\n", caddress);
	

	char starting_directory[PATH_MAX];
	if (getcwd(starting_directory, sizeof(starting_directory)) != NULL) {//vriskoume to twrino directory poy eimaste kata thn ektelesh, to xreiazomaste na gia na mporoume na epistrepsoume se afto argotera
	    ;
	}else{
	    printf("Error with getcwd!");
	    return 1;
	}

	int client_socket;
	struct sockaddr_in server_address;
	if((client_socket=socket(AF_INET,SOCK_STREAM,0))<0){//dhmiourgeia tou socket
		printf("Error Creating Client Socket\n");
		exit(1);
	}
	memcpy(&server_address,"\0",sizeof(server_address));//arxikopoioume to serveraddress, opws kai stis diafanies

	server_address.sin_family=AF_INET;//ipv4
	server_address.sin_port=htons(port);//porta
	server_address.sin_addr.s_addr=inet_addr(caddress);//diefthinsh thn opoia vrikame dynamika

	int connection_status=-2;
	do
	connection_status=connect(client_socket,(struct sockaddr*)&server_address,sizeof(server_address));//Prospathoume na syndethoume mexri na ginei h syndesh
	while (connection_status<0);
	struct pollfd fdarray[1];

	//kathe subdirectory sto string xwrizetai apo to allo me keno
	char * temp_subdirectories;
	temp_subdirectories=strdup(sub_directories);
	int counter=0;

	char *token, *tok;
	token=strtok(temp_subdirectories," ");
	//ypologizw to plhthos twn subdirectories xrhsimopoiwdas thn strtok
	while(token!=NULL){
		counter++;
		token=strtok(NULL," ");
	}
	//printf("counter[%d]\n",counter );
	char **split_directories;
	//desmevw ton antistoixo xwro kai ta apothikevw ksexwrista ston pinaka
	split_directories=(char**)malloc(counter * sizeof(char*));
	int counter2=0;
	tok=strtok(sub_directories," ");
	while(tok!=NULL){
		split_directories[counter2]=strdup(tok);
		//printf("Token:[%s]",tok);
		counter2++;
		tok=strtok(NULL," ");
	}

	srand(time(NULL));
	struct Citizen *headCitizen=NULL; //single linked list gia tous Citizens
	struct Country *rootC=NULL; //BST riza gia to Countries
	struct Virus *rootV=NULL; //BST riza gia to Viruses
	int noOfVirs = 0, noOfCountries = 0; //plithos viruses and countries


	int **bArrays;
	
	int result;
	int flag=0;
	struct Virus *tempV=NULL;
	int scandir_return=0;
	struct dirent **FilesList;
	int *files_counter_array;
	files_counter_array=malloc(counter*sizeof(int));
	struct LinkedList * filenamelist=NULL;


	
	for(int i=0;i<counter;i++)
		files_counter_array[i]=0;
	for(int i=0;i<counter;i++){
		int ret=chdir(split_directories[i]);//mpainoume sto subdirectory
		//printf("split_directories[%d]:[%s]\n",i,split_directories[i] );
		if(ret==-1){
			printf("ERROR WITH CHDIR: split_directories[%d]:[%s]\n",i,split_directories[i] );
		}
		//diavazoume me thn voithia ths scandir alfavitika ta arxeia pou yparxoun sto subdirectory
		scandir_return= scandir(".",&FilesList,NULL,alphasort);
		int files_counter=0;
		files_counter=scandir_return-2;
		files_counter_array[i]=files_counter;
		int splitsize=strlen(split_directories[i]);

		
		//gia kathe txt pou diavasame se kathe subdirectory
		for(int j=2;j<scandir_return;j++){
			
			int textsize=strlen(FilesList[j]->d_name);
			int total_txt_size=0;
			total_txt_size=textsize+splitsize;//prosthetoume megethos twn string gia na ftiaksoume to string gia to cyclic buffer
			char* cyclic_buffer_string=NULL;
			cyclic_buffer_string= malloc((total_txt_size+2)*sizeof(char));
			strcpy(cyclic_buffer_string,"");
			strcat(cyclic_buffer_string,split_directories[i]);
			strcat(cyclic_buffer_string,"/");
			strcat(cyclic_buffer_string,FilesList[j]->d_name);
			cyclic_buffer_string[total_txt_size+1]='\0';
			//printf("%s\n",cyclic_buffer_string );

			
			//to apothikevoume sthn lista
			if (filenamelist==NULL){//an einai to prwto filename
				filenamelist=malloc(sizeof(struct LinkedList));
				filenamelist->folder=i;
				filenamelist->txtname=strdup(cyclic_buffer_string);
				filenamelist->next=NULL;
			}
			else{//an den einai to prwto filename
				struct LinkedList * filenamelistPointer=filenamelist;
				while(filenamelistPointer->next!=NULL)
					filenamelistPointer=filenamelistPointer->next;
				filenamelistPointer->next=malloc(sizeof(struct LinkedList));
				filenamelistPointer->next->folder=i;
				filenamelistPointer->next->txtname=strdup(cyclic_buffer_string);
				filenamelistPointer->next->next=NULL;
			}
			free(cyclic_buffer_string);
			

			
		}
		ret=chdir(starting_directory);// epistrofh sto arxiko directory
		for(int i=0;i<scandir_return;i++)
    		free(FilesList[i]);//eleftherwnoume ton xwro pou desmefse h scandir
    	free(FilesList);
	}
	//rootV=tempV;

	struct RoundBuffer* round_pointer;
	round_pointer=Create_Circular_Buffer(cyclicbufferSize);//dhmiourgeia tou round buffer
    //thr_args->round_pointer=round_pointer;

	struct LinkedList* filenamelistPointer=filenamelist;
	while(filenamelist!=NULL){
		int returnfromadd=add_to_RB(round_pointer,filenamelist->txtname);//gemizoume to round buffer mexri eite na mhn xwraei alla eite na teleiwsoun ta strings pou prepei na valoume
		if(returnfromadd==1)
			break;
		filenamelist=filenamelist->next;
	}
	
	int no_more_records_flag=0;
	struct Arguments_for_threads* thr_args;//Dhmiourgeia tou struct pou periexei ta arguments gia ta threads
	thr_args=malloc(sizeof(struct Arguments_for_threads));
	thr_args->noOfVirs=&noOfVirs;
    thr_args->noOfCountries=&noOfCountries;
    thr_args->round_pointer=round_pointer;
    thr_args->rootV=NULL;
    thr_args->rootC=NULL;
    thr_args->no_more_records_flag=&no_more_records_flag;
    //init ta mutexes
    pthread_mutex_init(&virus_country_root_mutex,NULL);
    pthread_mutex_init(&round_buffer_mutex,NULL);
    pthread_mutex_init(&virus_mutex,NULL);
    pthread_mutex_init(&country_mutex,NULL);
    pthread_mutex_init(&test_mutex,NULL);
    pthread_mutex_init(&error_checking_mutex,NULL);


    //dhmiourgeia kai klhsh twn threads
	pthread_t thread_ids[numThreads];
	for(int i=0; i<numThreads;i++)
		thread_ids[i]=0;

	for(int i=0; i<numThreads;i++){
		if(pthread_create(&thread_ids[i],NULL,&Thread_function,thr_args)!=0){
			printf("Error Creating Threads\n");
			exit(1);
		}
	}


	//Oso ta threads diavazoun arxeia, h main prospathei na eisxwrhsei stis theseis pou eleftheronontai ta ypoloipa arxeia.
	//Afto ginetai mexri mexri na teleiwsoun ta arxeia pou prepei na diavastoun, 

	while(filenamelist!=NULL){
		//printf("aa\n");
		pthread_mutex_lock(&round_buffer_mutex);
		int ret=RB_Is_Full(round_pointer);
		pthread_mutex_unlock(&round_buffer_mutex);
		if(ret==0){
			pthread_mutex_lock(&round_buffer_mutex);
			int returnfromadd=add_to_RB(round_pointer,filenamelist->txtname);
			pthread_mutex_unlock(&round_buffer_mutex);
			filenamelist=filenamelist->next;
		}
		
		
	}

	//while(1){
		//if(RB_Is_Empty(round_pointer)==1)
			//break;
	//}

	pthread_mutex_lock(&test_mutex);
	no_more_records_flag=1;//otan teleiwsoun ta arxeia pou prepei na diavastoun, enhmerwnete to flag 
	pthread_mutex_unlock(&test_mutex);
	struct Citizen* temp_head_citizen=NULL;
	for(int i=0;i<numThreads;i++){
		void *return_value;
		if(pthread_join(thread_ids[i],&return_value)!=0){
			printf("Error Joining Threads\n");
			exit(1);
		}

		//Ama to tread epistrepsei kapoia lista, thn syndeeoume me thn ypoloiph
		if(return_value!=NULL){
			if(temp_head_citizen==NULL){//ama einai to prwto thread poy diavazoume
		    	temp_head_citizen = return_value;
			}
			else{//ama den einai to prwto thread pou diavazoume
				struct Citizen *headCitizenPointer=temp_head_citizen;
		    	while(headCitizenPointer->next!=NULL)
		    		headCitizenPointer=headCitizenPointer->next;
		    	headCitizenPointer->next=return_value;
		    }
		}
	}

	if (temp_head_citizen == NULL)
	{
	   	printf("No records loaded. Exiting...\n");
	    return 0;
	}
	
	rootV=thr_args->rootV;
	rootC=thr_args->rootC;
	headCitizen=temp_head_citizen;


	filenamelist=filenamelistPointer;

	//display citizen list, # of viruses for debug
	//printf("\nList Contains (sorted by ID ascending):\n");
	//printList(headCitizen);
	//printf("\nCountries\n");
	//innorderC(rootC);
	//printf("\n# of viruses: %d\n",noOfVirs);

	// traverse BSTs for debug
	 // innorderV(rootV);

	//Bloom filter apo prwth ergasia:
	//bloom: Xrhsimopoiw int. Kathe int exei sizeof(int) bytes. Ara an to bloomsize einai stai 100.000 bytes kai sizeof(int)=4, xreiazomaste pinaka me 25.000 akeraious
	//long int bloomSize = atol(argv[4]); //parameter
	long int bArrayLength = bloomSize / (long)sizeof(int);
	//printf("\nChild:There are %d viruses (rows), each with a bloom 1-d array of size %ld (columns)\n",noOfVirs,bArrayLength);
	bArrays = (int **)malloc(sizeof (int *)*(noOfVirs));
	for (int v=0; v<noOfVirs; v++) bArrays[v] = (int *) malloc(sizeof(int)*bArrayLength);
	for (int lines = 0; lines<noOfVirs; lines++)
	    for (long cols=0; cols<bArrayLength; cols++)
	        bArrays[lines][cols]=0;

	bArrays = makeBloom(headCitizen,rootV,bArrays,bloomSize);// dimiourgeia toy bloomfilter
	//Skip list apo prwth ergasia
	struct SkipListHead **Table_of_Heads;
	Table_of_Heads=CreateAllSkipLists(headCitizen,rootV,noOfVirs);//dimiourgeia twn SkipList
	//print_TableOfSkipList(Table_of_Heads,noOfVirs); // emfanizei ola ta skipList, xrhsimopoeithike gia debug
	
	int fd2=client_socket;
	int fd=client_socket;
	fdarray[0].fd=fd;//Gia thn poll gia na diabazoume mono afou  exei ginei write
	fdarray[0].events=POLLIN;

	write_int(fd2,0);

	//printf("Child noOfVirs before writing:%d\n",noOfVirs );
	write_int(fd2,noOfVirs);//grafoume to plithos twn iwn pou exei to monitor

	writeInOrder(rootV,fd2,bufferSize);//kai epeita metaferoume to onoma tou kathe iou
	//innorderV(rootV);
	
	
	
	//synarthsh steile to bloomfilter sou
	send_bloom_filter(rootV,bArrays,bloomSize,noOfVirs,fd2, bufferSize);//stelnoume to bloom filter tou monitor ston patera
	int log_total=0,log_accepted=0,log_rejected=0;//gia to log file
//printf("aaaa\n");
	int command;
	command=read_int_notimeout(fd);//diavazei akeraio xwris na kanei timeout
	result=0;
	while (command!=0)
    {
        switch (command)
        {
        case 1://travelRequest
            //printf("Child:1st option\n");
            result = travel_request(Table_of_Heads,rootV,rootC,fd,fd2,fdarray,bufferSize,noOfVirs,&log_total,&log_accepted,&log_rejected);
            break;
        case 2:
            //printf("child: 2nd option\n");
            result = searchVaccinationStatus(Table_of_Heads,noOfVirs,fd,fd2,fdarray,bufferSize,headCitizen);
            break;//searchVaccinationStatus
        case 3:
            //printf("Child:3rd option\n");
            	Table_of_Heads = addVaccinationRecords(starting_directory,split_directories,counter,&files_counter_array,filenamelist,rootC, rootV,&noOfVirs,&noOfCountries,Table_of_Heads,headCitizen,&bArrays,bloomSize,bufferSize,fd2,cyclicbufferSize,numThreads);
            break; //addVaccinationRecords
        default:
            printf("Error in command, try again\n");
            break;
        }
        command=read_int_notimeout(fd);
        
    }
    
    
    create_logfile(rootC,noOfCountries,log_total,log_accepted,log_rejected);
    


    
	printf("Child:Exiting....freeing memory...\n");



	close(fd);
	//close(fd2);
	freeBSTC (rootC);
    freeBSTV(rootV);
    for (int v=0; v<noOfVirs; v++) 
        free(bArrays[v]);
    free(bArrays);
    struct SkipListHead *Headpointer=NULL;
    struct SkipListNode *Nodepointer=NULL;
    struct SkipListNode *NodepointerHolder=NULL;
    int dnoOfVirs=noOfVirs*2;
    int maxdepth=0;
    for(int i=0;i<dnoOfVirs;i++){
        if(Table_of_Heads[i]!=NULL){
            Headpointer=Table_of_Heads[i];
            maxdepth=Headpointer->depth;
            for(int j=0;j<DEPTH_MAX_LIMIT;j++){
                if(Headpointer->next[j]!=NULL){    
                    Nodepointer=Headpointer->next[j];
                    while(Nodepointer!=NULL){
                        NodepointerHolder=Nodepointer->next;
                        free(Nodepointer);
                        Nodepointer=NodepointerHolder;
                    }
                }
            }
            free(Headpointer);
        }
    }
    free(Table_of_Heads);
    free(NodepointerHolder);

    struct Citizen *headCitizenHold=NULL;
    //free Citizen single linked list
    while(headCitizen!=NULL)
    {
        free(headCitizen->citizenID);
        free(headCitizen->firstName);
        free(headCitizen->lastName);
        free(headCitizen->vaccinated);
        free(headCitizen->dateVaccinated);
        headCitizenHold=headCitizen;
        headCitizen=headCitizen->next;
        free(headCitizenHold);
    }
    free(temp_subdirectories);
    for(int i=0;i<counter;i++){
		free(split_directories[i]) ;
	}
	free (split_directories);
	
	free(files_counter_array);

	struct LinkedList *linkedlisthold=NULL;
	while(filenamelist!=NULL){
		free(filenamelist->txtname);
		linkedlisthold=filenamelist;
		filenamelist=filenamelist->next;
		free(linkedlisthold);
	}
	free(caddress);

	//for(int i=0;i<cyclicbufferSize;i++)
	//	free((round_pointer->file_name)[i]);
	free(round_pointer->file_name);
	free(round_pointer);
	free(thr_args);
	pthread_mutex_destroy(&addVaccinationRecordsMutex);
	pthread_mutex_destroy(&virus_country_root_mutex);
    pthread_mutex_destroy(&round_buffer_mutex);
    pthread_mutex_destroy(&virus_mutex);
    pthread_mutex_destroy(&country_mutex);
    pthread_mutex_destroy(&test_mutex);
    pthread_mutex_destroy(&error_checking_mutex);


	return 0;
	}
// 3h Ergasia SysPro - MAVROMMATIS PANAGIOTIS - sdi1800115 //
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include<time.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <dirent.h>
#include "structs.h"
#include "skipListFunctions.h"
#include "StartersAndHelpFunctions.h"
#include "bloomfilter.h"
#include "bloomFunctions.h"
#include "BST.h"
#include "monitornewfunc.h"
#include "monitor_main_questions.h"
#include "ThreadFunctions.h"


pthread_mutex_t addVaccinationRecordsMutex;

//tropopoihmenh synarthsh apo thn prwth ergasia, xrhsimopoieitai kata thn xrhsh ths travelRequest. Grafei, vash twn dedomenw ths skiplist, an kapoios exei emvoliastei kai pote h oxi. 
//Epishs krataei statistika gia ta logfiles
int search_in_SkipList_with_virus(struct SkipListHead ** Table_of_Heads,int noOfVirs,char *citizenID,char *virusName,long int bufferSize,int fds2,int *log_total,int *log_accepted,int *log_rejected){
    //char *citizenID = cmd->args[1];
    //char *virusName = cmd->args[2];
    int dnoOfVirs=noOfVirs*2;
    struct SkipListHead* Headpointer=NULL;
    struct SkipListNode * Nodepointer=NULL;
    int i,j,k,flag=0,flag2=0,local_max_depth=0;
    *log_total=*log_total+1;
    //printf("Den mpainwww\n");
    for(i=0;i<dnoOfVirs;i++){
        Headpointer=Table_of_Heads[i];
        if(strcmp(Headpointer->vaccinated_or_no,"YES")==0){//an vrikame to swsto skiplist
            if(strcmp(Headpointer->virusName,virusName)==0){
                flag=1;
                local_max_depth=Headpointer->depth;//apothikevoume to megisto vathos/ypsos ths sygkekrimenhs skiplist
                for(j=local_max_depth-1;j>=0;j--){// paw anapoda gia mikroterh polyplokothta, an to vrei se pio psilo shmeio
                    Nodepointer=Headpointer->next[j];
                    while(Nodepointer!=NULL){//psaxnoume to skiplistnode pou mas endiaferei
                        if(strcmp(Nodepointer->citizen_node->citizenID,citizenID)==0){
                            flag2=1;
                            if(strcmp(Nodepointer->citizen_node->vaccinated,"YES")==0){//an to vroume grafoume analogo minima
                                //printf("Mphkaaaaa\n");
                                *log_accepted=*log_accepted+1;//kratame statistika
                                write_string(fds2,"YES",bufferSize);
                                write_string(fds2,Nodepointer->citizen_node->dateVaccinated,bufferSize);
                                //printf("VACCINATED ON %s\n",Nodepointer->citizen_node->dateVaccinated );
                                return 0;}
                        }
                        Nodepointer=Nodepointer->next;
                    }
                }
            }
        }

    }
    if (flag==0 || flag2==0){
    	//printf("Mphkaaaaa2\n");
        *log_rejected=*log_rejected+1;
    	write_string(fds2,"NO",bufferSize);//an telika den to vrikame pali grafoume analogo mnm
    }
        //printf("NOT VACCINATED\n");//an telika den to vrikame pali emfanizoume analogo mnm
    return 1;
}

//H synarthsh travel request kaleitai otan to travel monitor dextei ena travelrequest aithma, kai to bloomfilter tou epistrepsei maybe. Tote xreiazetai thn voitheia twn monitor kai kata epektash twn skiplist
int travel_request(struct SkipListHead ** Table_of_Heads, struct Virus *rootV, struct Country *rootC,int fds,int fds2,struct pollfd* fdarray,long int bufferSize,int noOfVirs,int *log_total,int *log_accepted,int *log_rejected){
	
    //printf("log_total:%d\n",*log_total );
    int id =read_int(fds,fdarray);//diavazoume ton io apo to namedpipe, antistoixh diadikasia me htn synarthsh write_string, aplws diavazoume
	int size=0;
	size=read_int(fds,fdarray);
    char virus_received[size];
    strcpy(virus_received,"");
    int rc=0;
    if(size<=bufferSize){
        rc=poll(fdarray,1,300);
        if(rc==1){
            read(fds,virus_received,size);
            virus_received[size]='\0';}
        else{
            printf("Error! Poll timeout!\n");
            exit(1);
        }
    }
    else{
        //strcpy(virus_received,"");
        int division=0,modulus=0;
        char constring[size];
        strcpy(constring,"");
        division=size/bufferSize;
        modulus=size%bufferSize;
        int rc=0;
        //printf("division: %d modulus: %d\n", division,modulus);
        for(int j=0;j<division;j++){
            rc=poll(fdarray,1,300);
            if(rc==1)
                read(fds,constring,bufferSize);
            else{
                printf("Error! Poll timeout!\n");
                exit(1);
            }
            constring[bufferSize]='\0';
            strcat(virus_received,constring);
            strcpy(constring,"");
            }
            if(modulus>0){
                int rc=poll(fdarray,1,300);
                if(rc==1)
                    read(fds,constring,modulus);
                else{
                    printf("Error! Poll timeout!\n");
                    exit(1);
                }
                //printf("%s\n",constring );
                strcat(virus_received,constring);
                strcpy(constring,"");
            }
        virus_received[size]='\0';
    }
    //printf("ID receive:[%d] virus_received:[%s]\n",id,virus_received );
    char citizenID[6];
    sprintf(citizenID,"%d",id);//metatrepoume to id se string, to opoio gia efkolia to diabasame se int
   // printf("%s\n",citizenID );
    //xrhsimopoioume thn tropopoihmenh synarthsh ths prwths ergasias pou eidame parapanw
    search_in_SkipList_with_virus(Table_of_Heads,noOfVirs,citizenID,virus_received,bufferSize,fds2,log_total,log_accepted,log_rejected);
    //printf("log_total:%d\n",*log_total );
    return 0;
}
//Allh mia tropopoihmenh synarthsh apo thn prwth ergasia pou grafei se namedpipes ta dedomena pou yparxoun sthn skip list gia to sygkekrimeno id 
int search_in_SkipList(struct SkipListHead ** Table_of_Heads,int noOfVirs,char* citizenID,int fd2,long int bufferSize){
    //char *citizenID = cmd->args[1];
    int counter=0;
    int dnoOfVirs=noOfVirs*2;
    struct SkipListHead* Headpointer=NULL;
    struct SkipListNode * Nodepointer=NULL;
    int i,j,k,flag=0,flag2=0,flag3=0,local_max_depth=0;
    for(i=0;i<dnoOfVirs;i++){
        Headpointer=Table_of_Heads[i];
        
        local_max_depth=Headpointer->depth;//apothikevoume to megisto vathos/ypsos ths sygkekrimenhs skiplist
        flag2=0;
            for(j=local_max_depth-1;j>=0;j--){// paw anapoda gia mikroterh polyplokothta, an to vrei se pio psilo shmeio
                if(flag2==0){
                    Nodepointer=Headpointer->next[j];
                    while(Nodepointer!=NULL){
                        if(strcmp(Nodepointer->citizen_node->citizenID,citizenID)==0){//an vrikame to id pou mas endiaferei
                            flag2=1;
                            flag=1;
                            counter++;
                            if(flag3==0){//grafoume ta stoixeia tou mia fora mono
	                            write_string(fd2,Nodepointer->citizen_node->firstName,bufferSize);
	                            write_string(fd2,Nodepointer->citizen_node->lastName,bufferSize);
	                            write_string(fd2,Nodepointer->citizen_node->country->countryName,bufferSize);
	                            write_int(fd2,Nodepointer->citizen_node->age);
	                            flag3=1;
                            }
                            //printf("%s %s %s \n",Nodepointer->citizen_node->virusName->virusName,Nodepointer->citizen_node->vaccinated,Nodepointer->citizen_node->dateVaccinated );//ektypwnoume ta stoixeia tou, opws zhteite sthn ekfwnhsh
                        }
                        Nodepointer=Nodepointer->next;
                    }
                }
            }
            
    }
    flag=0;
    flag2=0;
    local_max_depth=0;
    Headpointer=NULL;
    Nodepointer=NULL;
    write_int(fd2,counter);
    for(i=0;i<dnoOfVirs;i++){
        Headpointer=Table_of_Heads[i];//to idio me panw aplws grafoume kathe egrafh pou yparxei sthn skip list gia to id pou zhththike
        
        local_max_depth=Headpointer->depth;
        flag2=0;
            for(j=local_max_depth-1;j>=0;j--){
                if(flag2==0){
                    Nodepointer=Headpointer->next[j];
                    while(Nodepointer!=NULL){
                        if(strcmp(Nodepointer->citizen_node->citizenID,citizenID)==0){//an vrikame to id pou mas endiaferei
                            flag2=1;
                            flag=1;
                            
                            write_string(fd2,Nodepointer->citizen_node->virusName->virusName,bufferSize);//grafoume yes /no
                            if(strcmp(Nodepointer->citizen_node->vaccinated,"YES")==0){// an yes grafoume kai to date
                            	write_int(fd2,0);
                            	write_string(fd2,Nodepointer->citizen_node->dateVaccinated,bufferSize);
                            }else{
                            	write_int(fd2,1);
                            }
                            //printf("%s %s %s \n",Nodepointer->citizen_node->virusName->virusName,Nodepointer->citizen_node->vaccinated,Nodepointer->citizen_node->dateVaccinated );//ektypwnoume ta stoixeia tou, opws zhteite sthn ekfwnhsh
                        }
                        Nodepointer=Nodepointer->next;
                    }
                }
            }
            
    }
    if (flag==0){
        printf("ERROR! SOMETHING WENT TERRIBLY WRONG.... GIVEN ID DOES NOT EXIST\n" );
        return 1;
    }else 
        return 0;
    
}
//synarthsh processfile apo prwth ergasia, allagmenh gia na xhrismopoiei thn synarthsh insertnew citizen record ths prwths ergasias
struct SkipListHead** NewFileProcess(char *fname, struct Country *rootC, struct Virus *rootV, int *noOfVirs,int *noOfCountries,struct SkipListHead ** Table_of_Heads,struct Citizen *head,int ***bArrays,long int bloomSize)
{
    FILE *file;
    char line[MAX_LINE];
    char lineData[9][50];// To prwto pou xrhsimopoiw einai sth thesh 1
    int error, dataCounter=0;
    struct Citizen *headC=NULL;
    file = fopen( fname, "r");
    if (! file)
    {
        printf("file not found\n");
        return NULL;
    }
    while (fgets(line, MAX_LINE, file)!=0)
    {
        dataCounter++;
        checkLine(line,&error,lineData );
        if (error)
            printf("ERROR IN RECORD WITH ID %s in line %d\n",lineData[1], dataCounter);
        else
        {
            struct uInput *input;
            input=malloc(sizeof(struct uInput));
            strcpy(input->args[1],lineData[1]);
            strcpy(input->args[2],lineData[2]);
            strcpy(input->args[3],lineData[3]);
            strcpy(input->args[4],lineData[4]);
            strcpy(input->args[5],lineData[5]);
            strcpy(input->args[6],lineData[6]);
            strcpy(input->args[7],lineData[7]);
            strcpy(input->args[8],lineData[8]);
            Table_of_Heads=insert_new_citizen_record(Table_of_Heads,noOfVirs,noOfCountries,head,bArrays,bloomSize,rootV,rootC,input);//synarthsh apo prwth ergasia
            free(input);
        }
    }
    fclose(file);
    return Table_of_Heads;
}


struct SkipListHead** addVaccinationRecords(char* starting_directory,char ** split_directories,int counter_split,int** files_counter_array,struct LinkedList * filenamelist,struct Country *rootC, struct Virus *rootV, int *noOfVirs,int *noOfCountries,struct SkipListHead ** Table_of_Heads,struct Citizen *head,int ***bArrays,long int bloomSize,long int bufferSize,int fd2,int cyclicbufferSize,int numThreads){
    int scandir_return=0;
    struct dirent **FilesList;
    struct LinkedList * tempLinked=NULL;
    struct LinkedList *newTempList=NULL;
    struct LinkedList *newTempList_pointer=NULL;
    for(int i=0;i<counter_split;i++){
        int strlen1=strlen(split_directories[i]);
        int ret=chdir(split_directories[i]);
        if(ret==-1){
            printf("ERROR WITH CHDIR: split_directories[%d]:[%s]\n",i,split_directories[i] );
        }//oti kaname kai sthn main sth arxh
        scandir_return= scandir(".",&FilesList,NULL,alphasort);
        int files_counter=0;
        files_counter=scandir_return-2;
        if(files_counter >((*files_counter_array)[i])){//ama ston fakelo exoume perissotera arxei apo oti eixame prin 
            for(int j=2;j<scandir_return;j++){
                //ftiaksimo string me olo to directory giati xrhsimopoihsa thn lista gia to kykliko buffer
                int strlen2=strlen(FilesList[j]->d_name);
                int total=strlen1+strlen2;
                char concatstring[total+2];
                strcpy(concatstring,"");
                strcat(concatstring,split_directories[i]);
                strcat(concatstring,"/");
                strcat(concatstring,FilesList[j]->d_name);
                concatstring[total+2]='\0';

                int flag1=0;
                tempLinked=filenamelist;//tote fvriskoume poio arxei den yparxei den yparxei sthn lista, dhladh poio h poia arxei einai ta kainouria
                while(tempLinked!=NULL){
                    //printf("tempLinked::::%s\n",tempLinked->txtname );
                    if((tempLinked->folder==i)&& (strcmp(concatstring,tempLinked->txtname)==0)){
                        flag1=1;
                        //printf("%s\n",FilesList[j]->d_name );
                    }
                    tempLinked=tempLinked->next;
                }
                if(flag1!=1){
                    //printf("%s\n",FilesList[j]->d_name );
                    //printf("%s\n",concatstring );
                    ((*files_counter_array)[i])=((*files_counter_array)[i])+1;//eisxwroume to neo arxeio sthn lista
                    tempLinked=filenamelist;
                    while(tempLinked->next!=NULL)
                        tempLinked=tempLinked->next;
                    tempLinked->next=malloc(sizeof(struct LinkedList));
                    tempLinked->next->folder=i;
                    tempLinked->next->txtname=strdup(concatstring);
                    tempLinked->next->next=NULL;

                    if(newTempList==NULL){
                        newTempList=malloc(sizeof(struct LinkedList));
                        newTempList->folder=i;
                        newTempList->txtname=strdup(concatstring);
                        //printf("%s\n",concatstring );
                        newTempList->next=NULL;
                        newTempList_pointer=newTempList;

                    }else{
                        while(newTempList_pointer->next!=NULL)
                            newTempList_pointer=newTempList_pointer->next;
                        newTempList_pointer->next=malloc(sizeof(struct LinkedList));
                        newTempList_pointer->next->folder=i;
                        newTempList_pointer->next->txtname=strdup(concatstring);
                        newTempList_pointer->next->next=NULL;
                    }
                    //kalw synarthsh poy eisxwrei
                    //printf("Onoma:%s",FilesList[j]->d_name);
                   //Table_of_Heads= NewFileProcess(tempLinked->next->txtname, rootC, rootV, noOfVirs,noOfCountries,Table_of_Heads,head,bArrays,bloomSize);//kai kaloume thn synarthsh gia na to anoiksei na to diavasei kai na eisxwrhsei ta nea records sthn skiplist bloomfilters
                }


                

            }
        }
        ret=chdir(starting_directory);
        for(int k=0;k<scandir_return;k++)
            free(FilesList[k]);
        free(FilesList);
    }
    newTempList_pointer=newTempList;

    struct RoundBuffer* round_pointer2;
    round_pointer2=Create_Circular_Buffer(cyclicbufferSize);//dhmiourgeia tou round buffer


    int no_more_records_flag=0;

    struct Arguments_for_threads_addvaccination *new_args;//oti kanoume kai sthn main
    new_args=malloc(sizeof(struct Arguments_for_threads_addvaccination));
    new_args->noOfVirs=noOfVirs;
    new_args->noOfCountries=noOfCountries;
    new_args->round_pointer=round_pointer2;
    new_args->rootV=rootV;
    new_args->rootC=rootC;
    new_args->no_more_records_flag=&no_more_records_flag;
    new_args->Table_of_Heads=Table_of_Heads;
    new_args->head=head;
    new_args->bArrays=bArrays;
    new_args->bloomSize=bloomSize;

    pthread_mutex_init(&addVaccinationRecordsMutex,NULL);

    while(newTempList!=NULL){
        int returnfromadd=add_to_RB(round_pointer2,newTempList->txtname);//gemizoume to round buffer mexri eite na mhn xwraei alla eite na teleiwsoun ta strings pou prepei na valoume
        if(returnfromadd==1)
            break;
        newTempList=newTempList->next;
    }

    pthread_t thread_ids[numThreads];
    for(int i=0; i<numThreads;i++)
        thread_ids[i]=0;


    for(int i=0; i<numThreads;i++){
        if(pthread_create(&thread_ids[i],NULL,&add_vaccination_thread_function,new_args)!=0){
            printf("Error Creating Threads\n");
            exit(1);
        }
    }
   // newTempList=newTempList_pointer;

   // printf("mphka1\n");
    //pthread_mutex_lock(&addVaccinationRecords);

    while(newTempList!=NULL){
        //printf("mphka2\n");
        pthread_mutex_lock(&addVaccinationRecordsMutex);
        int ret=RB_Is_Full(round_pointer2);
        if(ret==0){
            int returnfromadd=add_to_RB(round_pointer2,newTempList->txtname);
            pthread_mutex_unlock(&addVaccinationRecordsMutex);
            newTempList=newTempList->next;
        }
       // printf("%s\n",newTempList->txtname);
        //Table_of_Heads= NewFileProcess(newTempList->txtname, rootC, rootV, noOfVirs,noOfCountries,Table_of_Heads,head,bArrays,bloomSize);//kai kaloume thn synarthsh gia na to anoiksei na to diavasei kai na eisxwrhsei ta nea records sthn skiplist bloomfilters
        //newTempList=newTempList->next;
    }
    //printf("mphka3\n");
    no_more_records_flag=1;
    //printf("Mphka4\n");
    for(int i=0;i<numThreads;i++){
        
        if(pthread_join(thread_ids[i],NULL)!=0){
            printf("Error Joining Threads\n");
            exit(1);
        }
    }

    Table_of_Heads=new_args->Table_of_Heads;

    newTempList=newTempList_pointer;

    while(newTempList!=NULL){
        free(newTempList->txtname);
        newTempList_pointer=newTempList;
        newTempList=newTempList->next;
        free(newTempList_pointer);
    }
    free(round_pointer2->file_name);
    free(round_pointer2);
    //printf("aaaaaaaa\n");
    write_int(fd2,1);//Enhmerwnw oti einai etoimo gia na steilei to ananeomeno bloom//afou teleiwsoume to divasma twn new arxeiwn, enhmerwnoume to travelmonitor oti eimaste etoimh na tou steiloyme to neo bloom filter
    send_bloom_filter(rootV,*bArrays,bloomSize,*noOfVirs,fd2,bufferSize);// kai tou stelnoume to neo bloom filter
    //printf("finished\n");
    return Table_of_Heads;
}





int searchVaccinationStatus(struct SkipListHead ** Table_of_Heads,int noOfVirs,int fd,int fd2,struct pollfd* fdarray,long int bufferSize,struct Citizen *headC){
	int id=0;
	id=read_int(fd,fdarray);//diavazei to id mou mas endiaferei se int
	char citizenID[6];
    sprintf(citizenID,"%d",id);//to metatrepoume se char* (string)
    struct Citizen *retC=NULL;
    retC=searchList(headC,citizenID);//elegxoume oti odws yparxei sthn lista, an den yparxei tote enhmerwnei to travelmonitor
    if(retC==NULL){
    	//printf("NULLLLLL\n");
    	write_int(fd2,0);
    	return 1;
    }
    else{

    	write_int(fd2,1);//an yparxei, kai eimaste dhladh sto swsto monitor tote enhmrwnei kai xrhsimopoiei thn tropopoihmenh synarthsh ths prwths ergasias pou eidame parapanw
    	int ret=0;
    	ret=search_in_SkipList(Table_of_Heads,noOfVirs,citizenID,fd2,bufferSize);
    	return ret;

    }
    
    

}

//pairnei orismata thn riza tou dentrou me tis xwres, to plhthos twn xwrwn kai ta statistika
void create_logfile(struct Country *rootC,int noOfCountries,int log_total,int log_accepted,int log_rejected){
    //kai dhmiourgei to logfile etsi opws zhteitai apo thn ekwfnhsh
    pid_t pid=getpid();
    int j=0;
    char** Table_of_Countries=NULL;
    Table_of_Countries=malloc(noOfCountries*sizeof(char*));
    makearrayCountry(rootC,Table_of_Countries, &j );
    char ch_pid[10];
    char citizenID[17];
    sprintf(ch_pid,"%d",pid);
    strcpy(citizenID,"");
    strcpy(citizenID,"log_file.");
    //int length=strlen(citizenID);
    strcat(citizenID,ch_pid);
    //printf("citizenID\n");
    FILE *fp = NULL;
    fp = fopen(citizenID ,"w");
    for(int i=0;i<j;i++){
        fprintf(fp,"%s\n",Table_of_Countries[i]);
    }
    fprintf(fp,"TOTAL TRAVEL REQUESTS %d\n",log_total);
    fprintf(fp,"ACCEPTED %d\n",log_accepted);
    fprintf(fp,"REJECTED %d\n",log_rejected);
    free(Table_of_Countries);
    fclose(fp);
    return;


}
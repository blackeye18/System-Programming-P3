// 3h Ergasia SysPro - MAVROMMATIS PANAGIOTIS - sdi1800115 //
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
// #include <sys/stat.h> 
// #include <sys/types.h>
// #include <sys/errno.h>
 #include <sys/wait.h> 
#include <unistd.h>
#include <fcntl.h> 
#include <poll.h>

#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "structs_travel.h"
#include "newfunctions_travelmonitor.h"
#include "Functions_travelmonitor.h"
#include "travel_main_questions.h"
#include "bloomFunctions.h"




int main(int argc, char *argv[] )
{   
if (!argsOK(argc, argv)) return 0; //error checks on arguments
//printf("%s\n",argv[0] );


long int numMonitors=0;
numMonitors=atol(argv[2]);
long int bufferSize=atol(argv[4]);
int cyclicbufferSize=atoi(argv[6]);
long int bloomSize = atol(argv[8]);
char* input_dir=argv[10];
int numThreads=0;
numThreads=atoi(argv[12]);

struct dirent **countriesList;
int countries_counter=0;
int scandir_return=0;
scandir_return= scandir(input_dir,&countriesList,NULL,alphasort);
//diavazoume me thn voithia ths scandir alfavitika ta arxeia pou yparxoun sto subdirectory

if(scandir_return<0){
    printf("ERROR WITH SCANDIR\n");
    return 1;
}
countries_counter=scandir_return-2;

printf("Total No Of Countries: %d\n",countries_counter);


int modulus=0;
long int directories_per_monitor=0;
directories_per_monitor= countries_counter/numMonitors;//Kanw prwth moirasia 
if ((countries_counter % numMonitors) != 0 )
    modulus=countries_counter % numMonitors;//Elegxw an den xwrane akrivws gia na kanw round robin


if (directories_per_monitor == 0){//An exw perissotera monitors apo oti xwres, tote den xreiazomai ola ta monitors
    printf("%ld monitors won't take paths! (numMonitors>countries_counter)\n",numMonitors - countries_counter );
    directories_per_monitor=1;
    numMonitors=countries_counter;
    modulus=0;
}
printf("Number Of Monitors:%ld\nsocketBufferSize:%ld\ncyclicBufferSize:%d\nBloomSize:%ld\nInput Directory:%s\nnumThreads:%d\n",numMonitors,bufferSize,cyclicbufferSize,bloomSize,input_dir,numThreads);
struct Country *rootC=NULL; //BST riza gia to Countries
struct Country *tempC=NULL;

//printf("MODULUS::::: [%d]\n",modulus );
int fds[numMonitors];
for(int i=0;i<numMonitors;i++)//apothikevw ta file descriptors tou kathe monitor
    fds[i]=0;
int fds2[numMonitors];
for(int i=0;i<numMonitors;i++)
    fds2[i]=0;
char** sub_directories;
int country_array_list=2;
sub_directories = (char **)malloc(sizeof (char *)*(numMonitors));
//megalyterh xwra poy exw einai h "UnitedArabEmirates", 19 xarakthres dhladh +1 gia to keno sto telos 20.
//+11 gia to "input_dir/". Ara theloume countries_counter*31

for(int i=0; i<numMonitors;i++){
    sub_directories[i]=(char*)malloc((countries_counter*31)*sizeof(char));
    strcpy(sub_directories[i],"");
}
//ftiaxnw to string gia to kathe monitor kai apothikevw tis xwres sto dentro
for (int i=0; i<directories_per_monitor;i++){
    for(int j=0; j<numMonitors;j++){
        tempC=searchCountry(&rootC,countriesList[country_array_list]->d_name);
        if(tempC==NULL){
            rootC=addCountry(rootC,countriesList[country_array_list]->d_name,j);
        }
        //strcat(sub_directories[j],"/");
        strcat(sub_directories[j],input_dir);
        strcat(sub_directories[j],"/");
        strcat(sub_directories[j],countriesList[country_array_list]->d_name);
        strcat(sub_directories[j]," ");
        country_array_list++;

    }   
}
//ama exw perisevma to prosthetw sto telos. Dhladh an exw 4 monitors kai 5 xwres, apo panw pairnei 1 xwra to kathe monitor kai apo katw dinoume thn 5h xwra sto 1o monitor. Round robin
if(modulus!=0){
    for(int i=0;i<modulus;i++){
        //strcat(sub_directories[i],"/");
        tempC=searchCountry(&rootC,countriesList[country_array_list]->d_name);
        if(tempC==NULL){
            rootC=addCountry(rootC,countriesList[country_array_list]->d_name,i);
        }
        strcat(sub_directories[i],input_dir);
        strcat(sub_directories[i],"/");
        strcat(sub_directories[i],countriesList[country_array_list]->d_name);
        strcat(sub_directories[i]," ");
        country_array_list++;
    }
}

//innorderC(rootC);

for(int i=0; i<numMonitors;i++){
    int length=strlen(sub_directories[i]);
    sub_directories[i][length]='\0';//vazw \0 sto telos kathe string
}

//for (int i=0; i<numMonitors;i++)
   // printf("%s\n",sub_directories[i] );



char hostname[HOST_NAME_MAX+1];//opws kai sthn main2, o kwdikas exei parthei apo tis diafanies me kapoies allages poy eginan me thn voithia twn post sto piazza
gethostname(hostname,HOST_NAME_MAX+1);
//printf("Parent: hostname %s\n\n",hostname );
struct hostent *ghb; 
struct in_addr address;
ghb=gethostbyname(hostname);
memcpy(&address,*ghb->h_addr_list,sizeof(address));
char * caddress;
caddress=strdup(inet_ntoa(address));
printf("Address: %s\n", caddress);





int server_socket[numMonitors];
struct sockaddr_in server_address[numMonitors];

for(int i=0;i<numMonitors;i++){
    if((server_socket[i]=socket(AF_INET,SOCK_STREAM,0))<0){//dhmiourgeia twn sockets
        printf("Error Creating Server Socket\n");
        exit(1);
    }
    memcpy(&server_address[i],"\0",sizeof(server_address[i]));
}

//Create_All_Fifos(numMonitors);//dhmiourgw ta fifos den xreiazetai gia ta sockets


char* path="./monitorServer";
//dhmiourgw ta strings gia ta orismata sthn execl

//Oi portes einai metaksy 49152-65535 opote megisto strlen einai 6
int port=0;
char portstring[6];
srand(time(NULL));
int portsarray[numMonitors];
port=rand() % (65535 + 1 - 49152) + 49152;
portsarray[0]=port;
sprintf(portstring,"%d",port);
char* p="-p";
char* t="-t";
char* b="-b";
char* c="-c";
char* s="-s";

int *socketfds;
socketfds=malloc(sizeof(int)*numMonitors);

int *id2;
id2=malloc(sizeof(int)*numMonitors);//apothikevw ta proccess id gia ta signals
int  id[numMonitors];
for(int i=0;i<numMonitors;i++)
    id[numMonitors]=0;

id[0]=fork();//kanw to prwto fork
if (id[0]==0){//an eimaste sto paidi   
//execl(path,path,p,port,t,numThreads,b,bufferSize,c,cyclicbufferSize,s,bloomSize,sub_directories[0],NULL);
execl(path,path,p,portstring,t,argv[12],b,argv[4],c,argv[6],s,argv[8],sub_directories[0],NULL);// kai ektelw thn monitor
}
//twra eimai ston patera
    for(int i=1; i<numMonitors;i++){//gia kathe allo monitor pou prepei na dhmiourghsw
        port=port+1;
        portsarray[i]=port;
        sprintf(portstring,"%d",port);
        id[i]=fork();// to idio me panw, ftiaxnw ta orismata, kanw fork kai an eimai sto paidi ektelw to monitor
        if(id[i]==0)
            execl(path,path,p,portstring,t,argv[12],b,argv[4],c,argv[6],s,argv[8],sub_directories[i],NULL);;// kai ektelw thn monitor
        //id[i]=getpid();
    }
//printf("%d \n",id[0] );
for(int i=0;i<numMonitors;i++)//kratw to process id
    id2[i]=id[i];
pid_t temp;
int temp2=0;
//sleep(1);


for(int i=0;i<numMonitors;i++){
    server_address[i].sin_family=AF_INET;//ipv4
    server_address[i].sin_port=htons(portsarray[i]);//port
    server_address[i].sin_addr.s_addr=inet_addr(caddress);//diefthnsh


    if((bind(server_socket[i],(struct sockaddr*)&server_address[i],sizeof(server_address[i])))<0){//kanoume bind ta sockets sthn ip poy vrikame dinamika prin
        printf("Error Binding\n");
        exit(1);
    }
}

socklen_t addr_size;
int size;
struct sockaddr_in new_address;

for(int i=0;i<numMonitors;i++){
    if(listen(server_socket[i],1)==0){//akouei gia connections
        ;//printf("Listening..\n");

    }else{
        printf("Error Listening \n");
        exit(1);
    }
    size=sizeof(new_address);
    socketfds[i]=accept(server_socket[i],(struct sockaddr*)&new_address,&size);//kanei accept ta connections
        //printf("eixame\n");
    if(socketfds[i]==-1){
        printf("Error Accepting!\n");
        exit(1);
    }
}
//Sygxronismos.....
for(int i=0;i<numMonitors;i++){
    read_int_notimeout(socketfds[i]);
}   
//Sygkledronw olous tous ious pou yparxoun.
//Diavazw apo kathe monitor to plhthos twn iwn, meta diavazw to onoma tou kathe iou kai an den to exw apothikefsei hdh sto dentro me tous ious to apothikevw
struct Virus *rootV=NULL;
int noOfVirs = 0;
int fd2=0;
int flag=0;
struct pollfd fdarray[1];
for(int i=0;i<numMonitors;i++){
    
    //printf("%s\n",FIFO_2 );
    fd2=socketfds[i];
    fds[i]=socketfds[i];
    fds2[i]=socketfds[i];
    //printf("fd2=[%d] fds2[%d]=[%d] \n",fd2,i,fds2[i] );
    fdarray[0].fd=fd2;
    fdarray[0].events=POLLIN;
    int viruses_in_monitor=0;
    viruses_in_monitor=read_int(fd2, fdarray);
    //printf("viruses_in_monitor:%d\n",viruses_in_monitor );
    //printf("viruses_in_monitor:%d\n",viruses_in_monitor );
    for(int k=0; k<viruses_in_monitor;k++){
        struct Virus *tempV=NULL;
        int size=0;
        size=read_int(fd2,fdarray);
        char virus_received[size];
        strcpy(virus_received,"");
        int rc=0;
        if(size<=bufferSize){
            rc=poll(fdarray,1,300);
            if(rc==1){
                read(fd2,virus_received,size);
                virus_received[size]='\0';}
            else{
                printf("Error! Poll timeout!\n");
                exit(1);
            }
        }else{
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
                    read(fd2,constring,bufferSize);
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
                    read(fd2,constring,modulus);
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
        //printf("ektypwnw\n");
        //printf("virus_received: %s\n",virus_received );
        tempV = searchVirus(&rootV,virus_received); //search BST for virus
        if (tempV == NULL)
        {
        if (flag==0){
            rootV = addVirus(rootV, virus_received, noOfVirs); //if virus is not found, add it to the BST
            flag=1;
        }
        else
            tempV= addVirus(rootV, virus_received, noOfVirs); //if virus is not found, add it to the BST
        noOfVirs++;

        }
    
    }

}

//printf("fd:[%d] fds2[0]:[%d]\n",fd2,fds2[0] );
//printf("noOfVirs %d\n",noOfVirs );
//innorderV(rootV);

//dhmiourgw kai arxikopoiow me 0 apo 1 bloom filter gia kathe monitor gia kathe io poy yparxei
int ***bArrays;

long int bArrayLength = bloomSize / (long)sizeof(int);
printf("\nParent:There are %ld monitors that have at max %d viruses (rows), each with a bloom 1-d array of size %ld (columns)\n",numMonitors,noOfVirs,bArrayLength);
bArrays=(int***)malloc(numMonitors*sizeof(int**)) ;
for(int i=0; i<numMonitors;i++) {  
    bArrays[i] = (int **)malloc(sizeof (int *)*(noOfVirs));
    for (int v=0; v<noOfVirs; v++) 
        bArrays[i][v] = (int *) malloc(sizeof(int)*bArrayLength);
        for (int lines = 0; lines<noOfVirs; lines++)
            for (long cols=0; cols<bArrayLength; cols++)
                bArrays[i][lines][cols]=0;
}
//innorderV(rootV);
//diavazw ta bloom filter apo kathe monitor
printf("Parent: Now getting BloomFilters from all childs\n");
for(int i=0;i<numMonitors;i++){
    fdarray[0].fd=fds2[i];
    fdarray[0].events=POLLIN;
    printf("Parent: Now copying from child %d\n",i );
get_bloom_from_monitors(rootV,bArrays,bloomSize,noOfVirs,fds2[i],bufferSize,fdarray,i);   
}


//main loop
struct uInput *command;
int result;
 printf("\nyour wish is my command, bro!>");
    command = checkUserInput();
    while (command->command !=0)
    {   
        
        switch (command->command)
        {
        case 1://travelRequest
            result = travel_request(command ,bArrays, rootV,rootC,bloomSize,fds,fds2,bufferSize);
            break;
        case 2:
            //printf("2nd option\n");
            result = travelStats(command,rootC,countries_counter);
            break;//travelStats 
        case 3:
            printf("3rd option\n");

            result = addVaccinationRecords(command,fds,fds2,numMonitors,rootC,bufferSize,noOfVirs,bloomSize,bArrays,rootV,id2);
            //addVaccinationRecords
            break; 
        case 4:
            result = searchVaccinationStatus(command,fds,fds2,bufferSize,numMonitors);
            break; //searchVaccinationStatus
        default:
            printf("Error in command, try again\n");
            break;
        }
	    free(command);
        printf("your wish is my command, bro!>");
        command = checkUserInput();
        

    }
    int ret1=1;
    create_logFile(rootC,countries_counter);//ftiakse logfiles
    for(int i=0;i<numMonitors;i++){
        write_int(fds[i],0);//leei sta paidia na vgoun apo thn loupa tous
    }
    
   
    
while((temp=wait(&temp2))>0);//perimenei na pethanoun ola

//printf("sigchldflag:%d\n",sigchldflag);
printf("Parent:Exiting....freeing memory...\n");  

//close_all_fifos(numMonitors,fds,fds2);
for (int i=0;i<numMonitors;i++){
    close(fds[i]);
}
free(command);  
free(id2); 
//sleep(100);
for(int i=0;i<numMonitors;i++){
    for (int v=0; v<noOfVirs; v++){ 
            free(bArrays[i][v]);
        }
    free(bArrays[i]);
}
free(bArrays);
freeBSTV(rootV);
freeBSTC(rootC);
for(int i=0; i<numMonitors;i++)
    free(sub_directories[i]);
free(sub_directories);
for(int i=0;i<scandir_return;i++)
    free(countriesList[i]);
free(countriesList);
free(caddress);
free(socketfds);
//Delete_All_Fifos(numMonitors);

return 0;
}


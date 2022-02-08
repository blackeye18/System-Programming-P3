//3h ergasia syspro - Panagiotis Mavrommatis
//ThreadFunctions.h
#ifndef __ThreadFunctions__
#define __ThreadFunctions__

struct RoundBuffer* Create_Circular_Buffer(int cyclicbufferSize);
int RB_Is_Full(struct RoundBuffer* round_pointer);
int RB_Is_Empty(struct RoundBuffer* round_pointer);
int add_to_RB(struct RoundBuffer* round_pointer,char* string);
char* get_from_RB(struct RoundBuffer* round_pointer);
void *Thread_function(void* arg);
void *add_vaccination_thread_function(void* arg);


#endif
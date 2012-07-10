#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
//#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>

/* Device Component headers */
#include "dpfp_api.h"
#include "dpfp_api_errors.h"

/* toolkit headers */
#include "dpRCodes.h"
#include "dpDefs.h"
#include "dpFtrEx.h"
#include "dpMatch.h"


#define MAX_TEMPLATES 10

typedef struct{
	char* identifier;
	int   vTemplateSizes[MAX_TEMPLATES];
	void* vTemplates[MAX_TEMPLATES];
} DBUser_t;


typedef struct{
   int status; /* sucess, fingerprint read error, Enrollment canceled, Bad IMG quality, per printcapture depending on MC_MAXprint_count*/
   int MC_MAXprint_count; /* how many times will the print be captured */
   int MC_print_count; /*how may captured so far */
		
   int template_created;
   int template_saved;/*user datastructure updated. if true. go for next finger. save to DB from main */
   int trying_again; /*UI should respond accordingly*/
   int currentFinger;
} DPstatus;


/*
 * waiter thread implementation
 * to wait for user input to cancel active GetTemplate() action
 */

/*
 * parameters for waiter thread
 */
typedef struct {
	char* szCancelPrompt;
	int* stopThread;
	dp_uid_t idDeviceToCancel;
} WaitParams;

typedef struct {
	char sn[25];//serialNo
	char pn[25];//pin
	char id[50];//identifier
	int* stopThread;
	DPstatus * status;
} RegData;

int verifyCode(DBUser_t *jambite,int nFinger,int * stopThread);
int RegisterCode(DBUser_t *jambite,int nFinger,int* stopThread,DPstatus * status);
int daser_FetchTemplate(DBUser_t* pUser,int nFinger,int * doit,int reset);//call other parameters with NULL while last variale as boolen
void* start_registration(void *dataf);
void* start_verification(void* data);
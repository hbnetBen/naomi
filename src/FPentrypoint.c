#include "DPcommon.h"
/*
gcc FPentrypoint.c actions.c database.c -I /opt/DigitalPersona/OneTouchSDK/include/ -L /opt/DigitalPersona/lib -m32 -ldpfpapi -ldpFtrEx -ldl -lc -lm -lpthread -ldpMatch -lsqlite3 `pkg-config --libs --cflags gtk+-2.0` -o output
*/

int RegisterCode(DBUser_t *jambite,int nFinger,int * stopThread,DPstatus * status){
	/* initialize device component */
	int response = 0;
	uint32_t res1 = DPFPInit();
	dp_uid_t idActiveReader = DP_UID_NULL;

	if(0 == res1){

		/* initialize fx */
		FT_RETCODE res2 = FX_init();
		if(FT_OK == res2){

			/* initialize matcher */
			res2 = MC_init();
			
			if(FT_OK == res2){
						
			if(jambite == NULL) return 0;
				
				if(status == NULL) return 0;
								
	int response = ActionRegister(&idActiveReader,nFinger,jambite,stopThread,status);
					
				if(response == 1){
				printf("The finger was Enrolled.\n");
				return response;
				}else{
				printf("The finger was not enrolled.\n");
				}
				
			}
			
			/* release matcher */
				MC_terminate();
				
			/* release fx */
			FX_terminate();
		}

		/* release Device Component */
		DPFPTerm();
	}else{
		switch(res1){
		case DPFP_EUNKNOWN:
			printf("An unexpected error has ocurred.\n");
			break;
		case DPFP_EDRV_NO_LIBRARY:
			printf("The driver library cannot be found.\n");
			break;
		case DPFP_EDRV_NO_INTERFACE:
		printf("The driver library does not export the expected interface.\n");
			break;
		case DPFP_EDEVMGR_CANNOT_OPEN:
			printf("An error occured when opening the driver library.\n");
			break;
		default:
			printf("An unknown error %d has ocurred.\n", res1);
		}
	}

	return response;
}



int verifyCode(DBUser_t *jambite,int nFinger,int *stopThread){
	/* initialize device component */
	int captured = 0; //sent down the pit of hell to get notification 
	int response = 0;
	uint32_t res1 = DPFPInit();
	dp_uid_t idActiveReader = DP_UID_NULL;

	if(0 == res1){

		/* initialize fx */
		FT_RETCODE res2 = FX_init();
		if(FT_OK == res2){

			/* initialize matcher */
			res2 = MC_init();
			if(FT_OK == res2){
			
			if(jambite == NULL) return 0;
				
//response = ActionLoopVerify(&idActiveReader,jambite,nFinger,stopThread);
		
response = ActionPrintRecongnition(&idActiveReader,jambite,nFinger,stopThread,&captured);
				if(1 == response){
				
					printf("The finger was verified.\n");
					}else{
					printf("The finger was not registered.\n");
					}
			}
			
			/* release matcher */
				MC_terminate();
				
			/* release fx */
			FX_terminate();
		}

		/* release Device Component */
		DPFPTerm();
	}
	else{
		switch(res1){
		case DPFP_EUNKNOWN:
			printf("An unexpected error has ocurred.\n");
			break;
		case DPFP_EDRV_NO_LIBRARY:
			printf("The driver library cannot be found.\n");
			break;
		case DPFP_EDRV_NO_INTERFACE:
		printf("The driver library does not export the expected interface.\n");
			break;
		case DPFP_EDEVMGR_CANNOT_OPEN:
			printf("An error occured when opening the driver library.\n");
			break;
		default:
			printf("An unknown error %d has ocurred.\n", res1);
		}
	}

	return response;
}

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
 * actions.c
 * Copyright (C) Daser Retnan 2011 <dasersolomon@gmail.com>
 * 
 * fingerscan is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * daserfostscan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This code contains some codes from digital Persona */

#include <gtk/gtk.h>
#include "DPcommon.h"
#include "database.h"

static gboolean stopfingeraction(void *pData)
{
	if(NULL != pData){
		WaitParams* pwp = (WaitParams*)pData;
			
			if(*(pwp->stopThread) == 1){
			printf("daserfost: Fingerprint Enrollment Interrupted from the GUI\n");
			DPFPUnsubscribe(&pwp->idDeviceToCancel);
			return false;
			}
	}
	
	return true;
}

int DBAddTemplate(DBUser_t* pUser, int nFinger, int cbTemplateSize, void* pTemplate){
	int retval = 0;
	if(nFinger >= MAX_TEMPLATES) return 0;

	if(NULL != pUser){
		if(NULL != pUser->vTemplates[nFinger]) free(pUser->vTemplates[nFinger]);
		pUser->vTemplateSizes[nFinger] = 0;

		pUser->vTemplates[nFinger] = malloc(cbTemplateSize);
		if(NULL != pUser->vTemplates[nFinger]){
			memcpy(pUser->vTemplates[nFinger], pTemplate, cbTemplateSize);
			pUser->vTemplateSizes[nFinger] = cbTemplateSize;
			retval = 0;
		}
		else retval = ENOMEM;
	}
	else retval = ENOENT;

	return retval;
}




/*
 * GetTemplate() acquires fingerprint from the sensor and extracts features according to the nFtrType
 *
 * returns: 
 * 0 - template acquired
 * 1 - operation was cancelled by user
 * 2 - cannot access the reader
 * 3 - template quality is not good enough
 */

int GetTemplate(dp_uid_t* pIdDev, FT_HANDLE hFxContext, FT_FTR_TYPE nFtrType, 
		int* pTemplateSize, FT_BYTE** ppTemplate, FT_IMG_QUALITY* pImgQ, FT_FTR_QUALITY* pFtrQ,int* stopThread){
	int retval = 0;
	int32_t res = DPFPSubscribe(pIdDev, DP_CLIENT_PRIORITY_NORMAL);
	if(0 == res){
		WaitParams *wp = malloc(sizeof(WaitParams));
		int bToContinue = 1;
		wp->szCancelPrompt = NULL;
		wp->stopThread = (int*) malloc(sizeof(int));
		wp->stopThread = stopThread;
		wp->idDeviceToCancel = *pIdDev;

//gdk_threads_add_timeout(100,stopfingeraction,&wp);
gdk_threads_add_timeout_full(G_PRIORITY_DEFAULT,100,stopfingeraction,
                                                         wp,
                                                         NULL);

		/* acquire fingerprint */

		while(bToContinue){
			dp_device_event_t* pEvent = NULL;
			int32_t res = DPFPGetEvent(pIdDev, &pEvent, DP_TIMEOUT_INFINITE);
			if(0 != res){
				/* error while waiting for event */
				retval = 2;
				bToContinue = 0;
			}
			if(NULL != pEvent){
				if(DP_EVENT_STOPPED == pEvent->nEvent){
					/* cancelled */
					retval = 1;
					bToContinue = 0;
				} 
				else if(DP_EVENT_COMPLETED == pEvent->nEvent){
					/* fingerprint accquired, extract features */
					FT_RETCODE tres = ExtractFeatures(hFxContext, nFtrType, 
							      pEvent->uDataSize, pEvent->Data,
							      pTemplateSize, ppTemplate, pImgQ, pFtrQ);
					if(0 != tres) bToContinue = 1;
					else{
						bToContinue = 0;
						if(FT_GOOD_IMG != *pImgQ || FT_GOOD_FTR != *pFtrQ) retval = 3;
						printf("Fingerprint acquired.\n");
					}
				}
				/* release memory */
				DPFPBufferFree(pEvent);
			}
		}

		/* unsubscribe form the reader */
		DPFPUnsubscribe(pIdDev);
	}
	else{
		/* cannot access the reader */
		retval = 2;
	}

	return retval;
}




/*
 * ExtractFeatures() invokes feature extraction             needed by gettemplate
 */
FT_RETCODE ExtractFeatures(FT_HANDLE hContext, FT_FTR_TYPE nFtrType, uint32_t uDataSize, uint8_t* pData, int* pFtrSize, FT_BYTE** ppFtrData, FT_IMG_QUALITY* pImgQ, FT_FTR_QUALITY* pFtrQ){
	FT_RETCODE retval = FT_OK;
	FT_BYTE* pFeatures = NULL;
	int nMinLen = 0, nLen = 0;

	*pFtrSize = 0;
	*ppFtrData = NULL;
	*pImgQ = FT_UNKNOWN_IMG_QUALITY;
	*pFtrQ = FT_UNKNOWN_FTR_QUALITY;

	/* allocate memory for a template */
	retval = FX_getFeaturesLen(nFtrType, &nLen, &nMinLen);
	if(FT_OK == retval) pFeatures = (FT_BYTE*)malloc(nLen);

	if(NULL != pFeatures){
		/* extract features */
		FT_BOOL bOk = 0;
		retval = FX_extractFeatures(hContext, uDataSize, pData, nFtrType, nLen,
					 pFeatures, pImgQ, pFtrQ, &bOk);
		if(FT_OK == retval){
			if(!bOk){
				free(pFeatures);
				*pFtrSize = 0;
				*ppFtrData = NULL;
			}
			else{
				/* return template */
				*pFtrSize = nLen;
				*ppFtrData = pFeatures;
			}
		}
	}
	
	return retval;
}


/*
 * ActionRegister() acquires fingerprints and registers finger 
 */
int ActionRegister(dp_uid_t* pIdDev, int nFinger, DBUser_t* pUser,int * stopThread,DPstatus* status){
	int bRegistered = 0;

	const FT_REG_OPTIONS nRegOptions = FT_DEFAULT_REG;

	FT_BYTE** vpPreRegTemplates = NULL;
	uint8_t* pRegTemplate = NULL;
	int nRegTemplateSize = 0, nMinSize = 0;
	int bToContinue = 1;

	/* create fx and matcher context */
	int bOk = 0;
	FT_HANDLE hFxContext = NULL;
	FT_HANDLE hMcContext = NULL;
	MC_SETTINGS mcSettings;
	FT_RETCODE tres = FX_createContext(&hFxContext);
	if(FT_OK == tres){
		tres = MC_createContext(&hMcContext);
		if(FT_OK == tres){
			tres = MC_getSettings(&mcSettings);
			if(FT_OK == tres) bOk = 1;
		}
	}

	/* allocate memory for pre-reg templates and for registration template*/

	if(bOk){
		tres = MC_getFeaturesLen(FT_REG_FTR, nRegOptions, &nRegTemplateSize, &nMinSize);
		if(FT_OK == tres){
			size_t len = sizeof(FT_BYTE*) * mcSettings.numPreRegFeatures;
			vpPreRegTemplates = (FT_BYTE**)malloc(len);
			if(NULL != vpPreRegTemplates) memset(vpPreRegTemplates, 0, len);
			pRegTemplate = (FT_BYTE*)malloc(nRegTemplateSize);
		}
		if(NULL == vpPreRegTemplates || NULL == pRegTemplate) bOk = 0;
	}

	/* try to register untill success or canceled */

	while(bOk && bToContinue){

		/* clear pre-reg templates */
		int nCnt = 0;
		int nPreRegTemplateSize = 0;
		FT_BOOL bMcOk = 0;
		for(nCnt = 0; nCnt < mcSettings.numPreRegFeatures; nCnt++){
			if(NULL != vpPreRegTemplates[nCnt]) free(vpPreRegTemplates[nCnt]);
			vpPreRegTemplates[nCnt] = NULL;
		}

		/* acquire pre-registration templates */
		
		status->MC_MAXprint_count = mcSettings.numPreRegFeatures;
			
		for(nCnt = 0; nCnt < mcSettings.numPreRegFeatures; nCnt++){
			FT_IMG_QUALITY qImg = FT_UNKNOWN_IMG_QUALITY;
			FT_FTR_QUALITY qFtr = FT_UNKNOWN_FTR_QUALITY;
			int res = GetTemplate(pIdDev, hFxContext, FT_PRE_REG_FTR, 
					      &nPreRegTemplateSize, &vpPreRegTemplates[nCnt], &qImg, &qFtr, stopThread);
					      
		status->MC_print_count = nCnt + 1;
		
			if(0 == res){
				/* success */
		status->status = 1;
			}
			else if(1 == res){
				/* canceled */
		status->status = 2;
				bToContinue = 0;
				printf("Enrollment canceled.\n");
				break;
			}
			else if(2 == res){
				/* reader error */
		status->status = 3;
				printf("  A reader error has ocurred.\n");
				bToContinue = 0;
				break;
			}
			else if(3 == res){
				/* bad quality */
		status->status = 4;
				printf("  Bad quality: Image quality is: %d. Features quality is: %d.\n", qImg, qFtr);
				bToContinue = 1;
				nCnt--;
			}
		}//end for
		if(!bToContinue) break;
		
		/* create a registration template */
		bMcOk = 0;
		tres = MC_generateRegFeatures(hMcContext, nRegOptions, mcSettings.numPreRegFeatures,
					      nPreRegTemplateSize, vpPreRegTemplates,
					      nRegTemplateSize, pRegTemplate, NULL, &bMcOk); 
		if(FT_OK == tres && bMcOk){
				/* success, store registration template in the database */
		status->template_created = 1;
				printf("daser_Sucsfully created and simulated template here\n");
				DBAddTemplate(pUser, nFinger, nRegTemplateSize, pRegTemplate);
		status->template_saved = 1;
				
				bToContinue = 0;
				bRegistered = 1;
		}
		else{
			/* failure */
			printf("  Enrollment failed; try again.\n");
			status->trying_again = 1;
			bToContinue = 1;
		}
			
	}//end while bok && bcontinue

	/* release memory */
	if(NULL != vpPreRegTemplates){
		int nCnt = 0;
		for(nCnt = 0; nCnt <  mcSettings.numPreRegFeatures; nCnt++){
			if(NULL != vpPreRegTemplates[nCnt]) free(vpPreRegTemplates[nCnt]);
		}
		free(vpPreRegTemplates);
	}
	free(pRegTemplate);

	/* release fx and mc contexts */
	if(NULL != hFxContext) FX_closeContext(hFxContext);
	if(NULL != hMcContext) MC_closeContext(hMcContext);

	return bRegistered;
}


/*
 * ActionVerify() acquires fingerprint and matches it with the one passed
 */
int ActionVerify(dp_uid_t* pIdDev, DBUser_t* pUser, int nFinger,int* stopThread){
	int bVerified = 0;

	/* create fx and matcher context */
	int bOk = 0, bToContinue;
	FT_HANDLE hFxContext = NULL;
	FT_HANDLE hMcContext = NULL;
	MC_SETTINGS mcSettings;
	FT_RETCODE tres = FX_createContext(&hFxContext);
	if(FT_OK == tres){
		tres = MC_createContext(&hMcContext);
		if(FT_OK == tres){
			tres = MC_getSettings(&mcSettings);
			if(FT_OK == tres) bOk = 1;
		}
	}

	/* try to register untill success or canceled */
	bToContinue = 1;
	while(bOk && bToContinue){
		/* acquire verification template */
		FT_IMG_QUALITY qImg = FT_UNKNOWN_IMG_QUALITY;
		FT_FTR_QUALITY qFtr = FT_UNKNOWN_FTR_QUALITY;
		FT_BYTE* pTemplate = NULL;
		int nTemplateSize = 0;
		int res = GetTemplate(pIdDev, hFxContext, FT_VER_FTR, 
				      &nTemplateSize, &pTemplate, &qImg, &qFtr,stopThread);
		if(0 == res){
			/* acquired */
			bVerified = 0;

				double faRate = 0;
				FT_BOOL bFtVerified = 0;
				char* szFinger = NULL;	/*Store name of finger for debuging purpose*/
				if(NULL == pUser->vTemplates[nFinger]) continue;

				
				tres = MC_verifyFeaturesEx(hMcContext, 
							   pUser->vTemplateSizes[nFinger], 
							   (FT_BYTE*)pUser->vTemplates[nFinger],
							   nTemplateSize,
							   pTemplate,
							   0, 
							   NULL,
							   NULL,
							   NULL,
							   &faRate,
							   &bFtVerified
							   );
				if(FT_OK != tres){
					printf("An error occured during verification. Touch the reader again.\n");
				}
				else if(bFtVerified){
					/* success */
					printf("  Match   : %s. Achieved FAR: %e\n", szFinger, faRate);
					bVerified = 1;
					bToContinue = 0;
				} else {
					printf("  No match: %s. Achieved FAR: %e\n", szFinger, faRate); // for testing
					bToContinue = 0;
				}
			if (!bVerified){
			printf("  A matching finger was not found; try again.\n");
			}
		}
		else if(1 == res){
			/* canceled */
			bToContinue = 0;
			printf("Verification canceled.\n");
		}
		else if(2 == res){
			/* reader error */
			printf("  A reader error has occured.\n");
			bToContinue = 0;
		}
		else if(3 == res){
			/* bad quality */
			printf("  Bad quality: Image quality is: %d. Features quality is: %d.\n", qImg, qFtr);
			bToContinue = 1;
		}

		/* relese template memory */
		free(pTemplate);
	}//end while bok bcontinue

	/* release fx and mc contexts */
	if(NULL != hFxContext) FX_closeContext(hFxContext);
	if(NULL != hMcContext) MC_closeContext(hMcContext);

	return bVerified;
}





/*
 * ActionLoopVerify() acquires fingerprint and matches it with the one passed
 */
int ActionLoopVerify(dp_uid_t* pIdDev, DBUser_t* pUser, int nFinger,int* stopThread){
	int bVerified = 0;

	/* create fx and matcher context */
	int bOk = 0, bToContinue;
	FT_HANDLE hFxContext = NULL;
	FT_HANDLE hMcContext = NULL;
	MC_SETTINGS mcSettings;
	FT_RETCODE tres = FX_createContext(&hFxContext);
	if(FT_OK == tres){
		tres = MC_createContext(&hMcContext);
		if(FT_OK == tres){
			tres = MC_getSettings(&mcSettings);
			if(FT_OK == tres) bOk = 1;
		}
	}

	/* try to register untill success or canceled */
	bToContinue = 1;
	while(bOk && bToContinue){

		if(1){
			/* acquired */
			bVerified = 0;

				double faRate = 0;
				FT_BOOL bFtVerified = 0;
				char* szFinger = NULL;	/*Store name of finger for debuging purpose*/
				if(NULL == pUser->vTemplates[nFinger]) continue;
				
				
				FT_BYTE* pTemplate = NULL;
				int nTemplateSize = 0;
				int doit = 0;
			while(daser_FetchTemplate(pUser,nFinger,&doit,0) != 0){

				tres = MC_verifyFeaturesEx(hMcContext, 
							   pUser->vTemplateSizes[nFinger], 
							   (FT_BYTE*)pUser->vTemplates[nFinger],
							   nTemplateSize,
							   pTemplate,
							   0, 
							   NULL,
							   NULL,
							   NULL,
							   &faRate,
							   &bFtVerified
							   );
				if(FT_OK != tres){
					printf("An error occured during verification.\n");
					bToContinue = 0;
					break;
				}
				else if(bFtVerified){
					/* success */
					printf("  Match   : %s. Achieved FAR: %e\n", szFinger, faRate);
					bVerified = 1;
					bToContinue = 0;
					break;
				} else {
				printf("  No match: %s. Achieved FAR: %e\n", szFinger, faRate); // for testing
					bToContinue = 0;
				}
			}//WEND
			
			if (!bVerified){
			printf("  A matching finger was not found; try again.\n");
			}
		}//FI(1)

		/* relese template memory */
		//free(pTemplate); /*FIX ME*/
	}//end while bok bcontinue

	/* release fx and mc contexts */
	if(NULL != hFxContext) FX_closeContext(hFxContext);
	if(NULL != hMcContext) MC_closeContext(hMcContext);

	return bVerified;
}


int daser_FetchTemplate(DBUser_t* pUser,int nFinger,int * doit,int reset)
{	
	static DBusers* data = NULL;
	static int check = 1;
	
	if(reset){
	check = 1;
	data = NULL;
	return 0;
	}
	
	if(check){
	data = fetch_DBusers(3,NULL); //all users in the DB
	check = 0;
	}
	
	if(data == NULL)return 0;
	
	int size = 0;
	char* filename = getFilePath(data->serialNo,&size,nFinger);
	
printf("file name=%s and size of file=%d filename=%s\n",data->serialNo,size,filename);
		if(!filename){
		data = data->next; //being a static variable.
		
				if(data == NULL){
				return 0;
				}else{
				return 1;
				}
		}else{
		size_t namelen = strlen(data->serialNo) + 1;
		pUser->identifier = (char*)malloc(namelen);
		
			if(NULL == pUser->identifier){
			data = data->next; //being a static variable.
		
				if(data == NULL){
				return 0;
				}else{
				return 1;
				}
			}
			
	strncpy(pUser->identifier,data->serialNo,namelen);//+null char
	
		pUser->vTemplates[nFinger] = (FT_BYTE*)malloc(size);
		pUser->vTemplateSizes[nFinger] = size;
		FILE* input;
		input=fopen(filename,"rb");
		fread(pUser->vTemplates[nFinger],size,1,input);
		fclose(input);
		printf("file read sucessfuly\n");
		data = data->next; //being a static variable.
		*doit = 1;
		return 1;
		}
	
}



/*
 * ActionPrintRecongnition() acquires fingerprint and matches it with the one passed
 */
int ActionPrintRecongnition(dp_uid_t* pIdDev, DBUser_t* pUser, int nFinger,int* stopThread,int * captured){
	int bVerified = 0;

	/* create fx and matcher context */
	int bOk = 0, bToContinue;
	FT_HANDLE hFxContext = NULL;
	FT_HANDLE hMcContext = NULL;
	MC_SETTINGS mcSettings;
	FT_RETCODE tres = FX_createContext(&hFxContext);
	if(FT_OK == tres){
		tres = MC_createContext(&hMcContext);
		if(FT_OK == tres){
			tres = MC_getSettings(&mcSettings);
			if(FT_OK == tres) bOk = 1;
		}
	}

	/* try to register untill success or canceled */
	bToContinue = 1;
	while(bOk && bToContinue){
		/* acquire verification template */
		FT_IMG_QUALITY qImg = FT_UNKNOWN_IMG_QUALITY;
		FT_FTR_QUALITY qFtr = FT_UNKNOWN_FTR_QUALITY;
		FT_BYTE* pTemplate = NULL;
		int nTemplateSize = 0;
		int res = GetTemplate(pIdDev, hFxContext, FT_VER_FTR, 
				      &nTemplateSize, &pTemplate, &qImg, &qFtr,stopThread);
		if(0 == res){
			/* acquired  tell GUI via captured*/
		bToContinue = 0;
		*captured = 1;
		
			bVerified = 0;

				double faRate = 0;
				FT_BOOL bFtVerified = 0;
	char* szFinger = NULL;	/*Store name of finger for debuging purpose*/
				int doit = 0;
			while(daser_FetchTemplate(pUser,nFinger,&doit,0)){
		//generate the usersdatastructure and return the pointer to the newuser structure goten from the database or likedlist
		
			if(doit){
			doit = 0;
				tres = MC_verifyFeaturesEx(hMcContext, 
							  pUser->vTemplateSizes[nFinger], 
						(FT_BYTE*)pUser->vTemplates[nFinger],
							   nTemplateSize,
							   pTemplate,
							   0, 
							   NULL,
							   NULL,
							   NULL,
							   &faRate,
							   &bFtVerified
							   );
				if(FT_OK != tres){
					printf("An error occured during verification.\n");
					break;
				}
				else if(bFtVerified){
					/* success */
			printf("  Match   : %s. Achieved FAR: %e\n", szFinger, faRate);
					bVerified = 1;
					break;
				} else {
	printf("  No match: %s. Achieved FAR: %e\n", szFinger, faRate); // for testing
				}
			}//FI
		  }//WEND
		  
daser_FetchTemplate(NULL,0,NULL,1);//Call this to reset global static variable 
		  
			if (!bVerified){
			printf("  A matching finger was not found;.\n");
			}
		}
		else if(1 == res){
			/* canceled */
			printf("Verification canceled.\n");
		}
		else if(2 == res){
			/* reader error */
			printf("  A reader error has occured.\n");
		}
		else if(3 == res){
			/* bad quality */
			printf("  Bad quality: Image quality is: %d. Features quality is: %d.\n", qImg, qFtr);
		}

		/* relese template memory */
		free(pTemplate);
	bToContinue = 0;
	}//end while bok bcontinue

	/* release fx and mc contexts */
	if(NULL != hFxContext) FX_closeContext(hFxContext);
	if(NULL != hMcContext) MC_closeContext(hMcContext);

	return bVerified;
}


void* start_registration(void * dataf)
{
	RegData* data = (RegData*)dataf;
	DBUser_t *jambite = NULL;
	DBusers* myDBuser = NULL;

	myDBuser = malloc(sizeof(DBusers));
	if(myDBuser == NULL) return 0;

	jambite = (DBUser_t*) malloc(sizeof(DBUser_t));			
	if(jambite == NULL) return 0;

	size_t namelen = strlen(data->sn) + 1;
	jambite->identifier = (char*)malloc(namelen);
								
	if(NULL == jambite->identifier){
	free(jambite);
	jambite = NULL;
	}else{
	int i;
	strncpy(jambite->identifier, data->sn, namelen);
		for(i = 0; i < MAX_TEMPLATES; i++){
			jambite->vTemplateSizes[i] = 0;
			jambite->vTemplates[i] = NULL;
		}//NEXT
	}//FI

				if(data->status == NULL) return 0;
				
	int nFinger;
	int response=0;
	int proceed = 1;	
		while(proceed){//enter a while loop her for more than 1 fingers
				
			//grab finger
				nFinger = getnFinger(0); //static var
				data->status->currentFinger = nFinger;
				if(nFinger == -1){//-1=no more left
				proceed = 0;
				break;
				}
				
			//tell UI here that we are to capture for nFinger
			response = RegisterCode(jambite,nFinger,data->stopThread,data->status);
			
				if(response != 1){
					if(*(data->stopThread) == 1){
					response=0; //just incase shit happens
					return 0;
					}else{
					fprintf(stderr,"Error registring\n");
					return 0;
					}
				}
			//tell UI here that we have captured for nFinger
			
		} //wend
	if(*(data->stopThread) == 1)response=0;

	if(response == 1){
			strcpy(myDBuser->serialNo,data->sn);
			
			strcpy(myDBuser->pin,data->pn);
			
			strcpy(myDBuser->identifier,data->id);//from textbox
			myDBuser->status = 0;
			myDBuser->synced = 0;
			
			int res = insert_DBuser(myDBuser);
			
		if(res){
			//char * getToSaveFinger(char * serial)
			char * output = (char*)getToSaveFinger(jambite->identifier);
			proceed = 1;
			while(proceed){//foreach of the fingers registerd above
				
				//grab finger
				
				nFinger = getnFinger(0); //static var
				if(nFinger == -1){
				proceed = 0;
				break;
				}
				
				
				if(output != NULL){
				FILE* output;
				output=fopen(getFingerFileName(nFinger),"wb");
				fwrite(jambite->vTemplates[nFinger],jambite->vTemplateSizes[nFinger],1,output);
				fclose(output);
				}else{
				fprintf(stderr,"oopse! error geting finger dir\n");
				return 0;
				}
				
			}//wend
			
				}else{
				fprintf(stderr,"Couldnot insert record to DB\n");
				}
			
			
		}else{
		fprintf(stderr,"could not enroll this user\n");
		}
return 1;
}

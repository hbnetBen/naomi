/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Daser Retnan 2011 <dasersolomon@gmail.com>
 * 
 * daserfostscan is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * fdaserfostscan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <config.h>
#include "database.h"

static char *finger[10] = {"l0.bin","l1.bin","l2.bin","l3.bin","l4.bin","r5.bin","r6.bin","r7.bin","r8.bin","r9.bin"};

char* database = "fingerDB.db";
DBusers* UserSList = NULL;
int finger_supported[] = {5,7};/*CHANGE THIS ASAP*/

#define A_SIZE(A) sizeof(A)/sizeof(A[0])

char* getFingerFileName(nFinger)
{
return finger[nFinger];
}

gboolean createDatabaseForUser()
{
	/*Source*/
	GString* file = g_string_new((gchar*) PACKAGE_DATA_DIR);
	file = g_string_append (file,"/"); /*hidden file .PACKAGE*/
	file = g_string_append (file,(gchar*)PACKAGE);
	file = g_string_append (file,"/");
	file = g_string_append (file,(gchar*)database);
	/*Destination*/
	//char *filestr = get_database_home_dir();
	
	GString* inode = g_string_new((gchar*) g_get_home_dir());
	inode = g_string_append (inode,"/."); /*hidden file .PACKAGE*/
	inode = g_string_append (inode,PACKAGE);
	inode = g_string_append (inode,"/");
	
	GFile * sgfile = g_file_new_for_path((char*)file->str);

	//
	//if(g_chdir(ginode->str) == 0){
	//GFile *             g_file_new_for_path                 (const char *path);
	if(g_mkdir(inode->str,S_IRWXU) == 0){
		inode = g_string_append (inode,(gchar*)database);
		GFile * dgfile = g_file_new_for_path((char*)inode->str);
			//creating database for this user and populating it with dbfinger in PACKAGE_DATA_DIR
			if(!g_file_copy(sgfile, dgfile, G_FILE_COPY_NONE, NULL, NULL, NULL,NULL)){
				fprintf(stderr,"Fatal Error! contact me @ dasersolomon@gmail.com\n");
				exit(2);
			}
		
	}else{
	return FALSE;
	}

return TRUE;
}

gboolean doesDBexist(){
char* file = get_database_path();
	if(file==NULL){
	return FALSE;
	}else{
	return TRUE;
	}
}
char* get_database_path()
{
	
	GString* inode = g_string_new((gchar*) g_get_home_dir());
	inode = g_string_append (inode,"/."); /*hidden file .PACKAGE*/
	inode = g_string_append (inode,PACKAGE);
	inode = g_string_append (inode,"/");
	inode = g_string_append (inode,(gchar*)database);

	if(g_file_test(inode->str,G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)){
	return (char*)inode->str;
	}else{
	return NULL;
	}
	
}

static int callback(void* notused,int num_columns,char** row,char** colname)
{
	DBusers* temp = (DBusers *) malloc(sizeof(DBusers));
	DBusers* current = NULL;
	DBusers* new = NULL;
	
	if(temp == NULL){
	fprintf(stderr,"memory exausted");
	return;
	}
	
	temp->status = 0;
	temp->synced = 0;
	
	size_t lent = 0;
	
	int i=0;
	while(i !=num_columns){

		if(g_strcmp0(colname[i],"serialNo") == 0){
			//lent = strlen((char *)row[i]) + 1;
			//temp->serialNo = (char *) malloc(sizeof(lent));
			//strncpy(temp->serialNo, (char *)row[i], lent);
			strcpy(temp->serialNo, (char *)row[i]);
		}
		
		
		if(g_strcmp0(colname[i],"pin") == 0){
			//lent = strlen(row[i]) + 1;
			//temp->pin = (char *) malloc(sizeof(lent));
			//strncpy(temp->pin, row[i], lent);
			strcpy(temp->pin, row[i]);
		}
		
		if(g_strcmp0(colname[i],"identifier") == 0){
			strcpy(temp->identifier, row[i]);
		}

		if(g_strcmp0(colname[i],"status") == 0){
			temp->status = atoi(row[i]);			
		}
		
		if(g_strcmp0(colname[i],"synced") == 0){
			temp->synced = atoi(row[i]);			
		}
	
			
	i++;
	}

	if(UserSList == NULL){
	UserSList = temp;
	UserSList->next = new;
	}else{
	current = temp;
	current->next = UserSList;
	UserSList = current;
	}

return 0;
}


DBusers* fetch_DBusers(int x,char * serialNo)
{	
	/*Keeps appending to previous list upone a recurcivee all to this func.*/
	UserSList = NULL;
	
	sqlite3* conn;
	int status;
	char* error_mesg = NULL;
	char * sql = NULL;
	
	if(serialNo == NULL){
		if(x == 1){
		sql ="SELECT * FROM enrolled WHERE status = 1";
		}else
		if(x == 2){
		sql ="SELECT * FROM enrolled WHERE synced = 1";
		}else{
		sql ="SELECT * FROM enrolled";
		}
	}else{
	GString *sq = g_string_new("SELECT * FROM enrolled WHERE serialNo='");
	sq = g_string_append(sq,(gchar*)serialNo);
	sq = g_string_append(sq,"'");
	sql = (char*) sq->str;
	}
	
	char *dbname = get_database_path();
	if(dbname == NULL){
		if(createDatabaseForUser()){
		dbname = get_database_path();
				if(dbname==NULL){
				fprintf(stderr,"Fatal Error..database not found\n");
				exit(2);
				}
		}
	}
	
	status = sqlite3_open(dbname,&conn);
	
	if(status != SQLITE_OK){
	fprintf(stderr,"Couldnot Connect or locate the database file");
	return;
	}
	
	
	status = sqlite3_exec(conn,sql,callback,NULL,&error_mesg);
	
	if(status != SQLITE_OK){
	fprintf(stderr,"%s\n",error_mesg);
	return;
	}
	//printf("addrese in database=%p\n",UserSList);
	return UserSList;

}


int insert_DBuser(DBusers* jambite)
{
	sqlite3* conn;
	int status;
	char* error_mesg = NULL;
	GString * sql = NULL;
	
	//insert into enrolled VALUES("972732932","87283232","Zipporah",0,0)
	sql = g_string_new("INSERT INTO enrolled VALUES('");
	
	sql = g_string_append(sql,(gchar *)jambite->serialNo);
	sql = g_string_append(sql,"','");
	
	sql = g_string_append(sql,(gchar *)jambite->pin);
	sql = g_string_append(sql,"','");
	
	sql = g_string_append(sql,(gchar *)jambite->identifier);
	sql = g_string_append(sql,"','");
	
		if(jambite->status){
		sql = g_string_append(sql,(gchar *)"1");
		}else{
		sql = g_string_append(sql,(gchar *)"0");
		}
		
	sql = g_string_append(sql,"','");
	
		if(jambite->status){
		sql = g_string_append(sql,(gchar *)"1");
		}else{
		sql = g_string_append(sql,(gchar *)"0");
		}
	
	sql = g_string_append(sql,"')");

	fprintf(stdout,"sql:%s\n\n",(char *)sql->str);

	char *dbname = get_database_path();
	if(dbname == NULL){
		if(createDatabaseForUser()){
		dbname = get_database_path();
				if(dbname==NULL){
				fprintf(stderr,"Fatal Error..database not found\n");
				exit(2);
				}
		}
	}
	status = sqlite3_open(dbname,&conn);
	
	if(status != SQLITE_OK){
	fprintf(stderr,"Couldnot Connect or locate the database file");
	return;
	}
	
	
	status = sqlite3_exec(conn,(char *)sql->str,NULL,NULL,&error_mesg);
	
	if(status == SQLITE_OK){
	return 1;
	}else{
	fprintf(stderr,"%s\n",error_mesg);
	return 0;
	}
return 0;
}


char * getFilePath(char * serialNo,int * size,int nFinger){
	struct stat tbuf;
	GString* filenode = NULL;
	filenode =  g_string_new((gchar *) getUserFingerDir());
	
	filenode = g_string_append(filenode,"/");
	
	filenode = g_string_append(filenode,(gchar *)serialNo);
	
	filenode = g_string_append(filenode,"/");
	
	filenode = g_string_append(filenode,(gchar *)finger[nFinger]);
	
	if(g_file_test(filenode->str,G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)){
	
		if(stat(filenode->str,&tbuf) == 0){
		printf("file size:%d\n",(int) tbuf.st_size);	
		*(size) = (int) tbuf.st_size;
		return filenode->str;
		}else{
		perror ("error stating files\n");
		return NULL;
		}
	}
}

char * getUserFingerDir(){

	GString* filenode = NULL;
	filenode =  g_string_new(g_get_home_dir());
	filenode = g_string_append(filenode,(gchar *)FIN_DIR);	
	
	return (char *) filenode->str;
}

char * getToSaveFinger(char* serialNo){
	char * inode = NULL;
	GString *ginode = g_string_new(getUserFingerDir());
	ginode = g_string_append(ginode,"/");
	ginode = g_string_append(ginode,(gchar *)serialNo);	
	
	if(g_file_test(ginode->str,G_FILE_TEST_IS_DIR)){
	//chdirec
			if(g_chdir(ginode->str) == 0){
			printf("sucessfuly ch'ed directory\n");
			return (char *)ginode->str;
			}else{
			perror("Cannot change directory\n");
			return NULL;
			}
	
	
	}else{
	//make directory and chdirectory to it
	printf("folder to create:%s\n",ginode->str);
	
		if(g_mkdir(ginode->str,S_IRWXU) == 0){
		//chdir now
		printf("directory has been created\n");
		
			if(g_chdir(ginode->str) == 0){
			printf("sucessfuly ch'ed directory\n");
			return (char *)ginode->str;
			}else{
			perror("Cannot change directory\n");
			return NULL;
			}
			
		}else{
		perror("could not mkdirectory");
		return NULL;
		}
	}
}


int getnFinger(int reset)
{
	static int y = -1;

	if(reset){
	y=-1;
	return y;
	}
	
	if(y == -1){
	y = A_SIZE(finger_supported) -1;
	}else{
	y = y - 1;
	}
	
	if(y == -1){
	return y;
	}else{
	return finger_supported[y];
	}
}

int getNumberofFingers()
{
return A_SIZE(finger_supported);
}

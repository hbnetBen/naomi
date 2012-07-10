/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * 
 * Copyright (C) Daser Retnan 2011 <dasersolomon@gmail.com>
 * 
 * daserfostscan is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FIN_DIR "/.finger" /*Dont Touch*/

typedef struct dbusers{
char serialNo[50];
char pin[50];
char identifier[80];
int status;
int synced;

struct dbusers* next;

}DBusers;


DBusers* fetch_DBusers(int result_t,char * serialNo); //NULL for all, 1 for synced and 2 for ready to synced

int insert_DBuser(DBusers* jambite);

char * getFilePath(char * serialNo,int * size,int nFinger);

char * getUserFingerDir(void);/* /home/daser/.finger */
char * getToSaveFinger(char * folder); //serialNo: create folder with serialno and chdir to it

int getnFinger(int reset);
char* getFingerFileName(int nFinger);//eg r7.bin
int getNumberofFingers(void);
char* get_database_path();
gboolean createDatabaseForUser();
gboolean doesDBexist(void);

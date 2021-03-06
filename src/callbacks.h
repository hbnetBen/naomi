/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) Daser Retnan 2011 <dasersolomon@gmail.com>
 * 
 * daserfostscan is free software: you can redistribute it and/or modify it
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

#include <gtk/gtk.h>
#include "DPcommon.h"
typedef struct{
GtkBuilder *builder;
RegData *reg;
GtkWidget *pbar;
}GUIdata;

typedef struct{
GtkBuilder *builder;
int *wait;
int currentnFinger;
GtkWidget *pbar;
}pbar_data;
void destroy (GtkWidget *widget, gpointer data);

void get_enrollment_form (GtkWidget *widget, gpointer data);

void enroll_user(GtkWidget *widget, gpointer data);

void start_syncing(GtkWidget *widget, gpointer data);

void delete_student_fromtable(GtkWidget *widget, gpointer data);

void get_verification_dialog(GtkWidget *widget, gpointer data);

void get_report(GtkWidget *widget, gpointer data);

void get_ready_data(GtkWidget *widget, gpointer data);

void get_all_data(GtkWidget *widget, gpointer data);

void get_synced_data(GtkWidget *widget, gpointer data);

void start_syncing(GtkWidget *widget, gpointer data);

void get_enrollment_form(GtkWidget *widget, gpointer data);

void kill_winenroll_winevent(GtkWidget *widget,gpointer data);

void kill_winenroll_winverify(GtkWidget *widget,gpointer *data);
//void get_ready_report(GtkTreeView *tview);
void tell_pbar_to_cont(GtkWidget *widget,gpointer *data);



/*call back hooks*/
void* start_verification(void* data);
static gboolean progress_update(void* guidata);
void progress_destroyed(void* guidata);
char *getFingerImagepath(int nFinger);
char *getProgessText(int nFinger);
char * getFingerScore(int nFinger);
static gboolean progress_update_verify(void* guidata);
char *getCompleteFilepath(char* absolute_name);
GtkTreeModel * create_and_fill_model (int report_type);

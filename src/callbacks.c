/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "callbacks.h"
#include "database.h"
#include <pthread.h>

int *vstopThread = NULL;
int * stop_fingerprint= NULL;
int * stop_pbar = NULL;//(int*)malloc(sizeof(int));
int *glowait = NULL;
GtkBuilder *diagwin = NULL; //kill it when winevent is down

enum
{
  COL_IDENT = 0,
  COL_SERIAL,
  COL_PIN,
  COL_STATUS,
  COL_SYNCED,
  NUM_COLS
} ;

char *getCompleteFilepath(char* absolute_name)
{
#if !defined(PACKAGE_DATA_DIR)
#define PACKAGE_DATA_DIR "/home/daser/crapos/src/
#endif
	GString* file = g_string_new((gchar*) PACKAGE_DATA_DIR);
	file = g_string_append (file,"/");
	file = g_string_append (file,PACKAGE);
	file = g_string_append (file,"/");
	file = g_string_append (file,absolute_name);
	return (char*)file->str;
}
GtkTreeModel *
create_and_fill_model (int report_type)//1=status,2=synced,3=all
{
  GtkListStore  *store;
  GtkTreeIter    iter;
  
  store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_UINT,G_TYPE_UINT);
	
	DBusers* data = NULL;
	data = fetch_DBusers(report_type,NULL);
	DBusers* temp = data;
	
	if(data == NULL)return NULL; //empty

	while(data){

	// Append a row and fill in some data 
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter,
                COL_IDENT, data->identifier,
                COL_SERIAL, data->serialNo,
  				COL_PIN, data->pin,
  				COL_STATUS, data->status,
				COL_SYNCED, data->synced,
                -1);
		data = data->next;
	}

  return GTK_TREE_MODEL (store);
}


void
get_report(GtkWidget *widget, gpointer data)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, getCompleteFilepath("winreport.ui"), &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* This is important */
	gtk_builder_connect_signals (builder, NULL);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "winreport"));
	
	GdkPixbuf *icon = gdk_pixbuf_new_from_file(getCompleteFilepath("daserfostscan.png"),NULL);
	gtk_window_set_icon(window,icon);
	
	GtkWidget *toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "toolbar1"));

	GtkWidget *ready = gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *synced = gtk_image_new_from_stock(GTK_STOCK_YES,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *all = gtk_image_new_from_stock(GTK_STOCK_INFO,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *upload = gtk_image_new_from_stock(GTK_STOCK_INFO,GTK_ICON_SIZE_LARGE_TOOLBAR);

	GtkToolItem *iall = gtk_tool_button_new(all,"All");
	GtkToolItem *iready = gtk_tool_button_new(ready,"Ready");
	GtkToolItem *isynced = gtk_tool_button_new(synced,"Synced");
	GtkToolItem *iupload = gtk_tool_button_new(upload,"Upload to server");

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), iall, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), iready, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), isynced, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), iupload, -1);

	GtkWidget *tview = gtk_tree_view_new();

	g_signal_connect(G_OBJECT(iall), "clicked", G_CALLBACK(get_all_data),tview);

	g_signal_connect(G_OBJECT(iready), "clicked", G_CALLBACK(get_ready_data),tview);
	g_signal_connect(G_OBJECT(isynced), "clicked", G_CALLBACK(get_synced_data),tview);
	g_signal_connect(G_OBJECT(iupload), "clicked", G_CALLBACK(start_syncing),NULL);
//LIST STORE MODEL

	GtkWidget *Vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox2")); 

	if(Vbox == NULL){
	fprintf(stderr,"ERROR PARSING FILE\n");
		exit(2);
	}

	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL,NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
                                    
	gtk_box_pack_start(Vbox,scrolled_window,TRUE,TRUE,0);
	gtk_box_reorder_child(Vbox,scrolled_window,2);
	
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), tview);

	GtkCellRenderer  *renderer;
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tview),
                                               -1,      
                                               "Identifier",  
                                               renderer,
                                               "text", COL_IDENT,
                                               NULL);
	
    renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tview),
                                               -1,      
                                               "Serial no/Username",  
                                               renderer,
                                               "text", COL_SERIAL,
                                               NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tview),
                                               -1,      
                                               "Pin/password",  
                                               renderer,
                                               "text", COL_PIN,
                                               NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tview),
                                               -1,      
                                               "Validated",  
                                               renderer,
                                               "text", COL_STATUS,
                                               NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tview),
                                               -1,      
                                               "Uploaded",  
                                               renderer,
                                               "text", COL_SYNCED,
                                               NULL);
	
	GtkTreeModel *model;
	model = create_and_fill_model(3);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (tview), model);

	//g_object_unref (model);
	
	gtk_widget_show_all(window);

	//g_object_unref (builder);
}

void get_ready_data(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model;
	model = create_and_fill_model(1);
	gtk_tree_view_set_model (GTK_TREE_VIEW ( (GtkWidget *)data), model);
}

void get_all_data(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model;
	model = create_and_fill_model(3);
	gtk_tree_view_set_model (GTK_TREE_VIEW ( (GtkWidget *)data), model);
}

void get_synced_data(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model;
	model = create_and_fill_model(2);
	gtk_tree_view_set_model (GTK_TREE_VIEW ( (GtkWidget *)data), model);
}

void
destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}


void
get_enrollment_form (GtkWidget *widget, gpointer data)
{   
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;
	
		builder = gtk_builder_new ();
		if (!gtk_builder_add_from_file (builder, getCompleteFilepath("winenroll.ui"), &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free (error);
		}	/* This is important */
	gtk_builder_connect_signals (builder, builder);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "winenroll"));
	
	GdkPixbuf *icon = gdk_pixbuf_new_from_file(getCompleteFilepath("daserfostscan.png"),NULL);
	gtk_window_set_icon(window,icon);
	
	g_object_ref(builder);
	diagwin = builder;
	gtk_widget_show (window);

	//g_object_unref (builder);

}


void
enroll_user(GtkWidget *widget, gpointer data)
{
	GtkBuilder *builder = NULL;
	GtkBuilder *diag_builder;
	GError* error = NULL;
	diag_builder = data;

		builder = gtk_builder_new ();
		if (!gtk_builder_add_from_file (builder, getCompleteFilepath("winevent.ui"), &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free (error);
		}
	/* This is important */

	stop_pbar = malloc(sizeof(int));
	*stop_pbar = 0;
		
	gtk_builder_connect_signals (builder, builder);
	
	GtkWidget* serialNo = GTK_WIDGET (gtk_builder_get_object (diag_builder, "txtusername_enroll"));
	GtkWidget* pin = GTK_WIDGET (gtk_builder_get_object (diag_builder, "txtpasswd_enroll"));
	GtkWidget* identifier = GTK_WIDGET (gtk_builder_get_object (diag_builder, "txtname_enroll"));
	GtkWidget *header = GTK_WIDGET (gtk_builder_get_object (builder, "imghead"));
	GdkPixbuf *pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("imgheader.png"),NULL);
	gtk_image_set_from_pixbuf(header,pixbufer);

	char * sn = (char*)gtk_entry_get_text(serialNo);
	char * pn = (char*)gtk_entry_get_text(pin);
	char * id = (char*)gtk_entry_get_text(identifier);

	
	/* BUILD THE USER DATASTRUCTURE BEFORE DESTROYING THE BOX */

		/*START A THREAD HERE*/
		RegData *regdata = (RegData*)malloc(sizeof(RegData));
		strcpy(regdata->sn,sn);
		strcpy(regdata->pn,pn);
		strcpy(regdata->id,id);
		regdata->stopThread = malloc(sizeof(int));
		stop_fingerprint = regdata->stopThread; /* global variable to kill scanner*/
		*regdata->stopThread = 0;

		regdata->status = (DPstatus *) malloc(sizeof(DPstatus));
				regdata->status->status = 0;
				regdata->status->MC_MAXprint_count = 0;
				regdata->status->MC_print_count = 0;
				regdata->status->template_created = 0;
				regdata->status->template_saved = 0;
				regdata->status->trying_again = 0;

		GtkWidget *pbar;
		pbar = GTK_WIDGET (gtk_builder_get_object (builder, "pbar"));
	
		GUIdata *guidata = (GUIdata*)malloc(sizeof(GUIdata));
		guidata->builder = builder;
		guidata->reg = regdata;
		guidata->pbar = pbar;

		gdk_threads_add_timeout_full(G_PRIORITY_DEFAULT,100,progress_update,
                                                         guidata,
                                                         progress_destroyed);
		pthread_t thread_id;
		getnFinger(1);//just avoid the trouble...problems with static vars
		pthread_create (&thread_id, NULL, &start_registration, regdata);
	
}

void kill_winenroll_winevent(GtkWidget *widget,gpointer data)
{
	static close_win = 0;
	if(close_win){
	gtk_widget_destroy(GTK_WIDGET (gtk_builder_get_object ((GtkBuilder*)diagwin, "winenroll")));
	gtk_widget_destroy(GTK_WIDGET (gtk_builder_get_object ((GtkBuilder*)data, "winevents")));
	g_object_unref((GtkBuilder*)diagwin);
	g_object_unref((GtkBuilder*)data);
	*stop_fingerprint = 0;
	*stop_pbar = 0;
	close_win=0;
	return;
	}else{
	*stop_fingerprint = 1;
	*stop_pbar = 1;
	}
gtk_button_set_label(widget,"Close");
close_win=1;

}

static gboolean progress_update(void* guidata)
{
	GUIdata *guidat = (GUIdata*) guidata;
	static int start_again = 1;
	static int track = 0;
	
	if(*stop_pbar){
	*stop_pbar = 0;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),"Enrollment has been Cancelled");		
	start_again = 1;
	track = 0;
	return false;
	}
	
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR(guidat->pbar));
	int max = guidat->reg->status->MC_MAXprint_count;
	int sofar = guidat->reg->status->MC_print_count;
	GtkWidget *image;
	GtkWidget *imgscore;
	GdkPixbuf *pixbufer;
	int num_fingers = getNumberofFingers();
	int cur_finger = guidat->reg->status->currentFinger;
	
	if(start_again){
	track = cur_finger;
	image = GTK_WIDGET (gtk_builder_get_object (guidat->builder, "imgfinger"));
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(getFingerImagepath(cur_finger),NULL);
	gtk_image_set_from_pixbuf(image,pixbuf);

		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),getProgessText(cur_finger));
		
		if(cur_finger == -1){
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
			return FALSE;
		}
		
	start_again = 0;
	}

	if(track != cur_finger){
	start_again = 1;
	}

		if(sofar == 0){
		imgscore = GTK_WIDGET (gtk_builder_get_object (guidat->builder, "imgscore"));
		pixbufer = gdk_pixbuf_new_from_file(getFingerScore(0),NULL);
		gtk_image_set_from_pixbuf(imgscore,pixbufer);
		}else{
		imgscore = GTK_WIDGET (gtk_builder_get_object (guidat->builder, "imgscore"));
		pixbufer = gdk_pixbuf_new_from_file(getFingerScore(sofar),NULL);
		gtk_image_set_from_pixbuf(imgscore,pixbufer);
		}
	
	
	//
	//
//printf("max=%d==captureing for finger=%d\n",max,guidat->reg->status->currentFinger);
//printf("Fin path=%s\n",getFingerImagepath(guidat->reg->status->currentFinger));
	

	return TRUE;
}

char * getFingerScore(int nFinger)
{

		char *szFinger=NULL;
	switch(nFinger){
				case 0: szFinger = (char*)getCompleteFilepath("score0.png"); break;
				case 1: szFinger = (char*)getCompleteFilepath("score1.png"); break;
				case 2: szFinger = (char*)getCompleteFilepath("score2.png"); break;
				case 3: szFinger = (char*)getCompleteFilepath("score3.png"); break;
				case 4: szFinger = (char*)getCompleteFilepath("score4.png"); break;
				//case default: szFinger = (char*)"/home/daser/crapos/src/images/score0.png"; break;
				}
return szFinger;


	
}

char *getProgessText(int nFinger)
{
char *szFinger=NULL;
	switch(nFinger){
				case 0: szFinger = (char*)"Place your left little finger"; break;
				case 1: szFinger = (char*)"Place your left ring finger"; break;
				case 2: szFinger = (char*)"Place your left middle finger"; break;
				case 3: szFinger = (char*)"Place your left index finger"; break;
				case 4: szFinger = (char*)"Place your left thumb"; break;
				case 5: szFinger = (char*)"Place your right thumb"; break;
				case 6: szFinger = (char*)"Place your right index finger"; break;
				case 7: szFinger = (char*)"Place your right middle finger"; break;
				case 8: szFinger = (char*)"Place your right ring finger"; break;
				case 9: szFinger = (char*)"Place your right little finger"; break;
				case -1: szFinger = (char*)"End of Enrollment"; break;
				}
return szFinger;
}

char *getFingerImagepath(int nFinger)
{
	char *szFinger = NULL;
			switch(nFinger){
				case 0: szFinger = (char*)getCompleteFilepath("l0.png"); break;
				case 1: szFinger = (char*)getCompleteFilepath("l1.png"); break;
				case 2: szFinger = (char*)getCompleteFilepath("l2.png"); break;
				case 3: szFinger = (char*)getCompleteFilepath("l3.png"); break;
				case 4: szFinger = (char*)getCompleteFilepath("l4.png"); break;
				case 5: szFinger = (char*)getCompleteFilepath("r0.png"); break;
				case 6: szFinger = (char*)getCompleteFilepath("r1.png"); break;
				case 7: szFinger = (char*)getCompleteFilepath("r2.png"); break;
				case 8: szFinger = (char*)getCompleteFilepath("r3.png"); break;
				case 9: szFinger = (char*)getCompleteFilepath("r4.png"); break;
				case -1: szFinger = (char*)getCompleteFilepath("touchedSensor.png"); break;
				}
return szFinger;
}


void progress_destroyed(void *guidata)
{
printf("this chaiman has been called progress destroyed callback\n");
}

void
start_syncing(GtkWidget *widget, gpointer data)
{

}

void
delete_student_fromtable(GtkWidget *widget, gpointer data)
{

}

void
get_verification_dialog(GtkWidget *widget, gpointer data)
{

	GtkWidget *window;
	GtkBuilder *builder = NULL;
	GError* error = NULL;
	
		builder = gtk_builder_new ();
		if (!gtk_builder_add_from_file (builder, getCompleteFilepath("winevent_verify.ui"), &error))
		{
			g_warning ("Couldn't load builder file: %s", error->message);
			g_error_free (error);
		}

	/* This is important */
	gtk_builder_connect_signals (builder, builder);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "winevents_verify"));
	GtkWidget *header = GTK_WIDGET (gtk_builder_get_object (builder, "imghead"));
	GdkPixbuf *pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("imgheader.png"),NULL);
	gtk_image_set_from_pixbuf(header,pixbufer);
	//gtk_widget_show (window);

	//g_object_unref (builder);

	pthread_t thread_id;
	getnFinger(1);//avoid trouble pls
	pthread_create (&thread_id, NULL, &start_verification, builder);
			
}


void* start_verification(void* data)
{
			GtkBuilder *builder = (GtkBuilder *)data;
			int count = 0;
			DBUser_t *jambite = NULL;
			jambite = (DBUser_t*) malloc(sizeof(DBUser_t));			
			if(jambite == NULL) return 0;
// Initialize Everything to NULL or 0 
			jambite->identifier = NULL;
			int i;
			for(i = 0; i < MAX_TEMPLATES; i++){
			jambite->vTemplateSizes[i] = 0;
			jambite->vTemplates[i] = NULL;
			}
				
			vstopThread = malloc(sizeof(int));
			*vstopThread = 0;
			int nFinger;
			int response = 0;
			int proceed = 1;
		GtkWidget *pbar;
		pbar = GTK_WIDGET (gtk_builder_get_object (builder, "pbar"));
		GtkWidget *lblresult;
		lblresult = GTK_WIDGET (gtk_builder_get_object (builder, "lblresult"));	
	
		pbar_data *guidata = (pbar_data*)malloc(sizeof(pbar_data));
		guidata->builder = builder;
		guidata->currentnFinger = 0;
		guidata->wait = malloc(sizeof(int));
		*guidata->wait = 0;
		glowait = guidata->wait;
		guidata->pbar = pbar;
	
		stop_pbar = malloc(sizeof(int));
		*stop_pbar = 0;
gdk_threads_add_timeout_full(G_PRIORITY_DEFAULT,100,progress_update_verify,
                                                         guidata,
                                                         NULL);
	GtkWidget *imgresult = GTK_WIDGET (gtk_builder_get_object (builder, "imgresult"));
	GtkWidget *nextbtn = GTK_WIDGET (gtk_builder_get_object (builder, "btnnext"));
	GdkPixbuf *pixbufer;
	pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("touchedSensor.png"),NULL);
	gtk_image_set_from_pixbuf(imgresult,pixbufer);
			while(proceed){//enter a while loop her for more than 1 fingers
				
			//grab finger
				nFinger = getnFinger(0); //static var
				guidata->currentnFinger = nFinger;
				if(nFinger == -1){//-1=no more left
				//UPDATE user's status here if all fingers are verified Suxessfuly
				//create a Upload-to-server button or later-button
				proceed = 0;
				break;
				}
				response = verifyCode(jambite,nFinger,vstopThread);
				
				if(1 == response){
				printf("The finger verified.\n");					

				gtk_widget_show (nextbtn);
					
				pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("finger_matched.png"),NULL);
				gtk_image_set_from_pixbuf(imgresult,pixbufer);
				DBusers* myDBuser = fetch_DBusers(0,jambite->identifier);
				gtk_label_set_text(lblresult,myDBuser->identifier);
				gtk_widget_show (lblresult);
					
				count = count + 1;
				if(count == getNumberofFingers()){
				*guidata->wait = 2;
				gtk_widget_hide (nextbtn);
				return NULL;
				}
					
				guidata->currentnFinger = nFinger;
					*guidata->wait = 1;
						while(*guidata->wait){
						//waste some time here until the next cmd come to resq
						printf(NULL);
						}
				gtk_widget_hide (nextbtn);
				pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("touchedSensor.png"),NULL);
				gtk_image_set_from_pixbuf(imgresult,pixbufer);
					
				gtk_widget_hide (lblresult);		
					
				printf("Student Name:%s\n",myDBuser->identifier);
				printf("Pin:%s\n",myDBuser->pin);
				printf("status:%d\n",myDBuser->status);
				printf("synced:%d\n",myDBuser->synced);
				//sleep for 2sec here after showing this
				}else{
					
					if(*vstopThread == 1){
					*vstopThread = 0;
					return NULL;
					}else{
					pixbufer = gdk_pixbuf_new_from_file(getCompleteFilepath("finger_disabled.png"),NULL);
					gtk_image_set_from_pixbuf(imgresult,pixbufer);
					printf("The finger was not enrolled.\n");
					*stop_pbar = 2;
					*vstopThread = 1;
					//call a mesg box;
					}
					
				
				}
			}//WEND
return NULL;
}

void kill_winenroll_winverify(GtkWidget *widget,gpointer *data)
{		
	if(*stop_pbar == 2){
		gtk_widget_destroy(GTK_WIDGET (gtk_builder_get_object ((GtkBuilder*)data, "winevents_verify")));
		g_object_unref((GtkBuilder*)data);
		*stop_pbar = 0;
	}
		static close_win = 0;
		if(close_win){
		gtk_widget_destroy(GTK_WIDGET (gtk_builder_get_object ((GtkBuilder*)data, "winevents_verify")));
		g_object_unref((GtkBuilder*)data);
		close_win = 0;
		}else{
		*vstopThread = 1;
		close_win = 1;
		*stop_pbar = 1;
		}
*glowait = 0;	
}

static gboolean progress_update_verify(void* guidata)
{
	pbar_data *guidat = (pbar_data*) guidata;

	if(*guidat->wait == 1){
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),"Finger has been Verified");		
	return true;	
	}
	
	if(*guidat->wait == 2){
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),"All Fingers Verified");		
		*stop_pbar = 2;
		return false;
	}
	
	if(*stop_pbar == 1){
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),"Verification has been cancelled");		
	*stop_pbar = 0;
	return false;	
	}

	if(*stop_pbar == 2){
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (guidat->pbar), 1.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),"No matching finger found");		
	//*stop_pbnset it
	return false;	
	}
	
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR(guidat->pbar));
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(guidat->pbar),getProgessText(guidat->currentnFinger));

	GtkWidget *image;
	GdkPixbuf *pixbufer;
	image = GTK_WIDGET (gtk_builder_get_object (guidat->builder, "imgfinger"));
	pixbufer = gdk_pixbuf_new_from_file(getFingerImagepath(guidat->currentnFinger),NULL);
	gtk_image_set_from_pixbuf(image,pixbufer);
	

	return TRUE;
}

void tell_pbar_to_cont(GtkWidget *widget,gpointer *data)
{
*glowait = 0;
}

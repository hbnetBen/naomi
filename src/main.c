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
 * daserfostscan is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "database.h"
	
GtkWidget*
create_window (void)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, getCompleteFilepath("crapos.ui"), &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* This is important */
	gtk_builder_connect_signals (builder, NULL);
	window = GTK_WIDGET (gtk_builder_get_object (builder, "mainwin"));
	GtkWidget *image = GTK_WIDGET (gtk_builder_get_object (builder, "image1"));
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(getCompleteFilepath("pple.png"),NULL);
	gtk_image_set_from_pixbuf(image,pixbuf);

	GdkPixbuf *icon = gdk_pixbuf_new_from_file(getCompleteFilepath("daserfostscan.png"),NULL);
	gtk_window_set_icon(window,icon);
	GtkWidget *toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "toolbar1"));

	GtkWidget *new = gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *very = gtk_image_new_from_stock(GTK_STOCK_YES,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *report = gtk_image_new_from_stock(GTK_STOCK_INFO,GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *about = gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_LARGE_TOOLBAR);

	GtkToolItem *inew = gtk_tool_button_new(new,"New Registration");
	GtkToolItem *ivery = gtk_tool_button_new(very,"Verification");
	GtkToolItem *ireport = gtk_tool_button_new(report,"Report");
	GtkToolItem *iabout = gtk_tool_button_new(about,"About");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), inew, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ivery, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ireport, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), iabout, -1);

	g_signal_connect(G_OBJECT(inew), "clicked", G_CALLBACK(get_enrollment_form),NULL);
	g_signal_connect(G_OBJECT(ivery), "clicked", G_CALLBACK(get_verification_dialog),NULL);
	g_signal_connect(G_OBJECT(ireport), "clicked", G_CALLBACK(get_report),NULL);
	
	g_object_unref (builder);
	
	return window;
}


int
main (int argc, char *argv[])
{
	//the pointer to our main window
	GtkWidget *window;
	/*important for any gtk application*/
	gtk_set_locale ();
	gtk_init (&argc, &argv);

	//u mostly see this error if you dont have sqlite3 installed
	if(!doesDBexist()){
		if(!createDatabaseForUser()){
			fprintf(stderr,"fatalerror, COULD not create database\n");
			exit(2);
		}
	}

	//create the main window
	window = create_window ();

	//showing the generated main window
	gtk_widget_show_all (window);
	
	gtk_main ();
	return 0;
}

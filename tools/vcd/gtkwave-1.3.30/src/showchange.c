/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* This file extracted from the GTK tutorial. */

/* radiobuttons.c */

#include <gtk/gtk.h>
#include "analyzer.h"
#include "aet.h"
#include "debug.h"

static GtkWidget *button1, *button2, *button3, *button4, *button5, *button6;
static GtkWidget *toggle1, *toggle2, *toggle3, *toggle4;
static GtkWidget *window;
static GtkSignalFunc cleanup;
static Trptr tcache;
static Ulong flags;

static void toggle_generic(GtkWidget *widget, Ulong msk)
{
if(GTK_TOGGLE_BUTTON(widget)->active)
	{
	flags|=msk;
	}
	else
	{
	flags&=(~msk);
	}
}

static void toggle1_callback(GtkWidget *widget, GtkWidget *nothing)
{
toggle_generic(widget, TR_RJUSTIFY);
}
static void toggle2_callback(GtkWidget *widget, GtkWidget *nothing)
{
toggle_generic(widget, TR_INVERT);
}
static void toggle3_callback(GtkWidget *widget, GtkWidget *nothing)
{
toggle_generic(widget, TR_REVERSE);
}
static void toggle4_callback(GtkWidget *widget, GtkWidget *nothing)
{
toggle_generic(widget, TR_EXCLUDE);
}

static void enter_callback(GtkWidget *widget, GtkWidget *nothing)
{
  flags=flags&(~(TR_HIGHLIGHT|TR_NUMMASK));

  if(GTK_TOGGLE_BUTTON(button1)->active)
	{
	flags|=TR_HEX;
	}
  else
  if(GTK_TOGGLE_BUTTON(button2)->active)
	{
	flags|=TR_DEC;
	}
  else
  if(GTK_TOGGLE_BUTTON(button3)->active)
	{
	flags|=TR_BIN;
	}
  else
  if(GTK_TOGGLE_BUTTON(button4)->active)
	{
	flags|=TR_OCT;
	}
  else
  if(GTK_TOGGLE_BUTTON(button5)->active)
	{
	flags|=TR_SIGNED;
	}
  else
  if(GTK_TOGGLE_BUTTON(button6)->active)
	{
	flags|=TR_ASCII;
	}

  tcache->flags=flags;

  gtk_grab_remove(window);
  gtk_widget_destroy(window);

  cleanup();
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  gtk_grab_remove(window);
  gtk_widget_destroy(window);
}


void showchange(char *title, Trptr t, GtkSignalFunc func)
{
  GtkWidget *main_vbox;
  GtkWidget *ok_hbox;
  GtkWidget *hbox;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *separator;
  GSList *group;
  GtkWidget *frame1, *frame2;

  cleanup=func;
  tcache=t;
  flags=t->flags;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_grab_add(window);  

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(destroy_callback),
		      NULL);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_container_border_width (GTK_CONTAINER (window), 0);


  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_widget_show (main_vbox);

  label=gtk_label_new(t->name);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);


  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 5);
  gtk_widget_show (hbox);

  box2 = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box2), 5);
  gtk_widget_show (box2);

  button1 = gtk_radio_button_new_with_label (NULL, "Hex");
  gtk_box_pack_start (GTK_BOX (box2), button1, TRUE, TRUE, 0);
  if(flags&TR_HEX) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button1), TRUE);
  gtk_widget_show (button1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button1));

  button2 = gtk_radio_button_new_with_label(group, "Decimal");
  gtk_box_pack_start (GTK_BOX (box2), button2, TRUE, TRUE, 0);
  if(flags&TR_DEC) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button2), TRUE);
  gtk_widget_show (button2);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button2));

  button5 = gtk_radio_button_new_with_label(group, "Signed Decimal");
  gtk_box_pack_start (GTK_BOX (box2), button5, TRUE, TRUE, 0);
  if(flags&TR_SIGNED) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button5), TRUE);
  gtk_widget_show (button5);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button5));

  button3 = gtk_radio_button_new_with_label(group, "Binary");
  gtk_box_pack_start (GTK_BOX (box2), button3, TRUE, TRUE, 0);
  if(flags&TR_BIN) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button3), TRUE);
  gtk_widget_show (button3);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button3));

  button4 = gtk_radio_button_new_with_label(group, "Octal");
  gtk_box_pack_start (GTK_BOX (box2), button4, TRUE, TRUE, 0);
  if(flags&TR_OCT) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button4), TRUE);
  gtk_widget_show (button4);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button4));

  button6 = gtk_radio_button_new_with_label(group, "ASCII");
  gtk_box_pack_start (GTK_BOX (box2), button6, TRUE, TRUE, 0);
  if(flags&TR_ASCII) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button6), TRUE);
  gtk_widget_show (button6);

  frame2 = gtk_frame_new ("Base");
  gtk_container_border_width (GTK_CONTAINER (frame2), 3);
  gtk_container_add (GTK_CONTAINER (frame2), box2);
  gtk_widget_show (frame2);
  gtk_box_pack_start(GTK_BOX (hbox), frame2, TRUE, TRUE, 0);

/****************************************************************************************************/

  box1 = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box1), 5);
  gtk_widget_show (box1);


  frame1 = gtk_frame_new ("Attributes");
  gtk_container_border_width (GTK_CONTAINER (frame1), 3);
  gtk_container_add (GTK_CONTAINER (frame1), box1);
  gtk_box_pack_start(GTK_BOX (hbox), frame1, TRUE, TRUE, 0);
  gtk_widget_show (frame1);

  toggle1=gtk_check_button_new_with_label("Right Justify");
  gtk_box_pack_start (GTK_BOX (box1), toggle1, TRUE, TRUE, 0);
  if(flags&TR_RJUSTIFY)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle1), TRUE);
  gtk_widget_show (toggle1);
  gtk_signal_connect (GTK_OBJECT (toggle1), "toggled", GTK_SIGNAL_FUNC(toggle1_callback), NULL);

  toggle2=gtk_check_button_new_with_label("Invert");
  gtk_box_pack_start (GTK_BOX (box1), toggle2, TRUE, TRUE, 0);
  if(flags&TR_INVERT)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle2), TRUE);
  gtk_widget_show (toggle2);
  gtk_signal_connect (GTK_OBJECT (toggle2), "toggled", GTK_SIGNAL_FUNC(toggle2_callback), NULL);

  toggle3=gtk_check_button_new_with_label("Reverse");
  gtk_box_pack_start (GTK_BOX (box1), toggle3, TRUE, TRUE, 0);
  if(flags&TR_REVERSE)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle3), TRUE);
  gtk_widget_show (toggle3);
  gtk_signal_connect (GTK_OBJECT (toggle3), "toggled", GTK_SIGNAL_FUNC(toggle3_callback), NULL);

  toggle4=gtk_check_button_new_with_label("Exclude");
  gtk_box_pack_start (GTK_BOX (box1), toggle4, TRUE, TRUE, 0);
  if(flags&TR_EXCLUDE)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle4), TRUE);
  gtk_widget_show (toggle4);
  gtk_signal_connect (GTK_OBJECT (toggle4), "toggled", GTK_SIGNAL_FUNC(toggle4_callback), NULL);

  gtk_container_add (GTK_CONTAINER (main_vbox), hbox);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);

/****************************************************************************************************/

  ok_hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (ok_hbox), 1);
  gtk_widget_show (ok_hbox);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC(destroy_callback),
                             GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (ok_hbox), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  gtk_container_add (GTK_CONTAINER (main_vbox), ok_hbox);

  button = gtk_button_new_with_label ("  OK  ");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC(enter_callback),
                             GTK_OBJECT (window));

  gtk_signal_connect_object (GTK_OBJECT (button), 
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button));

  gtk_box_pack_end (GTK_BOX (ok_hbox), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

/****************************************************************************************************/

  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (window);
}

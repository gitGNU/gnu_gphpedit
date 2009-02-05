/* This file is part of gPHPEdit, a GNOME2 PHP Editor.
 
   Copyright (C) 2003, 2004, 2005 Andy Jeffries
      andy@gphpedit.org
	  
   For more information or to find the latest release, visit our 
   website at http://www.gphpedit.org/
 
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
 
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.
 
   The GNU General Public License is contained in the file COPYING.*/


#ifndef CALLTIP_H
#define CALLTIP_H

#include "main.h"

void show_call_tip(GtkWidget *scintilla, gint position);
void autocomplete_word(GtkWidget *scintilla, gint wordStart, gint wordEnd);
void function_list_prepare(void);
GString *complete_function_list(gchar *original_list);
void css_autocomplete_word(GtkWidget *scintilla, gint wordStart, gint wordEnd);
void sql_autocomplete_word(GtkWidget *scintilla, gint wordStart, gint wordEnd);

#endif
/* 
 * Copyright (c) Tony Bybell 1999-2001.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "tree.h"
#include "vcd.h"

struct tree *treeroot=NULL;
static char *module=NULL;

char hier_delimeter='.';		/* default is dot unless aet is 
					   selected, then it's slash */
char hier_was_explicitly_set=0;

char alt_hier_delimeter=0x00;		/* for vcds with both [hierarchies or .] and / -- used in vcd only */

extern GtkCTree *ctree_main;

enum TreeBuildTypes { MAKETREE_FLATTEN, MAKETREE_LEAF, MAKETREE_NODE };


/*
 * init pointers needed for n-way tree
 */
void init_tree(void)
{
treeroot=(struct tree *)calloc_2(1,sizeof(struct tree));
module=(char *)malloc_2(longestname+1);
}


/*
 * extract the next part of the name in the flattened
 * hierarchy name.  return ptr to next name if it exists
 * else NULL
 */
static char *get_module_name(char *s)
{
char ch;
char *pnt;

pnt=module;

for(;;)
	{
	ch=*s;

	if(ch==hier_delimeter) 
		{
		*(pnt)=0;	
		s++;
		return(s);		
		}
	if(!ch)
		{
		*(pnt)=0;
		return(NULL);	/* nothing left to extract */		
		}

	s++;
	*(pnt++)=ch;
	}
}


/*
 * build the tree.  to keep the logic simple, we have
 * dead leaves at the ends.
 */
void build_tree_from_name(char *s, int which)
{
struct tree *t;

if(s==NULL) return;
if(!strlen(s)) return;

t=treeroot;
while(s)
	{
	s=get_module_name(s);

	if(!t->name)
		{
		char *newname;
		struct tree *t2;

		newname=(char *)malloc_2(strlen(module)+1);
		strcpy(newname,module);
		t->name=newname;		
		if(!s) t->which=which; else t->which=-1;

		t2=(struct tree *)calloc_2(1,sizeof(struct tree));
		t->child=t2;			
		t2->parent=t;
		t=t2;

		continue;
		}

	if(!strcmp(t->name, module)) 
		{
		t=t->child;
		continue;
		}
		else
		{	
		struct tree *t2, *t3;
		char *newname;

		newname=(char *)malloc_2(strlen(module)+1);
		strcpy(newname,module);
		t2=(struct tree *)calloc_2(1,sizeof(struct tree));
		memcpy(t2, t, sizeof(struct tree));

		t->name=newname;
		t->next=t2;
		if(!s) t->which=which; else t->which=-1;

		t3=(struct tree *)calloc_2(1,sizeof(struct tree));
		t->child=t3;
		t3->parent=t;
		t=t3;		

		continue;
		}
	}
}


/*
 * remove dead leaves from the tree
 */
int treeprune(struct tree *t)
{
while(t)
	{
	if(t->name)
		{
		if(t->child)
			{
			if(!(treeprune(t->child)))
				{
				t->child=NULL; /* remove empty leaf */
				}
			}
		}
		else
		{
		free_2(t);		       /* free empty leaf */
		return(0);
		}

	t=t->next;
	}
return(1);
}


/*
 * for debugging purposes only
 */
void treedebug(struct tree *t, char *s)
{
while(t)
	{
	char *s2;

	s2=(char *)malloc_2(strlen(s)+strlen(t->name)+2);
	strcpy(s2,s);
	strcat(s2,".");
	strcat(s2,t->name);
	
	if(t->child)
		{
		treedebug(t->child, s2);
		}
		else
		{
		printf("%d) %s\n", t->which, s2);
		}

	free_2(s2);
	t=t->next;
	}
}


static GtkCTreeNode *maketree_nodes(GtkCTreeNode *subtree, struct tree *t2, GtkCTreeNode *sibling, int mode)
{
GtkWidget *titem;
GtkWidget *added;
char *tmp, *tmp2, *tmp3;
gchar *text [1];

if(t2->which!=-1)
	{
        if(facs[t2->which]->vec_root)
        	{
                if(autocoalesce)
                	{
                        if(facs[t2->which]->vec_root!=facs[t2->which])
                        	{
				return(NULL);
                                }

                        tmp2=makename_chain(facs[t2->which]);
                        tmp3=leastsig_hiername(tmp2);
                        tmp=wave_alloca(strlen(tmp3)+4);
                        strcpy(tmp,   "[] ");
                        strcpy(tmp+3, tmp3);
                        free_2(tmp2);
                        }
                        else
                        {
                        tmp=wave_alloca(strlen(t2->name)+4);
                        strcpy(tmp,   "[] ");
                        strcpy(tmp+3, t2->name);
                        }
		}
                else
                {
                tmp=t2->name;
                }
	}
        else
        {
        tmp=t2->name;
        }

text[0]=tmp;
switch(mode)
	{
	case MAKETREE_FLATTEN:
		if(t2->child)
			{
		        sibling = gtk_ctree_insert_node (ctree_main, subtree, sibling, text, 3,
                	                       NULL, NULL, NULL, NULL,
                	                       FALSE, FALSE);
			gtk_ctree_node_set_row_data(ctree_main, sibling, t2);
			maketree(sibling, t2->child);
			}
			else
			{
		        sibling = gtk_ctree_insert_node (ctree_main, subtree, sibling, text, 3,
                	                       NULL, NULL, NULL, NULL,
                	                       TRUE, FALSE);
			gtk_ctree_node_set_row_data(ctree_main, sibling, t2);
			}
		break;

	default:
	        sibling = gtk_ctree_insert_node (ctree_main, subtree, sibling, text, 3,
               	                       NULL, NULL, NULL, NULL,
               	                       (mode==MAKETREE_LEAF), FALSE);
		gtk_ctree_node_set_row_data(ctree_main, sibling, t2);
		break;
	}

return(sibling);
}


void maketree(GtkCTreeNode *subtree, struct tree *t)
{
GtkCTreeNode *sibling=NULL, *sibling_test;
struct tree *t2;

if(!hier_grouping)
	{
	t2=t;
	while(t2)
		{
		sibling_test=maketree_nodes(subtree, t2, sibling, MAKETREE_FLATTEN);	
		sibling=sibling_test?sibling_test:sibling;
		t2=t2->next;
		}
	}
	else
	{
	t2=t;
	while(t2)
		{
		if(!t2->child)
			{
			sibling_test=maketree_nodes(subtree, t2, sibling, MAKETREE_LEAF);
			if(sibling_test)
				{
				maketree(sibling=sibling_test, t2->child);
				}
			}
	
		t2=t2->next;
		}

	t2=t;
	while(t2)
		{
		if(t2->child)
			{
			sibling_test=maketree_nodes(subtree, t2, sibling, MAKETREE_NODE);
			if(sibling_test)
				{
				maketree(sibling=sibling_test, t2->child);
				}
			}
	
		t2=t2->next;
		}
	}
}


/*
 * return least significant member name of a hierarchy
 * (used for tree and hier vec_root search hits)
 */
char *leastsig_hiername(char *nam)
{
char *t, *pnt=NULL;
char ch;

if(nam)
	{
	t=nam;
	while(ch=*(t++))
		{
		if(ch==hier_delimeter) pnt=t;
		}
	}

return(pnt?pnt:nam);
}


extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "easy_code.h"
}

#include "easy.hpp"
#include "easy_node.hpp"

node *elist=NULL;
node *begn=NULL;
node *current=NULL;

extern const char * debug_type (int dtype);
extern bool subBlock;

fpos_t myftell (FILE *fp);

//======================================================================
// What follows is a bunch of routines for a doubly linked list.
// As this was started in C, rather than C++, I didn't use the STL
// templates. I wanted some specific functionality and my
// C++ was a bit rusty. I didn't feel like debugging this over.

node * lftNode(node * n) {
  return n->lft;
}

node * rghtNode(node * n) {
  return n->rght;
}

//---------------------------------
// copy nodes - make a copy of the content of a set of nodes
//
node * copyNodes (node * n) {
  node * f=NULL;
  node * c;

  do {
    c=(node *) malloc(sizeof(node));
    c->dtype=n->dtype;
    c->value=n->value;
    c->str=n->str;
    c->lft=n->lft;
    c->rght=n->rght;

    if (f==NULL) {
      f=c;           //remember first link
    }
    n=n->rght;
    
  }
  while (n!=NULL);

  f->lft=NULL;   // first link goes to NULL
  
  return f;
}


// is list empty

int isEmpty() {
  return (elist==NULL);
}

int listLength() {
  int length=0;
  node *current;

  for (current=elist;current!=NULL;current=current->lft) {

    length++;
  }

  return length;
}


void replaceNode(node *n, int type, float v, char * s) {
  n->dtype=type;
  n->value=v;
  n->str=s;
}

//------------------------------------------
// We don't free the contents of nodes because they
// can be reused in macros, etc and this would lead
// to a memory nightmare. Trivial memory leak, well,
// depends on how you think about it.
//

void emptyList() {
  begn=NULL;
  elist=NULL;
  // printf("NC\n");
}

//------------------------

int isCommand(node * n) {
  if ((n->dtype>1)&&(n->dtype>100)) {
    return 1;
  }
  return 0;
}

//------------------------

int countArgs(node * n) {
  int count=0;
  node *ptr=n;

  while ((ptr!=NULL)&&(!isCommand(ptr))) {
    count++;
    ptr = ptr->lft;
  }

  return count;
}

void displayNode(node *ptr) {

     printf("NODE:   %s... %f,%s\n",debug_type(ptr->dtype),ptr->value,ptr->str);

}


void displayBackward() {
   printf("COMMANDS BACKWARD:\n");

   //start from the beginning
  
   node *ptr = elist;
	
   //navigate till the end of the list
	
   int i=0;
   while(ptr != NULL) {
     i++;
     printf("%d    (%s,%f,%s,%d)\n",i,debug_type(ptr->dtype),ptr->value,ptr->str,ptr->filled);
     ptr = ptr->lft;
   }
	
}

//display the list from begn to first
void displayForward() {

   printf("COMMANDS FORWARD:\n");

   //start from the begn
   node *ptr = begn;
	
   //navigate till the start of the list
	
   int i=0;
   while(ptr != NULL) {
     i++;
	
     //print data

     printf("%d    (%s,%f,%s,%d)\n",i,debug_type(ptr->dtype),ptr->value,ptr->str,ptr->filled);
		
     //move to lft item
     ptr = ptr ->rght;
   }

}

//-----------------------------------
// inserts a list of nodes from another list (a)
// in the middle of this list (n)
//
// node we are at gets replaced (this is for macros)
//
// yes, this kind of nasty and there
// are 5 scenarios to be checked for
// 1) variable is the only item on list
// 2) variable is at start of list
// 3) variable is at end of list
// 4) variable is in the middle of list
// 5) recursive variables

void insertNodes(node *to, node*from) {
  node * z;
  node *lhs=to->lft;
  // node *eolist;
  node *eocopy;

  node * copy=copyNodes(from);
  for (z=copy;z->rght!=NULL;z=z->rght) { 
  }
  eocopy=z;

  // (the 'to' node gets replaced and patched over)

  copy->lft=lhs;
  if (lhs!=NULL) {
    lhs->rght=copy;
  }
  else {              // start of list edge case
    begn=copy;
  }

  // end of list: patch it

  if (eocopy->rght==NULL) {  // if it is at end of line
    elist=eocopy;
  }
}

//insert link at the first location --------------------

void push(int dtype, float value, char * str) {


  // printf("push debug: %d %f\n",dtype,value);

  //create a link
  node *link = (node*) malloc(sizeof(node));
  link->dtype = dtype;
  link->value = value;
  link->str = str;
  link->filled = false;
  link->lft=NULL;
  link->rght=NULL;

  if(isEmpty()) {
     //make it the begn link
     begn = link;
     link->rght=NULL;
  } else {
     //update first rght link
     elist->rght = link;
  }

  //point it to old first link
  link->lft = elist;

  //point first to new first link
  elist = link;
}

// //insert link at the begn location
// void shift(FILE *fp, int dtype, float value, char * str) {
//   fpos_t posi;
    
//    //create a link
//    struct node *link = (struct node*) malloc(sizeof(struct node));
//    link->dtype = dtype;
//    link->value = value;
//    link->str = str;
//    link->filled = false;
	
//    if(isEmpty()) {
//       //make it the begn link
//       begn = link;
//    } else {
//       //make link a new begn link
//       begn->lft = link;     
      
//       //mark old begn node as rght of new link
//       link->rght = begn;
//    }

//    //point begn to new begn node
//    begn = link;
// }

//delete first item
node* pop() {

   //save reference to first link
   node *tempLink = elist;
	
   //if only one link
   if(elist->lft == NULL){
      begn = NULL;
   } else {
      elist->lft->rght = NULL;
   }
	
   elist = elist->lft;
   //return the deleted link
   return tempLink;
}

//delete link at the begn location

node* unshift() {
   //save reference to begn link
   node *tempLink = begn;
	
   //if only one link
   if(elist->lft == NULL) {
      elist = NULL;
   } else {
      begn->rght->lft = NULL;
   }
	
   begn = begn->rght;
	
   //return the deleted link
   return tempLink;
}


// flags a node for later deletion

void zeroNode(node * n) {
  if (n->str!=NULL) free(n->str);
  n->str=NULL;
  n->dtype=NOOP;
  n->value=0;
}

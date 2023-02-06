
extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#ifndef easy_node_hpp
#define easy_node_hpp 1

//======================================================================
// What follows is a bunch of routines for a doubly linked list.
// As this was started in C, rather than C++, I didn't use the STL
// templates. I wanted some specific functionality and my
// C++ was a bit rusty. I didn't feel like debugging this over.

class node {
  public:
  int dtype;                 // opcode
  float value;               // float value
  char *str;                 // string value
  bool filled;               // was float value of variable filled?
  NumberDriver *nd;

  struct node *lft;          // node to left
  struct node *rght;         // node to right
};

// some forward declarations are over in easy_code.h
// because they have to be called from C

node * lftNode(node * n);
node * rghtNode(node * n);
int isEmpty();
int listLength();
void replaceNode(node *n, int type, float v, char * s);
void emptyList();
int isCommand(struct node * n);
int countArgs(struct node * n);
void insertNodes(node *to, node*from);
struct node* pop();
struct node* unshift();
void deleteNode(node * n);
void zeroNode(node * n);
void displayNode(node *ptr);

#endif

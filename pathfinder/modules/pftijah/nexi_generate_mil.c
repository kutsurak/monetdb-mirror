/**
 * Copyright Notice:
 * -----------------
 *
 * The contents of this file are subject to the PfTijah Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://dbappl.cs.utwente.nl/Legal/PfTijah-1.1.html
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is the PfTijah system.
 * 
 * The Initial Developer of the Original Code is the "University of Twente".
 * Portions created by the "University of Twente" are 
 * Copyright (C) 2006-2007 "University of Twente".
 * All Rights Reserved.
 *
 * Author(s): Vojkan Mihajlovic
 *	      Jan Flokstra
 *            Henning Rode
 *            Roel van Os
 *
 * Module that generates MIL query plans out of SRA query plans
 */

#include <pf_config.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "pftijah.h"
#include "nexi.h"
#include "nexi_generate_mil.h"

int tree_traverse_count(command_tree *p_command, command_tree *com_lifo[], int *com_sp, int op_number[], int *op_num, int topic_num) {

   /*printf("H:%d\t%d\t%d\n",p_command->operator,p_command->number, *op_num); */

  if (p_command->left != NULL)
    tree_traverse_count(p_command->left, com_lifo, com_sp, op_number, op_num, topic_num);

  if(p_command->right != NULL)
    tree_traverse_count(p_command->right, com_lifo, com_sp, op_number, op_num, topic_num);


  op_number[p_command->number]++;
  (*com_sp)++;
  PUSH_COMMAND_REV(p_command);
  /*    printf("C:%d\t%d\n",p_command->number, p_command->operator); */

  (*op_num)++;

  return 1;

}


int tree_traverse_opt(command_tree *p_command, command_tree *com_lifo[], int *com_sp, int op_number[], int *op_num, int topic_num) {

  /*printf("H:%d\t%d\t%d\n",p_command->operator,p_command->number, *op_num); */

  if (op_number[p_command->number] > 0) {

    /*printf("H:%d\t%d\t%d\n",p_command->operator,p_command->number, *op_num); */

    op_number[p_command->number]++;

  }

  else {

    if (p_command->left != NULL)
      tree_traverse_opt(p_command->left, com_lifo, com_sp, op_number, op_num, topic_num);

    if(p_command->right != NULL)
      tree_traverse_opt(p_command->right, com_lifo, com_sp, op_number, op_num, topic_num);

    op_number[p_command->number]++;
    (*com_sp)++;
    PUSH_COMMAND_REV(p_command);
       /* printf("C:%d\t%d\n",p_command->number, p_command->operator); */

    (*op_num)++;

  }

  return 1;

}

char *unquote(char *q_term) {

  if ( *q_term != '"' ) {
      fprintf(stderr,"WARNING: unquote: term not quoted.\n"); 
      return q_term; /* term was not quoted, JF */
  }

  int cnt;

  cnt = 1;

  while (q_term[cnt] != '"') {
    unq_term[cnt-1]=q_term[cnt];
    cnt++;
  }

  unq_term[cnt-1] = '\0';

  return unq_term;

}

char *split_terms(char *adj_term){

  int cnt;
  char *t_term;


  int c_len = strlen(adj_term);

  cnt = 2;
  /* new_cnt = 0; */

  t_term = term_cut;

  while (cnt < c_len - 2) {

    while (adj_term[cnt] != '\"' && cnt < c_len - 2) {
      *term_cut = adj_term[cnt];
      /* printf("S%c\n",*term_cut); */
      cnt++;
      term_cut++;
    }

    cnt = cnt + 3;
    *term_cut = '\0';
    term_cut++;

  }

  *term_cut = '\n';

  term_cut = t_term;
  /*printf("SS%s\n", term_cut); */

  return term_cut;

}

int SRA_to_MIL(TijahParserContext* parserCtx, int query_num, struct_RMT *txt_retr_model, struct_RMI *img_retr_model, struct_RF *rel_feedback, char *mil_fname, char *sxqxl_fname, command_tree **p_command_array, bool phrase_in)
{
  (void)rel_feedback;
  (void)mil_fname;
  (void)sxqxl_fname;
  (void)phrase_in;

  /* operator count */
  int op_num;

  /* adjacent term count */
  int t_count;

  /* stack pointers */
  int com_sp;

  /* operator number count */
  int com_num;
  int com_nr_left = 0, com_nr_right = 0;
  float score_mul;
  int modifier;

  /* number of MIL variables */
  int var_num;

  /* topic number (based on a query num imput parameter) */
  int topic_num;

  /* retrieval model parameters: lambda */
  /* float lambda, eta, k1, b;
   * int A;
   */

  /* operator argument */
  char *argument = NULL, *argument1, *adj_arg, *t_argument;

  /* command stacks */
  command_tree *com_lifo[STACK_MAX];
  command_tree *p_com;
  int op_number[STACK_MAX];
  int op_newnum[STACK_MAX];

  /* pointers for command tree structure */
  command_tree **p_com_array;
  command_tree *p1_command;

  /* variable for avoiding numerical problems */
  bool set_reset;

  /* memory allocation for string manipulation */
  argument1 = calloc(TERM_LENGTH, sizeof(char));
  term_cut = calloc(TERM_LENGTH, sizeof(char));
  unq_term = calloc(ADJ_TERM_MAX * TERM_LENGTH, sizeof(char));

  /* formating the mil header */

  MILPRINTF(MILOUT, "#\n# COLLECTION: \"%s\"\n",parserCtx->collection);
  MILPRINTF(MILOUT, "# NEXI-QUERY:\n");
  char *np;
  char *p = (char*)parserCtx->queryText;
  while( (np=index(p,'\n')) ) {
	*np = 0;
        MILPRINTF(MILOUT, "# %s\n",p);
	*np = '\n';
	p = ++np;
  }
  MILPRINTF(MILOUT, "# %s\n",p);
  MILPRINTF(MILOUT, "\tVAR ");

  for (var_num = 0; var_num < MAX_VARS-1; var_num++) {
    MILPRINTF(MILOUT, "R%d, ", var_num);
  }
  MILPRINTF(MILOUT, "R%d;\n", MAX_VARS-1);


  /* command array initialization */
  p_com_array = p_command_array;
  p1_command = *p_com_array;

  /* default region score setup */
  MILPRINTF(MILOUT, "var base := scoreBase;\n\n");

  /*   printf("%d\n",p_com_array); */
  /*   printf("%d\n",p1_command); */

  /* for number initialization */
  topic_num = query_num - 1;

  MILPRINTF(MILOUT, "var terms;\nvar modifiers;\nvar tid;\n\n");
  MILPRINTF(MILOUT, "\n");
  
  if ( TDEBUG(5) ) {
       MILPRINTF(MILOUT,"printf(\"# tijah-mil-exec: start computation.\\n\");\n");
  }
  while (p1_command != NULL) {
    op_num = 0;
    com_sp = 0;
    com_num = 0;

    int i;
    for (i=0; i<STACK_MAX; i++) {
      op_number[i] = 0;
      op_newnum[i] = 0;
    }

    set_reset = FALSE;

    topic_num++;

    /* performing tree traversal with optimization of SRA query plan */
    tree_traverse_opt(p1_command, com_lifo, &com_sp, op_number, &op_num, topic_num);

    com_sp++;
    PUSH_COMMAND(NULL);

    com_sp = 1;

    POP_COMMAND();
    com_sp++;
    /*printf("%d\n",com_sp); */
    /*printf("%d\n",p_com->operator); */

    /*   if (topic_num == 3) { */
    /*      for (i=1; i<=94; i++) */
    /*	printf("H:%d\t%d\n",com_lifo[i]->number,com_lifo[i]->operator); */
    /*} */

    while (p_com != NULL) {

      com_num = com_sp - 1;

      if (com_num > RESET_NUM && p_com->operator == P_AND && set_reset == FALSE) {
        MILPRINTF(MILOUT, "R%d := [*](R%d,dbl(1e+38).pow(dbl(4)));\n", p_com->left->number, p_com->left->number);
        set_reset = TRUE;
      }

      switch(p_com->operator) {
      case SELECT_NODE:

        argument = unquote(p_com->argument);
	
        if (p_com->left == NULL && p_com->right == NULL) {

          if (!strcmp(p_com->argument,"\"Root\"")) {
            MILPRINTF(MILOUT, "R%d := select_root%s();\n", com_num,parserCtx->ffPfx);
          }
          else {
            MILPRINTF(MILOUT, "R%d := select_node%s(%s,%s);\n", com_num, parserCtx->ffPfx,p_com->argument, txt_retr_model->e_class);
          }

        }
        else if (p_com->left != NULL) {

          com_nr_left = p_com->left->number;

          if (!strcmp(p_com->argument,"")) {
            MILPRINTF(MILOUT, "R%d := R%d.select_node%s(%d);\n", com_num, com_nr_left,parserCtx->ffPfx,0);
          }
          else {
            MILPRINTF(MILOUT, "R%d := R%d.select_node%s(%s,%s);\n", com_num, com_nr_left, parserCtx->ffPfx, p_com->argument, txt_retr_model->e_class);
          }

        }

        break;

      case SELECT_NODE_VAGUE:

         MILPRINTF(MILOUT, "R%d := select_node_vague%s(%s,%s,\"%s\");\n", com_num, parserCtx->ffPfx, p_com->argument, txt_retr_model->e_class, txt_retr_model->exp_class);

         break;

      case P_SELECT_NODE_T:

      MILPRINTF(MILOUT, "R%d := p_select_node_t%s(%s,%d,%d,%d,%d,%d,%s,\"%s\",%d,%d,%f,%f,%d,%d,%d,%d,%s,%s,%d);\n", com_num,parserCtx->ffPfx, p_com->argument, txt_retr_model->model, 	txt_retr_model->or_comb, txt_retr_model->and_comb, txt_retr_model->up_prop, txt_retr_model->down_prop, txt_retr_model->e_class, txt_retr_model->exp_class, 		txt_retr_model->stemming, txt_retr_model->size_type, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->param3, txt_retr_model->prior_type, txt_retr_model->prior_size, img_retr_model->model, img_retr_model->descriptor, img_retr_model->attr_name, img_retr_model->computation);

         break;

      case SELECT_TERM:

        argument = unquote(p_com->argument);

        if (p_com->left == NULL && p_com->right == NULL) {
          
          MILPRINTF(MILOUT, "R%d := select_term(%s,%d);\n", com_num, p_com->argument, txt_retr_model->stemming);

        }
        else if (p_com->left != NULL) {

          com_nr_left = p_com->left->number;
          
          if (!strcmp(p_com->argument,"")) {
            MILPRINTF(MILOUT, "R%d := R%d.select_term();\n", com_num, com_nr_left);
          }
          else {
            MILPRINTF(MILOUT, "R%d := R%d.select_term(%s,%d);\n", com_num, com_nr_left, p_com->argument, txt_retr_model->stemming);  
          }
          
        }

        break;
      
      case CREATE_QUERY_OBJECT:
        
    		MILPRINTF(MILOUT, "terms := new(void,str).seqbase(oid(0));\n");
    		MILPRINTF(MILOUT, "modifiers := new(void,int).seqbase(oid(0));\n");

        break;
      
      case QUERY_ADD_TERM:
        
        MILPRINTF(MILOUT, "terms.append(%s);\n", p_com->argument );

       break;


      case SELECT_ADJ:

        /*printf("RRRRRRRRRRRRRRRRRRRRR:%d\n", p_com->number); */
        t_count = 0;

        /*printf("X:%s\n", p_com->argument); */
        adj_arg = split_terms(p_com->argument);
        
        t_argument = argument;
        
        while (*adj_arg != '\0') {
          *argument = *adj_arg;
          adj_arg++;
          argument++;
        }

        *argument = '\0';
        
        t_count++;
        argument = t_argument;
        /*printf("ZX:%s\n",argument); */
        MILPRINTF(MILOUT, "var phrase := new(int,str);\n");
        MILPRINTF(MILOUT, "phrase.insert(%d,\"%s\");\n", t_count, argument);

        adj_arg++;
        
        t_argument = argument1;
        while (*adj_arg != '\0') {
          *argument1 = *adj_arg;
          adj_arg++;
          argument1++;
        }
        
        *argument1 = '\0';
        
        t_count++;
        argument1 = t_argument;
        /*printf("ZX:%s\n",argument1); */
        /*MILPRINTF(MILOUT, "var phrase := new(int,str);\n"); */
        MILPRINTF(MILOUT, "phrase.insert(%d, \"%s\");\n", t_count, argument1);
        
        adj_arg++;
        
        while (*adj_arg != '\n') {
          t_argument = argument;
          while (*adj_arg != '\0') {
            *argument = *adj_arg;
            adj_arg++;
            argument++;
          }

          *argument = '\0';
          argument = t_argument;
          t_count++;

          MILPRINTF(MILOUT, "phrase.insert(%d, \"%s\");\n", t_count, argument);
          /*printf("ZX:%s\n",argument); */
          /*printf("ZX1:%s\n",argument); */
          adj_arg++;

          free(argument1);

        }
        
        break;

      case P_SELECT_IMAGE:

        argument = unquote(p_com->argument);

        MILPRINTF(MILOUT, "R%d := select_image(\"%s\",\"%s\",%s);\n", com_num, img_retr_model->descriptor, img_retr_model->attr_name, p_com->argument);

        break;

      case CONTAINING:

        if (p_com->right != NULL) {

          com_nr_left = p_com->left->number;
          com_nr_right = p_com->right->number;

          MILPRINTF(MILOUT, "R%d := R%d.containing(R%d);\n", com_num, com_nr_left, com_nr_right);

        }
        else {

          com_nr_left = p_com->left->number;

          MILPRINTF(MILOUT, "R%d := R%d.containing();\n", com_num, com_nr_left);

        }

        break;

      case CONTAINED_BY:

        if (p_com->left != NULL) {

          com_nr_left = p_com->left->number;
          com_nr_right = p_com->right->number;

          MILPRINTF(MILOUT, "R%d := R%d.contained_by%s(R%d%s);\n", com_num, com_nr_left, parserCtx->ffPfx, com_nr_right, parserCtx->flastPfx);

        }
        else {

          com_nr_right = p_com->right->number;

          MILPRINTF(MILOUT, "R%d := R%d.contained_by();\n", com_num, com_nr_right);

        }

        break;

      case UNION:
      case P_UNION:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        MILPRINTF(MILOUT, "R%d := R%d.union(R%d);\n", com_num, com_nr_left, com_nr_right);

        break;

      case INTERSECT:
      case P_INTERSECT:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        MILPRINTF(MILOUT, "R%d := R%d.intersect(R%d);\n", com_num, com_nr_left, com_nr_right);
      
        break;
        
      case P_CONTAINING:
        
        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch (txt_retr_model->up_prop) {
        case UP_SUM :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_sum(R%d);\n", com_num, com_nr_left, com_nr_right);
          
          break;

        case UP_AVG :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_avg(R%d);\n", com_num, com_nr_left, com_nr_right);
          
          break;

        case UP_WSUMD :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_wsumd(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case UP_WSUMA :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_wsuma(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        }

        break;

      case P_CONTAINED_BY:

        if (p_com->left != NULL) {

          com_nr_left = p_com->left->number;
          com_nr_right = p_com->right->number;

          switch (txt_retr_model->down_prop) {
          case DOWN_SUM :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_sum(R%d);\n", com_num, com_nr_left, com_nr_right);

            break;

          case DOWN_AVG :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_avg(R%d);\n", com_num, com_nr_left, com_nr_right);

            break;

          case DOWN_WSUMD :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_wsumd(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

            break;

          case DOWN_WSUMA :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_wsuma(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

            break;

          }

        }
        else {

          com_nr_right = p_com->right->number;


          switch (txt_retr_model->down_prop) {
          case DOWN_SUM :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_sum();\n", com_num, com_nr_right);
          
            break;

          case DOWN_AVG :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_avg();\n", com_num, com_nr_right);
          
            break;

          case DOWN_WSUMD :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_wsumd(%d);\n", com_num, com_nr_right, txt_retr_model->size_type);
            
            break;

          case DOWN_WSUMA :

            MILPRINTF(MILOUT, "R%d := R%d.p_contained_by_wsuma(%d);\n", com_num, com_nr_right, txt_retr_model->size_type);

            break;

          }

        }
        
        break;
        
      case P_PRIOR:

        com_nr_left = p_com->left->number;
        
        MILPRINTF(MILOUT, "R%d := R%d.prior_ls(%d);\n", com_num, com_nr_left, txt_retr_model->size_type);
        
        break;   

      case MUST_CONTAIN_T:

        MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_Bool(R%d);\n", com_num, com_nr_left, com_nr_right);

        break;
        
      case MUST_NOT_CONTAIN_T:
        
        MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_Bool(R%d);\n", com_num, com_nr_left, com_nr_right);

        break;

      case P_CONTAINING_T:
        
        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch (txt_retr_model->model) {
        case MODEL_BOOL :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_Bool(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LM :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_LM(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case MODEL_LMS :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_LMs(R%d, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->size_type);

          break;

        case MODEL_TFIDF :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_tfidf(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case MODEL_OKAPI :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_Okapi(R%d, %f, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

        case MODEL_GPX :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_GPX(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LMA :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_LMA(R%d, \"%s\", %f, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->context, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

 
        }
        
        break;

      case P_SELECT_NODE_Q:
        
        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

       //only updated for language models. Bats "terms" and "modifiers" should be replace by the query object.
        switch (txt_retr_model->model) {
        case MODEL_BOOL :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_Bool(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LM :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_q_LM(terms, modifiers, %d, %d);\n", com_num, com_nr_left, txt_retr_model->stemming, txt_retr_model->size_type);

          break;

        case MODEL_LMS :
         
          MILPRINTF(MILOUT, "R%d := R%d.p_containing_q_LMs(terms, modifiers, %f, %d, %d);\n", com_num, com_nr_left, txt_retr_model->param1, txt_retr_model->stemming, txt_retr_model->size_type);

          break;
        
        case MODEL_NLLR :
            
          MILPRINTF(MILOUT, "R%d := R%d.p_containing_q_NLLR%s(terms, %f%s);\n", com_num, com_nr_left, parserCtx->ffPfx, txt_retr_model->param1, parserCtx->flastPfx);
        
          break;
        
        case MODEL_TFIDF :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_tfidf(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case MODEL_OKAPI :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_Okapi(R%d, %f, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

        case MODEL_GPX :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_GPX(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LMA :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_t_LMA(R%d, %f, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

        }

        break;

      case P_NOT_CONTAINING_T:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch (txt_retr_model->model) {
        case MODEL_BOOL :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_Bool(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LM :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_LM(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case MODEL_LMS :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_LMs(R%d, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->size_type);

          break;

        case MODEL_TFIDF :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_tfidf(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->size_type);

          break;

        case MODEL_OKAPI :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_Okapi(R%d, %f, %f, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

        case MODEL_GPX :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_GPX(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case MODEL_LMA :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_t_LMA(R%d, \"%s\", %f, %f, %d);\n", com_num, com_nr_left, com_nr_right,  txt_retr_model->context, txt_retr_model->param1, txt_retr_model->param2, txt_retr_model->size_type);

          break;

        }
        
        break;

      case P_CONTAINING_I:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch (img_retr_model->computation) {
        case IMAGE_AVG :

          MILPRINTF(MILOUT, "R%d := R%d.p_containing_i_avg(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        }

        break;

      case P_NOT_CONTAINING_I:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch (img_retr_model->computation) {
        case IMAGE_AVG :

          MILPRINTF(MILOUT, "R%d := R%d.p_not_containing_i_avg(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;
        
        }
        
      case SCALE:
        
        com_nr_left = p_com->left->number;
        score_mul = atof(p_com->argument);

        MILPRINTF(MILOUT, "R%d := R%d.scale(%f);\n", com_num, com_nr_left, score_mul);
        
        break;
      
     case QUERY_ADD_MODIFIER:
        
        modifier = atoi(p_com->argument);

        MILPRINTF(MILOUT, "modifiers.append(%d);\n", modifier);
        
        break;

      case P_AND:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch(txt_retr_model->and_comb) {
        case AND_PROD :

          MILPRINTF(MILOUT, "R%d := R%d.and_prod(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case AND_MIN :

          MILPRINTF(MILOUT, "R%d := R%d.and_min(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case AND_SUM :

          MILPRINTF(MILOUT, "R%d := R%d.and_sum(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case AND_EXP :

          MILPRINTF(MILOUT, "R%d := R%d.and_exp(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param3);

          break;

        }

        break;

      case P_OR:
        
        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        switch(txt_retr_model->or_comb) {
        case OR_SUM :
          
          MILPRINTF(MILOUT, "R%d := R%d.or_sum(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case OR_MAX :

          MILPRINTF(MILOUT, "R%d := R%d.or_max(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case OR_PROB :

          MILPRINTF(MILOUT, "R%d := R%d.or_prob(R%d);\n", com_num, com_nr_left, com_nr_right);

          break;

        case OR_EXP :

          MILPRINTF(MILOUT, "R%d := R%d.or_exp(R%d, %d);\n", com_num, com_nr_left, com_nr_right, txt_retr_model->param3);

          break;

        }

        break;
        
      case P_SELECT_GR:
        
        com_nr_left = p_com->left->number;
        argument = unquote(p_com->argument);

        MILPRINTF(MILOUT, "R%d := R%d.near_val(%d,%s);\n", com_num, com_nr_left, P_SELECT_GR, p_com->argument);

        break;

      case P_SELECT_LS:
        
        com_nr_left = p_com->left->number;
        argument = unquote(p_com->argument);

        MILPRINTF(MILOUT, "R%d := R%d.near_val(%d,%s);\n", com_num, com_nr_left, P_SELECT_LS, p_com->argument);

        break;

      case P_SELECT_EQ:
        
        com_nr_left = p_com->left->number;
        argument = unquote(p_com->argument);
        
        MILPRINTF(MILOUT, "R%d := R%d.near_val(%d,%s);\n", com_num, com_nr_left, P_SELECT_EQ, p_com->argument);

        break;
        
      case P_SELECT_GEQ:

        com_nr_left = p_com->left->number;
        argument = unquote(p_com->argument);

        MILPRINTF(MILOUT, "R%d := R%d.near_val(%d,%s);\n", com_num, com_nr_left, P_SELECT_GEQ, p_com->argument);

        break;
        
      case P_SELECT_LEQ:
        
        com_nr_left = p_com->left->number;
        argument = unquote(p_com->argument);
        
        MILPRINTF(MILOUT, "R%d := R%d.near_val(%d,%s);\n", com_num, com_nr_left, P_SELECT_LEQ, p_com->argument);

        break;
        
      case P_ADJ:

        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        if (txt_retr_model->model == MODEL_OKAPI) 
          MILPRINTF(MILOUT, "R%d := R%d.adj_term(phrase,%d,true,0.5);\n", com_num, com_nr_left, txt_retr_model->size_type);
        else
          MILPRINTF(MILOUT, "R%d := R%d.adj_term(phrase,%d,true,%f);\n", com_num, com_nr_left, txt_retr_model->size_type, txt_retr_model->param2);

        break;

      case P_ADJ_NOT:
      
        com_nr_left = p_com->left->number;
        com_nr_right = p_com->right->number;

        if (txt_retr_model->model == MODEL_OKAPI)
          MILPRINTF(MILOUT, "R%d := R%d.adj_term_not(phrase,%d,true,0.5);\n", com_num, com_nr_left, txt_retr_model->size_type);
        else
          MILPRINTF(MILOUT, "R%d := R%d.adj_term_not(phrase,%d,true,%f);\n", com_num, com_nr_left, txt_retr_model->size_type, txt_retr_model->param2);

        break;

      }
      if (p_com->operator != QUERY_ADD_TERM && p_com->operator != QUERY_ADD_MODIFIER) {

        if (p_com->operator != P_ADJ && p_com->operator != P_ADJ_NOT) {

          if (p_com->left != NULL) {
            if (op_newnum[p_com->left->number] == 1) {
              MILPRINTF(MILOUT, "R%d := nil;\n", p_com->left->number);
            }
            op_newnum[p_com->left->number]--;
          }

          if (p_com->right != NULL) {
            if (op_newnum[p_com->right->number] == 1) {
              MILPRINTF(MILOUT, "R%d := nil;\n", p_com->right->number);
            }
            op_newnum[p_com->right->number]--;
          }

          if (p_com->operator == P_SELECT_NODE_Q) {
              MILPRINTF(MILOUT, "terms := nil;\n");
              MILPRINTF(MILOUT, "modifiers := nil;\n");
          }
        }

        else {
        
        if (op_newnum[p_com->left->number] == 1) {
          MILPRINTF(MILOUT, "R%d := nil;\n", p_com->left->number);
        }

        op_newnum[p_com->left->number]--;

        if (op_newnum[p_com->right->number] == 1) {
          MILPRINTF(MILOUT, "phrase := nil;\n");
        }

        op_newnum[p_com->right->number]--;

        }
      }
      op_newnum[com_num] = op_number[p_com->number];
      p_com->number = com_num;

      if ( TDEBUG(5) ) {
          MILPRINTF(MILOUT,"printf(\"# tijah-mil-exec: computed R%d.\\n\");\n",com_num);
      }
      
      POP_COMMAND();
      com_sp++;


    }

    if ( parserCtx->useFragments ) {
      MILPRINTF(MILOUT, "var collect := new(oid,dbl);\n");
      MILPRINTF(MILOUT, "R%d@batloop() {\n", com_num);
      MILPRINTF(MILOUT, "  collect.insert($t);\n");
      MILPRINTF(MILOUT, "}\n");
      MILPRINTF(MILOUT, "R%d := collect;\n",com_num);
      MILPRINTF(MILOUT, "collect := nil;\n");
    }
    MILPRINTF(MILOUT, "R%d := R%d.tsort_rev();\n", com_num, com_num);
    MILPRINTF(MILOUT, "if ( retNum >= 0 ) { R%d := R%d.slice(0, retNum - 1); }\n", com_num, com_num);
    MILPRINTF(MILOUT, "R%d.persists(true).rename(\"nexi_result\");\n", com_num);
    MILPRINTF(MILOUT, "}\n");
      
    p_com_array++;
    p1_command = *p_com_array;
  
    txt_retr_model = txt_retr_model->next;

    if ( TDEBUG(5) ) {
       MILPRINTF(MILOUT,"printf(\"# tijah-mil-exec: finish computation result in R%d.\\n\");\n",com_num);
    }
    /* printf("%d\n",p_com_array); */
    /* printf("%d\n",p1_command);   */

  }

  free(term_cut);
  free(unq_term);
  free(argument1);
  
  return 1;
}

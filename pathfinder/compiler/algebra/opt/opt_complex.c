/**
 * @file
 *
 * Optimize relational algebra expression DAG
 * based on multiple properties.
 * (This requires no burg pattern matching as we
 *  apply optimizations in a peep-hole style on
 *  single nodes only.)
 *
 * Copyright Notice:
 * -----------------
 *
 * The contents of this file are subject to the Pathfinder Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://monetdb.cwi.nl/Legal/PathfinderLicense-1.1.html
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Pathfinder system.
 *
 * The Original Code has initially been developed by the Database &
 * Information Systems Group at the University of Konstanz, Germany and
 * is now maintained by the Database Systems Group at the Technische
 * Universitaet Muenchen, Germany.  Portions created by the University of
 * Konstanz and the Technische Universitaet Muenchen are Copyright (C)
 * 2000-2005 University of Konstanz and (C) 2005-2008 Technische
 * Universitaet Muenchen, respectively.  All Rights Reserved.
 *
 * $Id$
 */

/* always include pathfinder.h first! */
#include "pathfinder.h"
#include <assert.h>
#include <stdio.h>

#include "algopt.h"
#include "properties.h"
#include "alg_dag.h"
#include "mem.h"          /* PFmalloc() */

/** mnemonic algebra constructors */
#include "logical_mnemonic.h"

/* mnemonic column list accessors */
#include "alg_cl_mnemonic.h"

/* Easily access subtree-parts */
#include "child_mnemonic.h"

#define SEEN(p) ((p)->bit_dag)

/* lookup the input name of an output column
   of a projection */
static PFalg_col_t
find_old_name (PFla_op_t *op, PFalg_col_t col)
{
    assert (op->kind == la_project);

    for (unsigned int i = 0; i < op->sem.proj.count; i++)
         if (op->sem.proj.items[i].new == col)
             return op->sem.proj.items[i].old;

    return col_NULL;
}

/**
 * This function checks for the following bigger pattern:
 *
 *                    |
 *                   sel_d
 *                    |
 *                   eq_d:<pos1,pos2>
 *                    |
 *                   pi*
 *                    |
 *                   |X|_<iter1,iter2>
 *         __________/ \_________
 *        /                      \
 *       pi*                     pi*
 *       |                        |
 *      row#_pos1:<col1>/iter1   row#_pos2:<col2>/iter2
 *       |                        |
 *       pi*                     pi*
 *        \__________   _________/
 *                   \ /
 *                    op
 *
 * where at operator op the following conditions need to
 * be fulfilled: col1 = col2 and iter1 = iter2.
 *
 * The pattern describes a split of two columns that
 * are merged back together (zip) based on their iter and pos
 * values. (The aligned row# operators ensure that iter+pos
 * provide a comparable key.)
 *
 * The result pattern avoids the splitting and correctly
 * reflects the renaming of the various projection operators
 * (in pi_...):
 *
 *                    |
 *                   pi_pos1:pos',pos2:pos',d:d',...
 *                    |
 *                    @_d':true
 *                    |
 *                   row#_pos':<col1>/iter1
 *                    |
 *                    op
 *
 * As the pattern checking is more complicated than the
 * replacement we marked all code snippets that prepare
 * the transformation as ACTION code to make the pattern
 * detection code more readible.
 */
static bool
zip_alignment (PFla_op_t *p)
{
    PFalg_col_t   pos1,
                  pos2,
                  iter1,
                  iter2,
                  sort1,
                  sort2;
    PFla_op_t    *op = p,
                 *lop,
                 *rop;
    /* ACTION: local variables needed for the transformation */
    unsigned int  count  = p->schema.count,
                  lcount = 0,
                  rcount = 0;
    PFalg_proj_t *proj,
                 *lproj,
                 *rproj;
    PFalg_col_t   new_rownum,
                  new_eq,
                  rownum1,
                  rownum2,
                  eq;
    /* end of action code part */

    /* check for pattern 'sel (eq (_))' where the selection
       consumes the output of comparison */
    if (op->kind != la_select ||
        L(op)->kind != la_num_eq ||
        op->sem.select.col != L(op)->sem.binary.res)
        return false;

    /* ACTION: initialize column name mapping
               to correctly rename all columns */
    proj = PFmalloc (count * sizeof (PFalg_proj_t));
    for (unsigned int i = 0; i < count; i++)
        proj[i] = PFalg_proj (p->schema.items[i].name,
                              p->schema.items[i].name);
    /* end of action code part */

    pos1 = L(op)->sem.binary.col1;
    pos2 = L(op)->sem.binary.col2;

    op = LL(op);

    /* update the important column names
       for all projection operators on the way */
    while (op->kind == la_project) {
        pos1 = find_old_name (op, pos1); if (!pos1) return false;
        pos2 = find_old_name (op, pos2); if (!pos2) return false;
        /* ACTION: update column name mapping */
        for (unsigned int i = 0; i < count; i++)
            proj[i].old = find_old_name (op, proj[i].old);
        /* end of action code part */
        op   = L(op);
    }

    /* check for join */
    if (op->kind != la_eqjoin)
        return false;

    iter1 = op->sem.eqjoin.col1;
    iter2 = op->sem.eqjoin.col2;

    lop = L(op);
    rop = R(op);
    
    /* ACTION: split up column name mapping into
               a left and a right mapping */
    lproj = PFmalloc (lop->schema.count * sizeof (PFalg_proj_t));
    rproj = PFmalloc (rop->schema.count * sizeof (PFalg_proj_t));
    for (unsigned int i = 0; i < count; i++) {
        if (PFprop_ocol (lop, proj[i].old))
            lproj[lcount++] = proj[i];
        else
            rproj[rcount++] = proj[i];
    }
    /* end of action code part */

    /* update the important column names
       for all projection operators on the way */
    while (lop->kind == la_project) {
        iter1 = find_old_name (lop, iter1); if (!iter1) return false;
        pos1  = find_old_name (lop, pos1);  if (!pos1)  return false;
        /* ACTION: update left column name mapping */
        for (unsigned int i = 0; i < lcount; i++)
            lproj[i].old = find_old_name (lop, lproj[i].old);
        /* end of action code part */
        lop   = L(lop);
    }
    while (rop->kind == la_project) {
        iter2 = find_old_name (rop, iter2); if (!iter2) return false;
        pos2  = find_old_name (rop, pos2);  if (!pos2)  return false;
        /* ACTION: update right column name mapping */
        for (unsigned int i = 0; i < rcount; i++)
            rproj[i].old = find_old_name (rop, rproj[i].old);
        /* end of action code part */
        rop   = L(rop);
    }

    /* check for the correct rownumber usage in the left
       and the right side of the equi-join */
    if (lop->kind != la_rownum ||
        rop->kind != la_rownum ||
        lop->sem.sort.res != pos1 ||
        rop->sem.sort.res != pos2 ||
        lop->sem.sort.part != iter1 ||
        rop->sem.sort.part != iter2 ||
        PFord_count (lop->sem.sort.sortby) != 1 ||
        PFord_count (rop->sem.sort.sortby) != 1 ||
        PFord_order_dir_at (lop->sem.sort.sortby, 0) != DIR_ASC ||
        PFord_order_dir_at (rop->sem.sort.sortby, 0) != DIR_ASC)
        return false;
   
    sort1 = PFord_order_col_at (lop->sem.sort.sortby, 0);
    sort2 = PFord_order_col_at (rop->sem.sort.sortby, 0);

    lop = L(lop);
    rop = L(rop);

    /* update the important column names
       for all projection operators on the way */
    while (lop->kind == la_project) {
        iter1 = find_old_name (lop, iter1); if (!iter1) return false;
        sort1 = find_old_name (lop, sort1); if (!sort1) return false;
        /* ACTION: update left column name mapping */
        for (unsigned int i = 0; i < lcount; i++)
            lproj[i].old = find_old_name (lop, lproj[i].old);
        /* end of action code part */
        lop   = L(lop);
    }
    while (rop->kind == la_project) {
        iter2 = find_old_name (rop, iter2); if (!iter2) return false;
        sort2 = find_old_name (rop, sort2); if (!sort2) return false;
        /* ACTION: update right column name mapping */
        for (unsigned int i = 0; i < rcount; i++)
            rproj[i].old = find_old_name (rop, rproj[i].old);
        /* end of action code part */
        rop   = L(rop);
    }

    /* check if we have a common operator
       and if all column names match */
    if (lop != rop ||
        iter1 != iter2 ||
        sort1 != sort2)
        return false;

    /* consistency check - we have to find all updated column names  */
    for (unsigned int i = 0; i < lcount; i++)
        if (lproj[i].old == col_NULL &&
            lproj[i].new != L(p)->sem.binary.col1)
            return false;

    /* consistency check - we have to find all updated column names  */
    for (unsigned int i = 0; i < rcount; i++)
        if (rproj[i].old == col_NULL &&
            rproj[i].new != L(p)->sem.binary.col2 &&
            rproj[i].new != L(p)->sem.binary.res)
            return false;

    /* ACTION: */
    /* create two new column names to avoid name conflicts */
    new_rownum = PFcol_new (col_pos);
    new_eq     = PFcol_new (col_item);
    rownum1    = L(p)->sem.binary.col1;
    rownum2    = L(p)->sem.binary.col2;
    eq         = L(p)->sem.binary.res;

    /* merge back the modified column names of the left
       name mapping into the initial list */
    for (unsigned int i = 0; i < lcount; i++)
        for (unsigned int j = 0; j < count; j++)
            if (lproj[i].new == proj[j].new) {
                proj[j].old = lproj[i].old;
                break;
            }
    /* merge back the modified column names of the right
       name mapping into the initial list */
    for (unsigned int i = 0; i < rcount; i++)
        for (unsigned int j = 0; j < count; j++)
            if (rproj[i].new == proj[j].new) {
                proj[j].old = rproj[i].old;
                break;
            }
    /* Adjust the column names of the three columns
       generated in the pattern (comparison and rownumber
       operators). */
    for (unsigned int j = 0; j < count; j++) {
        if (rownum1 == proj[j].new ||
            rownum2 == proj[j].new)
            proj[j].old = new_rownum;
        else if (eq == proj[j].new)
            proj[j].old = new_eq;
    }

    /* link the base only once and thus ignore the aligning
       join on columns iter and pos */
    *p = *PFla_project_ (
              attach (rownum (lop, new_rownum, sortby (sort1), iter1),
                      new_eq,
                      PFalg_lit_bln (true)),
              count,
              proj);
    /* end of action code part */

    /* we have rewritten the query plan
       based on the pattern */
    return true;
}


/**
 * find_last_base checks for the following
 * small DAG fragment:
 *
 *     /   pi*  \*
 *    |    |     |
 *     \  |X|   /     Note: All joins |X| are mapping joins
 *        / \               (the right join column is key).
 *      |X|  \
 *      / \
 *     |   \
 *    pi*  pi*
 *     |    |
 *     |   count_last/iter
 *     |    |
 *     |   pi*
 *     \   /
 *     (...)
 */
static bool
find_last_base (PFla_op_t *op,
                PFalg_col_t item,
                PFalg_col_t iter,
                PFalg_col_t last)
{
    PFla_op_t  *base = NULL;
    PFalg_col_t base_iter = col_NULL,
                base_last = col_NULL;

    /* ignore projections */
    while (op->kind == la_project) {
        iter = find_old_name (op, iter); if (!iter) return false;
        item = find_old_name (op, item); if (!item) return false;
        last = find_old_name (op, last); if (!last) return false;
        op = L(op);
    }

    if (op->kind != la_eqjoin ||
        (op->sem.eqjoin.col1 != iter &&
         op->sem.eqjoin.col2 != iter))
        return false;

    /* Ensure that the join is a mapping join (and decide
       based on column item where to look for the aggregate). */
    if (PFprop_ocol (L(op), item) &&
        PFprop_key_right (op->prop, op->sem.eqjoin.col2) &&
        PFprop_subdom (op->prop,
                       PFprop_dom_left (op->prop,
                                        op->sem.eqjoin.col1),
                       PFprop_dom_right (op->prop,
                                         op->sem.eqjoin.col2))) {
        base_iter = op->sem.eqjoin.col1;
        base_last = last;
        iter = op->sem.eqjoin.col2;
        base = L(op);
        op   = R(op);
    }
    else if (PFprop_ocol (R(op), item) &&
             PFprop_key_left (op->prop, op->sem.eqjoin.col1) &&
             PFprop_subdom (op->prop,
                            PFprop_dom_right (op->prop,
                                              op->sem.eqjoin.col2),
                            PFprop_dom_left (op->prop,
                                             op->sem.eqjoin.col1))) {
        base_iter = op->sem.eqjoin.col2;
        base_last = last;
        iter = op->sem.eqjoin.col1;
        base = R(op);
        op   = L(op);
    }
    else
        return false;

    /* ignore projections */
    while (op->kind == la_project) {
        iter = find_old_name (op, iter); if (!iter) break;
        last = find_old_name (op, last); if (!last) break;
        op = L(op);
    }

    /* check for count aggregate */
    if (op->kind == la_count &&
        op->sem.aggr.part &&
        op->sem.aggr.part == iter &&
        op->sem.aggr.res  == last) {
        /* follow the projection list to the base operator */
        while (base->kind == la_project) {
            base_iter = find_old_name (base, base_iter);
            if (!base_iter) return false;
            base = L(base);
        }
        op = L(op);

        /* follow the projections underneath the count operator */
        while (op->kind == la_project) {
            iter = find_old_name (op, iter); if (!iter) return false;
            op = L(op);
        }

        /* ensure that the count operator stems from the same input
           as the order column (and refers to the same iterations) */
        return op == base && iter == base_iter;
    }
    else
        /* recursively call find_last_base() to ignore projections
           AND mapping joins */
        return find_last_base (base, item, base_iter, base_last);
}

/**
 * replace_pos_predicate checks for the following
 * small DAG fragment:
 *
 *        pi_1        This fragment has to fulfill some additional
 *         |          conditions:
 *        sel         o sel.item is not used above
 *         |          o sel.item == =.res (sel)
 *        pi*_2       o =.att1 is not used above
 *         |          o =.att2 is not used above
 *         =          o =.col[1|2] == @.res (optional match)
 *         |          o =.col[1|2] == cast.res
 *        pi*_3       o cast.type == aat_int
 *         |          o cast.res is only used in =
 *         @?         o cast.col == row#.res
 *         |          o row#.res == cast.col
 *        pi*_4       o row#.res is only used in cast
 *         |
 *        cast        Furthermore the operator underneath the rownum (..)
 *         |          forms the input to the new pos_select operator that
 *        pi*_5       replaces pi_1. The correct mapping of names is stored
 *         |          in variable map. If there was no attach operator (@?)
 *        row#        a further check for the positional predicate 'last()'
 *         |          is issued.
 *        (..)
 */
static void
replace_pos_predicate (PFla_op_t *p)
{
    PFalg_col_t sel, eq1, eq2, eq = col_NULL, cast,
                last = col_NULL, item, part;
    PFla_op_t *op, *base;
    long long int pos = 0;
    unsigned int count = p->schema.count;
    PFalg_proj_t *map = PFmalloc (count * sizeof (PFalg_proj_t));

    /* pi_1 && sel */
    if (p->kind != la_project ||
        L(p)->kind != la_select ||
        PFprop_icol (L(p)->prop, L(p)->sem.select.col))
        return;

    for (unsigned int i = 0; i < count; i++)
        map[i] = p->sem.proj.items[i];

    sel = L(p)->sem.select.col;
    op = LL(p);

    /* pi*_2 */
    while (op->kind == la_project) {
        for (unsigned int i = 0; i < count; i++) {
            map[i].old = find_old_name (op, map[i].old);
            if (!map[i].old) return;
        }
        sel  = find_old_name (op, sel); if (!sel) return;
        op = L(op);
    }

    /* = */
    if (op->kind != la_num_eq ||
        op->sem.binary.res != sel ||
        PFprop_icol (op->prop, op->sem.binary.col1) ||
        PFprop_icol (op->prop, op->sem.binary.col2))
        return;

    eq1 = op->sem.binary.col1;
    eq2 = op->sem.binary.col2;

    op = L(op);

    /* pi*_3 && @? && pi*_4 */
    while (op->kind == la_project || op->kind == la_attach) {
        if (op->kind == la_attach) {
            if (pos) return;

            if (op->sem.attach.res == eq1 &&
                op->sem.attach.value.type == aat_int) {
                pos = op->sem.attach.value.val.int_;
                eq  = eq2;
            }
            else if (op->sem.attach.res == eq2 &&
                     op->sem.attach.value.type == aat_int) {
                pos = op->sem.attach.value.val.int_;
                eq  = eq1;
            }

            if (pos <= 0) return;

            op = L(op);

        } else {
            for (unsigned int i = 0; i < count; i++) {
                map[i].old = find_old_name (op, map[i].old);
                if (!map[i].old) return;
            }

            if (pos) {
                eq = find_old_name (op, eq);
                if (!eq) return;
            } else {
                eq1 = find_old_name (op, eq1);
                eq2 = find_old_name (op, eq2);
                if (!eq1 || !eq2) return;
            }

            op = L(op);
        }
    }

    /* cast */
    if (op->kind != la_cast ||
        op->sem.type.ty != aat_int ||
        (op->sem.type.res != eq1 && op->sem.type.res != eq2) ||
        PFprop_icol (op->prop, op->sem.type.col))
        return;

    if (!pos) last = op->sem.type.res == eq1 ? eq2 : eq1;
    cast = op->sem.type.col;

    op = L(op);

    /* pi*_5 */
    while (op->kind == la_project) {
        for (unsigned int i = 0; i < count; i++) {
            map[i].old = find_old_name (op, map[i].old);
            if (!map[i].old) return;
        }

        cast = find_old_name (op, cast); if (!cast) return;

        if (!pos) {
            last = find_old_name (op, last);
            if (!last) return;
        }

        op = L(op);
    }

    /* row# */
    if (op->kind != la_rownum ||
        op->sem.sort.res != cast ||
        PFord_count (op->sem.sort.sortby) != 1 ||
        PFord_order_dir_at (op->sem.sort.sortby, 0) != DIR_ASC)
        return;

    /* check that the result of the rownum operator is not used above */
    for (unsigned int i = 0; i < count; i++)
        if (map[i].old == op->sem.sort.res)
            return;

    base = L(op);
    part = op->sem.sort.part;
    item = PFord_order_col_at (op->sem.sort.sortby, 0);

    if (!pos) {
        if (!part ||
            !find_last_base (base, item, part, last)) return;
        pos = -1;
    }

    *p = *PFla_project_ (
              PFla_pos_select (
                  base,
                  pos,
                  PFord_refine (PFordering (), item, DIR_ASC),
                  part),
              count,
              map);
    /* replace pattern here:
    fprintf(stderr,"pos %lli;", pos);
    fprintf(stderr," sort by %s; partition by %s;",
            PFcol_str(item),
            PFcol_str(part));
    for (unsigned int i = 0; i < count; i++)
        fprintf(stderr," map %s to %s%s",
                PFcol_str(map[i].new),
                PFcol_str(map[i].old),
                i == count - 1 ?"\n":";");
    */
}

/* worker for PFalgopt_complex */
static void
opt_complex (PFla_op_t *p)
{
    assert (p);

    /* rewrite each node only once */
    if (SEEN(p))
        return;
    else
        SEEN(p) = true;

    /* apply complex optimization for children */
    for (unsigned int i = 0; i < PFLA_OP_MAXCHILD && p->child[i]; i++)
        opt_complex (p->child[i]);

    /* action code */
    switch (p->kind) {
        case la_serialize_seq:
            if (PFprop_card (p->prop) == 1) {
                R(p) = PFla_attach (
                           PFla_project (
                               R(p),
                               PFalg_proj (p->sem.ser_seq.item,
                                           p->sem.ser_seq.item)),
                           p->sem.ser_seq.pos,
                           PFalg_lit_nat (1));
            }
            break;

        case la_attach:
            /**
             * if an attach column is the only required column
             * and we know its exact cardinality we can replace
             * the complete subtree by a literal table.
             */
            if (PFprop_icols_count (p->prop) == 1 &&
                PFprop_icol (p->prop, p->sem.attach.res) &&
                PFprop_card (p->prop) >= 1) {

                PFla_op_t *res;
                unsigned int count = PFprop_card (p->prop);
                /* create projection list to avoid missing columns */
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));

                /* create list of tuples each containing a list of atoms */
                PFalg_tuple_t *tuples = PFmalloc (count *
                                                  sizeof (*(tuples)));;

                for (unsigned int i = 0; i < count; i++) {
                    tuples[i].atoms = PFmalloc (sizeof (*(tuples[i].atoms)));
                    tuples[i].atoms[0] = p->sem.attach.value;
                    tuples[i].count = 1;
                }

                res = PFla_lit_tbl_ (collist (p->sem.attach.res),
                                     count, tuples);

                /* Every column of the relation will point
                   to the attach argument to avoid missing
                   references. (Columns that are not required
                   may be still referenced by the following
                   operators.) */
                for (unsigned int i = 0; i < p->schema.count; i++)
                    proj[i] = PFalg_proj (p->schema.items[i].name,
                                          p->sem.attach.res);

                *p = *PFla_project_ (res, p->schema.count, proj);
            }
/* ineffective without step operators */
            /* prune unnecessary attach-project operators */
            if (L(p)->kind == la_project &&
                L(p)->schema.count == 1 &&
                (LL(p)->kind == la_step || LL(p)->kind == la_guide_step) &&
                p->sem.attach.res == LL(p)->sem.step.iter &&
                PFprop_const (LL(p)->prop, LL(p)->sem.step.iter) &&
                PFalg_atom_comparable (
                    p->sem.attach.value,
                    PFprop_const_val (LL(p)->prop, LL(p)->sem.step.iter)) &&
                !PFalg_atom_cmp (
                    p->sem.attach.value,
                    PFprop_const_val (LL(p)->prop, LL(p)->sem.step.iter)) &&
                L(p)->sem.proj.items[0].new == LL(p)->sem.step.item_res) {
                *p = *PFla_dummy (LL(p));
                break;
            }
            /* prune unnecessary attach-project operators */
            if (L(p)->kind == la_project &&
                L(p)->schema.count == 1 &&
                (LL(p)->kind == la_step || LL(p)->kind == la_guide_step) &&
                PFprop_const (LL(p)->prop, LL(p)->sem.step.iter) &&
                PFalg_atom_comparable (
                    p->sem.attach.value,
                    PFprop_const_val (LL(p)->prop, LL(p)->sem.step.iter)) &&
                !PFalg_atom_cmp (
                    p->sem.attach.value,
                    PFprop_const_val (LL(p)->prop, LL(p)->sem.step.iter)) &&
                L(p)->sem.proj.items[0].old == LL(p)->sem.step.item_res) {
                *p = *PFla_project (PFla_dummy (LL(p)),
                                    PFalg_proj (p->sem.attach.res,
                                                LL(p)->sem.step.iter),
                                    L(p)->sem.proj.items[0]);
                break;
/* end of: ineffective without step operators */
            }

            break;

        case la_project:
            replace_pos_predicate (p);
            break;

        case la_eqjoin:
            /**
             * if we have a key join (key property) on a
             * domain-subdomain relationship (domain property)
             * where the columns of the argument marked as 'domain'
             * are not required (icol property) we can skip the join
             * completely.
             */
        {
            /* we can use the schema information of the children
               as no rewrite adds more columns to that subtree. */
            bool left_arg_req = false;
            bool right_arg_req = false;

            /* discard join columns as one of them always remains */
            for (unsigned int i = 0; i < L(p)->schema.count; i++) {
                left_arg_req = left_arg_req ||
                               (!PFprop_subdom (
                                     p->prop,
                                     PFprop_dom_left (
                                         p->prop,
                                         p->sem.eqjoin.col1),
                                     PFprop_dom_left (
                                         p->prop,
                                         L(p)->schema.items[i].name)) &&
                                PFprop_icol (
                                   p->prop,
                                   L(p)->schema.items[i].name));
            }
            if ((PFprop_key_left (p->prop, p->sem.eqjoin.col1) ||
                 PFprop_set (p->prop)) &&
                PFprop_subdom (p->prop,
                               PFprop_dom_right (p->prop,
                                                 p->sem.eqjoin.col2),
                               PFprop_dom_left (p->prop,
                                                p->sem.eqjoin.col1)) &&
                !left_arg_req) {
                /* Every column of the left argument will point
                   to the join argument of the right argument to
                   avoid missing references. (Columns that are not
                   required may be still referenced by the following
                   operators.) */
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));
                unsigned int count = 0;

                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        L(p)->schema.items[i].name,
                                        p->sem.eqjoin.col2);

                for (unsigned int i = 0; i < R(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        R(p)->schema.items[i].name,
                                        R(p)->schema.items[i].name);

                *p = *PFla_project_ (R(p), count, proj);
                break;
            }

            /* discard join columns as one of them always remains */
            for (unsigned int i = 0; i < R(p)->schema.count; i++) {
                right_arg_req = right_arg_req ||
                                (!PFprop_subdom (
                                      p->prop,
                                      PFprop_dom_right (
                                          p->prop,
                                          p->sem.eqjoin.col2),
                                      PFprop_dom_right (
                                          p->prop,
                                          R(p)->schema.items[i].name)) &&
                                 PFprop_icol (
                                     p->prop,
                                     R(p)->schema.items[i].name));
            }
            if ((PFprop_key_right (p->prop, p->sem.eqjoin.col2) ||
                 PFprop_set (p->prop)) &&
                PFprop_subdom (p->prop,
                               PFprop_dom_left (p->prop,
                                                p->sem.eqjoin.col1),
                               PFprop_dom_right (p->prop,
                                                 p->sem.eqjoin.col2)) &&
                !right_arg_req) {
                /* Every column of the right argument will point
                   to the join argument of the left argument to
                   avoid missing references. (Columns that are not
                   required may be still referenced by the following
                   operators.) */
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));
                unsigned int count = 0;

                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        L(p)->schema.items[i].name,
                                        L(p)->schema.items[i].name);

                for (unsigned int i = 0; i < R(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        R(p)->schema.items[i].name,
                                        p->sem.eqjoin.col1);

                *p = *PFla_project_ (L(p), count, proj);
                break;
            }

            /* this code makes the recognition of last()
               predicates easier */
            if (PFprop_subdom (p->prop,
                               PFprop_dom_left (p->prop,
                                                p->sem.eqjoin.col1),
                               PFprop_dom_right (p->prop,
                                                 p->sem.eqjoin.col2)) &&
                R(p)->kind == la_project &&
                RL(p)->kind == la_disjunion) {
                PFalg_proj_t *proj     = R(p)->sem.proj.items;
                unsigned int  count    = R(p)->schema.count;
                PFalg_col_t   col1     = p->sem.eqjoin.col1,
                              col2     = p->sem.eqjoin.col2;
                for (unsigned int i = 0; i < count; i++)
                    if (proj[i].new == col2) {
                        col2 = proj[i].old;
                        break;
                    }

                if (PFprop_disjdom (p->prop, 
                                    PFprop_dom_left (RL(p)->prop, col2),
                                    PFprop_dom_left (p->prop, col1)))
                    R(p) = PFla_project_ (RLR(p), count, proj);
                else if (PFprop_disjdom (p->prop, 
                                         PFprop_dom_right (RL(p)->prop, col2),
                                         PFprop_dom_left (p->prop, col1)))
                    R(p) = PFla_project_ (RLL(p), count, proj);
            }
            if (PFprop_subdom (p->prop,
                               PFprop_dom_right (p->prop,
                                                p->sem.eqjoin.col2),
                               PFprop_dom_left (p->prop,
                                                 p->sem.eqjoin.col1)) &&
                L(p)->kind == la_project &&
                LL(p)->kind == la_disjunion) {
                PFalg_proj_t *proj     = L(p)->sem.proj.items;
                unsigned int  count    = L(p)->schema.count;
                PFalg_col_t   col1     = p->sem.eqjoin.col1,
                              col2     = p->sem.eqjoin.col2;
                for (unsigned int i = 0; i < count; i++)
                    if (proj[i].new == col1) {
                        col1 = proj[i].old;
                        break;
                    }

                if (PFprop_disjdom (p->prop, 
                                    PFprop_dom_left (LL(p)->prop, col1),
                                    PFprop_dom_right (p->prop, col2)))
                    L(p) = PFla_project_ (LLR(p), count, proj);
                else if (PFprop_disjdom (p->prop, 
                                         PFprop_dom_right (LL(p)->prop, col1),
                                         PFprop_dom_right (p->prop, col2)))
                    L(p) = PFla_project_ (LLL(p), count, proj);
            }

#if 0 /* disable join -> semijoin rewrites */
            /* introduce semi-join operator if possible */
            if (!left_arg_req &&
                (PFprop_key_left (p->prop, p->sem.eqjoin.col1) ||
                 PFprop_set (p->prop))) {
                /* Every column of the left argument will point
                   to the join argument of the right argument to
                   avoid missing references. (Columns that are not
                   required may be still referenced by the following
                   operators.) */
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));
                unsigned int count = 0;
                PFla_op_t *semijoin;
                PFla_op_t *left = L(p);
                PFla_op_t *right = R(p);
                PFalg_col_t lcol = p->sem.eqjoin.col1;
                PFalg_col_t rcol = p->sem.eqjoin.col2;

                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        L(p)->schema.items[i].name,
                                        p->sem.eqjoin.col2);

                for (unsigned int i = 0; i < R(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        R(p)->schema.items[i].name,
                                        R(p)->schema.items[i].name);

                while (left->kind == la_project) {
                    for (unsigned int i = 0; i < left->sem.proj.count; i++)
                        if (lcol == left->sem.proj.items[i].new) {
                            lcol = left->sem.proj.items[i].old;
                            break;
                        }
                    left = L(left);
                }
                while (right->kind == la_project) {
                    for (unsigned int i = 0; i < right->sem.proj.count; i++)
                        if (rcol == right->sem.proj.items[i].new) {
                            rcol = right->sem.proj.items[i].old;
                            break;
                        }
                    right = L(right);
                }

                if (lcol == rcol && left == right)
                    semijoin = R(p);
                else
                    semijoin = PFla_semijoin (
                                   R(p),
                                   L(p),
                                   p->sem.eqjoin.col2,
                                   p->sem.eqjoin.col1);

                *p = *PFla_project_ (semijoin, count, proj);
                break;
            }

            /* introduce semi-join operator if possible */
            if (!right_arg_req &&
                (PFprop_key_right (p->prop, p->sem.eqjoin.col2) ||
                 PFprop_set (p->prop))) {
                /* Every column of the right argument will point
                   to the join argument of the left argument to
                   avoid missing references. (Columns that are not
                   required may be still referenced by the following
                   operators.) */
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));
                unsigned int count = 0;
                PFla_op_t *semijoin;
                PFla_op_t *left = L(p);
                PFla_op_t *right = R(p);
                PFalg_col_t lcol = p->sem.eqjoin.col1;
                PFalg_col_t rcol = p->sem.eqjoin.col2;

                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        L(p)->schema.items[i].name,
                                        L(p)->schema.items[i].name);

                for (unsigned int i = 0; i < R(p)->schema.count; i++)
                    proj[count++] = PFalg_proj (
                                        R(p)->schema.items[i].name,
                                        p->sem.eqjoin.col1);

                while (left->kind == la_project) {
                    for (unsigned int i = 0; i < left->sem.proj.count; i++)
                        if (lcol == left->sem.proj.items[i].new) {
                            lcol = left->sem.proj.items[i].old;
                            break;
                        }
                    left = L(left);
                }
                while (right->kind == la_project) {
                    for (unsigned int i = 0; i < right->sem.proj.count; i++)
                        if (rcol == right->sem.proj.items[i].new) {
                            rcol = right->sem.proj.items[i].old;
                            break;
                        }
                    right = L(right);
                }

                if (lcol == rcol && left == right)
                    semijoin = L(p);
                else
                    semijoin = PFla_semijoin (
                                   L(p),
                                   R(p),
                                   p->sem.eqjoin.col1,
                                   p->sem.eqjoin.col2);

                *p = *PFla_project_ (semijoin, count, proj);
                break;
            }
#endif
        }   break;

        case la_semijoin:
            /* if the semijoin operator does not prune a row
               because the domains are identical and the left
               side does not contain duplicates we can safely
               remove it. */
            if (PFprop_ckey (p->prop, p->schema) &&
                PFprop_subdom (
                    p->prop,
                    PFprop_dom_left (p->prop,
                                     p->sem.eqjoin.col1),
                    PFprop_dom_right (p->prop,
                                      p->sem.eqjoin.col2)) &&
                PFprop_subdom (
                    p->prop,
                    PFprop_dom_right (p->prop,
                                      p->sem.eqjoin.col2),
                    PFprop_dom_left (p->prop,
                                     p->sem.eqjoin.col1))) {
                *p = *dummy (L(p));
                break;
            }
            if (L(p)->kind == la_difference &&
                (L(p)->schema.count == 1 ||
                 PFprop_key (p->prop, p->sem.eqjoin.col1)) &&
                PFprop_subdom (
                    p->prop,
                    PFprop_dom_right (p->prop,
                                      p->sem.eqjoin.col2),
                    PFprop_dom_right (L(p)->prop,
                                      p->sem.eqjoin.col1))) {
                *p = *PFla_empty_tbl_ (p->schema);
                break;
            }

            if (!PFprop_key_left (p->prop, p->sem.eqjoin.col1) ||
                !PFprop_subdom (p->prop,
                                PFprop_dom_right (p->prop,
                                                  p->sem.eqjoin.col2),
                                PFprop_dom_left (p->prop,
                                                 p->sem.eqjoin.col1)))
                break;

            /* remove the distinct operator and redirect the
               references to the semijoin operator */
            if (R(p)->kind == la_distinct) {
                PFla_op_t *distinct = R(p);
                R(p) = L(distinct);
                *distinct = *PFla_project (p, PFalg_proj (p->sem.eqjoin.col2,
                                                          p->sem.eqjoin.col1));
            }
            else if (R(p)->kind == la_project &&
                     RL(p)->kind == la_distinct &&
                     R(p)->schema.count == 1 &&
                     RL(p)->schema.count == 1) {
                PFla_op_t *project = R(p),
                          *distinct = RL(p);
                R(p) = L(distinct);
                *project = *PFla_project (
                                p,
                                PFalg_proj (p->sem.eqjoin.col2,
                                            p->sem.eqjoin.col1));
                *distinct = *PFla_project (
                                 p,
                                 PFalg_proj (distinct->schema.items[0].name,
                                             p->sem.eqjoin.col1));

                /* we need to adjust the semijoin argument as well */
                p->sem.eqjoin.col2 = R(p)->schema.items[0].name;
            }
            break;

        case la_cross:
            /* PFprop_icols_count () == 0 is also true
               for nodes without inferred properties
               (newly created nodes). The cardinality
               constraint however ensures that the
               properties are available. */
            if (PFprop_card (L(p)->prop) == 1 &&
                PFprop_icols_count (L(p)->prop) == 0) {
                *p = *PFla_dummy (R(p));
                break;
            }
            if (PFprop_card (R(p)->prop) == 1 &&
                PFprop_icols_count (R(p)->prop) == 0) {
                *p = *PFla_dummy (L(p));
                break;
            }
            break;

        /* Remove unnecessary distinct operators */
        case la_distinct:
            if (PFprop_ckey (L(p)->prop, p->schema))
                *p = *PFla_dummy (L(p));
            break;

        case la_select:
            /* check for the alignment of columns produced
               by the zip operator in the ferryc compiler */
            if (zip_alignment (p))
                break;
        /**
         * Rewrite the pattern (1) into expression (2):
         *
         *          select_(col1) [icols:col2]          pi_(col2,...:col2)
         *            |                                  |
         *         ( pi_(col1,col2) )                 distinct
         *            |                                  |
         *           or_(col1:col3,col4)               union
         *            |                              ____/\____
         *         ( pi_(col2,col3,col4) )          /          \
         *            |                            pi_(col2)   pi_(col2:col5)
         *           |X|_(col2,col5)              /              \
         *        __/   \__                    select_(col3)   select_(col4)
         *       /         \                     |                |
         *      /1\       /2\                   /1\              /2\
         *     /___\     /___\                 /___\            /___\
         * (col2,col3) (col5,col4)            (col2,col3)      (col5,col4)
         *
         *           (1)                                 (2)
         */
        {
            unsigned int i;
            PFalg_col_t col_sel,
                        col_join1, col_join2,
                        col_sel_in1, col_sel_in2;
            PFla_op_t *cur, *left, *right;
            PFalg_proj_t *lproj, *rproj, *top_proj;

            if (p->schema.count != 2 ||
                PFprop_icols_count (p->prop) != 1 ||
                PFprop_icol (p->prop, p->sem.select.col))
                break;

            col_sel = p->sem.select.col;
            col_join1 = p->schema.items[0].name != col_sel
                        ? p->schema.items[0].name
                        : p->schema.items[1].name;
            cur = L(p);

            /* cope with intermediate projections */
            if (cur->kind == la_project) {
                for (i = 0; i < cur->sem.proj.count; i++)
                    if (L(p)->sem.proj.items[i].new == col_sel)
                        col_sel = L(p)->sem.proj.items[i].old;
                    else if (L(p)->sem.proj.items[i].new == col_join1)
                        col_join1 = L(p)->sem.proj.items[i].old;
                cur = L(cur);
            }

            if (cur->kind != la_bool_or ||
                col_sel != cur->sem.binary.res)
                break;

            col_sel_in1 = cur->sem.binary.col1;
            col_sel_in2 = cur->sem.binary.col2;

            cur = L(cur);

            /* cope with intermediate projections */
            if (cur->kind == la_project) {
                for (i = 0; i < cur->sem.proj.count; i++)
                    if (L(p)->sem.proj.items[i].new == col_join1)
                        col_join1 = L(p)->sem.proj.items[i].old;
                    else if (L(p)->sem.proj.items[i].new == col_sel_in1)
                        col_sel_in1 = L(p)->sem.proj.items[i].old;
                    else if (L(p)->sem.proj.items[i].new == col_sel_in2)
                        col_sel_in2 = L(p)->sem.proj.items[i].old;
                cur = L(cur);
            }

            if (cur->kind != la_eqjoin ||
                (col_join1 != cur->sem.eqjoin.col1 &&
                 col_join1 != cur->sem.eqjoin.col2) ||
                /* Make sure that both join arguments are key as
                   otherwise placing the distinct operator above
                   the plan fragment is incorrect. */
                !PFprop_key_left (cur->prop, cur->sem.eqjoin.col1) ||
                !PFprop_key_right (cur->prop, cur->sem.eqjoin.col2))
                break;

            if (PFprop_ocol (L(cur), col_sel_in1) &&
                PFprop_ocol (R(cur), col_sel_in2)) {
                col_join1 = cur->sem.eqjoin.col1;
                col_join2 = cur->sem.eqjoin.col2;
                left = L(cur);
                right = R(cur);
            }
            else if (PFprop_ocol (L(cur), col_sel_in2) &&
                    PFprop_ocol (R(cur), col_sel_in1)) {
                col_join1 = cur->sem.eqjoin.col2;
                col_join2 = cur->sem.eqjoin.col1;
                left = R(cur);
                right = L(cur);
            }
            else
                break;

            /* pattern (1) is now ensured: create pattern (2) */
            lproj = PFmalloc (sizeof (PFalg_proj_t));
            rproj = PFmalloc (sizeof (PFalg_proj_t));
            top_proj = PFmalloc (2 * sizeof (PFalg_proj_t));

            lproj[0] = PFalg_proj (col_join1, col_join1);
            rproj[0] = PFalg_proj (col_join1, col_join2);
            top_proj[0] = PFalg_proj (p->schema.items[0].name, col_join1);
            top_proj[1] = PFalg_proj (p->schema.items[1].name, col_join1);

            *p = *PFla_project_ (
                      PFla_distinct (
                          PFla_disjunion (
                              PFla_project_ (
                                  PFla_select (left, col_sel_in1),
                                  1, lproj),
                              PFla_project_ (
                                  PFla_select (right, col_sel_in2),
                                  1, rproj))),
                      2, top_proj);
        }   break;

        case la_difference:
        {   /**
             * If the domains of the first relation are all subdomains
             * of the corresponding domains in the second argument
             * the result of the difference operation will be empty.
             */
            unsigned int all_subdom = 0;
            for (unsigned int i = 0; i < L(p)->schema.count; i++)
                for (unsigned int j = 0; j < R(p)->schema.count; j++)
                    if (L(p)->schema.items[i].name ==
                        R(p)->schema.items[j].name &&
                        PFprop_subdom (
                            p->prop,
                            PFprop_dom_left (p->prop,
                                             L(p)->schema.items[i].name),
                            PFprop_dom_right (p->prop,
                                              R(p)->schema.items[j].name))) {
                        all_subdom++;
                        break;
                    }

            if (all_subdom == p->schema.count &&
                /* we have to make sure that the left side
                   does not contain duplicates */
                PFprop_ckey (p->prop, p->schema)) {
                *p = *PFla_empty_tbl_ (p->schema);
                SEEN(p) = true;
                break;
            }
        }   break;

        /* to get rid of the operator 'and' and to split up
           different conditions we can introduce additional
           select operators above comparisons whose required
           value is true. */
        case la_bool_and:
            if (PFprop_req_bool_val (p->prop, p->sem.binary.res) &&
                PFprop_req_bool_val_val (p->prop, p->sem.binary.res) &&
                PFprop_set (p->prop)) {
                *p = *PFla_attach (
                          PFla_select (
                              PFla_select (
                                  L(p),
                                  p->sem.binary.col1),
                              p->sem.binary.col2),
                          p->sem.binary.res,
                          PFalg_lit_bln (true));
            }
            break;

        case la_rownum:
            /* Replace the rownumber operator by a projection
               if only its value distribution (keys) are required
               instead of its real values. */
            if (!PFprop_req_value_col (p->prop, p->sem.sort.res) &&
                PFord_count (p->sem.sort.sortby) == 1 &&
                PFprop_key (p->prop,
                            PFord_order_col_at (p->sem.sort.sortby, 0))) {
                /* create projection list */
                PFalg_proj_t *proj_list = PFmalloc ((L(p)->schema.count + 1)
                                                    * sizeof (*(proj_list)));

                /* We cannot rewrite if we require the correct order
                   and the rownum operator changes it from descending
                   to ascending. */
                if (PFord_order_dir_at (p->sem.sort.sortby, 0) == DIR_DESC &&
                    PFprop_req_order_col (p->prop, p->sem.sort.res))
                    break;

                /* copy the child schema (as we cannot be sure that
                   the schema of the rownum operator is still valid) ...*/
                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj_list[i] = PFalg_proj (
                                       L(p)->schema.items[i].name,
                                       L(p)->schema.items[i].name);

                /* ... and extend it with the sort
                   criterion as new rownum column */
                proj_list[L(p)->schema.count] = PFalg_proj (
                                                    p->sem.sort.res,
                                                    PFord_order_col_at (
                                                        p->sem.sort.sortby, 0));

                *p = *PFla_project_ (L(p), L(p)->schema.count + 1, proj_list);
            }
            break;

        case la_rank:
            /* first of all try to replace a rank with a single
               ascending order criterion by a projection match
               match the pattern rank - (project -) rank and
               try to merge both rank operators if the nested
               one only prepares some columns for the outer rank.
                 As most operators are separated by a projection
               we also support projections that do not rename. */
        {
            PFla_op_t *rank;
            bool proj = false, renamed = false;
            unsigned int i;

            /* Replace a rank operator with a single ascending order
               criterion by a projection that links the output column
               to the order criterion. */
            if (PFord_count (p->sem.sort.sortby) == 1 &&
                PFord_order_dir_at (p->sem.sort.sortby, 0) == DIR_ASC) {
                PFalg_proj_t *proj_list;
                unsigned int count = 0;

                /* create projection list */
                proj_list = PFmalloc (p->schema.count *
                                      sizeof (*(proj_list)));

                /* adjust column name of the rank operator */
                proj_list[count++] = PFalg_proj (
                                         p->sem.sort.res,
                                         PFord_order_col_at (
                                             p->sem.sort.sortby,
                                             0));

                for (unsigned int i = 0; i < p->schema.count; i++)
                    if (p->schema.items[i].name != p->sem.sort.res)
                        proj_list[count++] = PFalg_proj (
                                                 p->schema.items[i].name,
                                                 p->schema.items[i].name);

                *p = *PFla_project_ (L(p), count, proj_list);
                SEEN(p) = false;
                break;
            }

            /* check for a projection */
            if (L(p)->kind == la_project) {
                proj = true;
                for (i = 0; i < L(p)->sem.proj.count; i++)
                    renamed = renamed || (L(p)->sem.proj.items[i].new !=
                                          L(p)->sem.proj.items[i].old);
                rank = LL(p);
            }
            else
                rank = L(p);

            /* don't handle patterns with renaming projections */
            if (renamed) break;

            /* check the remaining part of the pattern (nested rank)
               and ensure that the column generated by the nested
               row number operator is not used above the outer rank. */
            if (rank->kind == la_rank &&
                !PFprop_icol (p->prop, rank->sem.sort.res)) {

                PFord_ordering_t sortby;
                PFalg_proj_t *proj_list;
                PFalg_col_t inner_col = rank->sem.sort.res;
                unsigned int pos_col = 0, count = 0;

                /* lookup position of the inner rank column in
                   the list of sort criteria of the outer rank */
                for (i = 0; i < PFord_count (p->sem.sort.sortby); i++)
                    if (PFord_order_col_at (p->sem.sort.sortby, i)
                            == inner_col &&
                        /* make sure the order is the same */
                        PFord_order_dir_at (p->sem.sort.sortby, i)
                            == DIR_ASC) {
                        pos_col = i;
                        break;
                    }

                /* inner rank column is not used in the outer rank
                   (thus the inner rank is probably superfluous
                    -- let the icols optimization remove the operator) */
                if (i == PFord_count (p->sem.sort.sortby)) break;

                sortby = PFordering ();

                /* create new sort list where the sort criteria of the
                   inner rank substitute the inner rank column */
                for (i = 0; i < pos_col; i++)
                    sortby = PFord_refine (sortby,
                                           PFord_order_col_at (
                                               p->sem.sort.sortby,
                                               i),
                                           PFord_order_dir_at (
                                               p->sem.sort.sortby,
                                               i));

                for (i = 0; i < PFord_count (rank->sem.sort.sortby); i++)
                    sortby = PFord_refine (sortby,
                                           PFord_order_col_at (
                                               rank->sem.sort.sortby,
                                               i),
                                           PFord_order_dir_at (
                                               rank->sem.sort.sortby,
                                               i));

                for (i = pos_col + 1;
                     i < PFord_count (p->sem.sort.sortby);
                     i++)
                    sortby = PFord_refine (sortby,
                                           PFord_order_col_at (
                                               p->sem.sort.sortby,
                                               i),
                                           PFord_order_dir_at (
                                               p->sem.sort.sortby,
                                               i));

                if (proj) {
                    /* Introduce the projection above the new rank
                       operator to maintain the correct result schema.
                       As the result column name of the old outer rank
                       may collide with the column name of one of the
                       inner ranks sort criteria, we use the column name
                       of the inner rank as resulting column name
                       and adjust the name in the new projection. */

                    count = 0;

                    /* create projection list */
                    proj_list = PFmalloc (p->schema.count *
                                          sizeof (*(proj_list)));

                    /* adjust column name of the rank operator */
                    proj_list[count++] = PFalg_proj (
                                             p->sem.sort.res,
                                             rank->sem.sort.res);

                    for (i = 0; i < p->schema.count; i++)
                        if (p->schema.items[i].name !=
                            p->sem.sort.res)
                            proj_list[count++] = PFalg_proj (
                                                     p->schema.items[i].name,
                                                     p->schema.items[i].name);

                    *p = *PFla_project_ (PFla_rank (L(rank),
                                                    rank->sem.sort.res,
                                                    sortby),
                                         count, proj_list);
                }
                else
                    *p = *PFla_rank (rank, p->sem.sort.res, sortby);

                break;
            }

        }   break;

        case la_rowid:
            if (PFprop_card (p->prop) == 1) {
                *p = *PFla_attach (L(p), p->sem.rowid.res, PFalg_lit_nat (1));
            }
            /* Get rid of a rowid operator that is only used to maintain
               the correct cardinality or the correct order (in an unordered
               scenario). In case other columns provide a compound key we
               replace the rowid operator by a rank operator consuming the
               compound key. */
            else if (PFprop_req_unique_col (p->prop, p->sem.rowid.res) ||
                     PFprop_req_order_col (p->prop, p->sem.rowid.res)) {
                PFalg_collist_t *collist;
                unsigned int     count = PFprop_ckeys_count (p->prop),
                                 i,
                                 j;
                for (i = 0; i < count; i++) {
                    collist = PFprop_ckey_at (p->prop, i);
                    /* make sure the result is not part of the compound key
                       and all columns of the new key are already used
                       elsewhere. */
                    for (j = 0; j < clsize (collist); j++)
                        if (clat (collist, j) == p->sem.rowid.res ||
                            !PFprop_icol (p->prop, clat (collist, j)))
                            break;
                    if (j == clsize (collist)) {
                        PFord_ordering_t sortby = PFordering ();
                        for (j = 0; j < clsize (collist); j++)
                            sortby = PFord_refine (sortby,
                                                   clat (collist, j),
                                                   DIR_ASC);
                        *p = *PFla_rank (L(p), p->sem.rowid.res, sortby);
                        break;
                    }
                }
            }
            break;

        case la_cast:
            if (PFprop_req_order_col (p->prop, p->sem.type.res) &&
                p->sem.type.ty == aat_int &&
                PFprop_type_of (p, p->sem.type.col) == aat_nat) {
                PFalg_proj_t *proj = PFmalloc (p->schema.count *
                                               sizeof (PFalg_proj_t));
                for (unsigned int i = 0; i < L(p)->schema.count; i++)
                    proj[i] = PFalg_proj (L(p)->schema.items[i].name,
                                          L(p)->schema.items[i].name);
                proj[L(p)->schema.count] = PFalg_proj (p->sem.type.res,
                                                       p->sem.type.col);
                *p = *PFla_project_ (L(p), p->schema.count, proj);
            }
            break;

/* ineffective without step operators */
        case la_step:
            if (!LEVEL_KNOWN(p->sem.step.level))
                p->sem.step.level = PFprop_level (p->prop,
                                                  p->sem.step.item_res);

            if ((p->sem.step.spec.axis == alg_desc ||
                 p->sem.step.spec.axis == alg_desc_s) &&
                p->sem.step.level >= 1 &&
                p->sem.step.level - 1 == PFprop_level (R(p)->prop,
                                                       p->sem.step.item))
                p->sem.step.spec.axis = alg_chld;

            if (R(p)->kind == la_project &&
                RL(p)->kind == la_step) {
                if ((p->sem.step.item ==
                     R(p)->sem.proj.items[0].new &&
                     RL(p)->sem.step.item_res ==
                     R(p)->sem.proj.items[0].old &&
                     p->sem.step.iter ==
                     R(p)->sem.proj.items[1].new &&
                     RL(p)->sem.step.iter ==
                     R(p)->sem.proj.items[1].old) ||
                    (p->sem.step.item ==
                     R(p)->sem.proj.items[1].new &&
                     RL(p)->sem.step.item_res ==
                     R(p)->sem.proj.items[1].old &&
                     p->sem.step.iter ==
                     R(p)->sem.proj.items[0].new &&
                     RL(p)->sem.step.iter ==
                     R(p)->sem.proj.items[0].old))
                    *p = *PFla_project (PFla_step (
                                            L(p),
                                            RL(p),
                                            p->sem.step.spec,
                                            p->sem.step.level,
                                            RL(p)->sem.step.iter,
                                            RL(p)->sem.step.item,
                                            RL(p)->sem.step.item_res),
                                        R(p)->sem.proj.items[0],
                                        R(p)->sem.proj.items[1]);
                break;
            }
            else if (R(p)->kind == la_rowid &&
                     p->sem.step.spec.axis == alg_chld &&
                     p->sem.step.iter == R(p)->sem.rowid.res &&
                     !PFprop_icol (p->prop, p->sem.step.iter) &&
                     PFprop_key (p->prop, p->sem.step.item)) {
                R(p) = PFla_attach (RL(p),
                                    R(p)->sem.rowid.res,
                                    PFalg_lit_nat (1));
                break;
            }
            break;
/* end of: ineffective without step operators */

        case la_guide_step:
        case la_guide_step_join:
            if (!LEVEL_KNOWN(p->sem.step.level) && p->sem.step.guide_count) {
                int level = p->sem.step.guides[0]->level;
                for (unsigned int i = 1; i < p->sem.step.guide_count; i++)
                    if (level != p->sem.step.guides[i]->level)
                        break;
                p->sem.step.level = level;
            }

            if ((p->sem.step.spec.axis == alg_desc ||
                 p->sem.step.spec.axis == alg_desc_s) &&
                p->sem.step.level >= 1 &&
                p->sem.step.level - 1 == PFprop_level (R(p)->prop,
                                                       p->sem.step.item))
                p->sem.step.spec.axis = alg_chld;
            break;

        case la_step_join:
            if (!LEVEL_KNOWN(p->sem.step.level))
                p->sem.step.level = PFprop_level (p->prop,
                                                  p->sem.step.item_res);

            if ((p->sem.step.spec.axis == alg_desc ||
                 p->sem.step.spec.axis == alg_desc_s) &&
                p->sem.step.level >= 1 &&
                p->sem.step.level - 1 == PFprop_level (R(p)->prop,
                                                       p->sem.step.item))
                p->sem.step.spec.axis = alg_chld;

            /* combine steps if they are of the form:
               ``/descandent-or-self::node()/child::element()'' */
            if (R(p)->kind == la_project &&
                RL(p)->kind == la_step_join &&
                /* check for the different step combinations */
                ((/* descendant-or-self::node()/child:: */
                  p->sem.step.spec.axis == alg_chld &&
                  RL(p)->sem.step.spec.axis == alg_desc_s) ||
                 (/* child::node()/descendant-or-self:: */
                  p->sem.step.spec.axis == alg_desc_s &&
                  RL(p)->sem.step.spec.axis == alg_chld) ||
                 (/* coll_node/descendant::node()/child:: */
                  /* Works correctly as the collection node is not
                     reachable in the query and thus descendant behaves
                     like a descendant-or-self step starting from a
                     document node. */
                  p->sem.step.spec.axis == alg_chld &&
                  PFprop_level (RL(p)->prop, RL(p)->sem.step.item) == -1 &&
                  RL(p)->sem.step.spec.axis == alg_desc &&
                  /* check for node kind to avoid that this pattern
                     is triggered multiple times */
                  p->sem.step.spec.kind != node_kind_node) ||
                 (/* coll_node/child::node()/descendant:: */
                  /* Works correctly as the descendant step filter
                     discards the document nodes. */
                  p->sem.step.spec.axis == alg_desc &&
                  PFprop_level (RL(p)->prop, RL(p)->sem.step.item) == -1 &&
                  RL(p)->sem.step.spec.axis == alg_chld &&
                  p->sem.step.spec.kind != node_kind_node &&
                  p->sem.step.spec.kind != node_kind_doc)) &&
                RL(p)->sem.step.spec.kind == node_kind_node &&
                !PFprop_icol (p->prop, p->sem.step.item) &&
                (PFprop_set (p->prop) ||
                 PFprop_ckey (p->prop, p->schema) ||
                 PFprop_key (p->prop, p->sem.step.item_res))) {

                bool          item_link_correct = false;
                PFalg_col_t   item_res          = p->sem.step.item_res,
                              item_in           = p->sem.step.item,
                              old_item_res      = RL(p)->sem.step.item_res,
                              old_item_in       = RL(p)->sem.step.item;
                PFalg_proj_t *proj = PFmalloc (R(p)->schema.count *
                                               sizeof (PFalg_proj_t));

                for (unsigned int i = 0; i < R(p)->sem.proj.count; i++) {
                    PFalg_proj_t proj_item = R(p)->sem.proj.items[i];

                    if (proj_item.new == item_in &&
                        proj_item.old == old_item_res) {
                        item_link_correct = true;
                        proj[i] = PFalg_proj (item_in, old_item_in);
                    }
                    else if (proj_item.old == old_item_in)
                        /* the old input item may not appear in the result */
                        break;
                    else
                        proj[i] = proj_item;
                }
                if (item_link_correct) {
                    PFalg_step_spec_t spec = p->sem.step.spec;
                    spec.axis = alg_desc,
                    /* rewrite child into descendant
                       and discard descendant-or-self step */
                    *p = *PFla_step_join_simple (
                              L(p),
                              PFla_project_ (RLR(p), R(p)->schema.count, proj),
                              spec, item_in, item_res);
                    break;
                }
            }
            break;

        case la_element:
        {
            PFla_op_t *fcns = R(p);

            /* Traverse all children and try to merge more nodes into the twig
               (as long as the input provides for every iteration a node). */
            while (fcns->kind == la_fcns) {
                /**
                 * match the following pattern
                 *              _ _ _ _ _
                 *            |          \
                 *         content        |
                 *         /     \
                 *     frag_U    attach   |
                 *     /   \       |
                 *  empty  frag  roots    |
                 *  frag      \   /
                 *            twig        |
                 *              | _ _ _ _/
                 *
                 * and throw it away ( - - - )
                 */
                if (L(fcns)->kind == la_content &&
                    /* Make sure that all iterations of the parent
                       are present (no subdomain relationship). */
                    (PFprop_subdom (
                         p->prop,
                         PFprop_dom (p->prop,
                                     p->sem.iter_item.iter),
                         PFprop_dom (L(fcns)->prop,
                                     L(fcns)->sem.iter_pos_item.iter)) ||
                     (PFprop_card (p->prop) == 1 &&
                      PFprop_card (L(fcns)->prop) == 1 &&
                      PFprop_const (p->prop, p->sem.iter_item.iter) &&
                      PFprop_const (L(fcns)->prop,
                                    L(fcns)->sem.iter_pos_item.iter) &&
                      PFalg_atom_comparable (
                          PFprop_const_val (p->prop, p->sem.iter_item.iter),
                          PFprop_const_val (L(fcns)->prop,
                                            L(fcns)->sem.iter_pos_item.iter)) &&
                      !PFalg_atom_cmp (
                          PFprop_const_val (p->prop, p->sem.iter_item.iter),
                          PFprop_const_val (L(fcns)->prop,
                                            L(fcns)->sem.iter_pos_item.iter)))) &&
                    LR(fcns)->kind == la_attach &&
                    LRL(fcns)->kind == la_roots &&
                    LRLL(fcns)->kind == la_twig &&
                    LL(fcns)->kind == la_frag_union &&
                    LLL(fcns)->kind == la_empty_frag &&
                    LLR(fcns)->kind == la_fragment &&
                    LLRL(fcns) == LRLL(fcns) &&
                    /* input columns match the output
                       columns of the underlying twig */
                    L(fcns)->sem.iter_pos_item.iter ==
                    LRLL(fcns)->sem.iter_item.iter &&
                    L(fcns)->sem.iter_pos_item.item ==
                    LRLL(fcns)->sem.iter_item.item &&
                    /* input twig is referenced only once */
                    PFprop_refctr (LR(fcns)) == 1 &&
                    PFprop_refctr (LRL(fcns)) == 1) {
                    L(fcns) = L(LRLL(fcns));
                }
                fcns = R(fcns);
            }
        }   break;

        case la_string_join:
            if (PFprop_key_left (p->prop, p->sem.string_join.iter) &&
                PFprop_subdom (p->prop,
                               PFprop_dom_right (p->prop,
                                                 p->sem.string_join.iter_sep),
                               PFprop_dom_left (p->prop,
                                                p->sem.string_join.iter))) {
                *p = *PFla_project (L(p),
                                    PFalg_proj (p->sem.string_join.iter_res,
                                                p->sem.string_join.iter),
                                    PFalg_proj (p->sem.string_join.item_res,
                                                p->sem.string_join.item));
                break;
            }
            break;

        case la_roots:
            if (L(p)->kind == la_merge_adjacent &&
                PFprop_key_right (L(p)->prop,
                                  L(p)->sem.merge_adjacent.iter_in)) {
                *p = *PFla_project (
                          LR(p),
                          PFalg_proj (L(p)->sem.merge_adjacent.iter_res,
                                      L(p)->sem.merge_adjacent.iter_in),
                          PFalg_proj (L(p)->sem.merge_adjacent.pos_res,
                                      L(p)->sem.merge_adjacent.pos_in),
                          PFalg_proj (L(p)->sem.merge_adjacent.item_res,
                                      L(p)->sem.merge_adjacent.item_in));
                break;
            }
            break;

        case la_frag_union:
            if (L(p)->kind == la_fragment &&
                LL(p)->kind == la_merge_adjacent &&
                PFprop_key_right (LL(p)->prop,
                                  LL(p)->sem.merge_adjacent.iter_in))
                *p = *PFla_dummy (R(p));
            else if (R(p)->kind == la_fragment &&
                RL(p)->kind == la_merge_adjacent &&
                PFprop_key_right (RL(p)->prop,
                                  RL(p)->sem.merge_adjacent.iter_in))
                *p = *PFla_dummy (L(p));
            /* remove unreferenced twig constructors */
            else if (L(p)->kind == la_fragment &&
                     LL(p)->kind == la_twig &&
                     PFprop_refctr (LL(p)) == 1)
                *p = *PFla_dummy (R(p));
            else if (R(p)->kind == la_fragment &&
                     RL(p)->kind == la_twig &&
                     PFprop_refctr (RL(p)) == 1)
                *p = *PFla_dummy (L(p));
            break;

        default:
            break;
    }
}

/**
 * Invoke algebra optimization.
 */
PFla_op_t *
PFalgopt_complex (PFla_op_t *root)
{
    /* Infer key, icols, domain, reqval,
       and refctr properties first */
    PFprop_infer_key (root);
    PFprop_infer_level (root);
    PFprop_infer_composite_key (root);
    PFprop_infer_icol (root);
    PFprop_infer_set (root);
    PFprop_infer_reqval (root);
    PFprop_infer_dom (root);
    PFprop_infer_refctr (root);

    /* Optimize algebra tree */
    opt_complex (root);
    PFla_dag_reset (root);

    /* In addition optimize the resulting DAG using the icols property
       to remove inconsistencies introduced by changing the types
       of unreferenced columns (rule eqjoin). The icols optimization
       will ensure that these columns are 'really' never used. */
    root = PFalgopt_icol (root);

    return root;
}

/* vim:set shiftwidth=4 expandtab filetype=c: */
/* vim:set foldmarker=#if,#endif foldmethod=marker foldopen-=search: */

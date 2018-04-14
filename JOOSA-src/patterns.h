/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

#include <stdio.h>
#include <string.h>

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */
int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k) && 
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}

/* dup
 * istore x
 * pop
 * -------->
 * istore x
 */
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEistore(x,NULL));
  }
  return 0;
}

/* dup
 * putfield
 * pop
 * -------->
 * putfield
 */
int simplify_putfield(CODE **c)
{ char *f;
    if (is_dup(*c) &&
        is_putfield(next(*c), &f) &&
        is_pop(next(next(*c)))) {
        return replace(c, 3, makeCODEputfield(f, NULL));
    }
    return 0;
}


/* iload x
 * ldc k   (0<k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */ 
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      (x==y) && (0<k) && (k<=127)) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0>k>=-127)
 * isub
 * istore x
 * --------->
 * iinc x k
 */ 
int negative_increment(CODE **c)
{
    int x,y,k;
    if (is_iload(*c,&x) &&
            is_ldc_int(next(*c),&k) &&
            is_isub(next(next(*c))) &&
            is_istore(next(next(next(*c))),&y) &&
            (x==y) && (0<k) && (k<=127)) {
        return replace(c,4,makeCODEiinc(x,-k,NULL));
    }
    return 0;
}

/* iload x
 * ldc 0
 * isub | iadd
 * istore
 * ------------>
 */
int zero_sum(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_isub(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      (x==y) && (k==0)) {
     return replace(c, 4, NULL);
  }
  else if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      (x==y) && (k==0)) {
     return replace(c, 4, NULL);
  }
  return 0;
}


/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)  
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}

/* if_cmplt L1
 * iconst_0
 * goto L2
 * L1: (unique)
 * iconst_1
 * L2: (unique)
 * ifeq L3
 * ...
 * L3
 * ----------->
 * if_cmpge L3
 * ...
 * L3: (reference count unchanged)
 */
int simplify_branch_single_cond(CODE **c)
{
    int l1_t, l1_f, l2_t, l2_f, l3, k;
    if (is_if(c, &l1_t))
    {
        if (is_ldc_int(nextby(*c, 1), &k) &&
                is_goto(nextby(*c, 2), &l2_t) && 
                is_label(nextby(*c, 3), &l1_f) && 
                uniquelabel(l1_f) &&
                is_ldc_int(nextby(*c, 1), &k) &&
                is_label(nextby(*c, 5), &l2_f) && 
                uniquelabel(l2_f) &&
                is_ifeq(nextby(*c, 6), &l3))
        {
            if ((l2_t == l2_f) &&
                (l1_t == l1_f) &&
                l3 < l2_t && l2_t > l1_t)
            {
                if (is_if_icmplt(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmpge(l3, NULL));
                }
                else if (is_if_icmple(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmpgt(l3, NULL));
                }
                else if (is_if_icmpgt(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmple(l3, NULL));
                }
                else if (is_if_icmpge(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmplt(l3, NULL));
                }
                else if (is_ifeq(*c, &l1_t))
                {
                    return replace(c,7,makeCODEifne(l3, NULL));
                }
                else if (is_ifne(*c, &l1_t))
                {
                    return replace(c,7,makeCODEifeq(l3, NULL));
                }
                else if (is_if_icmpeq(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmpne(l3, NULL));
                }
                else if (is_if_icmpne(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_icmpeq(l3, NULL));
                }
                else if (is_if_acmpeq(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_acmpne(l3, NULL));
                }
                else if (is_if_acmpne(*c, &l1_t))
                {
                    return replace(c,7,makeCODEif_acmpeq(l3, NULL));
                }
                else if (is_ifnull(*c, &l1_t))
                {
                    return replace(c,7,makeCODEifnonnull(l3, NULL));
                }
                else if (is_ifnonnull(*c, &l1_t))
                {
                    return replace(c,7,makeCODEifnull(l3, NULL));
                }
            }
        }
    }
    return 0;
}

/* ldc c
 * dup
 * aload x
 * swap
 * ----------->
 * aload x
 * ldc c
 * dup
 */
int simplify_load_swap(CODE **c)
{
    int x, k;
    char *s;
    if (is_ldc_int(*c, &k) && 
            is_dup(next(*c)) &&
            is_aload(next(next(*c)), &x) &&
            is_swap(next(next(next(*c)))))
    {
        return replace(c, 4, makeCODEaload(x, makeCODEldc_int(k, makeCODEdup(NULL))));
    }
    else if (is_ldc_string(*c, &s) && 
            is_dup(next(*c)) &&
            is_aload(next(next(*c)), &x) &&
            is_swap(next(next(next(*c)))))
    {
        return replace(c, 4, makeCODEaload(x,
                             makeCODEldc_string(s,
                             makeCODEdup(NULL))));
    }
    return 0;
}

/* goto label1
 * ...
 * label1:
 * label2:
 * ------------->
 * goto label2
 * ...
 * label1: (reference count reduced by 1)
 * label2: (reference count icreased by 1)
 */
int simplify_label_label(CODE **c)
{ int l1, l2;
    if (is_goto(*c, &l1) && is_label(next(destination(l1)), &l2))
    {
        droplabel(l1);
        copylabel(l2);
        return replace(c, 1, makeCODEgoto(l2, NULL));
    }
    else if (is_ifeq(*c, &l1) && is_label(next(destination(l1)), &l2))
    {
        droplabel(l1);
        copylabel(l2);
        return replace(c, 1, makeCODEifeq(l2, NULL));
    }
    else if (is_ifne(*c, &l1) && is_label(next(destination(l1)), &l2))
    {
        droplabel(l1);
        copylabel(l2);
        return replace(c, 1, makeCODEifne(l2, NULL));
    }
    return 0;
}

/* label: (reference count == 0)
 * -------------->
 */
int remove_deadlabel(CODE **c)
{ int l;
    if (is_label(*c, &l) && deadlabel(l))
    {
        return kill_line(c);
    }
    return 0;
}

/* ldc string_literal (ie: "string")
 * dup
 * ifnull label1
 * goto label2
 * label1: (unique label)
 * pop
 * ldc "null" (unique label)
 * label2:
 * --------->
 * ldc string_literal
 */
int simplify_string(CODE **c)
{
    int l1, l2;
    char *s, *n; 
    if (is_ldc_string(*c, &s) &&
        is_dup(nextby(*c, 1)) &&
        is_ifnull(nextby(*c, 2), &l1) &&
        is_goto(nextby(*c, 3), &l2) &&
        is_label(nextby(*c, 4), &l1) &&
        uniquelabel(l1) &&
        is_pop(nextby(*c, 5)) &&
        is_ldc_string(nextby(*c, 6), &n) &&
        is_label(nextby(*c, 7), &l2) &&
        uniquelabel(l2) &&
        (s != NULL) && (*s != '\0') &&
        (strcmp(n, "null") == 0))
    {
        return replace(c, 8, makeCODEldc_string(s, NULL));
    }
    return 0;
}

/* invokevirtual (returns a string or the "null" string)
 * dup
 * ifnull label1
 * goto label2
 * label1: (unique)
 * pop
 * ldc "null" (unique label)
 * label2:
 * --------->
 * ldc string_literal
 */
int simplify_concatenate_string(CODE **c)
{
    int l1, l2;
    char *s, *n;
    if (is_invokevirtual(*c, &s) &&
        is_dup(nextby(*c, 1)) &&
        is_ifnull(nextby(*c, 2), &l1) &&
        is_goto(nextby(*c, 3), &l2) &&
        is_label(nextby(*c, 4), &l1) &&
        uniquelabel(l1) &&
        is_pop(nextby(*c, 5)) &&
        is_ldc_string(nextby(*c, 6), &n) &&
        is_label(nextby(*c, 7), &l2) &&
        uniquelabel(l2) &&
        (strcmp(s, "java/lang/String/concat(Ljava/lang/String;)Ljava/lang/String;") == 0) &&
        (strcmp(n, "null") == 0))
    {
        return replace(c, 8, makeCODEldc_string(s, NULL));
    }
    return 0;
}

/* dup
 * aload x
 * swap
 * pulfield s
 * pop
 * ----------->
 * aload x
 * swap
 * putfield s
 */
int simplify_new_objects(CODE **c)
{
    int k;
    char *s;
    if (is_dup(*c) &&
        is_aload(next(*c), &k) &&
        is_swap(nextby(*c, 2)) &&
        is_putfield(nextby(*c, 3), &s) &&
        is_pop(nextby(*c, 4)))
    {
        return replace(c, 5, makeCODEaload(k, makeCODEswap(makeCODEputfield(s, NULL))));
    }
    return 0;
}

void init_patterns(void) {
    ADD_PATTERN(simplify_multiplication_right);
    ADD_PATTERN(simplify_astore);
    ADD_PATTERN(positive_increment);
    ADD_PATTERN(simplify_goto_goto);

    ADD_PATTERN(negative_increment);
    ADD_PATTERN(zero_sum);
    ADD_PATTERN(simplify_istore);
    ADD_PATTERN(simplify_branch_single_cond);
    ADD_PATTERN(simplify_load_swap);
    ADD_PATTERN(simplify_putfield);
    ADD_PATTERN(simplify_label_label);
    ADD_PATTERN(simplify_string);
    ADD_PATTERN(simplify_concatenate_string);
    ADD_PATTERN(simplify_new_objects);

    ADD_PATTERN(remove_deadlabel);
}

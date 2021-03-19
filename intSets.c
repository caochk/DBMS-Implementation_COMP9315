/*
 * src/tutorial/intSet.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "access/hash.h"
#include "utils/builtins.h"
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <ctype.h>


PG_MODULE_MAGIC;

#define TRUE 1
#define FALSE 0

typedef struct intSet
{
    char v_len[VARHDRSZ];
    int lengthOfIntSetSting;
    int numOfIntegers;
    int iset[1];
} intSet;


static int cmp_num (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

static int duplicate_removal (int *array, int numOfIntegers){
    for(int i = 0; i < numOfIntegers; i++)
    {
        int ele = array[i];
        for(int j = i + 1; j < numOfIntegers; j++)
        {
            if(ele == array[j])
            {
                for(int ele = j; ele < numOfIntegers-1; ele++)
                {
                    array[ele] = array[ele + 1];
                }
                j--;
                numOfIntegers--;
            }
        }
    }
    return numOfIntegers;
}

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
    char	  *str = PG_GETARG_CSTRING(0);
    intSet   *result;
    int lengthOfIntSetsString = 0;
    char *intSetString = NULL;

    int j = 0;
    int count = 0;
    int numOfIntegers = 0;

    int m = 0;
    int n = 0;
    char *element = NULL;
    char *remaingElements = NULL;

    int i = 1;

    int *resultTmp;


    //calculate numOfIntegers
    while (str[j] != '\0'){
        while (isdigit(str[j])){
            count = 1;
            j ++;
        }
        if (count == 1){
            numOfIntegers++;
        }
        count = 0;
        j++;
    }
    elog(NOTICE,"numOfIntegers:%d\n",numOfIntegers);

    //calculate lengthOfIntSetsString
    j = 0;
    while (str[j] != '\0'){
        if (str[j] != ' '){
            lengthOfIntSetsString ++;
        }
        j ++;
    }

    elog(NOTICE,"lengthOfIntString:%d\n",lengthOfIntSetsString);

    intSetString = (char *)palloc((lengthOfIntSetsString+1)*sizeof(char));//测试过，这个内存分配应该是对的

    //remove spaces
    intSetString[lengthOfIntSetsString] = '\0';
    while (str[m] != '\0'){
        if (str[m] != ' '){
            intSetString[n] = str[m];
            n++;
        }
        m++;
    }

    //remove left bracket and right bracket
    for(m=n=0;intSetString[m]!='\0';m++){
        if(intSetString[m] != '{'){
            intSetString[n++] = intSetString[m];
        }
    }
    intSetString[n]='\0';
    for(m=n=0;intSetString[m]!='\0';m++){
        if(intSetString[m] != '}'){
            intSetString[n++] = intSetString[m];
        }
    }
    intSetString[n]='\0';



    if (numOfIntegers == 0){
        resultTmp = (int *) palloc(1*sizeof(int));
        resultTmp[0] = 0;

        result = (intSet*) palloc(VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultTmp, 1*sizeof(int));
    }
    else{
        resultTmp = (int *) palloc(numOfIntegers*sizeof(int));
        if (numOfIntegers > 0){
            element = strtok_r(intSetString, ",", &remaingElements);
            resultTmp[0] = atoi(element);
            while(element != NULL) {
                element = strtok_r(NULL, ",", &remaingElements);
                if (element != NULL) {
                    resultTmp[i] = atoi(element);
                    i++;
                }
            }
        }
        numOfIntegers = duplicate_removal(resultTmp, numOfIntegers);
        qsort(resultTmp, numOfIntegers, sizeof(int), cmp_num);

        result = (intSet*) palloc(VARHDRSZ + numOfIntegers*sizeof(int)+sizeof(int)+sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + numOfIntegers*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultTmp, numOfIntegers*sizeof(int));
    }
    result->lengthOfIntSetSting = lengthOfIntSetsString;
    result->numOfIntegers = numOfIntegers;

    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
    char *result;
    char *element;
    char    comma[2] = {','};
    char    leftBrace[2] = {'{'};
    char    rightBrace[2] = {'}'};
    char *empty_str = "{}";

    intSet   *intsets = (intSet *)PG_GETARG_POINTER(0);

    result = (char *) palloc((intsets->lengthOfIntSetSting + 1)*sizeof(char));

    element = (char *) palloc((intsets->lengthOfIntSetSting + 1)*sizeof(char));

    elog(NOTICE,"lengthOfIntSetSting:%d\n", intsets->lengthOfIntSetSting);
    elog(NOTICE,"numOfIntegers:%d\n", intsets->numOfIntegers);

    if (intsets->numOfIntegers == 0){
        PG_RETURN_CSTRING(empty_str);
    }
    else{
        result[0] = '\0';
        strcpy(result, leftBrace);
        for (int i = 0; i < intsets->numOfIntegers; ++i) {
            pg_ltoa(intsets->iset[i], element);
            strcat(result, element);
            if (i != intsets->numOfIntegers-1){
                strcat(result, comma);
            }
        }
        strcat(result, rightBrace);
        PG_RETURN_CSTRING(psprintf("%s", result));
    }
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/


/*****************************************************************************
 * New Operators
 *
 * A practical Complex datatype would provide much more than this, of course.
 *****************************************************************************/
static int intset_inclusion(intSet *A, intSet *B){
    int result = FALSE;
    int count = 0;

    for (int i = 0; i < B->numOfIntegers; ++i) {
        for (int j = 0; j < A->numOfIntegers; ++j) {
            if (B->iset[i] == A->iset[j]){
                count++;
                break;
            }
        }
    }
    if (count==B->numOfIntegers){
        result = TRUE;
    }
    return result;
}

static int num_of_same_elements(intSet *A, intSet *B){
    int numOfSameElements = 0;

    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->iset[i] == B->iset[j]){
                numOfSameElements++;
            }
        }
    }
    return numOfSameElements;
}

static int *sets_intersection(intSet *A, intSet *B, int *resultSet){
    int k = 0;

    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->iset[i] == B->iset[j]){
                resultSet[k] = A->iset[i];
                k++;
            }
        }
    }
    return resultSet;
}


static int *sets_difference(intSet *A, intSet *B, int *resultSet){
    int count = 0;
    int k = 0;

    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->iset[i] != B->iset[j]){
                count++;
            }
        }
        if (count == B->numOfIntegers){
            resultSet[k] = A->iset[i];
            k ++;
        }
        count = 0;
    }
    return resultSet;
}

// Operator: ?
PG_FUNCTION_INFO_V1(intset_contain);

Datum
intset_contain(PG_FUNCTION_ARGS)
{
    int        i = PG_GETARG_INT32(0);
    intSet   *S = (intSet *) PG_GETARG_POINTER(1);

    int result = FALSE;

    for (int j = 0; j < S->numOfIntegers; ++j) {
        if (i == S->iset[j]){
            result = TRUE;
        }
    }
    PG_RETURN_BOOL(result==TRUE);
}

//Operator: #
PG_FUNCTION_INFO_V1(intset_cardinality);

Datum
intset_cardinality(PG_FUNCTION_ARGS)
{
    intSet   *S = (intSet *) PG_GETARG_POINTER(0);

    PG_RETURN_INT32(S->numOfIntegers);
}

//Operator: >@
PG_FUNCTION_INFO_V1(intset_improper_superset);

Datum
intset_improper_superset(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(intset_inclusion(A, B)==TRUE);
}

//Operator: @<
PG_FUNCTION_INFO_V1(intset_improper_subset);

Datum
intset_improper_subset(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(intset_inclusion(B, A)==TRUE);
}

//Operator: =
PG_FUNCTION_INFO_V1(intset_eq);

Datum
intset_eq(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);

    int result1 = intset_inclusion(A, B);
    int result2 = intset_inclusion(B, A);
    int result = FALSE;
    if (result1 == TRUE && result2 == TRUE){
        result = TRUE;
    }
    PG_RETURN_BOOL(result==TRUE);
}

//Operator: <>
PG_FUNCTION_INFO_V1(intset_not_eq);

Datum
intset_not_eq(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);

    int result1 = intset_inclusion(A, B);
    int result2 = intset_inclusion(B, A);
    int result = FALSE;
    if (result1 == FALSE || result2 == FALSE){
        result = TRUE;
    }
    PG_RETURN_BOOL(result==TRUE);
}

//Operator: &&
PG_FUNCTION_INFO_V1(intset_intersection);

Datum
intset_intersection(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet   *result;
    int numOfSameElements;
    int *resultSet;

    numOfSameElements = num_of_same_elements(A, B);

    if (numOfSameElements == 0){
        resultSet = (int *) palloc(1 * sizeof(int));//
        resultSet[0] = 0;

        result = (intSet *)palloc(VARHDRSZ + 1*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultSet, 1*sizeof(int));
    }
    else{
        resultSet = (int *) palloc(numOfSameElements * sizeof(int));//

        result = (intSet *)palloc(VARHDRSZ + numOfSameElements*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + numOfSameElements*sizeof(int)+sizeof(int)+sizeof(int));

        sets_intersection(A, B, resultSet);
        memcpy(result->iset, resultSet, numOfSameElements*sizeof(int));
    }

    result->numOfIntegers = numOfSameElements;

    if (numOfSameElements > 0){
        result->lengthOfIntSetSting = numOfSameElements + numOfSameElements + 1;
    }
    else{
        result->lengthOfIntSetSting = 2;
    }

    PG_RETURN_POINTER(result);
}

//Operator: ||
PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet   *result;
    int numOfSameElements = 0;
    int lengthOfDifferenceSet = 0;
    int *differenceSet;
    int numOfResultSet = 0;
    int *resultSet;

    numOfSameElements = num_of_same_elements(A, B);

    lengthOfDifferenceSet = A->numOfIntegers - numOfSameElements;
    numOfResultSet = A->numOfIntegers - numOfSameElements + B->numOfIntegers;

    if (numOfResultSet == 0){
        resultSet = (int*) palloc(1*sizeof(int));
        resultSet[0] = 0;

        result = (intSet *)palloc(VARHDRSZ + 1*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultSet, 1*sizeof(int));
    }
    else{
        differenceSet = (int *) palloc(lengthOfDifferenceSet*sizeof(int));
        sets_difference(A, B, differenceSet);

        resultSet = (int*) palloc(numOfResultSet*sizeof(int));
        for (int i = 0; i < B->numOfIntegers; ++i) {
            resultSet[i] = B->iset[i];
        }
        for (int i = 0; i < lengthOfDifferenceSet; ++i) {
            resultSet[B->numOfIntegers+i] = differenceSet[i];
        }
        qsort(resultSet, numOfResultSet, sizeof(int), cmp_num);

        result = (intSet *) palloc(VARHDRSZ + numOfResultSet*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + numOfResultSet*sizeof(int)+sizeof(int)+sizeof(int));
        memcpy(result->iset, resultSet, numOfResultSet*sizeof(int));
    }

    result->numOfIntegers = numOfResultSet;
    if (numOfResultSet > 0){
        result->lengthOfIntSetSting = numOfResultSet + numOfResultSet + 1;
    }
    else{
        result->lengthOfIntSetSting = 2;
    }

    PG_RETURN_POINTER(result);
}

//Operator: ！！
PG_FUNCTION_INFO_V1(intset_disjunction);

Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet  *result;
    int numOfSameElements;
    int numOfResultSet1;
    int numOfResultSet2;
    int *differenceSet1;
    int *differenceSet2;
    int *resultSet;

    numOfSameElements = num_of_same_elements(A, B);

    numOfResultSet1 = A->numOfIntegers - numOfSameElements;

    numOfResultSet2 = B->numOfIntegers - numOfSameElements;

    if (numOfResultSet1 == 0 && numOfResultSet2 == 0){
        resultSet = (int*) palloc(1*sizeof(int));
        resultSet[0] = 0;

        result = (intSet *)palloc(VARHDRSZ + 1*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultSet, 1*sizeof(int));
    }
    else{
        differenceSet1 = (int *) palloc(numOfResultSet1*sizeof(int));
        sets_difference(A, B, differenceSet1);

        differenceSet2 = (int *) palloc(numOfResultSet2*sizeof(int));
        sets_difference(B, A, differenceSet2);

        resultSet = (int*) palloc((numOfResultSet1+numOfResultSet2)*sizeof(int));
        for (int i = 0; i < numOfResultSet1; ++i) {
            resultSet[i] = differenceSet1[i];
        }
        for (int i = 0; i < numOfResultSet2; ++i) {
            resultSet[numOfResultSet1+i] = differenceSet2[i];
        }
        qsort(resultSet, numOfResultSet1+numOfResultSet2, sizeof(int), cmp_num);

        result = (intSet *) palloc(VARHDRSZ + (numOfResultSet1+numOfResultSet2)*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + (numOfResultSet1+numOfResultSet2)*sizeof(int)+sizeof(int)+sizeof(int));
        memcpy(result->iset, resultSet, (numOfResultSet1+numOfResultSet2)*sizeof(int));
    }

    result->numOfIntegers = numOfResultSet1+numOfResultSet2;
    if (numOfResultSet1 > 0 || numOfResultSet2 > 0){
        result->lengthOfIntSetSting = 2*(numOfResultSet1 + numOfResultSet2) + 1;
    }
    else{
        result->lengthOfIntSetSting = 2;
    }

    PG_RETURN_POINTER(result);
}

//Operator: -
PG_FUNCTION_INFO_V1(intset_difference);

Datum
intset_difference(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *result;
    int numOfSameElements;
    int numOfResultSet;
    int *resultSet;

    numOfSameElements = num_of_same_elements(A, B);
    numOfResultSet = A->numOfIntegers - numOfSameElements;

    if (numOfResultSet == 0){
        resultSet = (int*) palloc(1*sizeof(int));
        resultSet[0] = 0;

        result = (intSet *)palloc(VARHDRSZ + 1*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + 1*sizeof(int)+sizeof(int)+sizeof(int));

        memcpy(result->iset, resultSet, 1*sizeof(int));
    }
    else{
        resultSet = (int*) palloc(numOfResultSet*sizeof(int));
        sets_difference(A, B, resultSet);

        result = (intSet *) palloc(VARHDRSZ + numOfResultSet*sizeof(int) + sizeof(int) + sizeof(int));
        SET_VARSIZE(result,VARHDRSZ + numOfResultSet*sizeof(int)+sizeof(int)+sizeof(int));
        memcpy(result->iset, resultSet, numOfResultSet*sizeof(int));
    }

    result->numOfIntegers = numOfResultSet;

    if (numOfResultSet > 0){
        result->lengthOfIntSetSting = numOfResultSet + numOfResultSet + 1;
    }
    else{
        result->lengthOfIntSetSting = 2;
    }

    PG_RETURN_POINTER(result);
}
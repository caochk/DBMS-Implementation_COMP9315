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
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

PG_MODULE_MAGIC;

typedef struct intSet
{
    int lengthOfIntSetSting;
    int numOfIntegers; //还没完全搞懂应该用哪种int型
    int iset[FLEXIBLE_ARRAY_MEMBER];
} intset;

static int valid_intSet(char *str){
    char *pattern = " *{( *\\d?,?)*} *";
    regex_t regex;
    int validOrNot = FALSE;
    if(regcomp(&regex, pattern, REG_EXTENDED)){ //必须先编译pattern，编译后的结果会存放在regex这个变量中
        return FALSE;
    }
    if(regexec(&regex, str, 0, NULL, 0) == 0) {
        validOrNot = TRUE;
    }
    regfree(&regex);
    return validOrNot;
}

static int length_of_intSetString(char *str){
    int numOfNonBlankCharacters = 0;
    while (*str != '\0'){
        if (*str != ' '){
            numOfNonBlankCharacters ++;
        }
        str ++;
    }
    return numOfNonBlankCharacters;
}

static char *remove_spaces(char *str, int lengthOfIntSetsString, char *intSetString){
    char *p = intSetString;
    intSetString[lengthOfIntSetsString] = '\0';
    while (*str != '\0'){
        if (*str != ' '){
            *p = *str;
            p++;
        }
        str++;
    }
    return intSetString;
}

static void remove_braces(char intSetString[], char targetCharacter){
    int i,j;
    for(i=j=0;intSetString[i]!='\0';i++){
        if(intSetString[i] != targetCharacter){
            intSetString[j++] = intSetString[i];
        }
    }
    intSetString[j]='\0';
}

static int num_of_integers(char *intSetString, int count){
    while (*intSetString != '\0'){
        if (*intSetString == ','){
            count++;
        }
        intSetString++;
    }
    return count;
}

static int transform_intSetString_to_intSetArray(char *intSetString, int lengthOfIntSetsString, int numOfIntegers, intset intsets){
    char *iSetsStrtingTemp;
    char *element;
    char *remaingElements;
    int *iset;
    int i = 1;

    iSetsStrtingTemp =(char *)palloc(lengthOfIntSetsString+1); //change to palloc
    snprintf(iSetsStrtingTemp, lengthOfIntSetsString+1, "%s", intSetString);



    iset = (int *) palloc(numOfIntegers);

    element = strtok_r(iSetsStrtingTemp, ",", &remaingElements);
    sscanf(element, "%d", iset);
    while(element != NULL) {
        element = strtok_r(NULL, ",", &remaingElements);
        if (element != NULL) {
            sscanf(element, "%d", iset + i);
            i++;
        }
    }
    for (int j = 0; j < numOfIntegers; ++j) {
        intsets.iset[j] = iset[j];
    }
//    free(iSetsStrtingTemp); // free() is unnecessary due to palloc()
    return i;
}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
    char	  *str = PG_GETARG_CSTRING(0);
    intset   result;
    int lengthOfIntSetsString;
    char *intSetString;
    int numOfIntegers = 1;
    intset *pointerOfResult;

    int length = strlen(str) + 1; // the length of intSet, including the length of '\0'
    result.lengthOfIntSetSting = length;

    if (!valid_intSet(str))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "intSet", str)));

//    result = (intSet *) palloc(VARHDRSZ+length);
//    SET_VARSIZE(result,VARHDRSZ+length);


    lengthOfIntSetsString = length_of_intSetString(str);

    intSetString = (char *) palloc(lengthOfIntSetsString+1);

    remove_spaces(str, lengthOfIntSetsString, intSetString);
    remove_braces(intSetString, '{');
    remove_braces(intSetString, '}');


    numOfIntegers = num_of_integers(intSetString, numOfIntegers);
    result.numOfIntegers = numOfIntegers;
//    int *intset;
//    intset = (int *) palloc(numOfIntegers);
    transform_intSetString_to_intSetArray(intSetString, lengthOfIntSetsString, numOfIntegers, result); //把字符串类型的intSet转换为真正的整数数组类型（即intSet的内部表示形式）
    //【真ERROR】上句有问题，这样写相当于在强行修改原先结构体定义好时iset的地址为新的transform_intSetString_to_intSetArray返回的地址！改地址是不允许的！只有复制被允许！

    pointerOfResult = &result;
    PG_RETURN_POINTER(pointerOfResult);
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

    intset   *intsets = (intset *)PG_GETARG_POINTER(0);

    result = (char *) palloc(intsets->lengthOfIntSetSting);

    element = (char *) palloc(intsets->lengthOfIntSetSting);


    strcpy(result, leftBrace);
    for (int i = 0; i < intsets->numOfIntegers; ++i) {
//        itoa(intset->iset[i],element, 10);//ERROR:无itoa函数
        snprintf(element, intsets->lengthOfIntSetSting,"%d", intsets->iset[i]);
        strcat(result, element);
        if (i != intsets->numOfIntegers-1){
            strcat(result, comma);
        }
    }
    strcat(result, rightBrace);

    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

//PG_FUNCTION_INFO_V1(complex_recv);
//
//Datum
//complex_recv(PG_FUNCTION_ARGS) // **no need to implement**
//{
//    StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
//    Complex    *result;
//
//    result = (Complex *) palloc(sizeof(Complex));
//    result->x = pq_getmsgfloat8(buf);
//    result->y = pq_getmsgfloat8(buf);
//    PG_RETURN_POINTER(result);
//}
//
//PG_FUNCTION_INFO_V1(complex_send);
//
//Datum
//complex_send(PG_FUNCTION_ARGS) // **no need to implement**
//{
//    Complex    *complex = (Complex *) PG_GETARG_POINTER(0);
//    StringInfoData buf;
//
//    pq_begintypsend(&buf);
//    pq_sendfloat8(&buf, complex->x);
//    pq_sendfloat8(&buf, complex->y);
//    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
//}

/*****************************************************************************
 * New Operators
 *
 * A practical Complex datatype would provide much more than this, of course.
 *****************************************************************************/
static int intset_inclusion(intset *A, intset *B){
//    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
//    intSet   *B = (intSet *) PG_GETARG_POINTER(1);

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

static int num_of_same_elements(intset *A, intset *B){
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

static int *sets_intersection(intset *A, intset *B, int *resultSet){
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

//static int *sets_union(intSet *A, intSet *B, int *resultSet){
//    int k = 0;
//    for (int i = 0; i < A->numOfIntegers; ++i) {
//        for (int j = 0; j < B->numOfIntegers; ++j) {
//            if (A->iset[i] == B->iset[j]){
//                resultSet[k] = A->iset[i];
//                k++;
//            }
//        }
//    }
//    return resultSet;
//}

static int *sets_difference(intset *A, intset *B, int *resultSet){
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
    int        i = PG_GETARG_INT32(0); //写法存疑！！！！！！！！！！！【真ERROR】warning: cast from pointer to integer of different size
    intset   *S = (intset *) PG_GETARG_POINTER(1);

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
    intset   *S = (intset *) PG_GETARG_POINTER(0);

    PG_RETURN_INT32(S->numOfIntegers);
}

//Operator: >@
PG_FUNCTION_INFO_V1(intset_improper_superset);

Datum
intset_improper_superset(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(intset_inclusion(A, B)==FALSE);
}

//Operator: @<
PG_FUNCTION_INFO_V1(intset_improper_subset);

Datum
intset_improper_subset(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(intset_inclusion(B, A)==TRUE);
}

//Operator: =
PG_FUNCTION_INFO_V1(intset_eq);

Datum
intset_eq(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);

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
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);

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
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);
    intset result;
    int numOfSameElements;
    int *resultSet;
    intset *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);
    result.numOfIntegers = numOfSameElements;
    result.lengthOfIntSetSting = numOfSameElements + numOfSameElements + 2;


    resultSet = (int *) palloc(numOfSameElements);
    sets_intersection(A, B, resultSet);

    for (int j = 0; j < numOfSameElements; ++j) {
        result.iset[j] = resultSet[j];
    }


    pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}

//Operator: ||
PG_FUNCTION_INFO_V1(intset_union); //【未通过】

Datum
intset_union(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);
    intset result;
    int numOfSameElements;
    int lengthOfDifferenceSet;
    int *differenceSet;
//    int *resultTemp;
    int lengthOfResultSet;
    int *resultSet;
    intset *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);

    //先求差集

    lengthOfDifferenceSet = A->numOfIntegers - numOfSameElements;

    differenceSet = (int *) palloc(lengthOfDifferenceSet);

    sets_difference(A, B, differenceSet);//resultTemp数组存的是差集



    lengthOfResultSet = A->numOfIntegers - numOfSameElements + B->numOfIntegers;
    result.numOfIntegers = lengthOfResultSet;
    result.lengthOfIntSetSting = lengthOfResultSet + lengthOfResultSet + 2;

    //将差集与集合B合到一起便是并集

    resultSet = (int*) palloc(lengthOfResultSet);
    for (int i = 0; i < B->numOfIntegers; ++i) {
        resultSet[i] = B->iset[i];
    }
    for (int i = 0; i < lengthOfDifferenceSet; ++i) {
        resultSet[B->numOfIntegers+i] = differenceSet[i];
    }
    for (int j = 0; j < lengthOfResultSet; ++j) {
        result.iset[j] = resultSet[j];
    }


    pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}

//Operator: ！！
PG_FUNCTION_INFO_V1(intset_disjunction);  //【未通过】

Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);
    intset  result;
    int numOfSameElements;
    int lengthOfResultSet1;
    int lengthOfResultSet2;
    int *differenceSet1;
    int *differenceSet2;
    int *resultTemp1;
    int *resultTemp2;
    int *resultSet;
    intset *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);

    lengthOfResultSet1 = A->numOfIntegers - numOfSameElements;

    lengthOfResultSet2 = B->numOfIntegers - numOfSameElements;

    differenceSet1 = (int *) palloc(lengthOfResultSet1);

    differenceSet2 = (int *) palloc(lengthOfResultSet2);

    resultTemp1 = sets_difference(A, B, differenceSet1); //get (A-B)
    resultTemp2 = sets_difference(A, B, differenceSet2); //get (B-A)

    //to get (A-B)∪(B-A),∵(A-B)∪(B-A)=set disjunction

    resultSet = (int *) palloc(lengthOfResultSet1 + lengthOfResultSet2);
    for (int i = 0; i < lengthOfResultSet1; ++i) {
        resultSet[i] = resultTemp1[i];
    }
    for (int i = 0; i < lengthOfResultSet2; ++i) {
        resultSet[lengthOfResultSet1+i] = resultTemp2[i];
    }
    result.numOfIntegers = lengthOfResultSet1 + lengthOfResultSet2;
    result.lengthOfIntSetSting = lengthOfResultSet1 + lengthOfResultSet2 + lengthOfResultSet1 + lengthOfResultSet2 + 2;

    for (int j = 0; j < lengthOfResultSet1 + lengthOfResultSet2; ++j) {
        result.iset[j] = resultSet[j];
    }


    pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}

//Operator: -
PG_FUNCTION_INFO_V1(intset_difference);

Datum
intset_difference(PG_FUNCTION_ARGS)
{
    intset   *A = (intset *) PG_GETARG_POINTER(0);
    intset   *B = (intset *) PG_GETARG_POINTER(1);
    intset result;
    int numOfSameElements;
    int lengthOfResultSet;
    int *resultSet;
    intset *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);

    lengthOfResultSet = A->numOfIntegers - numOfSameElements;
    result.numOfIntegers = lengthOfResultSet;
    result.lengthOfIntSetSting = lengthOfResultSet + 2;


    resultSet = (int *) palloc(lengthOfResultSet);
    sets_difference(A, B, resultSet);

    for (int j = 0; j < lengthOfResultSet; ++j) {
        result.iset[j] = resultSet[j];
    }


    pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}
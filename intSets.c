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
#include "regex.h"
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

PG_MODULE_MAGIC;

typedef struct intSet
{
    int lengthOfIntSetSting;
    int numOfIntegers; //还没完全搞懂应该用哪种int型
    int iset[FLEXIBLE_ARRAY_MEMBER];
} intSet;

static int valid_intSet(char *str){
    char *pattern = " *{( *\d?,?)*} *";
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

static int * transform_intSetString_to_intSetArray(char *intSetString, int lengthOfIntSetsString, int numOfIntegers, int *intSet){
    char *iSetsStrtingTemp=(char *)malloc(lengthOfIntSetsString+1); //change to palloc
    snprintf(iSetsStrtingTemp, lengthOfIntSetsString+1, "%s", intSetString);
    char *element;
    char *remaingElements;
    int i = 1;

    element = strtok_r(iSetsStrtingTemp, ",", &remaingElements);
    sscanf(element, "%d", intSet);
    while(element != NULL) {
        element = strtok_r(NULL, ",", &remaingElements);
        if (element != NULL) {
            sscanf(element, "%d", intSet + i);
            i++;
        }
    }
    free(iSetsStrtingTemp); // free() is unnecessary due to palloc()
    return intSet;
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

    int length = strlen(str) + 1; // the length of intSet, including the length of '\0'
    result->lengthOfIntSetSting = length;

    if (!valid_intSet(str))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "intSet", str)));

    result = (intSet *) palloc(VARHDRSZ+length);
    SET_VARSIZE(result,VARHDRSZ+length);

    int lengthOfIntSetsString = length_of_intSetString(str);
    char intSetString[lengthOfIntSetsString+1];
    remove_spaces(str, lengthOfIntSetsString, intSetString);
    remove_braces(intSetString, '{');
    remove_braces(intSetString, '}');
    int numOfIntegers = 1;
    numOfIntegers = num_of_integers(intSetString, numOfIntegers);
    result->numOfIntegers = numOfIntegers;
    int intSet[numOfIntegers];
    result->iset = transform_intSetString_to_intSetArray(intSetString, lengthOfIntSetsString, numOfIntegers, intSet); //把字符串类型的intSet转换为真正的整数数组类型（即intSet的内部表示形式）
    //上句有问题，这样写相当于在强行修改原先结构体定义好时iset的地址为新的transform_intSetString_to_intSetArray返回的地址！改地址是不允许的！只有复制被允许！
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(insets_out);

Datum
insets_out(PG_FUNCTION_ARGS)
{
    intSet   *intSet = (intSet *) PG_GETARG_POINTER(0);
    char	 result[intSet->lengthOfIntSetSting];
    char    element[intSet->lengthOfIntSetSting];
    char    comma[2] = {','};
    char    leftBrace[2] = {'{'};
    char    rightBrace[2] = {'}'};

    strcpy(result, leftBrace);
    for (int i = 0; i < intSet->numOfIntegers; ++i) {
        itoa(intSet->iset[i],element, 10);
        strcat(result, element);
        if (i != intSet->numOfIntegers-1){
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
static int intset_inclusion(intSet *A, intSet *B){
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
    int        i = (int) PG_GETARG_POINTER(0); //写法存疑！！！！！！！！！！！
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

    PG_RETURN_BOOL(intset_inclusion(A, B)==FALSE);
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
    intSet result;
    int numOfSameElements = num_of_same_elements(A, B);
    result.numOfIntegers = numOfSameElements;
    int resultSet[numOfSameElements];
    sets_intersection(A, B, resultSet);

    for (int j = 0; j < numOfSameElements; ++j) {
        result.iset[j] = resultSet[j];
    }

    intSet *pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}

//Operator: ||
PG_FUNCTION_INFO_V1(intset_union); //【未通过】

Datum
intset_union(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *result;
    int numOfSameElements = num_of_same_elements(A, B);

    //先求差集
    int lengthOfDifferenceSet = A->numOfIntegers - numOfSameElements;
    int differenceSet[lengthOfDifferenceSet];
    int *resultTemp;
    resultTemp = sets_difference(A, B, differenceSet);//resultTemp数组存的是差集


    int lengthOfResultSet = A->numOfIntegers - numOfSameElements + B->numOfIntegers;
    result->numOfIntegers = lengthOfResultSet;
    //将差集与集合B合到一起便是并集
    int resultSet[lengthOfResultSet];
    for (int i = 0; i < B->numOfIntegers; ++i) {
        resultSet[i] = B->iset[i];
    }
    for (int i = 0; i < lengthOfDifferenceSet; ++i) {
        resultSet[B->numOfIntegers+i] = differenceSet[i];
    }
    result->iset = resultSet;

    PG_RETURN_POINTER(result);
}

//Operator: ！！
PG_FUNCTION_INFO_V1(intset_disjunction);  //【未通过】

Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *result;

    int numOfSameElements = num_of_same_elements(A, B);
    int lengthOfResultSet1 = A->numOfIntegers - numOfSameElements;
    int lengthOfResultSet2 = B->numOfIntegers - numOfSameElements;
    int differenceSet1[lengthOfResultSet1];
    int differenceSet2[lengthOfResultSet2];
    int *resultTemp1;
    int *resultTemp2;
    resultTemp1 = sets_difference(A, B, differenceSet1); //get (A-B)
    resultTemp2 = sets_difference(A, B, differenceSet2); //get (B-A)

    //to get (A-B)∪(B-A),∵(A-B)∪(B-A)=set disjunction
    int resultSet[lengthOfResultSet1 + lengthOfResultSet2];
    for (int i = 0; i < lengthOfResultSet1; ++i) {
        resultSet[i] = resultTemp1[i];
    }
    for (int i = 0; i < lengthOfResultSet2; ++i) {
        resultSet[lengthOfResultSet1+i] = resultTemp2[i];
    }
    result->numOfIntegers = lengthOfResultSet1 + lengthOfResultSet2;
    result->iset = resultSet;

    PG_RETURN_POINTER(result);
}

//Operator: -
PG_FUNCTION_INFO_V1(intset_difference);

Datum
intset_difference(PG_FUNCTION_ARGS)
{
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet result;
    int numOfSameElements = num_of_same_elements(A, B);
    int lengthOfResultSet = A->numOfIntegers - numOfSameElements;
    result.numOfIntegers = lengthOfResultSet;

    int resultSet[lengthOfResultSet];
    sets_difference(A, B, resultSet);

    for (int j = 0; j < lengthOfResultSet; ++j) {
        result.iset[j] = resultSet[j];
    }

    intSet *pointerOfResult = &result;

    PG_RETURN_POINTER(pointerOfResult);
}

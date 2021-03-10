/*
 * src/tutorial/intSets.c
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

typedef struct intSets
{
    int32 numOfIntegers; //还没完全搞懂应该用哪种int型
    int isets[FLEXIBLE_ARRAY_MEMBER];
} intSets;

static int valid_intSets(char *str){
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

static int length_of_intSetsString(char *str){
    int numOfNonBlankCharacters = 0;
    while (*str != '\0'){
        if (*str != ' '){
            numOfNonBlankCharacters ++;
        }
        str ++;
    }
    return numOfNonBlankCharacters;
}

static char *remove_spaces(char *str, int lengthOfIntSetsString, char *intSetsString){
    char *p = intSetsString;
    intSetsString[lengthOfIntSetsString] = '\0';
    while (*str != '\0'){
        if (*str != ' '){
            *p = *str;
            p++;
        }
        str++;
    }
    return intSetsString;
}

static void remove_braces(char intSetsString[], char targetCharacter){
    int i,j;
    for(i=j=0;intSetsString[i]!='\0';i++){
        if(intSetsString[i] != targetCharacter){
            intSetsString[j++] = intSetsString[i];
        }
    }
    intSetsString[j]='\0';
}

static int num_of_integers(char *intSetsString, int count){
    while (*intSetsString != '\0'){
        if (*intSetsString == ','){
            count++;
        }
        intSetsString++;
    }
    return count;
}

static int * transform_intSetsString_to_intSetsArray(char *intSetsString, int lengthOfIntSetsString, int numOfIntegers, int *intSets){
    char *iSetsStrtingTemp=(char *)malloc(lengthOfIntSetsString+1); //change to palloc
    snprintf(iSetsStrtingTemp, lengthOfIntSetsString+1, "%s", intSetsString);
    char *element;
    char *remaingElements;
    int i = 1;

    element = strtok_r(iSetsStrtingTemp, ",", &remaingElements);
    sscanf(element, "%d", intSets);
    while(element != NULL) {
        element = strtok_r(NULL, ",", &remaingElements);
        if (element != NULL) {
            sscanf(element, "%d", intSets + i);
            i++;
        }
    }
    free(iSetsStrtingTemp); // free() is unnecessary due to palloc()
    return intSets;
}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intsets_in);

Datum
intsets_in(PG_FUNCTION_ARGS)
{
    char	  *str = PG_GETARG_CSTRING(0);
    intSets   *result;

    int length = strlen(str) + 1; // the length of intSets, including the length of '\0'

    if (!valid_intSets(str))
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                               "intSets", str)));

    result = (intSets *) palloc(VARHDRSZ+length);
    SET_VARSIZE(result,VARHDRSZ+length);

    int lengthOfIntSetsString = length_of_intSetsString(str);
    char intSetsString[lengthOfIntSetsString+1];
    remove_spaces(str, lengthOfIntSetsString, intSetsString);
    remove_braces(intSetsString, '{');
    remove_braces(intSetsString, '}');
    int numOfIntegers = 1;
    numOfIntegers = num_of_integers(intSetsString, numOfIntegers);
    result->numOfIntegers = numOfIntegers;
    int intSets[numOfIntegers];
    result->isets = transform_intSetsString_to_intSetsArray(intSetsString, lengthOfIntSetsString, numOfIntegers, intSets); //把字符串类型的intSets转换为真正的整数数组类型（即intSets的内部表示形式）

    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(insets_out);

Datum
insets_out(PG_FUNCTION_ARGS) // not completed
{
    intSets   *intSets = (intSets *) PG_GETARG_POINTER(0);
    char	  *result;

    int sizeOfIntSets = sizeof(intSets->isets);//尚未完成：不知道如何将整数数组中的元素一个个拿出来变成字符串（利用itoa()函数）->然后拼接字符串->最后赋给result |||或者有没有直接将整个整数数组转换为字符串数组


    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(complex_recv);

Datum
complex_recv(PG_FUNCTION_ARGS) // **no need to implement**
{
    StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
    Complex    *result;

    result = (Complex *) palloc(sizeof(Complex));
    result->x = pq_getmsgfloat8(buf);
    result->y = pq_getmsgfloat8(buf);
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(complex_send);

Datum
complex_send(PG_FUNCTION_ARGS) // **no need to implement**
{
    Complex    *complex = (Complex *) PG_GETARG_POINTER(0);
    StringInfoData buf;

    pq_begintypsend(&buf);
    pq_sendfloat8(&buf, complex->x);
    pq_sendfloat8(&buf, complex->y);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*****************************************************************************
 * New Operators
 *
 * A practical Complex datatype would provide much more than this, of course.
 *****************************************************************************/
static int intsets_inclusion(intSets *A, intSets *B){
//    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
//    intSets   *B = (intSets *) PG_GETARG_POINTER(1);

    int result = FALSE;
    for (int i = 0; i < B->numOfIntegers; ++i) {
        for (int j = 0; j < A->numOfIntegers; ++j) {
            if (B->isets[i] == A->isets[j]){
                result = TRUE;
            }
        }
        if (result == FALSE){
            break;
        }
    }
    return result;
}

static int num_of_same_elements(intSets *A, intSets *B){
    int numOfSameElements = 0;
    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->isets[i] == B->isets[j]){
                numOfSameElements++;
            }
        }
    }
    return numOfSameElements;
}

static int *sets_intersection(intSets *A, intSets *B, int *resultSet){
    int k = 0;
    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->isets[i] == B->isets[j]){
                resultSet[k] = A->isets[i];
                k++;
            }
        }
    }
    return resultSet;
}

//static int *sets_union(intSets *A, intSets *B, int *resultSet){
//    int k = 0;
//    for (int i = 0; i < A->numOfIntegers; ++i) {
//        for (int j = 0; j < B->numOfIntegers; ++j) {
//            if (A->isets[i] == B->isets[j]){
//                resultSet[k] = A->isets[i];
//                k++;
//            }
//        }
//    }
//    return resultSet;
//}

static int *sets_difference(intSets *A, intSets *B, int *resultSet){
    int count = 0;
    int k = 0;
    for (int i = 0; i < A->numOfIntegers; ++i) {
        for (int j = 0; j < B->numOfIntegers; ++j) {
            if (A->isets[i] != B->isets[j]){
                count++;
            }
        }
        if (count == B->numOfIntegers){
            resultSet[k] = A->isets[i];
            k ++;
        }
        count = 0;
    }
    return resultSet;
}

// Operator: ?
PG_FUNCTION_INFO_V1(intsets_contain); //完成【尚未验证】

Datum
intsets_contain(PG_FUNCTION_ARGS)
{
    int        i = (int) PG_GETARG_POINTER(0); //写法存疑！！！！！！！！！！！
    intSets   *S = (intSets *) PG_GETARG_POINTER(1);

    int result = FALSE;
    for (int j = 0; j < S->numOfIntegers; ++j) {
        if (i == S->isets[j]){
            result = TRUE;
        }
    }
    PG_RETURN_POINTER(result);
}

//Operator: #
PG_FUNCTION_INFO_V1(intsets_cardinality); //完成【尚未验证】

Datum
intsets_cardinality(PG_FUNCTION_ARGS)
{
    intSets   *S = (intSets *) PG_GETARG_POINTER(0);

    PG_RETURN_INT32(S->numOfIntegers);
}

//Operator: >@
PG_FUNCTION_INFO_V1(intsets_improper_superset); //完成【尚未验证】

Datum
intsets_improper_superset(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(intsets_inclusion(A, B));
}

//Operator: @<
PG_FUNCTION_INFO_V1(intsets_improper_subset); //完成【尚未验证】

Datum
intsets_improper_subset(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(intsets_inclusion(B, A));
}

//Operator: =
PG_FUNCTION_INFO_V1(intsets_eq); //完成【尚未验证】

Datum
intsets_eq(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);

    int result1 = intsets_inclusion(A, B);
    int result2 = intsets_inclusion(B, A);
    int result = FALSE;
    if (result1 == TRUE && result2 == TRUE){
        result = TRUE;
    }
    PG_RETURN_INT32(result);
}

//Operator: <>
PG_FUNCTION_INFO_V1(intsets_not_eq); //完成【尚未验证】

Datum
intsets_not_eq(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);

    int result = intsets_inclusion(A, B);
//    int result2 = intsets_inclusion(B, A);
//    int result = FALSE;
//    if (result1 == FALSE && result2 == TRUE){
//        result = TRUE;
//    }
    PG_RETURN_INT32(!result);
}

//Operator: &&
PG_FUNCTION_INFO_V1(intsets_intersection);

Datum
intsets_intersection(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);
    intSets *result;
    int numOfSameElements = num_of_same_elements(A, B);
    result->numOfIntegers = numOfSameElements;
    int resultSet[numOfSameElements];
    result->isets = sets_intersection(A, B, resultSet);

    PG_RETURN_POINTER(result);
}

//Operator: ||
PG_FUNCTION_INFO_V1(intsets_union); //完成【尚未验证】

Datum
intsets_union(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);
    intSets *result;
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
        resultSet[i] = B->isets[i];
    }
    for (int i = 0; i < lengthOfDifferenceSet; ++i) {
        resultSet[B->numOfIntegers+i] = differenceSet[i];
    }
    result->isets = resultSet;

    PG_RETURN_POINTER(result);
}

//Operator: ！！
PG_FUNCTION_INFO_V1(intsets_disjunction);  //完成【尚未验证】

Datum
intsets_disjunction(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);
    intSets *result;

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
    result->isets = resultSet;

    PG_RETURN_POINTER(result);
}

//Operator: -
PG_FUNCTION_INFO_V1(intsets_difference);  //完成【尚未验证】

Datum
intsets_difference(PG_FUNCTION_ARGS)
{
    intSets   *A = (intSets *) PG_GETARG_POINTER(0);
    intSets   *B = (intSets *) PG_GETARG_POINTER(1);
    intSets *result;
    int numOfSameElements = num_of_same_elements(A, B);
    int lengthOfResultSet = A->numOfIntegers - numOfSameElements;
    result->numOfIntegers = lengthOfResultSet;
    int resultSet[lengthOfResultSet];
    result->isets = sets_difference(A, B, resultSet);

    PG_RETURN_POINTER(result);
}
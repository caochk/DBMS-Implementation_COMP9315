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
    char v_len[VARHDRSZ];//关键，一定要写！
    int lengthOfIntSetSting;
    int numOfIntegers; //还没完全搞懂应该用哪种int型
    int iset[1];
} intSet;

//static int valid_intSet(char *str){
//    char *pattern = " *{( *\\d?,?)*} *";
//    regex_t regex;
//    int validOrNot = FALSE;
//    if(regcomp(&regex, pattern, REG_EXTENDED)){ //必须先编译pattern，编译后的结果会存放在regex这个变量中
//        return FALSE;
//    }
//    if(regexec(&regex, str, 0, NULL, 0) == 0) {
//        validOrNot = TRUE;
//    }
//    regfree(&regex);
//    return validOrNot;
//}

//static int length_of_intSetString(char *str){
//    int numOfNonBlankCharacters = 0;
//    while (*str != '\0'){
//        if (*str != ' '){
//            numOfNonBlankCharacters ++;
//        }
//        str ++;
//    }
//    return numOfNonBlankCharacters;
//}

//static char *remove_spaces(char *str, int lengthOfIntSetsString, char *intSetString){
//    char *p = intSetString;
//    intSetString[lengthOfIntSetsString] = '\0';
//    while (*str != '\0'){
//        if (*str != ' '){
//            *p = *str;
//            p++;
//        }
//        str++;
//    }
//    return intSetString;
//}

//static void remove_braces(char intSetString[], char targetCharacter){
//    int m,n;
//    for(m=n=0;intSetString[m]!='\0';m++){
//        if(intSetString[m] != targetCharacter){
//            intSetString[n++] = intSetString[m];
//        }
//    }
//    intSetString[n]='\0';
//}

//static int num_of_integers(char *intSetString, int count){
//    while (*intSetString != '\0'){
//        if (*intSetString == ','){
//            count++;
//        }
//        intSetString++;
//    }
//    return count;
//}

//static int transform_intSetString_to_intSetArray(char *intSetString, int lengthOfIntSetsString, int numOfIntegers, intSet *intsets){
////    char *iSetsStrtingTemp;
//    char *element;
//    char *remaingElements;
////    int *iset;
//    int i = 1;
//
////    iSetsStrtingTemp =(char *)palloc(lengthOfIntSetsString+1); //change to palloc
////    snprintf(iSetsStrtingTemp, lengthOfIntSetsString+1, "%s", intSetString);
//
//
//
////    iset = (int *) palloc(sizeof(int)*numOfIntegers);
//
//    element = strtok_r(intSetString, ",", &remaingElements);
////    sscanf(element, "%d", iset);
//    intsets->iset[0] = atoi(element);
//    while(element != NULL) {
//        element = strtok_r(NULL, ",", &remaingElements);
//        if (element != NULL) {
////            sscanf(element, "%d", iset + i);
//            intsets->iset[i] = atoi(element);
//            i++;
//        }
//    }
////    memcpy(intsets->iset, iset, numOfIntegers*sizeof(int));
////    for (int j = 0; j < numOfIntegers; ++j) {
////        intsets->iset[j] = iset[j];
////    }
//    return i;
//}
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
            if(ele == array[j])//判断数组中是否有重复元素
            {
                for(int ele = j; ele < numOfIntegers-1; ele++)//从找到重复元素位置开始，让后续所有元素前挪一位
                {
                    array[ele] = array[ele + 1];
                }
                j--;//这一步非常重要，即之前在j位置找到了重复元素，那么下一轮循环只需从j位置的前一位开始往后遍历即可（因为j前面肯定没有这个重复元素了），但是j需要减一，因为
                numOfIntegers--;//前挪后，数组长度-1
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
//    int length;
//    intset *pointerOfResult;


    //判断是最先的！
//    if (!valid_intSet(str)){
//        ereport(ERROR,
//                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
//                        errmsg("CNMD %s: \"%s\"",
//                               "intSet", str)));
//    }


    //计算numOfIntegers
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

    //计算lengthOfIntSetsString
    j = 0;
    while (str[j] != '\0'){
        if (str[j] != ' '){
            lengthOfIntSetsString ++;
        }
        j ++;
    }

    elog(NOTICE,"lengthOfIntString:%d\n",lengthOfIntSetsString);

    intSetString = (char *)palloc((lengthOfIntSetsString+1)*sizeof(char));//测试过，这个内存分配应该是对的

    //去除空格
//    char *p = intSetString;
    intSetString[lengthOfIntSetsString] = '\0';
    while (str[m] != '\0'){
        if (str[m] != ' '){
            intSetString[n] = str[m];
            n++;
        }
        m++;
    }

    //去除左、右括号
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

//    elog(NOTICE,"1111111111111\n");





//    elog(NOTICE,"2222222222\n");




//    transform_intSetString_to_intSetArray(intSetString, lengthOfIntSetsString, numOfIntegers, result);

    if (numOfIntegers == 0){
        resultTmp = (int *) palloc(numOfIntegers*sizeof(int));
        resultTmp[0] = 0;
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
    }

    result = (intSet*) palloc(VARHDRSZ + numOfIntegers*sizeof(int)+sizeof(int)+sizeof(int));//内存分配存疑
//    SET_VARSIZE(result,VARHDRSZ + sizeof(intSet));
    SET_VARSIZE(result,VARHDRSZ + numOfIntegers*sizeof(int)+sizeof(int)+sizeof(int));

    result->lengthOfIntSetSting = lengthOfIntSetsString;
    result->numOfIntegers = numOfIntegers;
    memcpy(result->iset, resultTmp, numOfIntegers*sizeof(int));

//    elog(NOTICE,"index:%d,value:%d\n",i,result->iset[i]);

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

//    result = (char *) calloc(intsets->lengthOfIntSetSting + 1, sizeof(char));//还没测试，感觉是对的
    result = (char *) palloc((intsets->lengthOfIntSetSting + 1)*sizeof(char));//还没测试，感觉是对的

    element = (char *) palloc((intsets->lengthOfIntSetSting + 1)*sizeof(char));//element是单个数字的字符串，感觉此处只有分多不会分少

    elog(NOTICE,"lengthOfIntSetSting:%d\n", intsets->lengthOfIntSetSting);
    elog(NOTICE,"numOfIntegers:%d\n", intsets->numOfIntegers);

    if (intsets->numOfIntegers == 0){
        PG_RETURN_CSTRING(empty_str);
    }
    else{
        result[0] = '\0';
        strcpy(result, leftBrace);
        for (int i = 0; i < intsets->numOfIntegers; ++i) {
//        snprintf(element, intsets->lengthOfIntSetSting + 1,"%d", intsets->iset[i]);
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

//    elog(NOTICE,"CNMD!!!\n");

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

//    elog(NOTICE,"CNMD!!!\n");

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

//    elog(NOTICE,"CNMD!!!\n");

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
    intSet   *S = (intSet *) PG_GETARG_POINTER(1);

    int result = FALSE;

//    elog(NOTICE,"CNMD!!!\n");

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

//    elog(NOTICE,"CNMD!!!\n");
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
    intSet result;
    int numOfSameElements;
    int *resultSet;
    intSet *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);
    result.numOfIntegers = numOfSameElements;
    result.lengthOfIntSetSting = numOfSameElements + numOfSameElements + 2;


    resultSet = (int *) palloc(numOfSameElements);//感觉没错
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
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet result;
    int numOfSameElements;
    int lengthOfDifferenceSet;
    int *differenceSet;
//    int *resultTemp;
    int lengthOfResultSet;
    int *resultSet;
    intSet *pointerOfResult;

    numOfSameElements = num_of_same_elements(A, B);

    //先求差集

    lengthOfDifferenceSet = A->numOfIntegers - numOfSameElements;

    differenceSet = (int *) palloc(lengthOfDifferenceSet);//应该没错

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
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet  result;
    int numOfSameElements;
    int lengthOfResultSet1;
    int lengthOfResultSet2;
    int *differenceSet1;
    int *differenceSet2;
    int *resultTemp1;
    int *resultTemp2;
    int *resultSet;
    intSet *pointerOfResult;

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
    intSet   *A = (intSet *) PG_GETARG_POINTER(0);
    intSet   *B = (intSet *) PG_GETARG_POINTER(1);
    intSet result;
    int numOfSameElements;
    int lengthOfResultSet;
    int *resultSet;
    intSet *pointerOfResult;

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
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

static char *remove_spaces(char *str, int lengthOfIntSetsString){
//    int lengthOfString = numOfNonBlankCharacters;
    char intSetsString[lengthOfIntSetsString] = {0};
    char *p = intSetsString;

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

static int * transform_intSetsString_to_intSetsArray(char *intSetsString, int lengthOfIntSetsString){
    char *iSetsStrtingTemp=(char *)malloc(lengthOfIntSetsString+1);
    snprintf(iSetsStrtingTemp, lengthOfIntSetsString+1, "%s", intSetsString);
    char *element;
    char *remaingElements;
    int intSets[lengthOfIntSetsString - 2];
    int i = 1;

    element = strtok_r(iSetsStrtingTemp, ",", &remaingElements);
    sscanf(element, "%d", intSets);
    while(element != NULL) {
//        printf("ptr=%s\n",ptr);
        element = strtok_r(NULL, ",", &remaingElements);
        if (element != NULL) {
            sscanf(element, "%d", intSets + i);
            i++;
        }
    }
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
    char *intSetsString = remove_spaces(str, lengthOfIntSetsString);
    remove_braces(intSetsString, '{');
    remove_braces(intSetsString, '}');
    result->isets = transform_intSetsString_to_intSetsArray(intSetsString, lengthOfIntSetsString); //把字符串类型的intSets转换为真正的整数数组类型（即intSets的内部表示形式）

    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(insets_out);

Datum
insets_out(PG_FUNCTION_ARGS)
{
    intSets   *isets = (intSets *) PG_GETARG_POINTER(0);
    char	   *result;

    result = psprintf("%s", isets->isets);
    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(complex_recv);

Datum
complex_recv(PG_FUNCTION_ARGS)
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
complex_send(PG_FUNCTION_ARGS)
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

PG_FUNCTION_INFO_V1(complex_add);

Datum
complex_add(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);
    Complex    *result;

    result = (Complex *) palloc(sizeof(Complex));
    result->x = a->x + b->x;
    result->y = a->y + b->y;
    PG_RETURN_POINTER(result);
}


/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

#define Mag(c)	((c)->x*(c)->x + (c)->y*(c)->y)

static int
complex_abs_cmp_internal(Complex * a, Complex * b)
{
    double		amag = Mag(a),
            bmag = Mag(b);

    if (amag < bmag)
        return -1;
    if (amag > bmag)
        return 1;
    return 0;
}


PG_FUNCTION_INFO_V1(complex_abs_lt);

Datum
complex_abs_lt(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(complex_abs_le);

Datum
complex_abs_le(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_eq);

Datum
complex_abs_eq(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(complex_abs_ge);

Datum
complex_abs_ge(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(complex_abs_gt);

Datum
complex_abs_gt(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(complex_abs_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(complex_abs_cmp);

Datum
complex_abs_cmp(PG_FUNCTION_ARGS)
{
    Complex    *a = (Complex *) PG_GETARG_POINTER(0);
    Complex    *b = (Complex *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(complex_abs_cmp_internal(a, b));
}
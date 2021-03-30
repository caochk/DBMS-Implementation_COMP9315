// query.c ... query scan functions
// part of signature indexed files
// Manage creating and using Query objects
// Written by John Shepherd, March 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"
#include "tsig.h"
#include "psig.h"
#include "bsig.h"

// check whether a query is valid for a relation
// e.g. same number of attributes

int checkQuery(Reln r, char *q)
{
	if (*q == '\0') return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',') nattr++;
	return (nattr == nAttrs(r));
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q, char sigs)
{
	Query new = malloc(sizeof(QueryRep));
	assert(new != NULL);
	if (!checkQuery(r,q)) return NULL;
	new->rel = r;
	new->qstring = q;
	new->nsigs = new->nsigpages = 0;
	new->ntuples = new->ntuppages = new->nfalse = 0;
	new->pages = newBits(nPages(r));
	switch (sigs) {
	case 't': findPagesUsingTupSigs(new); break;
	case 'p': findPagesUsingPageSigs(new); break;
	case 'b': findPagesUsingBitSlices(new); break;
	default:  setAllBits(new->pages); break;
	}
	new->curpage = 0;
	return new;
}

// scan through selected pages (q->pages)
// search for matching tuples and show each
// accumulate query stats

void scanAndDisplayMatchingTuples(Query q)
{
	assert(q != NULL);
	//TODO
	Reln rel = q->rel;
	Bits examinePageList = q->pages;
	PageID curPID = q->curpage;
	Count curTuple = q->curtup;
	Bool falseMatchFlag = TRUE;
	char *queryString = q->qstring;

    for (curPID = 0; curPID < nPages(rel); ++curPID) {
        if (bitIsSet(examinePageList, curPID) == FALSE) {
            continue;
        }

        Page curPage = getPage(dataFile(rel), curPID);//get a page
        Count nitemsInCurPage = pageNitems(curPage);
        for (curTuple = 0; curTuple < nitemsInCurPage; ++curTuple) {
            Tuple curTupleData = getTupleFromPage(rel, curPage, curTuple);// get the data of a tuple
            q->ntuples ++; // num of tuples examined
            if (tupleMatch(rel, curTupleData, queryString) == TRUE) { // data of tuple matches the query string
                showTuple(rel,curTupleData);

                falseMatchFlag = FALSE;
            }
        }
        q->ntuppages ++; // num of data pages read

        if (falseMatchFlag == TRUE) {
            q->nfalse ++; //num of false match
        }
    }
}

// print statistics on query

void queryStats(Query q)
{
	printf("# sig pages read:    %d\n", q->nsigpages);
	printf("# signatures read:   %d\n", q->nsigs);
	printf("# data pages read:   %d\n", q->ntuppages);
	printf("# tuples examined:   %d\n", q->ntuples);
	printf("# false match pages: %d\n", q->nfalse);
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->pages);
	free(q);
}


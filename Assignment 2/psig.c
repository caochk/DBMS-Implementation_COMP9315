// psig.c ... functions on page signatures (psig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"

Bits generateCodeWord (char **attr_value, int tupleSigBits, int codeWordBits)
{
    int nbits = 0;
    Bits cword = newBits(tupleSigBits);
    srandom(hash_any(attr_value, strlen(attr_value)));

    while (nbits < codeWordBits) {
        int i = random() % tupleSigBits;
        if (!bitIsSet(cword, i)) {
            setBit(cword, i);
            nbits++;
        }
    }
    return cword;
}

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
    int nbitsOfPsig = psigBits(r);//***页面签名（页面描述符）位数，参数m
    Bits Psig = newBits(nbitsOfPsig);//***新建页面签名本体，会构建页面签名位数的页面签名出来
    char **attrsArray = tupleVals(r, t);//属性数组
    int nattr = nAttrs(r);//属性个数，参数n

    for (int i = 0; i < nattr; ++i) {//遍历属性个数次
        if (strncmp(attrsArray[i], "?", 1) == 0) { //if we scan "?" in an attribute，即跳过未知属性
            continue;
        }
        else {
            Bits cword = generateCodeWord(attrsArray[i], psigBits(r), codeBits(r));//对于已知属性，计算出其码字
            orBits(Psig, cword);//对所有（已知）属性的码字执行OR运算，得到最后的页面签名（页面描述符）【疑问：只针对SIMC吗？】
            freeBits(cword);
        }
    }
    freeVals(attrsArray, nattr);// 这一句不确定需不需要
    return Psig;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	//TODO
    Reln r = q->rel;
    Bits querySig = makePageSig(r, q->qstring);//制作查询签名（查询描述符）
    unsetAllBits(q->pages);//Pages = AllZeroBits，制作了一个‘待检查页面’列表（实际是位串）
    Count numOfPageSigPages = nPsigPages(r);//***存放页面签名（页面描述符）的页面个数，参数b
    File pageSigFile = psigFile(r);//***页面签名（页面描述符）文件本体
    Count nPageSigPerPage = maxPsigsPP(r);//***每个页面可容纳的最大页面签名（页面描述符）个数
    Count nbitsOfPsig = psigBits(r);//***页面签名（页面描述符）位数，参数m
    //    Count nTuplePerPage = maxTupsPP(r);

    for (int pid = 0; pid < numOfPageSigPages; ++pid) {//***遍历 ‘存放页面签名（页面描述符）的页面个数，参数b’ 次。且每次的i值即为pid
        Page p = getPage(pageSigFile, pid);//***从页面签名文件内拿出该pid对应的存放了页面签名的数据页本体
        Count numOfPageSig = pageNitems(p);//？？？【不应改成numOfPageSig???】该页中页面签名（页面描述符）的个数
        Bits pageSig = newBits(nbitsOfPsig);//制作了一个新的页面签名（页面描述符），用于接收下面拿出的页面签名（页面描述符）（位串）数据本体
        for (int pSigID = 0; pSigID < numOfPageSig; ++pSigID) {//拿出一个个页面签名id（对应第几个页面签名）
            q->nsigs++;//每拿出一个，‘查询’变量中的‘签名个数’成员便+1【疑问：不知为何要做这一步】

            getBits(p, pSigID, pageSig);//内循环遍历拿出目前外循环所在页面中内循环遍历用的一个个页面签名id对应的实际页面签名数据（本质是位串）
            if (isSubset(querySig, pageSig) == TRUE) {//如果查询签名（查询描述符）（本质是位串）与当前取得的页面签名（页面描述符）（位串）匹配
                Offset pageSigOffset = pid * nPageSigPerPage + pSigID;//从所有页面来看，页面签名一共过到第几个了 = pageSigOffset，页面签名过掉的个数=页面过掉的个数，所以不用单独计算过到第几个页面了
//                Offset pageOffset = tupleSigOffset / nTuplePerPage;//页面过掉几个了
                setBit(q->pages, pageSigOffset);//在‘待检查页面’列表（实际是位串）中标记在第几位上（）匹配成功（即标记为1），相当于把匹配到了页面所在页面id加入‘待检查页面’列表中
            }
        }
        q->nsigpages++;//每取出一个存放元组签名的页面，'查询'变量中‘过掉的元祖签名页面个数’成员便+1【疑问：不知为何要做这一步】
    }
    // The printf below is primarily for debugging
    // Remove it before submitting this function
    printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}


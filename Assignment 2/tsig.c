// tsig.c ... functions on Tuple Signatures (tsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

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

// make a tuple signature

Bits makeTupleSig(Reln r, Tuple t)//制作元组签名（元组层级）
{
	assert(r != NULL && t != NULL);
	//TODO
	int nbitsOfTsig = tsigBits(r);//元组签名（元组描述符）位数，参数m
    Bits Tsig = newBits(nbitsOfTsig);//新建元组签名本体，会构建元组签名位数的元组签名出来
    char **attrsArray = tupleVals(r, t);//属性数组
    int nattr = nAttrs(r);//属性个数，参数n

    for (int i = 0; i < nattr; ++i) {//遍历属性个数次
        if (strncmp(attrsArray[i], "?", 1) == 0) { //if we scan "?" in an attribute，即跳过未知属性
            continue;
        }
        else {
            Bits cword = generateCodeWord(attrsArray[i], tsigBits(r), codeBits(r));//对于已知属性，计算出其码字
            orBits(Tsig, cword);//对所有（已知）属性的码字执行OR运算，得到最后的元组签名（元组描述符）【疑问：只针对SIMC吗？】
            freeBits(cword);
        }
    }
    freeVals(attrsArray, nattr);// 这一句不确定需不需要
	return Tsig;
}

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)//基于元组层级的签名索引的查询算法
{
	assert(q != NULL);
	//TODO
	Reln r = q->rel;
    Bits querySig = makeTupleSig(r, q->qstring);//制作查询签名（查询描述符）
    unsetAllBits(q->pages);//Pages = AllZeroBits，制作了一个‘待检查页面’列表（实际是位串）
    Count numOfTupleSigPages = nTsigPages(r);//存放元组签名（元组描述符）的页面个数，参数b
    File tupleSigFile = tsigFile(r);//元组签名（元组描述符）文件本体
    Count nTupleSigPerPage = maxTsigsPP(r);//每个页面可容纳的最大元组签名（元组描述符）个数
    Count nTuplePerPage = maxTupsPP(r);//每个页面可容纳的最大元组个数，参数c
    Count nbitsOfTsig = tsigBits(r);//元组签名（元组描述符）位数，参数m

    for (int pid = 0; pid < numOfTupleSigPages; ++pid) {//遍历 ‘存放元组签名（元组描述符）的页面个数，参数b’ 次。且每次的i值即为pid
        Page p = getPage(tupleSigFile, pid);//从元组签名文件内拿出该pid对应的存放了元组签名的数据页本体
        Count numOfTupleSig = pageNitems(p);//该页面中元组签名（元组描述符）的个数
        Bits tupleSig = newBits(nbitsOfTsig);//制作了一个新的元组签名（元组描述符），用于接收下面拿出的元组签名（元组描述符）（位串）数据本体
        for (int tSigID = 0; tSigID < numOfTupleSig; ++tSigID) {//拿出一个个元组签名id（对应第几个元组签名）
            q->nsigs++;//每拿出一个，‘查询’变量中的‘签名个数’成员便+1【疑问：不知为何要做这一步】

            getBits(p, tSigID, tupleSig);//内循环遍历拿出目前外循环所在页面中内循环遍历用的一个个元组签名id对应的实际元组签名数据（本质是位串）
            if (isSubset(querySig, tupleSig) == TRUE) {//如果查询签名（查询描述符）（本质是位串）与当前取得的元组签名（元组描述符）（位串）匹配
                Offset tupleSigOffset = pid * nTupleSigPerPage + tSigID;//从所有页面来看，元组签名一共过到第几个了 = tupleSigOffset
                Offset pageOffset = tupleSigOffset / nTuplePerPage;//页面过到第几个了
                setBit(q->pages, pageOffset);//在‘待检查页面’列表（实际是位串）中标记在第几位上（）匹配成功（即标记为1），相当于把匹配到了元组所在页面id加入‘待检查页面’列表中
            }
        }
        q->nsigpages++;//每取出一个存放元组签名的页面，'查询'变量中‘过掉的元祖签名页面个数’成员便+1【疑问：不知为何要做这一步】
    }
    // The printf below is primarily for debugging
    // Remove it before submitting this function
    printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}

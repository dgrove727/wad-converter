#include <stdio.h>
#include <stdlib.h>
#include "CarmackCompress.h"

// Taken from DMUtils, released by id Software to the public with no license attached
#define WINDOW_SIZE	4096
#define LENSHIFT 4		// this must be log2(LOOKAHEAD_SIZE)
#define LOOKAHEAD_SIZE	(1<<LENSHIFT)

typedef struct node_struct node_t;

///////////////////////////////////////////////////////////////////////////
//       IMPORTANT: FOLLOWING STRUCTURE MUST BE 16 BYTES IN LENGTH       //
///////////////////////////////////////////////////////////////////////////

struct node_struct
{
    const uint8_t *pointer;
    node_t *prev;
    node_t *next;
    int32_t	pad;
};

typedef struct list_struct
{
    node_t *start;
    node_t *end;
} list_t;

static list_t hashtable[256]; // used for faster encoding
static node_t hashtarget[WINDOW_SIZE]; // what the hash points to

//
//  Adds a node to the hash table at the beginning of a particular index
//  Removes the node in its place before.
//

void addnode(const uint8_t *pointer)
{
    list_t *list;
    int32_t targetindex;
    node_t *target;

    targetindex = reinterpret_cast<uintptr_t>(pointer) & (WINDOW_SIZE - 1);

    // remove the target node at this index

    target = &hashtarget[targetindex];
    if (target->pointer)
    {
        list = &hashtable[*target->pointer];
        if (target->prev)
        {
            list->end = target->prev;
            target->prev->next = 0;
        }
        else
        {
            list->end = 0;
            list->start = 0;
        }
    }

    // add a new node to the start of the hashtable list

    list = &hashtable[*pointer];

    target->pointer = pointer;
    target->prev = 0;
    target->next = list->start;
    if (list->start)
        list->start->prev = target;
    else
        list->end = target;
    list->start = target;

}

void derror(char *msg)
{
    fprintf(stdout, "\nerror: %s\n\n", msg);
}

uint8_t *encode(const uint8_t *input, int32_t inputlen, int32_t *size)
{
    int32_t putidbyte = 0;
    const uint8_t *encodedpos = NULL;
    int32_t encodedlen;
    int32_t i, pacifier = 0;
    int32_t len;
    int32_t numbytes, numcodes;
    int32_t codelencount;
    const uint8_t *window;
    const uint8_t *lookahead;
    uint8_t *idbyte = NULL;
    uint8_t *output, *ostart;
    node_t *hashp;
    int32_t lookaheadlen;
    int32_t samelen;

    // initialize the hash table to the occurences of bytes
    for (i = 0; i < 256; i++)
    {
        hashtable[i].start = 0;
        hashtable[i].end = 0;
    }

    // initialize the hash table target
    for (i = 0; i < WINDOW_SIZE; i++)
    {
        hashtarget[i].pointer = 0;
        hashtarget[i].next = 0;
        hashtarget[i].prev = 0;
    }

    int32_t allocSize = (inputlen * 9) / 8 + 1;

    // create the output
    ostart = output = (uint8_t *)malloc(allocSize);

    // initialize the window & lookahead
    lookahead = window = input;

    numbytes = numcodes = codelencount = 0;

    while (inputlen > 0)
    {

        // set the window position and size
        window = lookahead - WINDOW_SIZE;
        if (window < input) window = input;

        // decide whether to allocate a new id byte
        if (!putidbyte)
        {
            idbyte = output++;
            *idbyte = 0;
        }
        putidbyte = (putidbyte + 1) & 7;

        // go through the hash table of linked lists to find the strings
        // starting with the first character in the lookahead

        encodedlen = 0;
        lookaheadlen = inputlen < LOOKAHEAD_SIZE ? inputlen : LOOKAHEAD_SIZE;

        hashp = hashtable[lookahead[0]].start;
        while (hashp)
        {

            samelen = 0;
            len = lookaheadlen;
            while (len-- && hashp->pointer[samelen] == lookahead[samelen])
                samelen++;
            if (samelen > encodedlen)
            {
                encodedlen = samelen;
                encodedpos = hashp->pointer;
            }
            if (samelen == lookaheadlen) break;

            hashp = hashp->next;
        }

        // encode the match and specify the length of the encoding
        if (encodedlen >= 3)
        {
            *idbyte = (*idbyte >> 1) | 0x80;
            *output++ = ((lookahead - encodedpos - 1) >> LENSHIFT);
            *output++ = ((lookahead - encodedpos - 1) << LENSHIFT) | (encodedlen - 1);
            numcodes++;
            codelencount += encodedlen;
        }
        else { // or just store the unmatched byte
            encodedlen = 1;
            *idbyte = (*idbyte >> 1);
            *output++ = *lookahead;
            numbytes++;
        }

        // update the hash table as the window slides
        for (i = 0; i < encodedlen; i++)
            addnode(lookahead++);

        // reduce the input size
        inputlen -= encodedlen;

        // print pacifier dots
        pacifier -= encodedlen;
        if (pacifier <= 0)
        {
//            fprintf(stdout, ".");
            pacifier += 10000;
        }

    }

    // done with encoding- now wrap up

    if (inputlen != 0)
        fprintf(stdout, "warning: inputlen != 0\n");

    // put the end marker on the file
    if (!putidbyte)
    {
        idbyte = output++;
        *idbyte = 1;
    }
    else
        *idbyte = ((*idbyte >> 1) | 0x80) >> (7 - putidbyte);

    *output++ = 0;
    *output++ = 0;

    *size = output - ostart;

    /*
      fprintf(stdout, "\nnum bytes = %d\n", numbytes);
      fprintf(stdout, "num codes = %d\n", numcodes);
      fprintf(stdout, "ave code length = %f\n", (double) codelencount/numcodes);
      fprintf(stdout, "size = %d\n", *size);
    */

    if (*size > allocSize)
        return NULL; // Compressed size is larger than uncompressed size... this is not allowed.

    return ostart;

}

//
//  Return the size of compressed data
//

int decodedsize(uint8_t *input)
{

    uint16_t getidbyte = 0;
    int16_t len;
    uint8_t idbyte;
    int32_t accum = 0;

    while (1)
    {

        // get a new idbyte if necessary
        if (!getidbyte) idbyte = *input++;
        getidbyte = (getidbyte + 1) & 7;

        if (idbyte & 1)
        {
            // decompress
            input++;
            len = *input++ & 0xf;
            if (!len) break;
            accum += len + 1;
        }
        else
            accum++;
        input++;

        idbyte = idbyte >> 1;

    }

    return accum;

}

void decode(uint8_t *input, uint8_t *output)
{

    int32_t getidbyte = 0;
    int32_t len;
    int32_t pos;
    int32_t i;
    uint8_t *source;
    int32_t idbyte;

    while (1)
    {

        // get a new idbyte if necessary
        if (!getidbyte) idbyte = *input++;
        getidbyte = (getidbyte + 1) & 7;

        if (idbyte & 1)
        {
            // decompress
            pos = *input++ << LENSHIFT;
            pos = pos | (*input >> LENSHIFT);
            source = output - pos - 1;
            len = (*input++ & 0xf) + 1;
            if (len == 1) break;
            for (i = 0; i < len; i++)
                *output++ = *source++;
        }
        else {
            *output++ = *input++;
        }

        idbyte = idbyte >> 1;

    }

}

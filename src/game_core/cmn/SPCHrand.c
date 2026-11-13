#include "indep/src/speech/SEDWedge.hpp"

#include <stdlib.h>

static void requireRandomState(const SpeechRandomState* state)
{
    if (!state) abort();
}

uint32_t spchRandomNext(SpeechRandomState* state)
{
    requireRandomState(state);
    uint32_t value=state->words[5]+state->words[4];
    uint32_t carry=value<state->words[5] || value<state->words[4];
    state->words[4]=value;
    for (int32_t index=3;index>=0;--index) {
        const uint32_t previous=state->words[index];
        value=value+previous+carry;
        carry=value<previous;
        state->words[index]=value;
    }
    ++state->words[5];
    if (!state->words[5]) {
        for (int32_t index=4;index>=0;--index) {
            ++state->words[index];
            if (state->words[index]) break;
        }
    }
    return state->words[0];
}

void spchRandomSeed(SpeechRandomState* state,uint32_t seed)
{
    if (!state) abort();
    static const uint32_t increments[6]={
        0xF22D0E56u,0x96041893u,0x3DF3B646u,
        0x40DDE76Du,0x97327AE1u,0xD1A9FBE7u
    };
    for (size_t index=0;index<6;++index) {
        seed+=increments[index];
        state->words[index]=seed;
    }
}

int32_t spchRandomFindInQueue(const SpeechRandomState* state,uint16_t value,
    int32_t lookback,uint16_t category)
{
    requireRandomState(state);
    if (state->writeIndex<0 || state->writeIndex>=32) abort();
    const int32_t limit=lookback<32 ? lookback : 32;
    int32_t cursor=state->writeIndex;
    int32_t matchingCategory=0;
    // The release's physical traversal cap is33. Matching-category entries
    // alone consume the requested lookback count.
    for (int32_t examined=0;examined<33;++examined) {
        if (matchingCategory>=limit) break;
        if (state->history[cursor][1]==category) {
            ++matchingCategory;
            if (state->history[cursor][0]==value) return matchingCategory-1;
        }
        --cursor;
        if (cursor<0) cursor+=32;
    }
    return -1;
}

void spchRandomAddToQueue(SpeechRandomState* state,uint16_t value,uint16_t category)
{
    requireRandomState(state);
    if (state->writeIndex<0 || state->writeIndex>=32) abort();
    ++state->writeIndex;
    if (state->writeIndex>=32) state->writeIndex=0;
    state->history[state->writeIndex][0]=value;
    state->history[state->writeIndex][1]=category;
}

void spchRandomInitialize(SpeechRandomState* state,uint32_t seed)
{
    if (!state) abort();
    for (size_t index=0;index<32;++index) {
        state->history[index][0]=UINT16_MAX;
        state->history[index][1]=UINT16_MAX;
    }
    state->writeIndex=0;
    spchRandomSeed(state,seed+(seed<<16));
}

uint32_t spchRandomPick(SpeechRandomState* state,int32_t bound,uint16_t category)
{
    requireRandomState(state);
    if (bound<0) abort();
    const int32_t half=bound/2;
    const int32_t lookback=half<10 ? half : 10;
    uint32_t selected=((spchRandomNext(state)>>16)*(uint32_t)bound)>>16;
    uint32_t candidate=selected;
    int32_t bestAge=-1;
    for (int32_t attempt=0;attempt<32;++attempt) {
        candidate=((spchRandomNext(state)>>16)*(uint32_t)bound)>>16;
        const int32_t age=spchRandomFindInQueue(state,(uint16_t)candidate,lookback,category);
        if (age==-1) {
            selected=candidate;
            break;
        }
        if (age>bestAge) {
            bestAge=age;
            selected=candidate;
        }
    }
    spchRandomAddToQueue(state,(uint16_t)selected,category);
    // PAL295894 returns the last candidate, although the history receives
    // the selected best-age candidate. Preserve both distinct source writes.
    return candidate;
}

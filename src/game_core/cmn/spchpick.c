#include "indep/src/speech/SEDWedge.hpp"

#include <stdlib.h>

void spchInitializeSentenceChoice(SpeechSentenceChoice* choice)
{
    if (!choice) abort();
    choice->index=-1;
    choice->values[0]=0;
    choice->values[1]=0;
    for (size_t index=0;index<8;++index) {
        SpeechPhraseChoice* phrase=&choice->phrases[index];
        phrase->values[0]=0;
        phrase->values[1]=0;
        phrase->flags[0]=0;
        phrase->flags[1]=0;
        phrase->flags[2]=0;
    }
    for (size_t index=0;index<48;++index) {
        SpeechChoiceRow* row=&choice->rows[index];
        row->first=-1;
        row->second=-1;
        row->values[0]=0;
        row->values[1]=0;
    }
}

void spchSetPreloadTicks(SpeechEngineState* engine,uint32_t ticks)
{
    if (!engine) abort();
    engine->preloadTicks=ticks;
}

#include <string.h>

void spchGetEventDatabaseInfo(SpeechBytes database,uint32_t* first,uint32_t* second)
{
    if (!first || !second) abort();
    *first=spchRead8(database,8);*second=spchRead8(database,9);
}
void spchSetMatchParameter(SpeechEngineState* engine,int32_t parameter,uint32_t value)
{
    engine->matchParameter=parameter;engine->matchValue=value;
}
uint32_t spchGetMatchParameter(const SpeechEngineState* engine,int32_t parameter)
{
    return engine->matchParameter==parameter?engine->matchValue:0;
}
void spchClearMatchParameter(SpeechEngineState* engine)
{
    engine->matchParameter=-1;engine->matchValue=0;
}
int32_t spchMatchSample(const SpeechSample* sample,SpeechBytes phrase,const uint32_t parameters[20])
{
    const int32_t count=(int8_t)spchRead8(phrase,4);
    if (count>4) return 0;
    for (int32_t i=0;i<count;++i) {
        const uint32_t value=sample->parameters[i];
        if (value>=32) return 0;
        const uint32_t bit=1u<<value;
        const uint8_t parameter=spchRead8(phrase,8u+(uint32_t)i);
        if (!(bit&spchRead32(phrase,16u+(uint32_t)i*4u))) return 0;
        if (parameter && parameter!=255 && parameter!=254 &&
            (parameter>=20 || !(parameters[parameter]&bit))) return 0;
    }
    return 1;
}
int32_t spchGetPhraseBank(SpeechEngineState* engine,SpeechBytes phrase,const uint32_t parameters[20],SpeechPhrasePick* pick)
{
    pick->bank=-1;pick->subBank=-1;
    const uint16_t bank=spchRead16(phrase,0);
    const uint32_t parameter=spchRead8(phrase,2),kind=spchRead8(phrase,3);
    if (kind==0) pick->bank=(int16_t)spchBankFind(&engine->banks,bank);
    else if (kind==1) {
        if (parameter>=20) return 0;
        pick->bank=(int16_t)spchBankFindSubBank(&engine->banks,bank,(uint16_t)parameters[parameter]);
    } else if (kind==2) {
        if (parameter>=20) return 0;
        const int32_t index=spchBankFind(&engine->banks,bank);
        if (spchBankTestSubBankBounds(&engine->banks,index,parameters[parameter])) {
            pick->bank=(int16_t)index;pick->subBank=(int16_t)(uint16_t)parameters[parameter];
        }
    }
    return pick->bank>=0;
}
int32_t spchTestBit(SpeechBytes bits,int32_t index)
{
    if (index<0 || (uint32_t)index/8u>=bits.size) return 0;
    return spchRead8(bits,(uint32_t)index/8u)&(1u<<((uint32_t)index&7u));
}
static uint32_t bankTail49(SpeechBytes bank)
{
    const uint32_t flags=spchRead8(bank,4);
    return 12u+(uint32_t)spchRead8(bank,5)*((flags&15u)+2u)+((flags&112u)>>3);
}
void spchClearCycleBit(SpeechBankView* bank,int32_t index)
{
    if (!bank || index<0) return;
    const SpeechBytes bytes={bank->bytes,bank->size};
    const uint32_t at=bankTail49(bytes)+1u+(uint32_t)index/8u;
    if (at>=bank->size) return;
    bank->bytes[at]&=(uint8_t)~(1u<<((uint32_t)index&7u));
}
int32_t spchCheckTemplateSample(const SpeechPhrasePick* pick,const SpeechBankView* bank,int32_t sample)
{
    const SpeechBytes bytes={bank->bytes,bank->size};
    if (pick->subBank<0 || pick->subBank>=spchRead16(bytes,10) || sample<0) return 0;
    const uint32_t count=spchRead8(bytes,5),flags=spchRead8(bytes,4),queue=spchRead8(bytes,6);
    uint32_t at=bankTail49(bytes);
    if (flags&128u) at+=((count+7u)/8u)+1u;
    if (queue) at+=queue+1u;
    return spchTestBit(spchBytesSlice(bytes,at,0),(int32_t)(count*(uint32_t)pick->subBank+(uint32_t)sample));
}
int32_t spchSampleExists(const SpeechPhrasePick* pick,const SpeechBankView* bank,int32_t sample)
{
    if (!bank || !bank->bytes || bank->size<12 || sample<0) return 0;
    const SpeechBytes bytes={bank->bytes,bank->size};
    if ((uint32_t)sample>spchRead8(bytes,5)) return 0;
    if (pick->subBank!=-1 && !spchCheckTemplateSample(pick,bank,sample)) return 0;
    if (spchRead8(bytes,4)&128u)
        return spchTestBit(spchBytesSlice(bytes,bankTail49(bytes)+1u,0),sample);
    return 1;
}
int32_t spchChooseSamples(SpeechEngineState* engine,SpeechSentencePick* sentence,SpeechPhrasePick* pick,
    SpeechBytes phrase,const uint32_t parameters[20])
{
    const SpeechBankView* bank=spchBankGet(&engine->banks,pick->bank);
    if (!bank || !bank->bytes || bank->size<12) return 0;
    const SpeechBytes bytes={bank->bytes,bank->size};
    int32_t chosen=0,oldest=0,oldestIndex=-1;
    for (uint32_t i=0;i<spchRead8(bytes,5);++i) {
        SpeechSample sample;
        if (!spchUnpackSample(bank,(int32_t)i,&sample) || !spchMatchSample(&sample,phrase,parameters) ||
            !spchSampleExists(pick,bank,(int32_t)i)) continue;
        int32_t age=0;
        if (spchBankGetSampleTimeInQueue(bank,(uint8_t)i,&age)!=SPEECH_DATABASE_OK) continue;
        if (age>0) {if (age>oldest) {oldest=age;oldestIndex=(int32_t)i;}}
        else if (sentence->sampleCount<100) {sentence->samples[sentence->sampleCount++]=(uint8_t)i;++chosen;}
        else engine->operationResult=SPEECH_DATABASE_CAPACITY;
    }
    if (!chosen && oldestIndex!=-1 && sentence->sampleCount<100) {
        sentence->samples[sentence->sampleCount++]=(uint8_t)oldestIndex;++chosen;
    }
    return chosen;
}
int32_t spchConvertTime(const SpeechEngineState* engine,uint32_t bytes)
{
    if (!engine->bytesPerSecond) return 0;
    const uint32_t wrapped=bytes*100u;
    int32_t product,divisor;memcpy(&product,&wrapped,4);memcpy(&divisor,&engine->bytesPerSecond,4);
    if (product==INT32_MIN && divisor==-1) return INT32_MIN;
    return product/divisor;
}
int32_t spchSentenceLength(SpeechEngineState* engine,SpeechBytes sentence,const SpeechSentencePick* pick)
{
    uint32_t bytes=0;
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (count>12) return 0;
    for (uint32_t i=0;i<count;++i) {
        const SpeechPhrasePick* phrase=&pick->phrases[i];SpeechSample sample;
        if (phrase->choice>=pick->sampleCount) return 0;
        if (spchUnpackSample(spchBankGet(&engine->banks,phrase->bank),pick->samples[phrase->choice],&sample)) bytes+=sample.bytes;
    }
    return spchConvertTime(engine,bytes);
}
uint32_t spchDecodeWeight(uint8_t weight)
{
    /* Final PAL32B270 exact powers, no beta data imported. */
    static const uint32_t multipliers[8]={1,4,16,64,256,1024,4096,16384};
    return (weight&31u)*multipliers[weight>>5];
}
int32_t spchOrderSentences(SpeechEngineState* engine,SpeechBytes event,uint8_t order[100])
{
    const uint32_t count=spchRead8(event,6);
    if (count>100) {engine->operationResult=SPEECH_DATABASE_CAPACITY;return 0;}
    uint32_t weights[100],total=0,used=0;
    for (uint32_t i=0;i<count;++i) {
        const SpeechBytes sentence=spchBytesSlice(event,(uint32_t)spchRead16(event,12u+i*2u)*4u,4);
        if (!sentence.bytes) return 0;
        weights[i]=spchDecodeWeight(spchRead8(sentence,0));total+=weights[i];
    }
    while ((int32_t)total>0) {
        int32_t random=(int32_t)spchRandomPick(&engine->random,(int32_t)total,UINT16_MAX);
        uint32_t i=0;
        for (;i<count;++i) {random-=(int32_t)weights[i];if (random<0) break;}
        if (i>=count) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return 0;}
        order[used++]=(uint8_t)i;total-=weights[i];weights[i]=0;
    }
    for (uint32_t i=0;i<count;++i) {
        const SpeechBytes sentence=spchBytesSlice(event,(uint32_t)spchRead16(event,12u+i*2u)*4u,4);
        if (!spchDecodeWeight(spchRead8(sentence,0))) order[used++]=(uint8_t)i;
    }
    return (int32_t)used;
}
int32_t spchRepeatEvent(const SpeechEngineState* engine,SpeechBytes event,int32_t channel)
{
    if ((uint32_t)channel>=8) return 0;
    const WorldAudioMessageChannel* previous=&engine->messages->channels[channel];
    const int32_t maximum=(int8_t)spchRead8(event,8);
    return (uint16_t)previous->id!=spchRead16(event,0) || maximum<=0 || (int32_t)previous->state<maximum;
}
int32_t spchShortRuleStatus(SpeechBytes sentence,int32_t filter)
{
    const uint32_t rule=spchRead8(sentence,3)&3u;
    if (rule==1) return filter==2;
    if (rule==2) return filter!=2;
    return rule==0;
}
int32_t spchGetBankParameterId(const SpeechEngineState* engine,int32_t bankIndex,int32_t parameter)
{
    const SpeechBankView* bank=spchBankGet(&engine->banks,bankIndex);
    if (!bank) return -1;
    const SpeechBytes bytes={bank->bytes,bank->size};
    const uint32_t flags=spchRead8(bytes,4),count=(flags&112u)>>4;
    const uint32_t start=12u+(uint32_t)spchRead8(bytes,5)*((flags&15u)+2u);
    for (uint32_t i=0;i<count;++i) {
        const uint32_t packed=((uint32_t)spchRead8(bytes,start+i*2u)<<8)|spchRead8(bytes,start+i*2u+1u);
        if ((int32_t)(packed>>13)==parameter) return (int32_t)(packed&8191u);
    }
    return -1;
}
int32_t spchSentenceGetChoices(SpeechEngineState* engine,SpeechSentencePick* pick,SpeechBytes sentence,
    const uint32_t parameters[20],uint8_t rules,uint8_t unknownRules,int32_t filter)
{
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (count>12 || !spchShortRuleStatus(sentence,filter) || !spchCheckSentenceRules(rules,unknownRules,sentence)) return 0;
    pick->phraseCount=(uint8_t)count;
    for (uint32_t i=0;i<count;++i) {
        SpeechPhrasePick* part=&pick->phrases[i];part->begin=pick->sampleCount;
        const SpeechBytes phrase=spchBytesSlice(sentence,(uint32_t)spchRead8(sentence,4u+i)*4u,16);
        if (!phrase.bytes || !spchGetPhraseBank(engine,phrase,parameters,part)) return 0;
        const int32_t countSamples=spchChooseSamples(engine,pick,part,phrase,parameters);
        if (!countSamples) return 0;
        part->count=(uint8_t)countSamples;
    }
    return 1;
}
void spchRandomizePhrasePicks(SpeechEngineState* engine,uint8_t* samples,int32_t count)
{
    for (int32_t i=0;i<count/2;++i) {
        const uint32_t j=spchRandomPick(&engine->random,count,(uint16_t)count);
        const uint8_t value=samples[i];samples[i]=samples[j];samples[j]=value;
    }
}
void spchRandomizeSentencePicks(SpeechEngineState* engine,SpeechBytes sentence,SpeechSentencePick* pick)
{
    const uint32_t count=spchRead8(sentence,3)>>2;
    for (uint32_t i=0;i<count && i<12;++i) {
        SpeechPhrasePick* phrase=&pick->phrases[i];uint8_t* samples=pick->samples+phrase->begin;
        for (uint32_t j=0;j<phrase->count/2u;++j) {
            const uint32_t k=spchRandomPick(&engine->random,phrase->count,(uint16_t)phrase->bank);
            const uint8_t value=samples[j];samples[j]=samples[k];samples[k]=value;
        }
    }
}
int32_t spchIterateChoice(SpeechBytes sentence,SpeechSentencePick* pick)
{
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (!count || count>12) return 1;
    for (int32_t i=(int32_t)count-1;i>=0;--i) {
        SpeechPhrasePick* phrase=&pick->phrases[i];++phrase->choice;
        if (phrase->choice<(uint32_t)phrase->begin+phrase->count) return 0;
        phrase->choice=phrase->begin;
    }
    return 1;
}
int32_t spchChooseShortSentence(SpeechEngineState* engine,SpeechBytes sentence,SpeechSentencePick* pick,int32_t limit)
{
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (count>12) return 0;
    for (uint32_t i=0;i<count;++i) pick->phrases[i].choice=pick->phrases[i].begin;
    do {if (spchSentenceLength(engine,sentence,pick)<limit) return 1;}
    while (!spchIterateChoice(sentence,pick));
    return 0;
}
int32_t spchPickPhraseSample(SpeechEngineState* engine,SpeechSentencePick* sentence,SpeechBytes phrase,SpeechPhrasePick* pick)
{
    const SpeechBankView* bank=spchBankGet(&engine->banks,pick->bank);
    const int32_t count=(int8_t)spchRead8(phrase,4);
    int32_t chosen=0,special=0;
    if (count>4 || !bank || !pick->count) return 0;
    for (int32_t i=0;i<count && !chosen;++i) {
        const uint32_t kind=spchRead8(phrase,8u+(uint32_t)i);
        if (kind==254) {
            special=1;
            pick->choice=(uint8_t)(pick->begin+spchRandomPick(&engine->random,pick->count,(uint16_t)pick->bank));
            const int32_t parameter=spchGetBankParameterId(engine,pick->bank,i);SpeechSample sample;
            if (spchUnpackSample(bank,sentence->samples[pick->choice],&sample)) {
                const uint8_t value=sample.parameters[i];
                spchSetMatchParameter(engine,parameter,value==255?0:1u<<(value&31u));
            }
            chosen=1;
        } else if (kind==255) {
            special=1;const int32_t parameter=spchGetBankParameterId(engine,pick->bank,i);
            const uint32_t mask=spchGetMatchParameter(engine,parameter);
            if (pick->count>100) return 0;
            spchRandomizePhrasePicks(engine,sentence->samples+pick->begin,pick->count);
            for (uint32_t j=0;j<pick->count;++j) {
                const uint8_t index=(uint8_t)(pick->begin+j);SpeechSample sample;
                if (spchUnpackSample(bank,sentence->samples[index],&sample)) {
                    const uint8_t value=sample.parameters[i];
                    if (value!=255 && ((1u<<(value&31u))&mask)) {pick->choice=index;chosen=1;}
                }
            }
        }
    }
    if (!chosen && !special) {
        pick->choice=(uint8_t)(pick->begin+spchRandomPick(&engine->random,pick->count,(uint16_t)pick->bank));chosen=1;
    }
    return chosen;
}
int32_t spchSentenceMakeChoice(SpeechEngineState* engine,SpeechSentencePick* pick,SpeechBytes sentence,int32_t filter,int32_t limit)
{
    spchClearMatchParameter(engine);
    if (filter==1) {
        spchRandomizeSentencePicks(engine,sentence,pick);
        return spchChooseShortSentence(engine,sentence,pick,limit);
    }
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (count>12) return 0;
    for (uint32_t i=0;i<count;++i) {
        if (!pick->phrases[i].count) return 0;
        SpeechBytes phrase=spchBytesSlice(sentence,(uint32_t)spchRead8(sentence,4u+i)*4u,16);
        if (!phrase.bytes || !spchPickPhraseSample(engine,pick,phrase,&pick->phrases[i])) return 0;
    }
    return 1;
}
void spchInitializeSentencePick(SpeechSentencePick* pick)
{
    pick->phraseCount=0;pick->sampleCount=0;
}
int32_t spchSaveChosenSentence(SpeechEngineState* engine,const SpeechSentencePick* pick,int32_t channel,
    SpeechBytes event,SpeechBytes sentence,uint8_t ordinal,const uint32_t parameters[20])
{
    SpeechSentenceChoice* choice=&engine->sentenceChoice;
    if ((choice->index>=0 && choice->index!=channel) || choice->values[0]>=8) return 0;
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (count>12 || choice->values[1]>48u-count) return 0;
    const SpeechEventDatabaseView* database=spchFindEventDatabase(engine,parameters[0]);
    if (!database) return 0;
    const uintptr_t begin=(uintptr_t)database->bytes,finish=begin+database->size;
    const uintptr_t eventAddress=(uintptr_t)event.bytes,sentenceAddress=(uintptr_t)sentence.bytes;
    if (eventAddress<begin || eventAddress>=finish || sentenceAddress<begin || sentenceAddress>=finish) return 0;
    SpeechPhraseChoice* retained=&choice->phrases[choice->values[0]++];
    choice->index=channel;retained->values[0]=(uint32_t)(eventAddress-begin);retained->values[1]=(uint32_t)(sentenceAddress-begin);
    retained->database=(uint8_t)(database-engine->databases.databases);retained->flags[0]=ordinal;
    retained->flags[1]=1;memcpy(retained->parameters,parameters,sizeof(retained->parameters));
    for (uint32_t i=0;i<count;++i) {
        const SpeechPhrasePick* phrase=&pick->phrases[i];
        if (phrase->choice>=pick->sampleCount) return 0;
        SpeechChoiceRow* row=&choice->rows[choice->values[1]+i];
        row->first=phrase->bank;row->second=phrase->subBank;row->values[0]=pick->samples[phrase->choice];
    }
    choice->values[1]+=count;return 1;
}
int32_t spchOneChosen(const SpeechEngineState* engine,int32_t channel)
{
    return engine && engine->sentenceChoice.index==channel && (int32_t)engine->sentenceChoice.values[0]>0;
}
int32_t spchTestValidParameter(uint32_t descriptor,uint32_t value,uint32_t index)
{
    const uint32_t type=(descriptor>>((index*2u)&31u))&3u;
    return type==1?value!=0:type==2?value!=UINT32_MAX:0;
}
int32_t spchChooseSentence(SpeechEngineState* engine,const uint32_t parameters[20])
{
    SpeechBytes event=spchFindEvent(engine,parameters[0]);uint32_t channel=0;
    if (!event.bytes || !spchFindEventChannel(engine,parameters[0],&channel) || channel>=8 || !spchRepeatEvent(engine,event,(int32_t)channel)) return 0;
    int32_t filter=(int32_t)engine->eventCounters[channel];
    if (engine->messages->selectionState==1) {++filter;if (filter>=3) filter=2;}
    if (!(spchRead8(event,10)&1u) && filter==1) filter=0;
    const int32_t limit=filter?(int32_t)spchGetFilterLength(engine,parameters[0]):0;
    uint8_t unknown=0;const uint8_t rules=spchGetRuleSettings(engine,parameters[0],event,parameters,&unknown);
    spchEventLogTryEvent(engine,event,parameters,rules);
    uint8_t order[100];const int32_t ordered=spchOrderSentences(engine,event,order);
    const uint32_t count=spchRead8(event,6);
    if (!count) return -1;
    if (ordered!=(int32_t)count) return 0;
    const int32_t parameterFilter=(spchRead8(event,10)>>3)&1;
    uint16_t parameterMask=0;
    const uint32_t rulesCount=(uint32_t)(int32_t)(int8_t)spchRead8(event,7);
    const uint32_t filterOffset=12u+((count*2u+3u)&~3u)+((rulesCount*3u+3u)&~3u);
    if (parameterFilter) {
        const uint32_t descriptor=spchRead32(event,filterOffset);
        /* Source accumulates only16 bits. Its last four reads cannot affect
         * that mask; native code does not read past the20-word argument. */
        for (uint32_t i=0;i<16;++i) if (spchTestValidParameter(descriptor,parameters[i+1],i)) parameterMask|=(uint16_t)(1u<<i);
    }
    for (uint32_t i=0;i<count;++i) {
        const uint32_t ordinal=order[i];
        if (ordinal>=count) return 0;
        SpeechBytes sentence=spchBytesSlice(event,(uint32_t)spchRead16(event,12u+ordinal*2u)*4u,4);
        if (!sentence.bytes) return 0;
        if (parameterFilter && spchRead16(event,filterOffset+4u+ordinal*2u)!=parameterMask) continue;
        SpeechSentencePick pick;memset(&pick,0,sizeof(pick));spchInitializeSentencePick(&pick);
        if (!spchSentenceGetChoices(engine,&pick,sentence,parameters,rules,unknown,filter) ||
            !spchSentenceMakeChoice(engine,&pick,sentence,filter,limit)) continue;
        if (!spchSaveChosenSentence(engine,&pick,(int32_t)channel,event,sentence,(uint8_t)ordinal,parameters)) return 0;
        spchEventLogEventChoice(engine,event,ordinal,1);
        spchEventLogSentenceChoice(engine,event,sentence,parameters);spchEventLogRuleSettings(engine,rules);
        return 1;
    }
    return 0;
}
int32_t spchChooseSingleSentence(SpeechEngineState* engine,int32_t index)
{
    if (!engine || !engine->messages || (uint32_t)index>=16) return 0;
    const WorldAudioMessage* event=&engine->messages->pending[index];
    uint32_t parameters[20];parameters[0]=event->category;memcpy(parameters+1,event->parameters,sizeof(event->parameters));
    const int32_t result=spchChooseSentence(engine,parameters);
    if (!result && engine->reparameterizeOwner) engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;
    /* No custom reparameterizer is installed by final SED. Unsupported typed
     * bindings are retained as an explicit open API, never silently called. */
    return result>=0?result:0;
}
void spchConstantRuleSet(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,SpeechBytes sentence,uint32_t firstRow)
{
    if (!engine->ruleSetterOwner) return;
    uint32_t database=0;(void)spchGetDatabaseId(engine,eventId,&database);
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (firstRow>48 || count>48-firstRow) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return;}
    for (uint32_t i=0;i<count;++i) {
        const SpeechBytes phrase=spchBytesSlice(sentence,(uint32_t)spchRead8(sentence,4u+i)*4u,16);
        for (uint32_t parameter=0;parameter<4;++parameter) {
            const uint8_t rule=spchRead8(phrase,12u+parameter);
            if (rule==255) continue;
            const int32_t id=spchGetRuleId(event,rule);
            const SpeechChoiceRow* row=&engine->sentenceChoice.rows[firstRow+i];SpeechSample sample;
            if (spchUnpackSample(spchBankGet(&engine->banks,row->first),row->values[0],&sample)) {
                /* Exact typed SED SetRule callback has no stores. The actual
                 * rule/sample dependency is still decoded and validated. */
                (void)id;(void)database;(void)sample.parameters[parameter];
            }
        }
    }
}
int32_t spchMakeSampleRequests(SpeechEngineState* engine,SpeechBytes sentence,uint32_t eventId,uint32_t firstRow)
{
    if (!engine->sentenceOwner) {engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;return 0;}
    uint32_t database=0,channel=0,bytes=0;
    (void)spchGetDatabaseId(engine,eventId,&database);(void)spchFindEventChannel(engine,eventId,&channel);
    const uint32_t count=spchRead8(sentence,3)>>2;
    if (firstRow>48 || count>48-firstRow) return 0;
    for (uint32_t i=0;i<count;++i) {
        const SpeechChoiceRow* row=&engine->sentenceChoice.rows[firstRow+i];
        const SpeechBankView* view=spchBankGet(&engine->banks,row->first);
        if (!view) continue;
        SpeechBankView* bank=&engine->banks.banks[row->first];const SpeechBytes data={bank->bytes,bank->size};
        if ((spchRead8(data,4)&128u) && engine->consumeCycleBits) spchClearCycleBit(bank,row->values[0]);
        SpeechSample sample;
        if (!spchUnpackSample(bank,row->values[0],&sample)) continue;
        uint32_t offset=sample.offset;
        if (row->second!=-1) offset+=(uint32_t)(int32_t)row->second*(uint32_t)spchRead16(data,8)*(((uint32_t)spchRead8(data,7)+1u)<<8);
        const uint32_t request[7]={(uint32_t)(int32_t)row->first,offset,sample.bytes,eventId,channel,spchRead16(data,2),database};
        bytes+=sample.bytes;
        if (worldStreamAudioSentencePartRequest(engine->sentenceOwner,request,7)<0) {engine->operationResult=SPEECH_DATABASE_INVALID_BINDING;return 0;}
        (void)spchBankAddToQueue(bank,(uint8_t)row->values[0]);spchEventLogSampleRequest(engine,bank,row,request);
    }
    return spchConvertTime(engine,bytes);
}
int32_t spchPlayChosen(SpeechEngineState* engine,int32_t channel)
{
    uint32_t samples=0,totalTime=0;
    if (spchOneChosen(engine,channel)) {
        const uint32_t count=engine->sentenceChoice.values[0];
        if (count>8 || (uint32_t)channel>=8) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return 0;}
        for (uint32_t i=0;i<count;++i) {
            const SpeechPhraseChoice* phrase=&engine->sentenceChoice.phrases[i];
            if (phrase->database>=8) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;break;}
            const SpeechEventDatabaseView* database=&engine->databases.databases[phrase->database];
            const SpeechBytes bytes={database->bytes,database->size};
            const SpeechBytes event=spchBytesSlice(bytes,phrase->values[0],12),sentence=spchBytesSlice(bytes,phrase->values[1],4);
            if (!event.bytes || !sentence.bytes) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;break;}
            const uint32_t eventId=phrase->parameters[0];
            spchRuleSet(engine,eventId,event,phrase->flags[0],phrase->parameters);
            spchConstantRuleSet(engine,eventId,event,sentence,samples);
            totalTime+=(uint32_t)spchMakeSampleRequests(engine,sentence,eventId,samples);
            samples+=spchRead8(sentence,3)>>2;
            if (i==0) {
                WorldAudioMessageChannel* previous=&engine->messages->channels[channel];
                if ((uint16_t)previous->id==(uint16_t)eventId) ++previous->state;
                else {previous->id=(int16_t)(uint16_t)eventId;previous->databaseId=(uint16_t)(eventId>>16);previous->state=1;}
            }
        }
    }
    (void)totalTime;spchInitializeSentenceChoice(&engine->sentenceChoice);return (int32_t)samples;
}

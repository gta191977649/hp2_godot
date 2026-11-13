#include "indep/src/speech/SEDWedge.hpp"

/* PAL2950A0..29551C. Source SDK rules use eight bits and up to20 argument
 * selectors. SED's actual TestRule205A18 returns zero; SetRule205A20 has no
 * effects. These release callback roles are applied directly, with no fake
 * callback function or second rules store. */
int32_t spchSentenceUsesParameter(SpeechBytes sentence,uint32_t parameter)
{
    if (!sentence.bytes || sentence.size<4) return 0;
    const uint32_t count=spchRead8(sentence,3)>>2;
    for (uint32_t i=0;i<count;++i) {
        SpeechBytes phrase=spchBytesSlice(sentence,(uint32_t)spchRead8(sentence,4u+i)*4u,12);
        if (!phrase.bytes) return 0;
        if (spchRead8(phrase,2)==parameter) return 1;
        for (uint32_t j=0;j<4;++j) if (spchRead8(phrase,8u+j)==parameter) return 1;
    }
    return 0;
}
int32_t spchGetRuleId(SpeechBytes event,uint32_t index)
{
    if (!event.bytes || event.size<12 || index>=8) return -1;
    const uint32_t offset=12u+(((uint32_t)spchRead8(event,6)*2u+3u)&~3u)+index*3u;
    if (!spchBytesSlice(event,offset,3).bytes) return -1;
    return spchRead8(event,offset);
}
uint8_t spchGetRuleSettings(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,
    const uint32_t parameters[20],uint8_t* unknown)
{
    (void)eventId;
    if (!engine || !parameters || !unknown || !event.bytes || event.size<12) return 0;
    const int32_t count=(int8_t)spchRead8(event,7);
    const uint32_t begin=12u+(((uint32_t)spchRead8(event,6)*2u+3u)&~3u);
    if (count>8 || (count>0 && !spchBytesSlice(event,begin,(uint32_t)count*3u).bytes)) {
        engine->operationResult=SPEECH_DATABASE_INVALID_DATA;*unknown=0;return 0;
    }
    uint8_t accepted=0,unresolved=0;
    for (uint32_t parameter=1;parameter<=20;++parameter) {
        for (int32_t rule=0;rule<count;++rule) {
            const uint32_t offset=begin+(uint32_t)rule*3u;
            const uint8_t selector=spchRead8(event,offset+1),kind=spchRead8(event,offset+2);
            if (selector!=(parameter==20?0u:parameter)) continue;
            const uint8_t bit=(uint8_t)(1u<<(7u-(uint32_t)rule));
            const uint32_t value=parameter==20?0:parameters[parameter];
            if (kind==4) {
                if (parameters[selector]) accepted|=bit;
            } else if (!engine->ruleOwner) {
                accepted|=bit;unresolved|=bit;
            } else {
                /* Typed SED TestRule's exact released result is0. No side
                 * effect is discarded and no generic provider is assumed. */
                (void)value;
            }
        }
    }
    *unknown=unresolved;return accepted;
}
int32_t spchCheckSentenceRules(uint8_t rules,uint8_t unknown,SpeechBytes sentence)
{
    if (!sentence.bytes || sentence.size<4) return 0;
    return (((uint32_t)spchRead8(sentence,2)^rules)&spchRead8(sentence,1)&(uint8_t)~unknown)==0;
}
void spchRuleSet(SpeechEngineState* engine,uint32_t eventId,SpeechBytes event,uint8_t sentenceIndex,
    const uint32_t parameters[20])
{
    if (!engine || !engine->ruleSetterOwner) return;
    uint32_t database=0;(void)spchGetDatabaseId(engine,eventId,&database);
    const uint32_t count=spchRead8(event,6);
    if (sentenceIndex>=count) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return;}
    const SpeechBytes sentence=spchBytesSlice(event,(uint32_t)spchRead16(event,12u+sentenceIndex*2u)*4u,4);
    const uint32_t begin=12u+((count*2u+3u)&~3u);
    const int32_t rules=(int8_t)spchRead8(event,7);
    for (int32_t i=0;i<rules;++i) {
        const uint32_t at=begin+(uint32_t)i*3u;
        const uint32_t kind=spchRead8(event,at+2),parameter=spchRead8(event,at+1);
        /* The complete PAL switch has six entries: only kinds0 and3 enter
         * the selected-sentence parameter test. All other entries join the
         * loop. The selected SED SetRule is an actual no-effect callback. */
        if ((kind==0 || kind==3) && spchSentenceUsesParameter(sentence,parameter)) {
            if (parameter>=20) {engine->operationResult=SPEECH_DATABASE_INVALID_DATA;return;}
            (void)parameters[parameter];(void)database;(void)spchRead8(event,at);
        }
    }
}

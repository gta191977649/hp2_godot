// TASK-0112 E-0112-R07. Included by original endgamescreens.cpp after native
// result helpers. Source authority Debug SHA815b60c7...332f1b, revision unknown.
namespace port::race::detail {
// Debug001DA058; E-0112-R07. Tournament display reads already-settled points;
// it never awards them again. Source list comparator001DA048 is ascending rank.
static void Sub_001DA058(driving::Memory& m,std::uint32_t screen,const EndGameScreenServices& s){
 using namespace hp2::debug;
 // E-0112-R10: actual Retail001DD5D0 also initializes RACE_COUNT, offset0.
 ::port::race::apply_retail_result_track_header(m,screen,0);
 if(!FrontendDatabase::UseTheRaceEvent(m,0x2f3890)||m.signed_half(0x312cd0+0x6a)!=4)return;
 const auto race=m.word(0x327258),state=m.word(0x312cd4);if(!race||!state)hp2::not_implemented("0x001DA058","original tournament race/state owner absent");const int count=m.signed_word(race+0x18);if(count<0||count>8)hp2::not_implemented("0x001DA058","original tournament roster outside eight slots");
 struct Row {std::uint32_t racing;unsigned points,rank;};std::vector<Row> ranked;
 for(int i=0;i<count;++i){const auto racing=m.word(race+0x624+4*unsigned(i)),car=m.word(racing);const int index=RaceEvent::RacerIndexFromDriverNumber(m,0x312cd0,m.signed_half(car+0xa));if(index<0||index>=8)hp2::not_implemented("0x001DA058","original tournament driver index outside source state");ranked.push_back({racing,m.byte(state+unsigned(index)),EventTournamentState::RacerRank(m,state,index)});}
 // Source001DE8D8 comparator(node,next) keeps a strictly smaller rank first.
 // Rank's tie break makes all legitimate roster ranks unique.
 bool changed;do{changed=false;for(std::size_t i=1;i<ranked.size();++i)if(!(ranked[i-1].rank<ranked[i].rank)){if(ranked[i-1].rank==ranked[i].rank)hp2::not_implemented("0x001DE8D8","equal original tournament ranks would not terminate source sort");std::swap(ranked[i-1],ranked[i]);changed=true;}}while(changed);
 for(int line=0;line<8;++line){std::array<std::uint32_t,5> hashes;unsigned k=0;for(auto fmt:{0x3284c0u,0x3284c8u,0x3284d0u,0x3284d8u,0x2b0ba0u})hashes[k++]=result_hash(m,fmt,line+1);for(const auto hash:hashes)FlailerSetVisibility(m,screen,hash,line<count);if(line>=count)continue;
  const auto& row=ranked[unsigned(line)];const auto car=m.word(row.racing),driver=m.word(car+0x14),ride=m.word(car+0x10);const int car_type=m.signed_word(ride);if(car_type<0||car_type>=64)hp2::not_implemented("0x001DA058","original tournament vehicle type outside reviewed table");const auto type=m.word(0x3278e0)+0x560*unsigned(car_type);
  const int racer_index=RaceEvent::RacerIndexFromDriverNumber(m,0x312cd0,m.signed_byte(driver+1));if(racer_index<0||racer_index>=8)hp2::not_implemented("0x001DA058","original tournament finish-position index outside eight racers");const int finish=m.signed_word(0x312cd0+0x150+12*unsigned(racer_index));
  std::uint32_t name;auto color=0x3116e0u;const auto player=m.word(car+0x18);
  if(m.word(driver+8)==1){const auto own=World(m,m.word(0x3272f0)).GetCarByNumber(m.signed_byte(driver+1));if(!own||!m.word(own+0x18))hp2::not_implemented("0x001DA058","original tournament player name owner missing");name=m.word(own+0x18)+0x30;}else{const auto character=m.word(driver+20);name=character?character+4:0x3278f0;}
  const auto name_text=result_text(m,name);result_printf(m,screen,hashes[1],0x3284e0,name_text.c_str());result_printf(m,screen,hashes[0],0x3284e8,line+1);const auto translated=GetTranslatedString(m,CarTypeInfo::GetFullName(m,type));const auto car_name=result_text(m,translated);result_printf(m,screen,hashes[2],0x3284e0,car_name.c_str());
  if(finish){if(finish<1||finish>8)hp2::not_implemented("0x001FBAD8","original tournament point table index outside eight positions");result_printf(m,screen,hashes[3],0x3284f0,int(m.byte(0x328830+unsigned(finish-1))));}else result_printf(m,screen,hashes[3],0x3284f8,0);
  result_printf(m,screen,hashes[4],0x3284f0,int(row.points));
  if(player){const int number=m.signed_word(player+0x10);if(number<0||number>1)hp2::not_implemented("0x001DA058","original player color index outside two colors");color=0x3116c0+16*unsigned(number);const auto owner=m.word(0x32847c);if(!owner)hp2::not_implemented("0x001DA058","original EndGameScreens speech owner missing");if(!m.word(owner+0x30)){if(!s.tournament_speech)hp2::not_implemented("0x00202360","original TournamentRaceSpeech sink missing");const auto status=RaceEvent::GetStatus(m,0x312cd0);s.tournament_speech(unsigned(int(status)-3)<2,line);}m.word(owner+0x30,1);}
  for(auto hash:hashes)result_color(m,screen,hash,color);
 }
}
// Debug001DD368; E-0112-R07. Source lap slots1..9 and first strict-minimum
// highlight are retained. This screen reads transient rows, never race timers.
static void Sub_001DD368(driving::Memory& m,std::uint32_t screen){
 using namespace hp2::debug;
 FlailerSetLanguageID(m,int(screen),int(0xf601f2d4),GetTheRightTrackLanguageID(m,m.word(0x327398),m.signed_word(0x311f84)));
 const int players=m.signed_word(0x2f3890+0x10734);auto player=m.word(0x327778);if(!player)player=m.word(0x32777c);if(!player)return;
 const auto player_title=[&](std::uint32_t p,std::uint32_t hash){const auto car=m.word(p+4),type=m.word(car+12),language_info=m.word(type+0x558);if(!language_info)hp2::not_implemented("0x001860A8","original lap screen car language info absent");const auto name=result_text(m,p+0x30),short_name=result_text(m,GetTranslatedString(m,m.signed_word(language_info+20)));const auto title=result_format_text(m,0x328520,name.c_str(),short_name.c_str());if(title.size()>=128)hp2::not_implemented("0x001DD368","original lap player-name128-byte buffer overflow domain");result_printf(m,screen,hash,0x3284e0,title.c_str());};
 player_title(player,0x6dbbc830);if(players==2){const auto second=m.word(0x32777c);if(!second)return;player_title(second,0x6dbbc831);}
 std::array<std::uint32_t,2> rows{{result_driver(m,0),result_driver(m,1)}};if(!rows[0]){rows[0]=rows[1];rows[1]=0;}
 const int num_laps=std::max(rows[0]?m.signed_word(rows[0]+0x60):0,rows[1]?m.signed_word(rows[1]+0x60):0);
 std::array<int,2> best_time{{99999999,99999999}},best_index{{m.signed_word(0x328528),m.signed_word(0x32852c)}};
 for(int i=0;i<9;++i)for(unsigned p=0;p<2;++p)if(rows[p]){const int time=m.signed_word(rows[p]+0x34+4*unsigned(i+1));if(float(time)*m.scalar(0x325b68)!=0&&time<best_time[p]){best_index[p]=i;best_time[p]=time;}}
 for(int i=0;i<9;++i){const int item=i+1;const auto lapname=result_hash(m,0x2b0c08,item),lap=result_hash(m,0x328530,item);const std::array<std::uint32_t,2> times{{result_hash(m,0x2b0c18,item),result_hash(m,0x2b0c28,item)}};result_printf(m,screen,lap,0x3284f0,item);
  for(unsigned p=0;p<2;++p){if(!rows[p]){FlailerSetVisibility(m,screen,times[p],0);continue;}result_color(m,screen,times[p],best_index[p]==i?0x3116f0:0x3116e0);FlailerSetVisibility(m,screen,times[p],1);const int time=m.signed_word(rows[p]+0x34+4*unsigned(item));if(time&&time!=0x7fffffff){driving::Memory::Temporary buffer(m,64);Timer{time}.PrintToString(m,buffer.address(),0);const auto text=result_text(m,buffer.address());result_printf(m,screen,times[p],0x3284e0,text.c_str());result_format(m,screen,times[p],true);}else FlailerSetVisibility(m,screen,times[p],0);}
  FlailerSetVisibility(m,screen,lapname,i<num_laps);if(i>=num_laps){FlailerSetVisibility(m,screen,lap,0);for(const auto hash:times)FlailerSetVisibility(m,screen,hash,0);}
 }
}
// Debug001DD8A0; E-0112-R07. Back retains caller Noise; every other event clears it.
static void Sub_001DD8A0(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d){if(msg!=hp2::debug::SCREEN_BACK_BUTTON)d.Noise=0;}
}

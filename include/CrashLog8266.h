#ifndef CRASHLOG8266_H
#define CRASHLOG8266_H

#if defined(ESP8266) && CRASHLOG_ENABLED
#include <Arduino.h>
#include <EEPROM.h>

#define CRASHLOG_MAX 10
#define CRASHLOG_EEPROM_BASE 16
#define CRASHLOG_MAGIC 0xC248

struct CrashRtc8266 {
  uint32_t magic;
  uint32_t checkpoint;
  uint32_t extra;
  uint32_t checksum;
};
struct CrashEntry8266 {
  uint32_t bootNo;
  uint32_t checkpoint;
  uint8_t reason;
  uint8_t reserved[3];
};
struct CrashStore8266 {
  uint16_t magic;
  uint8_t count;
  uint8_t next;
  uint32_t bootNo;
  CrashEntry8266 entries[CRASHLOG_MAX];
};

static CrashStore8266 crashStore;
static CrashRtc8266 crashRtc;

static uint32_t crashlog_sum(const CrashRtc8266 &r){ return r.magic ^ r.checkpoint ^ r.extra ^ 0x826648C2UL; }
static void crashlog_store_save(){ EEPROM.put(CRASHLOG_EEPROM_BASE, crashStore); EEPROM.commit(); }

static bool crashlog_is_real_crash(uint8_t reason){
  return reason == REASON_WDT_RST || reason == REASON_EXCEPTION_RST || reason == REASON_SOFT_WDT_RST;
}

static const char* crashlog_reason_name(uint8_t reason){
  switch(reason){
    case REASON_WDT_RST: return "Hardware WDT";
    case REASON_EXCEPTION_RST: return "Exception";
    case REASON_SOFT_WDT_RST: return "Software WDT";
    default: return "Reset";
  }
}

static void crashlog_begin(){
  EEPROM.begin(512);
  EEPROM.get(CRASHLOG_EEPROM_BASE, crashStore);
  if(crashStore.magic != CRASHLOG_MAGIC || crashStore.count > CRASHLOG_MAX || crashStore.next >= CRASHLOG_MAX){
    memset(&crashStore,0,sizeof(crashStore)); crashStore.magic=CRASHLOG_MAGIC;
  }
  crashStore.bootNo++;
  bool rtcOk = ESP.rtcUserMemoryRead(0, (uint32_t*)&crashRtc, sizeof(crashRtc)) && crashRtc.magic==0x8266C248UL && crashRtc.checksum==crashlog_sum(crashRtc);
  const rst_info *ri = ESP.getResetInfoPtr();
  uint8_t reason = ri ? ri->reason : 0;
  if(crashlog_is_real_crash(reason)){
    CrashEntry8266 &e=crashStore.entries[crashStore.next];
    e.bootNo=crashStore.bootNo; e.reason=reason; e.checkpoint=rtcOk?crashRtc.checkpoint:0;
    crashStore.next=(crashStore.next+1)%CRASHLOG_MAX;
    if(crashStore.count<CRASHLOG_MAX) crashStore.count++;
  }
  crashlog_store_save();
  crashRtc.magic=0x8266C248UL; crashRtc.checkpoint=0; crashRtc.extra=0; crashRtc.checksum=crashlog_sum(crashRtc);
  ESP.rtcUserMemoryWrite(0,(uint32_t*)&crashRtc,sizeof(crashRtc));
}

static void crashlog_checkpoint(uint32_t cp, uint32_t extra=0){
  crashRtc.magic=0x8266C248UL; crashRtc.checkpoint=cp; crashRtc.extra=extra; crashRtc.checksum=crashlog_sum(crashRtc);
  ESP.rtcUserMemoryWrite(0,(uint32_t*)&crashRtc,sizeof(crashRtc));
}

static void crashlog_clear(){
  memset(&crashStore,0,sizeof(crashStore)); crashStore.magic=CRASHLOG_MAGIC; crashlog_store_save();
}

static String crashlog_html(){
  if(!crashStore.count) return "<div class='hint'>Keine Crash-Eintraege gespeichert.</div>";
  String h;
  for(uint8_t i=0;i<crashStore.count;i++){
    int idx=(int)crashStore.next-1-i; if(idx<0) idx+=CRASHLOG_MAX;
    const CrashEntry8266 &e=crashStore.entries[idx];
    h += "<div class='crash-entry'><b>" + String(crashlog_reason_name(e.reason)) + "</b><br>Boot #" + String(e.bootNo) + " · Checkpoint " + String(e.checkpoint) + "</div>";
  }
  return h;
}
#else
static void crashlog_begin(){}
static void crashlog_checkpoint(uint32_t,uint32_t=0){}
static void crashlog_clear(){}
static String crashlog_html(){return String();}
#endif
#endif

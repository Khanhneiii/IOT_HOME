#ifndef WTV020SD16P_h
#define WTV020SD16P_h

typedef struct WTV020SD16P
{
  int _clockPin;
  int _dataPin;
  int _busyPin;
  int _busyPinState;
  unsigned int _currentVolume;
  unsigned long _lastMicros;
} WTV020SD16P;


  void init(WTV020SD16P *,int clockPin,int dataPin,int busyPin);
  
  void playVoice(WTV020SD16P*,unsigned int voiceNumber);
  void asyncPlayVoice(WTV020SD16P*,unsigned int voiceNumber);
  void stopVoice(WTV020SD16P*);
  void pauseVoice(WTV020SD16P*);
  void mute(WTV020SD16P*);
  void unmute(WTV020SD16P*);
  void setVolume(WTV020SD16P*,unsigned int volume);

  void sendCommand(WTV020SD16P*,unsigned int command);
  void delayMicros(WTV020SD16P*,unsigned long time);


#endif
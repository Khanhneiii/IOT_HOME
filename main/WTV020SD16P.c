#include "WTV020SD16P.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>

const unsigned int PLAY_PAUSE = 0xFFFE;
const unsigned int STOP = 0xFFFF;
const unsigned int VOLUME_MIN = 0xFFF0;
const unsigned int VOLUME_MAX = 0xFFF7;

static void setGPIO(int pin,gpio_mode_t gpio_mode) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL<<pin),
        .mode = gpio_mode,
        .pull_up_en = 0,                           
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

void init(WTV020SD16P *wtv,int clockPin,int dataPin,int busyPin)
{
  // Save private values from constructor
  wtv->_clockPin=clockPin;
  wtv->_dataPin=dataPin;
  wtv->_busyPin=busyPin;
  
  // Set initial values
  wtv->_busyPinState=1;
  wtv->_currentVolume = VOLUME_MAX;

  // Initialize timer
  wtv->_lastMicros = 0;
  delayMicros(wtv,0);

  // Set pins
  
    setGPIO(wtv->_clockPin,GPIO_MODE_OUTPUT);
    setGPIO(wtv->_dataPin,GPIO_MODE_OUTPUT);
    setGPIO(wtv->_busyPin,GPIO_MODE_INPUT);
}



// Public: Play entire song and wait till song finishes
void playVoice(WTV020SD16P *wtv,unsigned int voiceNumber){
  sendCommand(wtv,voiceNumber);
  // Wait 20ms for busy to be active
  delayMicros(wtv,20000);
  // Wait till song ends
  wtv->_busyPinState=gpio_get_level(wtv->_busyPin);
  while(wtv->_busyPinState==1){
    wtv->_busyPinState=gpio_get_level(wtv->_busyPin);
  }
}

// Public: Start playing song and return
void asyncPlayVoice(WTV020SD16P *wtv,unsigned int voiceNumber){
  sendCommand(wtv,voiceNumber);
}

// Public: Stop playing song
void stopVoice(WTV020SD16P *wtv){
  sendCommand(wtv,STOP);
}

// Public: Pause/Resume song
void pauseVoice(WTV020SD16P *wtv){
  sendCommand(wtv,PLAY_PAUSE);
}

// Public: Mute sound - same as setVolume(0)
void mute(WTV020SD16P *wtv){
  sendCommand(wtv,VOLUME_MIN);
}

// Public: Unmute sound restoring the previous volume
void unmute(WTV020SD16P *wtv){
  sendCommand(wtv,wtv->_currentVolume);
}

// Public: Set volume
void setVolume(WTV020SD16P *wtv,unsigned int volume){
  if(volume < 0x0008) {
    wtv->_currentVolume = VOLUME_MIN + volume;
    sendCommand(wtv,wtv->_currentVolume);
  }
}


// Private: Send command to sound module
void sendCommand(WTV020SD16P *wtv,unsigned int command) {
  
  //Start bit Low level pulse.
  gpio_set_level(wtv->_clockPin,0);
  // Initialize timer
  delayMicros(wtv,0);
  // Wait Start bit length minus 50 us
  delayMicros(wtv,1950);
  for (unsigned int mask = 0x8000; mask > 0; mask >>= 1) {
    //Clock low level pulse.
    gpio_set_level(wtv->_clockPin,0);
    delayMicros(wtv,50);
    //Write data setup.
    if (command & mask) {
      gpio_set_level(wtv->_dataPin,1);
    } else {
      gpio_set_level(wtv->_dataPin,0);
    }
    //Write data hold.
    delayMicros(wtv,50);
    //Clock high level pulse.
    gpio_set_level(wtv->_clockPin,1);
    delayMicros(wtv,100);
  }
  //Stop bit high level pulse (additional 1900us).
  delayMicros(wtv,1900);
}


// Private: Wait specified time since the last wait
void delayMicros(WTV020SD16P *wtv,unsigned long time) {
  unsigned long stopMicros;
  unsigned long currentMicros;
  // set current time
  if (time == 0) {
    wtv->_lastMicros = esp_timer_get_time();
	return;
  }
  // Start waiting loop
  stopMicros = wtv->_lastMicros + time;
  currentMicros = esp_timer_get_time();
  // If time already passed then return withoit delay
  if ((stopMicros  == currentMicros) || (wtv->_lastMicros < stopMicros  && stopMicros < currentMicros) || (stopMicros < currentMicros && currentMicros < wtv->_lastMicros)) {
	  wtv->_lastMicros = currentMicros;
  } else {
    // Wait remaining time left 
    ets_delay_us(stopMicros - currentMicros);
    wtv->_lastMicros = stopMicros;
  }
  
}
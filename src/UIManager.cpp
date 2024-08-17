/* 

// has to be static - timer won't take class member function in callback 
static void StaticLedCallback(void* data) {
  GrannyChordApp* instance = static_cast<GrannyChordApp*>(data);
  instance->LedCallback();
}

void GrannyChordApp::SetupTimer(){
  TimerHandle::Config cfg;
  cfg.periph = TimerHandle::Config::Peripheral::TIM_5;
  cfg.period = 10000;
  cfg.enable_irq = true;
  timer_.Init(cfg);
  timer_.SetPrescaler(9999);
  timer_.SetCallback(StaticLedCallback, this);
}


void GrannyChordApp::StartLedPulse(){
  timer_.Start();
}

void GrannyChordApp::StopLedPulse(){
  timer_.Stop();
}



SelectFile: BLUE
RecordIn: WHITE
PlayWAV: CYAN
Synthesis: GREEN
ChordMode: GOLD (orange)

Error: solid RED
RecordIn: seed led flashing red (ie pod.seed.SetLed(1/0)

don't use purple - looks like blue
can use floats or ints
can use magenta (255,0,255)
can use yellow (255,255,0)

red 	1, 0, 0
green	0, 1, 0
blue	0, 0, 1
magenta 1,0 ,1
yellow  1,1,0
ORANGE  1, 0.7f, 0  
VIOLET  0.7f, 0, 1
PURPLE  0.7f, 0, 0.7f  
CYAN	0, 0.7f, 0.7f  

*/





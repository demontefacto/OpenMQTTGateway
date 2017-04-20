/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

  Copyright: (c)1technophile

  Contributors:
  - 1technophile
  - crankyoldgit
  - Spudtater
  - rickybrent
  - ekim from Home assistant forum
  - ronvl from Home assistant forum

IMPORTANT NOTE: On arduino connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
  
Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifdef ZgatewayIR

#ifdef ESP8266
  #include <IRremoteESP8266.h>
   IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend(IR_EMITTER_PIN);
#else
  #include <IRremote.h>
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend; //connect IR emitter pin to D9 on arduino, you need to comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library IRremote.h so as to free pin D3 for RF RECEIVER PIN
#endif

decode_results results;

void setupIR()
{
  //IR init parameters
#ifdef ESP8266
  irsend.begin();
#endif

  irrecv.enableIRIn(); // Start the receiver
  
}
boolean IRtoMQTT(){
  unsigned long MQTTvalue = 0;
  String valueAdvanced;
  if (irrecv.decode(&results)){
  trc(F("Receiving IR signal"));
    MQTTvalue=results.value;
    valueAdvanced = "Value " + String(MQTTvalue)+" Bit " + String(results.bits) + " Protocol " + String(results.decode_type);
    irrecv.resume(); // Receive the next value
    if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Sending advanced signal to MQTT"));
        client.publish(subjectIRtoMQTTAdvanced,(char *)valueAdvanced.c_str());
        trc(F("Sending IR to MQTT"));
        String value = String(MQTTvalue);
        trc(value);
        boolean result = client.publish(subjectIRtoMQTT,(char *)value.c_str());
        return result;
    }
  }
  return false;  
}

void MQTTtoIR(char * topicOri, char * datacallback) {
  String topic = topicOri;

  trc(F("Receiving data by MQTT"));
  trc(topic);  
  trc(F("Callback value"));
  trc(String(datacallback));
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  trc(F("Converted value to unsigned long"));
  trc(String(data));
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal (example of signal sent data = 1086296175)
  boolean signalSent = false;
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  if (topic.lastIndexOf("IR_COOLIX") != -1 ){
    irsend.sendCOOLIX(data, 24);
    signalSent = true;
  }
  #endif
  if (topic.lastIndexOf("IR_NEC")!= -1 || topic == subjectMQTTtoIR){
    irsend.sendNEC(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Whynter")!=-1){
    irsend.sendWhynter(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_LG")!=-1){
    irsend.sendLG(data, 28);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sony")!=-1){
    irsend.sendSony(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_DISH")!=-1){
    irsend.sendDISH(data, 16);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_RC5")!=-1){
    irsend.sendRC5(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sharp")!=-1){
    irsend.sendSharpRaw(data, 15);
    signalSent = true;
  }
   if (topic.lastIndexOf("IR_SAMSUNG")!=-1){
   irsend.sendSAMSUNG(data, 32);
    signalSent = true;
  }
  if (signalSent){
    boolean result = client.publish(subjectGTWIRtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
  }
   irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
}
#endif
/**
 * 
 * EEPROM:
 *  0(Scale Calibration factor)
 *  400(Last hour sent sms)
 *  600-611(Phone Number)
 * 
 */


#include <dht.h>
#include "Wire.h"
#include <CStringBuilder.h>
#include <SoftwareSerial.h>
#include <DS3231.h>
#include "HX711.h"
#include <EEPROM.h>
#include "LowPower.h"

#define DHT11_PIN 11
#define DS3231_I2C_ADDRESS 0x68

dht DHT;
SoftwareSerial sim900(7, 8);
DS3231  realTimeClock(SDA, SCL);
HX711 scale;

int gotSMS = 0;
String password = "1234 ";
String phoneNumber;
String receivedAnswer;

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  sim900.begin(9600);
  realTimeClock.begin();
  beginScale();
  
  setupListenerForSMS();
  setupGSMPowerPin();

  phoneNumber = getPhoneNumberFromMemory();
//  updatePhoneNumber("306940895179");
  lowPowerThePins();
  
//  realTimeClock.setDOW(SATURDAY);     // Set Day-of-Week to SUNDAY
//  realTimeClock.setTime(18, 16, 00);     // Set the time to 12:00:00 (24hr format)
//  realTimeClock.setDate(12, 8, 2019);   // Set the date to January 1st, 2014
  
  startGSMIfClosed();
  
  String message = String("Scale is "+String(getScaleReading())+"g\nIf not 0g, send the password and 0 to zero the scale\n\nYou have 10 minutes to send your commands");
  Serial.println(message);
  sendSMS(message);
  delay(5000);
  
  startSMSListenerForMin(10);
  
  closeGSMIfOpen();
}

void loop()
{
  if(shouldSendSMS())
  {
    startGSMIfClosed();
    
    EEPROM.put(400, realTimeClock.getTime().hour);

    String message = String("Weight = "+String(getScaleReading())+"g\n"+String(getTempHumidityStr())+"\n\n"+"You have 10 mins to send your commands");
    sendSMS(message);
    delay(5000);

    startSMSListenerForMin(10);

    closeGSMIfOpen();
  }
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void beginScale()
{
  scale.begin(A1, A0);
  scale.set_scale(getCalibrationFactor());
}
float getCalibrationFactor()
{
  float calibrationFactor;
  EEPROM.get(0, calibrationFactor);
  return calibrationFactor;
}

void setupGSMPowerPin()
{
  pinMode(9, OUTPUT);
}

void startGSMIfClosed()
{
  char character;
  while(sim900.available() > 0)
  {
    character = char(sim900.read());
  }
  
  sim900.println("AT");
  delay(1000);
  if(sim900.available() <= 0){
    powerSignalGSM();
    delay(15000);
  }
}
void closeGSMIfOpen()
{
  char character;
  while(sim900.available() > 0)
  {
    character = char(sim900.read());
  }
  
  sim900.println("AT");
  delay(1000);
  if(sim900.available() > 0){
    powerSignalGSM();
  }
}

void setupListenerForSMS()
{
  sim900.println("AT");
  delay(500);
  sim900.println("AT+CMGF=1");
  delay(500);
  sim900.println("AT+CNMI=1,2,0,0,0");
  delay(500);
}

void lowPowerThePins()
{
  for (int i = 0; i < 20; i++)
  {
    if(i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6 || i == 10 || i == 12 || i == 13 || i == A2 || i == A3 || i == A4 || i == A5)
    {
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
  }
}

void startSMSListenerForMin(int minutes)
{
  int timeToStop = (realTimeClock.getTime().hour * 100)+(realTimeClock.getTime().min+minutes);
  int currentTime = (realTimeClock.getTime().hour * 100)+(realTimeClock.getTime().min);
  while(currentTime <= timeToStop)
  {
    String receivedSMS = checkForSMSAndReturnIt();
    String command;
      
    if(isPasswordCorrect(receivedSMS))
    {
      command = receivedSMS.substring(5, receivedSMS.length());
      if(command.equals("0"))
      {
        scale.tare();
      }else if(command.equals("1"))
      {
        updatePhoneNumber(getPhoneNumberFromCommand());
      }else if(command.equals("2"))
      {
        String message = String("Weight = "+String(getScaleReading())+"\n"+String(getTempHumidityStr()));
        sendSMS(message);
        delay(1000);
      }else if(command.equalsIgnoreCase("help"))
      {
        sendSMS("Available commands:\n 0 => Zero Scale \n 1 => Set phone number \n 2 => Request scale reading\n\n There is an auto time\n syncronization for the clock");
        delay(1000);
      }
      setTimeAndDateFromCommand();
      delay(2000);
    }

    currentTime = (realTimeClock.getTime().hour * 100)+(realTimeClock.getTime().min);
  }
}

void updatePhoneNumber(String number)
{
  for(int i = 0; i < 12; i++)
  {
    EEPROM.write(600+i, number.charAt(i));
    phoneNumber = number;
  }
}

String getPhoneNumberFromMemory()
{
  String result;
  for(int i = 0; i < 12; i++)
  {
    result.concat(char(EEPROM.read(600+i)));
  }

  return result;
}

int shouldSendSMS()
{
  int lastHourSentSMS;
  EEPROM.get(400, lastHourSentSMS);
  return (((realTimeClock.getTime().hour == 7) || (realTimeClock.getTime().hour == 21)) && (realTimeClock.getTime().hour != lastHourSentSMS));
}

String getPhoneNumberFromCommand()
{
  return receivedAnswer.substring(receivedAnswer.indexOf('+')+5, receivedAnswer.indexOf('+')+13);
}

int getScaleReading()
{
  int reading = scale.get_units(20);
  if(reading < 0)
  {
    reading = 0;
  }
  
  return reading;
}

void powerSignalGSM()
{
  digitalWrite(9,LOW);
  delay(1000);
  digitalWrite(9,HIGH);
  delay(2000);
  digitalWrite(9,LOW);
  delay(3000);
}

String formatAndReturnDateTime()
{
  String result;
  result = String(result + convertDayOfWeek(realTimeClock.getDOWStr()) 
  + " " + extractFieldNumberFromDate(realTimeClock.getDateStr(), 0, 1) 
  + " " + convertMonthOfYear(realTimeClock.getDateStr()) 
  + " 20" + extractFieldNumberFromDate(realTimeClock.getDateStr(), 8, 9)
  + "\n" + realTimeClock.getTimeStr());

  return result;
}

String convertDayOfWeek(char* dayOfWeek)
{
  String result;
  if(dayOfWeek == "Monday")
  {
    result = "Δευτέρα";
  }else if(dayOfWeek == "Tuesday")
  {
    result = "Τρίτη";
  }else if(dayOfWeek == "Wednesday")
  {
    result = "Τετάρτη";
  }else if(dayOfWeek == "Thursday")
  {
    result = "Πέμπτη";
  }else if(dayOfWeek == "Friday")
  {
    result = "Παρασκεύη";
  }else if(dayOfWeek == "Saturday")
  {
    result = "Σάββατο";
  }else if(dayOfWeek == "Sunday")
  {
    result = "Κυριακή";
  }
  return result;
}

String convertMonthOfYear(char* monthOfYear)
{
  String result;
  String monthNumber = extractFieldNumberFromDate(monthOfYear, 3, 4);
  
  if(monthNumber == "1")
  {
    result = "Ιανουαρίου";
  }else if(monthNumber == "2")
  {
    result = "Φεβρουαρίου";
  }else if(monthNumber == "3")
  {
    result = "Μαρτίου";
  }else if(monthNumber == "4")
  {
    result = "Απριλίου";
  }else if(monthNumber == "5")
  {
    result = "Μαΐου";
  }else if(monthNumber == "6")
  {
    result = "Ιουνίου";
  }else if(monthNumber == "7")
  {
    result = "Ιουλίου";
  }else if(monthNumber == "8")
  {
    result = "Αυγούστου";
  }else if(monthNumber == "9")
  {
    result = "Σεπτεμβρίου";
  }else if(monthNumber == "10")
  {
    result = "Οκτωβρίου";
  }else if(monthNumber == "11")
  {
    result = "Νοεμβρίου";
  }else if(monthNumber == "12")
  {
    result = "Δεκεμβρίου";
  }
  return result;
}

String extractFieldNumberFromDate(char* fullDate, int firstDigit, int secondDigit)
{
  String result;
  
  if((char(fullDate[firstDigit])) != '0')
  {
    result = String((result + char(fullDate[firstDigit])) + char(fullDate[secondDigit]));
  }else
  {
    result = String(result + char(fullDate[secondDigit]));
  }

  return result;
}

String getTempHumidityStr()
{
  int *tempHumidity = getTempHumidity();
  String result;
  result = String(result + "Temperature =" +  tempHumidity[0] + "C\nHumidity = " + tempHumidity[1] + "%");
  return result;
}
int* getTempHumidity()
{
  DHT.read11(DHT11_PIN);
  
  int *data = new int[2];
  data[0] = DHT.temperature;
  data[1] = DHT.humidity;
  
  return data;
}

void sendSMS(String message)
{
  delay(1000);
  sim900.println("AT");
  delay(500);
  sim900.println("AT+CMGF=1");
  delay(500);
  sim900.println("AT+CMGS=\"+"+phoneNumber+"\"");
  delay(500);
  sim900.println(message);
  delay(500);
  sim900.write(26);
}
int isPasswordCorrect(String message)
{
  int result = 0;
  if(message.length() > password.length())
  {
    result = message.substring(0, password.length()).equals(password);
  }
  return result;
}
String checkForSMSAndReturnIt()
{
  gotSMS = 0;
  if(sim900.available()) 
  {
    if(isSMS())
    {
      gotSMS = 1;
      String result = "";
      while(sim900.available())
      { 
        result = String(result + char(sim900.read()));
      }

      receivedAnswer = result;
      
      result = result.substring(result.indexOf('\n')+1, result.length()-2);
      
      return result;
    }
  }

  return "";
}
int isSMS()
{
  char currentChar;
  char charactersToCheck[] = {'+','C','M','T',':'};
  int count = 0;
  while(sim900.available() && count < 5)
  {
    currentChar = charactersToCheck[count];
    if(sim900.read() != currentChar)
    {
      return 0;
    }
    count = count + 1;
  }
  return 1;
}

void setTimeAndDateFromCommand()
{
  String currDate = receivedAnswer.substring(receivedAnswer.lastIndexOf('\"')-20, receivedAnswer.lastIndexOf('\"')-12);
  int yearr = currDate.substring(0, 2).toInt();
  int month = currDate.substring(3, 5).toInt();
  int day = currDate.substring(6, 8).toInt();

  String currTime = receivedAnswer.substring(receivedAnswer.lastIndexOf('\"')-11, receivedAnswer.lastIndexOf('\"'));
  int hourr = currTime.substring(0, 2).toInt();
  int mins = currTime.substring(3, 5).toInt();
  int secs = currTime.substring(6, 8).toInt();

  realTimeClock.setTime(hourr, mins, secs);
  realTimeClock.setDate(day, month, 2000+yearr);
}

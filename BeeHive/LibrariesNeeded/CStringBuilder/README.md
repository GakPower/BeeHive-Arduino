
CStringBuilder is a simple class for Arduino to print content to a c-string (zero terminated char array).

It inherits from [Print](https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/Print.h) class all the print and println methods you know from Serial or various network clients like EthernetClient and WiFiClient.

CStringBuilder adds `printf` method for C style [formated printing](http://www.cplusplus.com/reference/cstdio/printf/). And you can `printf` with formating string from flash memory with F() macro.  

CStringBuilder is useful if you want to prepare the string to send it in one piece, or you need to know the size of the string before you send it (for example for the Content-length header or chunked-encoding in HTTP).

Example:
```
#include <CStringBuilder.h>

const char* s = "string";
char c = 'C';
int i = 42;
float f = 5.3;

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  char buff[100];
  CStringBuilder sb(buff, 100);

  sb.print(F("Some text: "));
  sb.println(s);
  sb.print(F("Some char: "));
  sb.println(c);
  sb.print(F("HEX of char: "));
  sb.println(c, HEX);
  sb.print(F("Some integer: "));
  sb.println(i);
  sb.print(F("Some float: "));
  sb.println(f, 2);

  Serial.print("size to print: ");
  Serial.println(sb.length());
  Serial.println();
  Serial.println(buff);

  sb.reset();
  sb.printf(F("Formated: %s;%c;%05d\r\n"), s, c, i);
  Serial.println(buff);
}

void loop()
{

}
```
Output:
```
size to print: 85

Some text: string
Some char: C
HEX of char 43
Some integer: 42
Some float: 5.30

Formated: string;C;00042
```

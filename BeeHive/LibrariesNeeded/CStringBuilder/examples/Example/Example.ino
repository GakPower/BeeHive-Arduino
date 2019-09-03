#include <CStringBuilder.h>

const char* s = "string";
char c = 'C';
int i = 42;
float f = 5.3;

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  const size_t BUFF_SIZE = 100;
  char buff[BUFF_SIZE];
  CStringBuilder sb(buff, BUFF_SIZE);

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

  int l = sb.length();
  sb.print("this text doesn't fit in the remaining space in the buffer");
  if (sb.getWriteError()) {
    sb.setLength(l);
  }
  sb.println("test test");

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

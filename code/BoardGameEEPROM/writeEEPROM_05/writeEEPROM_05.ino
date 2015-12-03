#include <Wire.h>
#define eeprom1 0x50    //Address of 24LC256 eeprom chip
#define WRITE_CNT 5*8

#define recordLength 160
#define fieldLength   20

unsigned char rdata[21];
char xdata[21];

unsigned int addr = 3200;

void setup(void)
{
  Serial.begin(9600);

  Wire.begin();
  Serial.println("--------------------------");
  SaveDataEEPROM();
  Serial.println("--------------------------");

  /*
  Serial.println();
  Serial.println("DATA READ");
  addr = 0;
  for (int i = 0; i < WRITE_CNT; i++) {
    readEEPROM(eeprom1, addr, rdata, fieldLength);
    strncpy(xdata, (char*)rdata, fieldLength);
    addr = addr + fieldLength;
    Serial.println(xdata);
  }
  */
}

void loop() {
}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data)
{
  // Uses Page Write for 24LC256
  // Allows for 64 byte page boundary
  // Splits string into max 16 byte writes
  unsigned char i = 0, counter = 0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page = 0;
  unsigned int  num_writes;
  unsigned int  data_len = 0;
  unsigned char first_write_size;
  unsigned char last_write_size;
  unsigned char write_size;

  // Calculate length of data
  do {
    data_len++;
  }
  while (data[data_len]);

  // Calculate space available in first page
  page_space = int(((eeaddress / 64) + 1) * 64) - eeaddress;

  // Calculate first write size
  if (page_space > 16) {
    first_write_size = page_space - ((page_space / 16) * 16);
    if (first_write_size == 0) first_write_size = 16;
  }
  else
    first_write_size = page_space;

  // calculate size of last write
  if (data_len > first_write_size)
    last_write_size = (data_len - first_write_size) % 16;

  // Calculate how many writes we need
  if (data_len > first_write_size)
    num_writes = ((data_len - first_write_size) / 16) + 2;
  else
    num_writes = 1;

  i = 0;
  address = eeaddress;
  for (page = 0; page < num_writes; page++)
  {
    if (page == 0) write_size = first_write_size;
    else if (page == (num_writes - 1)) write_size = last_write_size;
    else write_size = 16;

    Wire.beginTransmission(deviceaddress);
    Wire.write((int)((address) >> 8));   // MSB
    Wire.write((int)((address) & 0xFF)); // LSB
    counter = 0;
    do {
      Wire.write((byte) data[i]);
      i++;
      counter++;
    }
    while ((data[i]) && (counter < write_size));
    Wire.endTransmission();
    address += write_size; // Increment address for next write

    delay(6);  // needs 5ms for page write
  }
}

void readEEPROM(int deviceaddress, unsigned int eeaddress, unsigned char* data, unsigned int num_chars)
{
  unsigned char i = 0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, num_chars);

  while (Wire.available()) data[i++] = Wire.read();

}

void romWrite(String str) {
  // troca caracteres acentuados por caracteres simples ASC127
  str.replace("ã", "@");
  str.replace("á", "#");
  str.replace("â", "$");
  str.replace("Á", "&");
  str.replace("é", "*");
  str.replace("ê", "/");
  str.replace("í", "<");
  str.replace("Í", ">");
  str.replace("ó", "_");
  str.replace("õ", "]");
  str.replace("ô", "[");
  str.replace("ú", "{");
  str.replace("ç", "}");

  char strAddr[8];
  sprintf(strAddr, "%05d-", addr);

  char buf[21];
  str.toCharArray(buf, 21);
  writeEEPROM(eeprom1, addr, buf);
  Serial.print(strAddr);
  Serial.print(buf);
  Serial.println();
  addr = addr + fieldLength;
  delay(10);
}

void SaveDataEEPROM() {
   // pergunta 7
   romWrite("0017|04|ó           ");
   romWrite("O que significa a   ");
   romWrite("palavra hieróglifos?");
   romWrite("                    ");
   romWrite("                    ");
   romWrite("Escrita dos homens  ");
   romWrite("Escrita sagrada    *");
   romWrite("Escrita do deserto  ");
   // pergunta 8
   romWrite("0021|04|óêé         ");
   romWrite("Quem era o faraó do ");
   romWrite("Egito durante o     ");
   romWrite("êxodo dos hebreus?  ");
   romWrite("                    ");
   romWrite("Seti I              ");
   romWrite("Ramsés II          *");
   romWrite("Ramsés I            ");
   // pergunta 9
   romWrite("0022|03|            ");
   romWrite("Os Medjay endureciam");
   romWrite("seus cabelos com:   ");
   romWrite("                    ");
   romWrite("                    ");
   romWrite("Cera               *");
   romWrite("Lodo                ");
   romWrite("Barro               ");
   // pergunta 10
   romWrite("0023|03|ó           ");
   romWrite("Com qual idade      ");
   romWrite("Tutancamun se tornou");
   romWrite("Faraó?              ");
   romWrite("                    ");
   romWrite("10 anos             ");
   romWrite("9 anos             *");
   romWrite("11 anos             ");
   // pergunta 11
   romWrite("0024|03|úíó         ");
   romWrite("Qual era o principal");
   romWrite("problema de saúde   ");
   romWrite("dos egípcios?       ");
   romWrite("                    ");
   romWrite("P. Respiratórios   *");
   romWrite("Ebola               ");
   romWrite("Tuberculose         ");
}

#include <Wire.h>
#define eeprom1 0x50    //Address of 24LC256 eeprom chip
#define recCount 47
#define linesCount 47*8

#define recordLength 160
#define fieldLength   20

unsigned char rdata[21];
char xdata[21];

unsigned int addr = 0;

void setup(void)
{
  Serial.begin(9600);
  Wire.begin();

 
    //addr = 328;
    //writeEEPROM(eeprom1, addr, "*");
    //addr = 320;
    //readEEPROM(eeprom1, addr, rdata, fieldLength);
    //strncpy(xdata, (char*)rdata, fieldLength);
    //Serial.println(xdata);
  //updateData();
  //return;

  Serial.println("DATA READ");
  addr = 0;
  for (int i = 0; i < linesCount; i++) {
    readEEPROM(eeprom1, addr, rdata, fieldLength);
    strncpy(xdata, (char*)rdata, fieldLength);

    char strAddr[8];
    sprintf(strAddr, "%05d", addr);
    Serial.print(strAddr);
    Serial.print(char(9));
    Serial.println(xdata);

    addr = addr + fieldLength;
    delay(10);
  }
}

void loop() {
}

// atualiza os valores de espera entre as perguntas e as alternativas
char* t[]={ 
  "02","03","02","02","03","04","02","03","02","04",
  "02","04","03","03","03","03","03","02","03","02",
  "04","04","03","03","03","02","03","04","03","05",
  "03","04","04","03","04","02","03","03","02","02",
  "03","03","03","02","02","04","05"
};
void updateData() {
  Serial.println("DATA UPDATE");
  addr = 0;
  for (int i = 0; i < recCount; i++) {
    writeEEPROM(eeprom1, addr+5, t[i]);

    readEEPROM(eeprom1, addr, rdata, fieldLength);
    strncpy(xdata, (char*)rdata, fieldLength);
    
    char strAddr[8];
    sprintf(strAddr, "%05d-", addr);
    Serial.print(strAddr);
    Serial.print(xdata);
    Serial.println();

    addr = addr + recordLength;
    delay(10);
  }
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


#include "FS.h"
#include "Arduino.h"
#include "BluetoothA2DPSink.h"
#include "Audio.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//SPI
#define CS  26
#define MOSI  27
#define SCK  13
#define MISO  14

//i2s
#define I2S_DOUT  32
#define I2S_BCLK  33
#define I2S_LRC   25

//tombol2
const int up = 23;
const int down = 22;
const int vol_up = 19;
const int vol_down = 21;

//mp3 player
Audio audio;
String playlist[1000];
int playing_mode = 1; //1. play all urut, 2. play random, 3. play 1 track
File RootDir;
int i = 0, jml, stepp = 1, folderaktif = 0;
bool mode_volume = false;
int menu_mp3[] = {0, 1};
String namafolder[100];
int jmlfolder = 0;
//
//SPIClass spi = SPIClass(VSPI);

//oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//radio internet
String url[100][2];

String IP, ssid, password;
int jml_stasiun , indexradio = 0;


//blutut
//I2SStream out;
//BluetoothA2DPSink a2dp_sink(out);
BluetoothA2DPSink a2dp_sink;


//variabel umum
int modee = 0; //0. awal nyala, 1. radio internet, 2. mp3 player, 3. bluetooth
int volume = 9;


//-----------------------------FUNGSI MP3 PLAYER---------------------------
void listDir(fs::FS &fs, const char * dirname, uint8_t levels, bool dir) {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  jml = 0;
  while (file) {
    if (dir) {
      if (file.isDirectory()) {
        String nama = file.name();
        if (nama != "setting" && nama != "System Volume Information" && nama != "/") {
          namafolder[jmlfolder] = nama;
          Serial.println(namafolder[jmlfolder]);
          jmlfolder++;
        }
      }
    } else {
      if (!file.isDirectory()) {
        String nama = file.name();
        if (nama.substring(nama.length() - 3, nama.length()) == "mp3") {
          playlist[jml] = nama;
          // Serial.println(playlist[jml]);
          jml++;
        }
      }
    }
    file = root.openNextFile();
  }
}

void createPlaylist(String nama_folder) {
  PrintText(1, " Reading..");
  i = 0;
  nama_folder = "/" + nama_folder;
  int str_len = nama_folder.length() + 1;
  char char_array[str_len];
  nama_folder.toCharArray(char_array, str_len);
  Serial.println(char_array);
  listDir(SD, char_array, 0, false);
}

void createListFolder () {
  listDir(SD, "/", 0, true);
}

void PlayNextSong(int next, bool tombol) {
  if (playing_mode == 1) { // playAll
    i = i + next;
    if (i >= jml) i = 0;
    if (i < 0)i = jml - 1;
  } else if (playing_mode == 2) { //random
    i = random(jml - 1);
  } else if (playing_mode == 3) { //Play 1
    if (tombol == true) {
      i = i + next;
      if (i == jml) i = 0;
      if (i < 0)i = jml - 1;
    }
  }
  String lagu = "/" + namafolder[folderaktif] + "/" + playlist[i];
  int str_len = lagu.length() + 1;
  char char_array[str_len];
  lagu.toCharArray(char_array, str_len);
  Serial.print("play : "); Serial.println(char_array);
  audio.connecttoFS(SD, char_array);
  PrintText(3, "");
}

void audio_eof_mp3(const char *info) { //end of file
  PlayNextSong(1, false);
}

//-----------------------------END MP3 PLAYER---------------------------

String readFileSetting(fs::FS &fs, const char * path) {
  String variabel = "";
  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return variabel;
  }
  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.print(".");
    variabel += (char)file.read();

  }
  Serial.println(variabel);
  file.close();
  return variabel;
}

void ListtStasiunRadio(fs::FS &fs, const char * path) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
  } else {
    while (file.available()) {
      String listt = file.readStringUntil('\n');
      url[jml_stasiun][0] = getValue(listt, '*', 0);
      url[jml_stasiun][1] = getValue(listt, '*', 1);
      jml_stasiun++;
    }
    file.close();
  }
}

String getValue(String data, char separator, int index) {
  int stringData = 0;        //variable to count data part nr
  String dataPart = "";      //variable to hole the return text

  for (int i = 0; i < data.length() - 1; i++) { //Walk through the text one letter at a time
    if (data[i] == separator) {
      //Count the number of times separator character appears in the text
      stringData++;
    } else if (stringData == index) {
      //get the text when separator is the rignt one
      dataPart.concat(data[i]);
    } else if (stringData > index) {
      //return text and stop if the next separator appears - to save CPU-time
      return dataPart;
      break;
    }
  }
  //return text if this is the last part
  return dataPart;
}

//-----------------------------DISPLAY-------------------------
void PrintText(int a, String pesan) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  if (a == 1) {                                                                 //pesan
    display.drawLine(0, 0, display.width() - 1, 0, SSD1306_WHITE);
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setCursor(0, 7);            // Start at top-left corner
    display.println(pesan);
    display.drawLine(0, display.height() - 1, display.width() - 1, display.height() - 1 , SSD1306_WHITE);
  }  else if (a == 2) {                                                         //radio internet
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setCursor(0, 0);            // Start at top-left corner
    display.print("IP  : ");
    display.println(IP);
    display.print("Vol : ");
    display.print(volume / 3);
    display.print(" /Chnl : ");
    display.print(indexradio + 1);
    display.print("/");
    display.println(jml_stasiun);
    display.print("Name: ");
    String nama = url[indexradio][0];
    nama.trim();
    display.println(nama);
  }  else if (a == 3) {                                                         //mp3
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setCursor(0, 0);            // Start at top-left corner
    display.print("Foldr: ");
    display.print(namafolder[folderaktif]);
    display.print(" /Vol: ");
    display.println(volume / 3);
    display.print("Track: ");
    display.print(i + 1);
    display.print("/");
    display.print(jml);
    display.print(" >> ");
    if (playing_mode == 1) {
      display.println("A");
    } else if (playing_mode == 2) {
      display.println("R");
    } else if (playing_mode == 3) {
      display.println("1");
    }

    display.print("File : ");
    display.println(playlist[i]);
  }


  display.display();
}

int stepp_sementara = 1;
void Print_Pilihan_Menu() {
  if (menu_mp3[1] == 1) {
    PrintTextMenuMP3("PILIH MODE PLAY", "PLAY ALL");
  } else if (menu_mp3[1] == 2) {
    PrintTextMenuMP3("PILIH MODE PLAY", "RANDOM");
  } else if (menu_mp3[1] == 3) {
    PrintTextMenuMP3("PILIH MODE PLAY", "1 TRACK");
  } else if (menu_mp3[1] >= 4) {
    PrintTextMenuMP3("PILIH FOLDER", namafolder[menu_mp3[1] - 4]);
  }
}

void PrintTextMenuMP3(String msg1, String msg2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);        // Draw white text

  //display.drawLine(0, 0, display.width() - 1, 0, SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.println(msg1);
  //  display.println();
  display.setCursor(0, 10);
  display.setTextSize(2);             // Normal 1:1 pixel scale
  // display.setCursor(0, 7);            // Start at top-left corner
  display.println(msg2);
  //  display.drawLine(0, display.height() - 1, display.width() - 1, display.height() - 1 , SSD1306_WHITE);
  display.display();
}
//------------------------------END DISPLAY--------------------

void setupAudio() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume);         // Check volume level and adjust if necassary
  audio.setTone(6, 3, 0); //-40 - 6
}

void setup() {
  //------------setup umum------------------
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(vol_up, INPUT);
  pinMode(vol_down, INPUT);
  Wire.begin(5, 18);
  Serial.begin(115200);
  //------------setup umum------------------

  //------------setup OLED------------------
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  //------------setup OLED------------------


  //------------setup SD card SPI------------------
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin(SCK, MISO, MOSI);
  if (!SD.begin(CS))
  {
    Serial.println("Error talking to SD card!");
    while (true);                 // end program
  }
  createListFolder();
  PrintText(1, " < RADIO >");



}

int mode_sementara = 1;
void loop() {
  audio.loop();
  if (modee == 0) { //awal nyala, milih mode
    if (digitalRead(up) == HIGH) {
      mode_sementara++;
      if (mode_sementara == 4) mode_sementara = 1;
      if (mode_sementara == 1)PrintText(1, " < RADIO >");
      if (mode_sementara == 2)PrintText(1, " <  MP3  >");
      if (mode_sementara == 3)PrintText(1, " BLUETOOTH");
      delay(200);
    }
    if (digitalRead(down) == HIGH) {
      mode_sementara--;
      if (mode_sementara == 0) mode_sementara = 3;
      if (mode_sementara == 1)PrintText(1, " < RADIO >");
      if (mode_sementara == 2)PrintText(1, " <  MP3  >");
      if (mode_sementara == 3)PrintText(1, " BLUETOOTH");
      delay(200);
    }
    if (digitalRead(vol_up) == HIGH) {//pilih mode
      modee = mode_sementara;
      if (modee == 1) {//mode radio internet
        PrintText(1, " READ FILE");
        setupAudio();
        delay(200);
        ssid = readFileSetting(SD, "/setting/ssid.txt");
        password = readFileSetting(SD, "/setting/password.txt");
        ListtStasiunRadio(SD, "/setting/stasiun.txt");
        PrintText(1, "CONNECTING");

        WiFi.begin(ssid.c_str(), password.c_str());

        delay(200);
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        IP = WiFi.localIP().toString();
        PrintText(1, " CONNECTED");
        indexradio = 15;
        audio.connecttohost(url[indexradio][1].c_str());
        audio.setTone(6, 3, 0); //-40 - 6
        delay(100);
        PrintText(2, "");
      }
      if (modee == 2) {
        setupAudio();
        delay(100);
        createPlaylist(namafolder[folderaktif]);
        delay(100);
        PlayNextSong(0, false); //mode mp3 player
      }

      if (modee == 3) { //bluetooth
        //WiFi.mode( WIFI_MODE_NULL );
        //  ESP.restart() ;
        ESP.getFreeHeap();
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
        delay(500);

        i2s_pin_config_t my_pin_config = {
          // .mck_io_num = 0,
          .mck_io_num = I2S_PIN_NO_CHANGE,
          .bck_io_num = I2S_BCLK,
          .ws_io_num = I2S_LRC,
          .data_out_num = I2S_DOUT,
          .data_in_num = I2S_PIN_NO_CHANGE
          //.data_in_num = 16
        };
        a2dp_sink.set_pin_config(my_pin_config);
        a2dp_sink.start("Musik3in1");




        PrintText(1, " Musik3in1");
      }
    }
    //---------------------------------------------------------------------------------------- radio internet
  } else if (modee == 1) {//mode radio internet
    if (digitalRead(up) == HIGH) {
      Serial.println("up");
      indexradio = indexradio + 1;
      if (indexradio == jml_stasiun)indexradio = 0;
      audio.connecttohost(url[indexradio][1].c_str());
      //  Serial.print("connect to :"); Serial.println(url[indexradio][0].c_str());
      //  Serial.print("connect to :"); Serial.println(url[indexradio][1].c_str());
      PrintText(2, "");
      delay(200);
    }
    if (digitalRead(down) == HIGH) {
      Serial.println("down");
      indexradio = indexradio - 1;
      if (indexradio < 0)indexradio = jml_stasiun - 1;
      audio.connecttohost(url[indexradio][1].c_str());
      PrintText(2, "");
      delay(200);
    }

    if (digitalRead(vol_up) == HIGH) {
      Serial.println("vol_up");
      volume += 3;
      if (volume >= 22) volume = 21;
      audio.setVolume(volume);
      PrintText(2, "");
      delay(200);
    }
    if (digitalRead(vol_down) == HIGH) {
      Serial.println("vol up");
      volume -= 3;
      if (volume < 0) volume = 0;
      audio.setVolume(volume);
      PrintText(2, "");
      delay(200);
    }
    //---------------------------------------------------------------------------mode mp3
  } else if (modee == 2) {//mode mp3
    if (digitalRead(up) == HIGH) {
      if (mode_volume == false) {
        if (menu_mp3[0] == 0) {
          PlayNextSong(stepp, true);
        } else if (menu_mp3[0] == 1) {    //pilihan menu level 1
          menu_mp3[1]++;
          if (menu_mp3[1] == (3 + jmlfolder + 1))menu_mp3[1] = 1;
          Print_Pilihan_Menu();
        }
      } else if (mode_volume == true) {
        volume += 3;
        if (volume > 21) volume = 21;
        audio.setVolume(volume);
        PrintText(1, ("  VOL : " + String(volume / 3)));
        //       PrintText(3, "");
        delay(200);
      }
      delay(200);
    }
    if (digitalRead(down) == HIGH) {
      if (mode_volume == false) {
        if (menu_mp3[0] == 0) {
          PlayNextSong(-stepp, true);
        } else if (menu_mp3[0] == 1) {
          menu_mp3[1]--;
          if (menu_mp3[1] == 0)menu_mp3[1] = 3 + jmlfolder;
          Print_Pilihan_Menu();
        }
      } else if (mode_volume == true) {
        volume -= 3;
        if (volume < 0) volume = 0;
        audio.setVolume(volume);
        PrintText(1, ("  VOL : " + String(volume / 3)));
      }
      delay(200);
    }
    if (digitalRead(vol_up) == HIGH) {
      if (mode_volume == false) {
        if (menu_mp3[0] == 0) {
          menu_mp3[0] = 1;
          Print_Pilihan_Menu();
        } else if (menu_mp3[0] == 1) {
          if (menu_mp3[1] <= 3) {               //mode play
            playing_mode = menu_mp3[1];
            menu_mp3[0] = 0;
            menu_mp3[1] = 1;
            PrintText(3, "");
          } else if (menu_mp3[1] > 3) {
            folderaktif = menu_mp3[1] - 4;
            createPlaylist(namafolder[folderaktif]);
            menu_mp3[0] = 0;
            menu_mp3[1] = 1;
            PlayNextSong(0, false);
            PrintText(3, "");
          }
        }
      }
      delay(200);
    }
    if (digitalRead(vol_down) == HIGH) {
      if (menu_mp3[0] == 0) {
        if (mode_volume == false) {
          PrintText(1, ("  VOL : " + String(volume / 3)));
          mode_volume = true;
        } else {
          PrintText(3, "");
          mode_volume = false;
        }
      } else if (menu_mp3[0] == 1) {
        menu_mp3[0] = 0;
        menu_mp3[1] = 1;
        PrintText(3, "");
      }
      delay(200);
    }
  }
}


void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
  String mystring(info);
  if (mystring.indexOf("could not be initialized") > 0) {
    if (modee == 1) {
      audio.connecttohost(url[indexradio][1].c_str());
    }
  }
}

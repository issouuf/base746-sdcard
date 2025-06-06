#include "lvgl.h"
#include "SPI.h"


#define MOSI D11
#define MISO D12
#define SCLK D13
#define CS D10



void update_affichage_score_tour(); 

int tour =0;
uint16_t dernièrePosition = 0;


static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
    //LV_LOG_USER("Clicked");
    tour = 0;
    update_affichage_score_tour();
  }
  else if (code == LV_EVENT_VALUE_CHANGED)
  {
    //LV_LOG_USER("Toggled");
  }
}

void testLvgl()




{
  // Initialisations générales
  lv_obj_t *label;

  lv_obj_t *btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

  label = lv_label_create(btn1);
  lv_label_set_text(label, "reset tour");
  lv_obj_center(label);

  // lv_obj_t *btn2 = lv_button_create(lv_screen_active());
  // lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
  // lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
  // lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
  // lv_obj_set_height(btn2, LV_SIZE_CONTENT);

  // label = lv_label_create(btn2);
  // lv_label_set_text(label, "Toggle");
  // lv_obj_center(label);
}


void boutonResetTour() {
    // Initialisations générales
  lv_obj_t *label;

  lv_obj_t *btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

  label = lv_label_create(btn1);
  lv_label_set_text(label, "reset tour");
  lv_obj_center(label);

} 



static lv_obj_t *label_tour = NULL;

// void affichage_score_tour() {
//   // Supprime le label précédent s'il existe
//   if (label_tour != NULL) {
//     lv_obj_del(label_tour);
//     label_tour = NULL;
//   }
//   // Crée un nouveau label
//   label_tour = lv_label_create(lv_screen_active());
//   lv_label_set_text_fmt(label_tour, "Tour: %d", tour);
//   lv_obj_align(label_tour, LV_ALIGN_TOP_MID, 0, 10);
// }

void init_affichage_score_tour() {
  label_tour = lv_label_create(lv_screen_active());
  lv_label_set_text_fmt(label_tour, "Tour: %d", tour);
  lv_obj_align(label_tour, LV_ALIGN_TOP_MID, 0, 10);
}

// À appeler à chaque changement de tour
void update_affichage_score_tour() {
  if(label_tour) {
    lv_label_set_text_fmt(label_tour, "Tour: %d", tour);
  }
}




#ifdef ARDUINO

#include "lvglDrivers.h"
#include "STM32SD.h"


void as5047d_init() {
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH); // Désactive le CS

  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1)); // 1 MHz, MSB first, mode 3!
}


uint16_t as5047d_read() {
uint16_t command = 0xFFFF; // NOP pour récupérer la dernière valeur lue
  uint16_t angle = 0;

  // 1. Envoi de la commande de lecture d'angle (0x3FFF)
  digitalWrite(CS, LOW);
  SPI.transfer16(0x3FFF);
  digitalWrite(CS, HIGH);

  delayMicroseconds(1);

  // 2. Lecture de la réponse lors d'un NOP
  digitalWrite(CS, LOW);
  angle = SPI.transfer16(command);
  digitalWrite(CS, HIGH);

  // Les 14 bits de poids faible contiennent l'angle
  return angle & 0x3FFF;
}


void affichage(void *pvParameters)
{
  // Init
  TickType_t xLastWakeTime;
  // Lecture du nombre de ticks quand la tâche débute
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    // Loop
    update_affichage_score_tour();

    // Endort la tâche pendant le temps restant par rapport au réveil,
    // ici 200ms, donc la tâche s'effectue toutes les 200ms
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200)); // toutes les 100 ms
  }
}

void myTask(void *pvParameters)
{
  // Init
  TickType_t xLastWakeTime;
  // Lecture du nombre de ticks quand la tâche débute
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    // Loop

    //? test lecture de la postion
    //! valeur max = 16383 (0x3FFF) ou 2^14 -1

    uint16_t position = as5047d_read();
    // Serial.print("Position: ");
    // Serial.println(position); 

    int16_t delta = position - dernièrePosition;

    if(delta > 8192) {
      tour --;
    }else if (delta < -8192) {
      tour ++;
    }
    dernièrePosition = position;

    Serial.print("Tour: ");
    Serial.println(tour);


    // Endort la tâche pendant le temps restant par rapport au réveil,
    // ici 200ms, donc la tâche s'effectue toutes les 200ms
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); // toutes les 10 ms
  }
}




Sd2Card card;
SdFatFs fatFs;

void mySetup()
{  
  
  as5047d_init();


  
  bool disp = false;

  Serial.print("\nInitializing SD card...");
  while (!card.init(SD_DETECT_PIN))
  {
    if (!disp)
    {
      Serial.println("initialization failed. Is a card inserted?");
      disp = true;
    }
    delay(10);
  }

  Serial.println("A card is present.");

  // print the type of card
  Serial.print("\nCard type: ");
  switch (card.type())
  {
  case SD_CARD_TYPE_SD1:
    Serial.println("SD1");
    break;
  case SD_CARD_TYPE_SD2:
    Serial.println("SD2");
    break;
  case SD_CARD_TYPE_SDHC:
    Serial.println("SDHC");
    break;
  default:
    Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!fatFs.init())
  {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }

  // print the type and size of the first FAT-type volume
  uint64_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(fatFs.fatType(), DEC);
  Serial.println();

  volumesize = fatFs.blocksPerCluster(); // clusters are collections of blocks
  volumesize *= fatFs.clusterCount();    // we'll have a lot of clusters
  volumesize *= 512;                     // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  File root = SD.openRoot();

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
  root.close();
  Serial.println("###### End of the SD tests ######");

  lv_tjpgd_init();

  lv_obj_t *icon = lv_image_create(lv_screen_active());

  /*From file*/
  lv_image_set_src(icon, "A:/minion5.bmp");

  lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

  //testLvgl();
  init_affichage_score_tour();
  boutonResetTour();


  xTaskCreate(affichage, "Affichage", 2048, NULL, 1, NULL);

}

void loop(void)
{
  // do nothing
}

#else

#include "lvgl.h"
#include "app_hal.h"
#include <cstdio>

int main(void)
{
  printf("LVGL Simulator\n");
  fflush(stdout);

  lv_init();
  hal_setup();

  testLvgl();

  hal_loop();
  return 0;
}

#endif

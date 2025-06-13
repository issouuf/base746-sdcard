#include "lvgl.h"
#include "SPI.h"
#include "lvglDrivers.h"

#define MOSI D11
#define MISO D12
#define SCLK D13
#define CS D10



void update_affichage_score_tour(); 
void maj_arc_progression(uint16_t position);
void show_page_tours();
void show_page_arc();

volatile int tour =0;
volatile int bp =0;
uint16_t dernièrePosition = 0;


lv_obj_t *page_tours = NULL;
lv_obj_t *page_arc = NULL;
lv_obj_t *label_tour = NULL;
lv_obj_t *arc = NULL;
lv_obj_t *btn_switch1 = NULL;
lv_obj_t *btn_switch2 = NULL;
lv_obj_t *btn_reset = NULL;
lv_obj_t *label_arc_value = NULL;
lv_obj_t *page_roue = NULL;
lv_obj_t *img_roue = NULL;
lv_obj_t *rect_roue = NULL;
lv_obj_t *btn_retour_roue = NULL;
lv_obj_t *label_pourcentage = NULL;
lv_obj_t *page_tirage = NULL;
lv_obj_t *spinbox_min = NULL;
lv_obj_t *spinbox_max = NULL;
lv_obj_t *label_tirage_val = NULL;
lv_obj_t *btn_lancer = NULL;
lv_obj_t *btn_retour_tirage = NULL;
lv_obj_t *btn_tirage = NULL;
lv_obj_t *btn_roue = NULL;


volatile bool tirage_en_cours = false;
volatile int tirage_val = 0;
volatile int tirage_min = 0;
volatile int tirage_max = 100;
volatile uint16_t tirage_last_pos = 0;




void show_page_tours();
void show_page_arc();




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



static void event_handler_switch(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *target_page = (lv_obj_t *)lv_event_get_user_data(e);
        // Cache toutes les pages (si elles existent)
        if(page_tours) lv_obj_add_flag(page_tours, LV_OBJ_FLAG_HIDDEN);
        if(page_arc) lv_obj_add_flag(page_arc, LV_OBJ_FLAG_HIDDEN);
        if(page_roue) lv_obj_add_flag(page_roue, LV_OBJ_FLAG_HIDDEN);
        if(page_tirage) lv_obj_add_flag(page_tirage, LV_OBJ_FLAG_HIDDEN);
        // Affiche la page cible (seulement si elle existe)
        if(target_page) {
            lv_obj_clear_flag(target_page, LV_OBJ_FLAG_HIDDEN);
        } else {
            LV_LOG_ERROR("Page cible NULL dans event_handler_switch !");
        }
    }
}

static void event_handler_reset(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bp = 1;
        LV_LOG_USER("Appuie sur Reset tour");
    }
}



// static void event_handler_switch_tours(lv_event_t *e) {
//     lv_obj_add_flag(page_tirage, LV_OBJ_FLAG_HIDDEN);
//     lv_obj_clear_flag(page_tours, LV_OBJ_FLAG_HIDDEN);
// }



static void event_handler_lancer_tirage(lv_event_t *e) {
    // Récupère min et max
    tirage_min = lv_spinbox_get_value(spinbox_min);
    tirage_max = lv_spinbox_get_value(spinbox_max);
    if(tirage_min > tirage_max) {
        int tmp = tirage_min;
        tirage_min = tirage_max;
        tirage_max = tmp;
    }
    tirage_en_cours = true;
    tirage_last_pos = as5047d_read();
    lv_label_set_text(label_tirage_val, "Tourne la molette...");
}


void creer_page_tours() {
    page_tours = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_tours, LV_PCT(100), LV_PCT(100));

    label_tour = lv_label_create(page_tours);
    lv_label_set_text_fmt(label_tour, "Tours: %d", tour);
    lv_obj_align(label_tour, LV_ALIGN_CENTER, 0, 0);

    btn_reset = lv_button_create(page_tours);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_reset, event_handler_reset, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_reset = lv_label_create(btn_reset);
    lv_label_set_text(label_reset, "Reset tour");
    lv_obj_center(label_reset);

    btn_switch1 = lv_button_create(page_tours);
    lv_obj_align(btn_switch1, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_t *label_switch1 = lv_label_create(btn_switch1);
    lv_label_set_text(label_switch1, "Progression");
    lv_obj_center(label_switch1);

    btn_tirage = lv_button_create(page_tours);
    lv_obj_align(btn_tirage, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t *label_tirage = lv_label_create(btn_tirage);
    lv_label_set_text(label_tirage, "Tirage");
    lv_obj_center(label_tirage);
}

void creer_page_arc() {
    page_arc = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_arc, LV_PCT(100), LV_PCT(100));

    arc = lv_arc_create(page_arc);
    lv_obj_set_size(arc, 150, 150);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_range(arc, 0, 16383);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);

    label_arc_value = lv_label_create(page_arc);
    lv_label_set_text(label_arc_value, "0");
    lv_obj_align_to(label_arc_value, arc, LV_ALIGN_CENTER, 0, 0);

    label_pourcentage = lv_label_create(page_arc);
    lv_label_set_text_fmt(label_pourcentage, "Pourcentage: %d%%", 0);
    lv_obj_align_to(label_pourcentage, label_arc_value, LV_ALIGN_OUT_BOTTOM_MID, 0, -120);

    btn_switch2 = lv_button_create(page_arc);
    lv_obj_align(btn_switch2, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_t *label_switch2 = lv_label_create(btn_switch2);
    lv_label_set_text(label_switch2, "Tours");
    lv_obj_center(label_switch2);

    btn_roue = lv_button_create(page_arc);
    lv_obj_align(btn_roue, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_t *label_roue = lv_label_create(btn_roue);
    lv_label_set_text(label_roue, "Roue");
    lv_obj_center(label_roue);
}

void creer_page_roue() {
    page_roue = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_roue, LV_PCT(100), LV_PCT(100));

    // Image roue centrée
    img_roue = lv_img_create(page_roue);
    lv_img_set_src(img_roue, "A:/roue_250.bmp");
    lv_obj_align(img_roue, LV_ALIGN_CENTER, 0, 0);

    // Rectangle rouge centré sur la roue
    rect_roue = lv_obj_create(page_roue);
    lv_obj_set_size(rect_roue, 10, 65); // largeur, hauteur du rectangle
    lv_obj_set_style_bg_color(rect_roue, lv_color_hex(0xFF0000), 0); // rouge
    lv_obj_set_style_radius(rect_roue, 3, 0);
    // Aligne le rectangle exactement au centre de la roue, sans décalage
    lv_obj_align_to(rect_roue, img_roue, LV_ALIGN_CENTER, 0, -44);
    lv_obj_set_style_border_width(rect_roue, 0, 0);


    // Bouton retour (vers progression)
    btn_retour_roue = lv_button_create(page_roue);
    lv_obj_align(btn_retour_roue, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_retour_roue, event_handler_switch, LV_EVENT_CLICKED, page_arc);
    lv_obj_t *label_retour = lv_label_create(btn_retour_roue);
    lv_label_set_text(label_retour, "Retour");
    lv_obj_center(label_retour);

    // Cache la page au début
    lv_obj_add_flag(page_roue, LV_OBJ_FLAG_HIDDEN);
}


void creer_page_tirage() {
    page_tirage = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_tirage, LV_PCT(100), LV_PCT(100));

    // Spinbox min
    lv_obj_t *label_min = lv_label_create(page_tirage);
    lv_label_set_text(label_min, "Min:");
    lv_obj_align(label_min, LV_ALIGN_TOP_LEFT, 20, 20);

    spinbox_min = lv_spinbox_create(page_tirage);
    lv_spinbox_set_range(spinbox_min, 0, 9999);
    lv_spinbox_set_value(spinbox_min, 0);
    lv_obj_align_to(spinbox_min, label_min, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // Spinbox max
    lv_obj_t *label_max = lv_label_create(page_tirage);
    lv_label_set_text(label_max, "Max:");
    lv_obj_align(label_max, LV_ALIGN_TOP_LEFT, 20, 70);

    spinbox_max = lv_spinbox_create(page_tirage);
    lv_spinbox_set_range(spinbox_max, 0, 9999);
    lv_spinbox_set_value(spinbox_max, 100);
    lv_obj_align_to(spinbox_max, label_max, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // Label pour le tirage
    label_tirage_val = lv_label_create(page_tirage);
    lv_label_set_text(label_tirage_val, "Appuie sur Lancer !");
    lv_obj_align(label_tirage_val, LV_ALIGN_CENTER, 0, -30);

    // Bouton lancer
    btn_lancer = lv_button_create(page_tirage);
    lv_obj_align(btn_lancer, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn_lancer, event_handler_lancer_tirage, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_lancer = lv_label_create(btn_lancer);
    lv_label_set_text(label_lancer, "Lancer");
    lv_obj_center(label_lancer);

    // Bouton retour
    btn_retour_tirage = lv_button_create(page_tirage);
    lv_obj_align(btn_retour_tirage, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_retour_tirage, event_handler_switch, LV_EVENT_CLICKED, page_tours);
    lv_obj_t *label_retour = lv_label_create(btn_retour_tirage);
    lv_label_set_text(label_retour, "Retour");
    lv_obj_center(label_retour);

    
    // Cache la page au début
    lv_obj_add_flag(page_tirage, LV_OBJ_FLAG_HIDDEN);
}


void connecter_boutons_navigation() {
    if(btn_switch1 && page_arc)
        lv_obj_add_event_cb(btn_switch1, event_handler_switch, LV_EVENT_CLICKED, page_arc);
    if(btn_switch2 && page_tours)
        lv_obj_add_event_cb(btn_switch2, event_handler_switch, LV_EVENT_CLICKED, page_tours);
    if(btn_tirage && page_tirage)
        lv_obj_add_event_cb(btn_tirage, event_handler_switch, LV_EVENT_CLICKED, page_tirage);
    if(btn_retour_tirage && page_tours)
        lv_obj_add_event_cb(btn_retour_tirage, event_handler_switch, LV_EVENT_CLICKED, page_tours);
    if(btn_roue && page_roue)
        lv_obj_add_event_cb(btn_roue, event_handler_switch, LV_EVENT_CLICKED, page_roue);
    if(btn_retour_roue && page_arc)
        lv_obj_add_event_cb(btn_retour_roue, event_handler_switch, LV_EVENT_CLICKED, page_arc);
}


void update_rect_roue_rotation(uint16_t position) {
    if(rect_roue) {
        int32_t angle = (position * 3600) / 16384;
        lv_obj_set_style_transform_angle(rect_roue, angle, 0);
        // Centre le pivot de rotation à la base, au milieu du rectangle
        lv_obj_set_style_transform_pivot_x(rect_roue, lv_obj_get_width(rect_roue)/2, 0);
        lv_obj_set_style_transform_pivot_y(rect_roue, lv_obj_get_height(rect_roue), 0);
    }
}


static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
   bp=1;
   LV_LOG_USER("Appuie sur Reset tour");
   }
}


void boutonResetTour() {
    // Initialisations générales
  lv_obj_t *label;

  lv_obj_t *btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, 100);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

  LV_LOG_USER("Bouton reset tour");

  label = lv_label_create(btn1);
  lv_label_set_text(label, "reset tour");
  lv_obj_center(label);

} 



void init_affichage_score_tour() {
  label_tour = lv_label_create(lv_screen_active());
  lv_label_set_text_fmt(label_tour, "Tour: %d", tour);
  lv_obj_align(label_tour, LV_ALIGN_TOP_MID, 0, 10);
}

// À appeler à chaque changement de tour
void update_affichage_score_tour() {
  if(label_tour) {
    lv_label_set_text_fmt(label_tour, "Tour: %d", tour);
    LV_LOG_USER("Tour: %d", tour);
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






void affichage(void *pvParameters)
{
  // Init
  TickType_t xLastWakeTime;
  // Lecture du nombre de ticks quand la tâche débute
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    // Loop

    //update_affichage_score_tour();
    uint16_t position = as5047d_read();
    lvglLock();
    maj_arc_progression(position);
    update_rect_roue_rotation(position);
    


    if (tirage_en_cours) {
    int delta = abs((int)position - (int)tirage_last_pos);
    int range = tirage_max - tirage_min + 1;
    if(range <= 0) range = 1;
    // Fait défiler le nombre
    int val = tirage_min + (position % range);
    static char buf[32];
    snprintf(buf, sizeof(buf), "Nombre: %d", val);
    lv_label_set_text(label_tirage_val, buf);

    // Si l'encodeur est arrêté (delta < 30), on fige la valeur
    if(delta < 10) {
        snprintf(buf, sizeof(buf), "Tiré: %d", val);
        lv_label_set_text(label_tirage_val, buf);
        tirage_en_cours = false;
      }
    tirage_last_pos = position;
    }
    lvglUnlock();

    vTaskDelay(pdMS_TO_TICKS(5)); // toutes les 100 ms
  }
}



void maj_arc_progression(uint16_t position) {
    if(arc) {
        lv_arc_set_value(arc, position);
        if(label_arc_value) {
            static char buf[8];
            snprintf(buf, sizeof(buf), "%u", position);
            lv_label_set_text(label_arc_value, buf);
            lv_label_set_text_fmt(label_pourcentage, "Pourcentage: %d%%", (position * 100) / 16383);
        }
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

    if(bp ==1) {
      tour = 0;
    lvglLock();
    update_affichage_score_tour();
    lvglUnlock();
      bp= 0;
    }

    else if(delta > 8192) {
      tour --;
          lvglLock();
    update_affichage_score_tour();
    lvglUnlock();
    }else if (delta < -8192) {
      tour ++;
    lvglLock();
    update_affichage_score_tour();
    lvglUnlock();
    }
    dernièrePosition = position;
//    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); // toutes les 10 ms
    vTaskDelay(pdMS_TO_TICKS(5)); // toutes les 10 ms
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



  //testLvgl();
  creer_page_tours();
  creer_page_arc();
  creer_page_roue();
  creer_page_tirage();
  connecter_boutons_navigation();

  lv_obj_add_flag(page_arc, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(page_roue, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(page_tirage, LV_OBJ_FLAG_HIDDEN);

  xTaskCreate(affichage, "Affichage", 4096, NULL, 2, NULL);

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

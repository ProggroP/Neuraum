// Neuraum -- Nachbildung einer klassischen Flieger-Automatik fuer Gabbro.
//
// Aufbau von aussen nach innen, wie beim Vorbild:
// - schwarzer Flansch mit cremefarbenen Zahlen 12, 2, 4, 6, 8, 10, Balken bei
//   den ungeraden Stunden und einer feinen Minuterie an der Innenkante
// - cremefarbenes Zifferblatt mit Wochentagsfenster unter der 12, Schriftzug
//   und Fluegel-Logo, Datumsfenster bei 3 und "automatic" ueber der 6
// - schwarze Schwertzeiger mit Leuchtmasse-Einlage, duenner roter Sekundenzeiger
//
// Ein .c, canvas_update ist der einzige Zeichen-Einstiegspunkt. Ohne
// Sekundenzeiger zeichnet das Watchface nur einmal pro Minute.

#include <pebble.h>

// --------------------------------------------------------------- Diagnose

// Nur fuer Store-Screenshots: haelt die Anzeige auf 10:10:37 an einem
// Montag, dem 25. Vor dem Release wieder auskommentieren.
// #define NEURAUM_DEMO_TIME

// --------------------------------------------------------------- Farben

// Gabbro bildet die Palette entsaettigt ab: Icterine und PastelYellow kommen
// als Creme heraus, Rot als Lachs. Genau das passt hier zum Vorbild.
typedef struct {
  GColor bezel;        // Flansch
  GColor bezel_print;  // Zahlen, Balken und Minuterie auf dem Flansch
  GColor dial;         // Zifferblatt
  GColor print;        // Schrift, Logo und Indexstriche auf dem Blatt
  GColor hand;         // Zeigerkoerper und Mittelkappe
  GColor lume;         // Leuchtmasse in den Zeigern
  GColor seconds;      // Sekundenzeiger
  GColor accent;       // "automatic"
  GColor window;       // Hintergrund der Datumsfenster
  GColor window_text;
  GColor frame;        // Rahmen der Datumsfenster
} Theme;

#define THEME_COUNT 5

static const Theme THEMES[THEME_COUNT] = {
  { // 0 Cream -- das Vorbild
    .bezel = {GColorBlackARGB8},       .bezel_print = {GColorIcterineARGB8},
    .dial = {GColorPastelYellowARGB8}, .print = {GColorBlackARGB8},
    .hand = {GColorBlackARGB8},        .lume = {GColorIcterineARGB8},
    .seconds = {GColorRedARGB8},       .accent = {GColorDarkCandyAppleRedARGB8},
    .window = {GColorWhiteARGB8},      .window_text = {GColorBlackARGB8},
    .frame = {GColorLightGrayARGB8},
  },
  { // 1 Anthracite
    .bezel = {GColorBlackARGB8},       .bezel_print = {GColorIcterineARGB8},
    .dial = {GColorDarkGrayARGB8},     .print = {GColorWhiteARGB8},
    .hand = {GColorLightGrayARGB8},      .lume = {GColorIcterineARGB8},
    .seconds = {GColorRedARGB8},       .accent = {GColorRedARGB8},
    .window = {GColorWhiteARGB8},      .window_text = {GColorBlackARGB8},
    .frame = {GColorLightGrayARGB8},
  },
  { // 2 Blue
    .bezel = {GColorBlackARGB8},       .bezel_print = {GColorWhiteARGB8},
    .dial = {GColorCobaltBlueARGB8},   .print = {GColorWhiteARGB8},
    .hand = {GColorLightGrayARGB8},      .lume = {GColorIcterineARGB8},
    .seconds = {GColorRedARGB8},       .accent = {GColorIcterineARGB8},
    .window = {GColorWhiteARGB8},      .window_text = {GColorBlackARGB8},
    .frame = {GColorLightGrayARGB8},
  },
  { // 3 Silver
    .bezel = {GColorBlackARGB8},       .bezel_print = {GColorWhiteARGB8},
    .dial = {GColorLightGrayARGB8},    .print = {GColorBlackARGB8},
    .hand = {GColorBlackARGB8},        .lume = {GColorIcterineARGB8},
    .seconds = {GColorRedARGB8},       .accent = {GColorDarkCandyAppleRedARGB8},
    .window = {GColorWhiteARGB8},      .window_text = {GColorBlackARGB8},
    .frame = {GColorDarkGrayARGB8},
  },
  { // 4 Olive
    .bezel = {GColorBlackARGB8},       .bezel_print = {GColorIcterineARGB8},
    .dial = {GColorArmyGreenARGB8},    .print = {GColorIcterineARGB8},
    .hand = {GColorLightGrayARGB8},      .lume = {GColorIcterineARGB8},
    .seconds = {GColorRedARGB8},       .accent = {GColorIcterineARGB8},
    .window = {GColorWhiteARGB8},      .window_text = {GColorBlackARGB8},
    .frame = {GColorLightGrayARGB8},
  },
};

// --------------------------------------------------------------- Layout

// Alles am Flansch wird als Abstand vom Displayrand angegeben ("inset"),
// gemessen entlang des Strahls aus der Mitte. Auf Gabbro ist der Rand ein
// Kreis, auf Emery ein abgerundetes Rechteck -- edge_point() rechnet beides,
// die Zeichenroutinen bleiben dieselben. Die Verhaeltnisse sind am Foto des
// Vorbilds abgemessen (Blattrand bei rund 75 % des Radius, Tagesfenster bei
// 60 %, Datum bei 51 %).
//
// Der Flansch muss so breit sein, dass die "10" auf ihrer Diagonale Platz
// hat: gut 23 x 17 px Ziffernbild brauchen auf 300 Grad rund 28 px radial.
#if defined(PBL_PLATFORM_EMERY)
  #define CORNER_R        56   // Eckenradius am Displayrand; nach innen konzentrisch
  #define DIAL_INSET      28   // Blattrand = Innenkante des Flanschs
  #define BAR_INSET_OUT    5   // Balken bei 1, 3, 5, 7, 9, 11 auf dem Flansch
  #define BAR_INSET_IN    21
  #define NUMERAL_INSET   14   // Mitte der Flanschzahlen

  #define DAY_DIST        62   // Mitte des Tagesfensters ueber der Mitte
  #define BRAND_DIST      40   // Schriftzug ueber der Mitte
  #define LOGO_DIST       24   // Fluegel-Logo ueber der Mitte
  #define DATE_DIST       46   // Mitte des Datumsfensters rechts der Mitte
  #define AUTO_DIST       44   // "automatic" unter der Mitte
  #define MADE_DIST      100   // "MADE IN GERMANY" unter der Mitte
#else
  #define DIAL_INSET      32
  #define BAR_INSET_OUT    6
  #define BAR_INSET_IN    21
  #define NUMERAL_INSET   16

  #define DAY_DIST        76
  #define BRAND_DIST      50
  #define LOGO_DIST       32
  #define DATE_DIST       66
  #define AUTO_DIST       52
  #define MADE_DIST      114
#endif

#define MINUTE_LEN         4   // Minuterie: auf dem Blatt, endet am Flansch
#define INDEX_LEN          7   // dunkle Fortsetzung der Balken auf dem Blatt
#define BAR_W              3
#define NUMERAL_LIFT      14   // Abstand Textbox-Oberkante zur Ziffernmitte
#define MADE_X            33   // "MADE IN" / "GERMANY" links und rechts der 6

#define DAY_W             78
#define DAY_H             18
#define DATE_W            24
#define DATE_H            20
#define FRAME_W            2

// Gothic zeichnet mit Luft ueber den Grossbuchstaben. Diese Werte ziehen die
// Textbox so weit nach oben, dass die Schrift optisch mittig sitzt.
#define LIFT_GOTHIC_14     9
#define LIFT_GOTHIC_18    12
#define LIFT_GOTHIC_09     5

// Zeiger in lokalen Koordinaten: y negativ zeigt zur Spitze. Schwertform,
// zur Spitze hin breiter, mit schmaler Leuchtmasse-Einlage. Auf Emery reicht
// der Minutenzeiger bis kurz vor die Minuterie an den Laengsseiten.
#if defined(PBL_PLATFORM_EMERY)
  #define MIN_HAND_LEN    66
  #define HOUR_HAND_LEN   46
  #define SEC_HAND_LEN    70
  #define SEC_TAIL        18
  #define HUB_R            5
static const GPoint MIN_BODY[] = {{-3, 9}, {3, 9}, {5, -50}, {0, -MIN_HAND_LEN}, {-5, -50}};
static const GPoint MIN_LUME[] = {{-1, -14}, {1, -14}, {3, -48}, {0, -59}, {-3, -48}};
static const GPoint HOUR_BODY[] = {{-4, 9}, {4, 9}, {6, -31}, {0, -HOUR_HAND_LEN}, {-6, -31}};
static const GPoint HOUR_LUME[] = {{-1, -14}, {1, -14}, {3, -30}, {0, -39}, {-3, -30}};
#else
  #define MIN_HAND_LEN    92
  #define HOUR_HAND_LEN   64
  #define SEC_HAND_LEN    96
  #define SEC_TAIL        24
  #define HUB_R            6
static const GPoint MIN_BODY[] = {{-3, 10}, {3, 10}, {6, -70}, {0, -MIN_HAND_LEN}, {-6, -70}};
static const GPoint MIN_LUME[] = {{-1, -16}, {1, -16}, {3, -68}, {0, -82}, {-3, -68}};
static const GPoint HOUR_BODY[] = {{-4, 10}, {4, 10}, {7, -44}, {0, -HOUR_HAND_LEN}, {-7, -44}};
static const GPoint HOUR_LUME[] = {{-1, -16}, {1, -16}, {4, -43}, {0, -55}, {-4, -43}};
#endif
#define SEC_W              2

// Fluegel-Logo: Dreieck mit je drei Schwingen, Punkte relativ zur Logomitte.
static const GPoint LOGO_TRI[] = {{-5, -3}, {5, -3}, {0, 5}};
#define WING_X_IN          7
static const int WING_LEN[] = {17, 13, 9};

// --------------------------------------------------------------- Texte

static const char *DAY_NAMES[2][7] = {
  {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"},
  {"SONNTAG", "MONTAG", "DIENSTAG", "MITTWOCH", "DONNERSTAG", "FREITAG", "SAMSTAG"},
};

#define BRAND_TEXT  "neuraum aviation"
#define AUTO_TEXT   "automatic"

// --------------------------------------------------------------- Einstellungen

#define PERSIST_THEME      1
#define PERSIST_SECONDS    2
#define PERSIST_SHOW_DAY   3
#define PERSIST_SHOW_DATE  4
#define PERSIST_LANG       5

static int s_theme = 0;
static bool s_seconds = true;
static bool s_show_day = true;
static bool s_show_date = true;
static int s_lang = 0;

// --------------------------------------------------------------- Zustand

static Window *s_window;
static Layer *s_canvas;
static GFont s_font_numerals;
static struct tm s_now;

// --------------------------------------------------------------- Hilfen

// Punkt auf einem Kreis um c; Winkel wie beim Zifferblatt, 0 = oben,
// im Uhrzeigersinn.
static GPoint polar(GPoint c, int32_t angle, int r) {
  return GPoint(c.x + (int)(sin_lookup(angle) * r / TRIG_MAX_RATIO),
                c.y - (int)(cos_lookup(angle) * r / TRIG_MAX_RATIO));
}

#if defined(PBL_PLATFORM_EMERY)
// Die SDK bringt kein sqrt mit; ein paar Newton-Schritte reichen fuer Pixel.
static float fsqrt(float v) {
  if (v <= 0) return 0;
  float x = v > 1 ? v : 1;
  for (int i = 0; i < 12; i++) x = 0.5f * (x + v / x);
  return x;
}
#endif

// Punkt im Abstand inset vom Displayrand auf dem Strahl unter angle. Auf
// Emery schneidet der Strahl ein abgerundetes Rechteck, dessen Ecken nach
// innen konzentrisch kleiner werden (Radius CORNER_R - inset).
static GPoint edge_point(GPoint c, int32_t angle, int inset) {
#if defined(PBL_PLATFORM_EMERY)
  const float dx = (float)sin_lookup(angle) / TRIG_MAX_RATIO;
  const float dy = -(float)cos_lookup(angle) / TRIG_MAX_RATIO;
  const float ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  const float hx = c.x - inset, hy = c.y - inset;
  const float r = inset < CORNER_R ? CORNER_R - inset : 0;

  // erst gegen die geraden Seiten, im Eckbereich dann gegen den Viertelkreis
  float t = (ax * hy > ay * hx) ? hx / ax : hy / ay;
  const float ccx = hx - r, ccy = hy - r;
  if (t * ax > ccx && t * ay > ccy) {
    const float b = ax * ccx + ay * ccy;
    t = b + fsqrt(b * b - (ccx * ccx + ccy * ccy) + r * r);
  }
  return GPoint(c.x + (int)(dx * t + (dx < 0 ? -0.5f : 0.5f)),
                c.y + (int)(dy * t + (dy < 0 ? -0.5f : 0.5f)));
#else
  return polar(c, angle, c.x - inset);
#endif
}

static void edge_line(GContext *ctx, GPoint c, int32_t angle, int inset_out, int inset_in) {
  graphics_draw_line(ctx, edge_point(c, angle, inset_out), edge_point(c, angle, inset_in));
}

// Text so setzen, dass seine optische Mitte auf (cx, cy) liegt.
static void draw_text_at(GContext *ctx, const char *text, GFont font,
                         int cx, int cy, int lift, int w) {
  graphics_draw_text(ctx, text, font, GRect(cx - w / 2, cy - lift, w, 2 * lift + 8),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Polygon um c drehen und gefuellt zeichnen. Die Kante wird zusaetzlich mit
// Antialiasing nachgezogen, die Fuellung selbst ist auf Pebble immer hart.
#define MAX_POLY 6
static void draw_rotated(GContext *ctx, const GPoint *src, int n, GPoint c,
                         int32_t angle, GColor color) {
  GPoint pts[MAX_POLY];
  const int32_t s = sin_lookup(angle), co = cos_lookup(angle);
  for (int i = 0; i < n; i++) {
    pts[i].x = c.x + (int)((src[i].x * co - src[i].y * s) / TRIG_MAX_RATIO);
    pts[i].y = c.y + (int)((src[i].x * s + src[i].y * co) / TRIG_MAX_RATIO);
  }
  GPath path = { .num_points = n, .points = pts, .rotation = 0, .offset = GPointZero };
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 1);
  gpath_draw_filled(ctx, &path);
  gpath_draw_outline(ctx, &path);
}

// Fenster mit hellem Rahmen, wie die eingesetzten Datumsscheiben.
static void draw_window(GContext *ctx, GRect r, const Theme *t, const char *text,
                        GFont font, int lift) {
  graphics_context_set_fill_color(ctx, t->frame);
  graphics_fill_rect(ctx, r, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, t->window);
  graphics_fill_rect(ctx, grect_inset(r, GEdgeInsets(FRAME_W)), 0, GCornerNone);
  graphics_context_set_text_color(ctx, t->window_text);
  draw_text_at(ctx, text, font, r.origin.x + r.size.w / 2,
               r.origin.y + r.size.h / 2, lift, r.size.w);
}

// --------------------------------------------------------------- Zeichnen

static void draw_bezel(GContext *ctx, GRect bounds, GPoint c, const Theme *t) {
  graphics_context_set_fill_color(ctx, t->bezel);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, t->dial);
#if defined(PBL_PLATFORM_EMERY)
  graphics_fill_rect(ctx, grect_inset(bounds, GEdgeInsets(DIAL_INSET)),
                     CORNER_R - DIAL_INSET, GCornersAll);
#else
  graphics_fill_circle(ctx, c, c.x - DIAL_INSET);
#endif

  // Minuterie: feine Striche am Blattrand, ausser bei den Stunden. Sie
  // enden an der Flanschkante und ragen nicht in den schwarzen Rand.
  graphics_context_set_stroke_color(ctx, t->print);
  graphics_context_set_stroke_width(ctx, 1);
  for (int m = 0; m < 60; m++) {
    if (m % 5 == 0) continue;
    edge_line(ctx, c, TRIG_MAX_ANGLE * m / 60, DIAL_INSET + 1, DIAL_INSET + MINUTE_LEN);
  }

  // Ungerade Stunden: heller Balken auf dem Flansch, dunkler auf dem Blatt
  graphics_context_set_stroke_width(ctx, BAR_W);
  for (int h = 1; h < 12; h += 2) {
    const int32_t a = TRIG_MAX_ANGLE * h / 12;
    graphics_context_set_stroke_color(ctx, t->bezel_print);
    edge_line(ctx, c, a, BAR_INSET_OUT, BAR_INSET_IN);
    graphics_context_set_stroke_color(ctx, t->print);
    edge_line(ctx, c, a, DIAL_INSET + 1, DIAL_INSET + INDEX_LEN);
  }

  // Gerade Stunden: Zahlen, aufrecht wie beim Vorbild
  graphics_context_set_text_color(ctx, t->bezel_print);
  static const char *NUMERALS[] = {"12", "2", "4", "6", "8", "10"};
  for (int i = 0; i < 6; i++) {
    const GPoint p = edge_point(c, TRIG_MAX_ANGLE * i / 6, NUMERAL_INSET);
    draw_text_at(ctx, NUMERALS[i], s_font_numerals, p.x, p.y, NUMERAL_LIFT, 40);
  }

  // "MADE IN" und "GERMANY" beiderseits der 6
  GFont tiny = fonts_get_system_font(FONT_KEY_GOTHIC_09);
  draw_text_at(ctx, "MADE IN", tiny, c.x - MADE_X, c.y + MADE_DIST, LIFT_GOTHIC_09, 56);
  draw_text_at(ctx, "GERMANY", tiny, c.x + MADE_X, c.y + MADE_DIST, LIFT_GOTHIC_09, 56);
}

static void draw_logo(GContext *ctx, GPoint p, const Theme *t) {
  draw_rotated(ctx, LOGO_TRI, 3, p, 0, t->print);
  graphics_context_set_stroke_color(ctx, t->print);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < 3; i++) {
    const int y = p.y - 3 + 3 * i;
    graphics_draw_line(ctx, GPoint(p.x - WING_X_IN, y), GPoint(p.x - WING_X_IN - WING_LEN[i], y));
    graphics_draw_line(ctx, GPoint(p.x + WING_X_IN, y), GPoint(p.x + WING_X_IN + WING_LEN[i], y));
  }
}

static void draw_dial(GContext *ctx, GPoint c, const Theme *t) {
  GFont g14b = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  if (s_show_day) {
    const GRect r = GRect(c.x - DAY_W / 2, c.y - DAY_DIST - DAY_H / 2, DAY_W, DAY_H);
    draw_window(ctx, r, t, DAY_NAMES[s_lang][s_now.tm_wday], g14b, LIFT_GOTHIC_14);
  }

  graphics_context_set_text_color(ctx, t->print);
  draw_text_at(ctx, BRAND_TEXT, g14b, c.x, c.y - BRAND_DIST, LIFT_GOTHIC_14, 120);
  draw_logo(ctx, GPoint(c.x, c.y - LOGO_DIST), t);

  if (s_show_date) {
    static char date[4];
    snprintf(date, sizeof(date), "%d", s_now.tm_mday);
    const GRect r = GRect(c.x + DATE_DIST - DATE_W / 2, c.y - DATE_H / 2, DATE_W, DATE_H);
    draw_window(ctx, r, t, date, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), LIFT_GOTHIC_18);
  }

  graphics_context_set_text_color(ctx, t->accent);
  draw_text_at(ctx, AUTO_TEXT, fonts_get_system_font(FONT_KEY_GOTHIC_14), c.x,
               c.y + AUTO_DIST, LIFT_GOTHIC_14, 80);
}

static void draw_hands(GContext *ctx, GPoint c, const Theme *t) {
  const int32_t a_min = TRIG_MAX_ANGLE * (s_now.tm_min * 60 + s_now.tm_sec) / 3600;
  const int32_t a_hour = TRIG_MAX_ANGLE * ((s_now.tm_hour % 12) * 60 + s_now.tm_min) / 720;

  draw_rotated(ctx, HOUR_BODY, ARRAY_LENGTH(HOUR_BODY), c, a_hour, t->hand);
  draw_rotated(ctx, HOUR_LUME, ARRAY_LENGTH(HOUR_LUME), c, a_hour, t->lume);
  draw_rotated(ctx, MIN_BODY, ARRAY_LENGTH(MIN_BODY), c, a_min, t->hand);
  draw_rotated(ctx, MIN_LUME, ARRAY_LENGTH(MIN_LUME), c, a_min, t->lume);

  if (s_seconds) {
    const int32_t a_sec = TRIG_MAX_ANGLE * s_now.tm_sec / 60;
    graphics_context_set_stroke_color(ctx, t->seconds);
    graphics_context_set_stroke_width(ctx, SEC_W);
    graphics_draw_line(ctx, polar(c, a_sec + TRIG_MAX_ANGLE / 2, SEC_TAIL),
                       polar(c, a_sec, SEC_HAND_LEN));
  }

  graphics_context_set_fill_color(ctx, t->hand);
  graphics_fill_circle(ctx, c, HUB_R);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const GPoint c = grect_center_point(&bounds);
  const Theme *t = &THEMES[s_theme];

  graphics_context_set_antialiased(ctx, true);
  draw_bezel(ctx, bounds, c, t);
  draw_dial(ctx, c, t);
  draw_hands(ctx, c, t);
}

// --------------------------------------------------------------- Zeit

static void update_time(struct tm *tick_time) {
  s_now = *tick_time;
#ifdef NEURAUM_DEMO_TIME
  s_now.tm_hour = 10;
  s_now.tm_min = 10;
  s_now.tm_sec = 37;
  s_now.tm_wday = 1;
  s_now.tm_mday = 25;
#endif
  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  update_time(tick_time);
}

static void subscribe_ticks(void) {
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(s_seconds ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

// --------------------------------------------------------------- Einstellungen

static int tuple_int(const Tuple *t) {
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return (int)t->value->int32;
}

static void load_settings(void) {
  if (persist_exists(PERSIST_THEME)) s_theme = persist_read_int(PERSIST_THEME);
  if (persist_exists(PERSIST_SECONDS)) s_seconds = persist_read_bool(PERSIST_SECONDS);
  if (persist_exists(PERSIST_SHOW_DAY)) s_show_day = persist_read_bool(PERSIST_SHOW_DAY);
  if (persist_exists(PERSIST_SHOW_DATE)) s_show_date = persist_read_bool(PERSIST_SHOW_DATE);
  if (persist_exists(PERSIST_LANG)) s_lang = persist_read_int(PERSIST_LANG);
  if (s_theme < 0 || s_theme >= THEME_COUNT) s_theme = 0;
  if (s_lang < 0 || s_lang > 1) s_lang = 0;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t;
  if ((t = dict_find(iter, MESSAGE_KEY_THEME))) {
    s_theme = tuple_int(t);
    persist_write_int(PERSIST_THEME, s_theme);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SECONDS))) {
    s_seconds = tuple_int(t) != 0;
    persist_write_bool(PERSIST_SECONDS, s_seconds);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SHOW_DAY))) {
    s_show_day = tuple_int(t) != 0;
    persist_write_bool(PERSIST_SHOW_DAY, s_show_day);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SHOW_DATE))) {
    s_show_date = tuple_int(t) != 0;
    persist_write_bool(PERSIST_SHOW_DATE, s_show_date);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_LANG))) {
    s_lang = tuple_int(t);
    persist_write_int(PERSIST_LANG, s_lang);
  }
  load_settings();   // klemmt ungueltige Werte
  subscribe_ticks();
  layer_mark_dirty(s_canvas);
}

// --------------------------------------------------------------- Lebenszyklus

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void init(void) {
  load_settings();
  s_font_numerals = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_23));

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  const time_t now = time(NULL);
  update_time(localtime(&now));
  subscribe_ticks();

  app_message_register_inbox_received(inbox_received);
  const AppMessageResult res = app_message_open(256, 64);
  if (res != APP_MSG_OK) APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open: %d", (int)res);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
  fonts_unload_custom_font(s_font_numerals);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

/* @file celeste.c
 *
 * A C source port of the original Celeste game,
 * highly optimized for the Nokia N-Gage.
 *
 * Original game by Maddy Makes Games.
 * C source port by lemon32767.
 *
 * https://github.com/lemon32767/ccleste
 *
 */

 /*
  * This is where the actual celeste code sits.
  * It is mostly a line by line port of the original lua code.
  * Due to C limitations, modifications have to be made, mostly relating to static typing.
  * The PICO-8 functions such as music() are used here preceded by Celeste_P8,
  * so _init becomes Celeste_P8_init && music becomes P8music, etc
  */

#include <SDL3/SDL.h>
#include "celeste.h"

  // I cant be bothered to put all function declarations in an appropiate place so ill just toss them all here:
static void PRELUDE(void);
static void PRELUDE_initclouds(void);
static void PRELUDE_initparticles(void);
static void title_screen(void);
static void load_room(int x, int y);
static void next_room(void);
static void psfx(int num);
static void restart_room(void);

#define bool Celeste_P8_bool_t
#define false 0
#define true 1

static float clamp(float val, float a, float b);
static float appr(float val, float target, float amount);
static float sign(float v);
static bool maybe(void);
static bool solid_at(int x, int y, int w, int h);
static bool ice_at(int x, int y, int w, int h);
static bool tile_flag_at(int x, int y, int w, int h, int flag);
static int tile_at(int x, int y);
static bool spikes_at(float x, float y, int w, int h, float xspd, float yspd);

// Exported/imported functions.
static Celeste_P8_cb_func_t Celeste_P8_call = NULL;

// Exported.
void Celeste_P8_set_call_func(Celeste_P8_cb_func_t func) {
    Celeste_P8_call = func;
}
static void pico8_srand(unsigned seed);
void Celeste_P8_set_rndseed(unsigned seed) {
    pico8_srand(seed);
}

// PICO-8 functions.
static inline void P8music(int track, int fade, int mask) {
    Celeste_P8_call(CELESTE_P8_MUSIC, track, fade, mask);
}
static inline void P8spr(int sprite, int x, int y, int cols, int rows, bool flipx, bool flipy) {
    Celeste_P8_call(CELESTE_P8_SPR, sprite, x, y, cols, rows, flipx, flipy);
}
static inline bool P8btn(int b) {
    return Celeste_P8_call(CELESTE_P8_BTN, b);
}
static inline void P8sfx(int id) {
    Celeste_P8_call(CELESTE_P8_SFX, id);
}
static inline void P8pal(int a, int b) {
    Celeste_P8_call(CELESTE_P8_PAL, a, b);
}
static inline void P8pal_reset() {
    Celeste_P8_call(CELESTE_P8_PAL_RESET);
}
static inline void P8circfill(int x, int y, int r, int c) {
    Celeste_P8_call(CELESTE_P8_CIRCFILL, x, y, r, c);
}
static inline void P8rectfill(int x, int y, int x2, int y2, int c) {
    Celeste_P8_call(CELESTE_P8_RECTFILL, x, y, x2, y2, c);
}
static inline void P8print(const char* str, int x, int y, int c) {
    Celeste_P8_call(CELESTE_P8_PRINT, str, x, y, c);
}
static inline void P8line(int x, int y, int x2, int y2, int c) {
    Celeste_P8_call(CELESTE_P8_LINE, x, y, x2, y2, c);
}
static inline int P8mget(int x, int y) {
    return Celeste_P8_call(CELESTE_P8_MGET, x, y);
}
static inline bool P8fget(int t, int f) {
    return Celeste_P8_call(CELESTE_P8_FGET, t, f);
}
static inline void P8camera(int x, int y) {
    Celeste_P8_call(CELESTE_P8_CAMERA, x, y);
}
static inline void P8map(int mx, int my, int tx, int ty, int mw, int mh, int mask) {
    Celeste_P8_call(CELESTE_P8_MAP, mx, my, tx, ty, mw, mh, mask);
}
// These values dont matter as set_rndseed should be called before init, as long as they arent both zero.
static unsigned rnd_seed_lo = 0, rnd_seed_hi = 1;

static int pico8_random(int max) //decomp'd pico-8
{
    if (!max)
    {
        return 0;
    }
    rnd_seed_hi = ((rnd_seed_hi << 16) | (rnd_seed_hi >> 16)) + rnd_seed_lo;
    rnd_seed_lo += rnd_seed_hi;
    return rnd_seed_hi % (unsigned)max;
};

static void pico8_srand(unsigned seed) //also decomp'd
{
    int i;

    if (seed == 0)
    {
        rnd_seed_hi = 0x60009755;
        seed = 0xdeadbeef;
    }
    else
    {
        rnd_seed_hi = seed ^ 0xbead29ba;
    }
    for (i = 0x20; i > 0; i--) {
        rnd_seed_hi = ((rnd_seed_hi << 16) | (rnd_seed_hi >> 16)) + seed;
        seed += rnd_seed_hi;
    }
    rnd_seed_lo = seed;
}

inline double P8max(double left, double right) {
    return (left > right) ? left : right;
}
inline double P8min(double left, double right) {
    return (left < right) ? left : right;
}
#define P8abs SDL_fabsf
#define P8flr SDL_floorf
#define fmodf SDL_fmodf
#define sinf  SDL_sinf

// https://github.com/lemon-sherbet/ccleste/issues/1
static float P8modulo(float a, float b)
{
    return fmodf(fmodf(a, b) + b, b);
}

static float P8rnd(float max)
{
    int n = pico8_random(max * (1 << 16));
    return (float)n / (1 << 16);
}

static float P8sin(float x)
{
    return -sinf(x * 6.2831853071796f); //https://pico-8.fandom.com/wiki/Math
}

#define P8cos(x) (-P8sin((x)+0.25f)) //cos(x) = sin(x+pi/2)

#define MAX_OBJECTS 30
#define FRUIT_COUNT 30

////////////////////////////////////////////////

// ~celeste~
// matt thorson + noel berry

// globals
//////////////
typedef struct
{
    float x, y;
} VEC;
typedef struct
{
    int x, y;
} VECI;

static VECI room = { .x = 0, .y = 0 };
static int  freeze = 0;
static int  shake = 0;
static bool will_restart = false;
static int  delay_restart = 0;
static bool got_fruit[FRUIT_COUNT] = { false };
static bool has_dashed = false;
static int  sfx_timer = 0;
static bool has_key = false;
static bool pause_player = false;
static bool flash_bg = false;
static int  music_timer = 0;

// These are originally implicit globals defined in title_screen()
static bool  new_bg = false;
static int   frames, seconds;
static short minutes; // This variable can overflow in normal gameplay (after +500 hours).
static int   deaths, max_djump;
static bool  start_game;
static int   start_game_flash;

enum
{
    k_left = 0,
    k_right = 1,
    k_up = 2,
    k_down = 3,
    k_jump = 4,
    k_dash = 5
};

// With this X macro table thing we can define the properties that each object type has, in the original lua code these properties
// are inferred from the `types` table
#define OBJ_PROP_LIST() \
    /* TYPE        TILE   HAS INIT  HAS UPDATE  HAS DRAW    IF_NOT_FRUIT */ \
    X(PLAYER,       -1,     Y,        Y,          Y,            false) \
    X(PLAYER_SPAWN,  1,     Y,        Y,          Y,            false) \
    X(SPRING,       18,     Y,        Y,          N,            false) \
    X(BALLOON,      22,     Y,        Y,          Y,            false) \
    X(SMOKE,        -1,     Y,        Y,          N,            false) \
    X(PLATFORM,     -1,     Y,        Y,          Y,            false) \
    X(FALL_FLOOR,   23,     Y,        Y,          Y,            false) \
    X(FRUIT,        26,     Y,        Y,          N,             true) \
    X(FLY_FRUIT,    28,     Y,        Y,          Y,             true) \
    X(FAKE_WALL,    64,     N,        Y,          Y,             true) \
    X(KEY,           8,     N,        Y,          N,             true) \
    X(CHEST,        20,     Y,        Y,          N,             true) \
    X(LIFEUP,       -1,     Y,        Y,          Y,            false) \
    X(MESSAGE,      86,     N,        N,          Y,            false) \
    X(BIG_CHEST,    96,     Y,        N,          Y,            false) \
    X(ORB,          -1,     Y,        N,          Y,            false) \
    X(FLAG,        118,     Y,        N,          Y,            false) \
    X(ROOM_TITLE,   -1,     Y,        N,          Y,            false)

typedef enum
{
#if defined (SDL_PLATFORM_NGAGE)
#define X(t,ARGS...) OBJ_##t,
#else
#define X(t,...) OBJ_##t,
#endif
    OBJ_PROP_LIST()
#undef X
    OBJTYPE_COUNT
} OBJTYPE;

// entry point //
/////////////////
static void PRELUDE()
{
    // Top-level init code has been moved into functions that are called here.
    PRELUDE_initclouds();
    PRELUDE_initparticles();
}

void Celeste_P8_init() // Identifiers beginning with underscores are reserved in C.
{
    if (!Celeste_P8_call)
    {
        SDL_Log("Warning: Celeste_P8_call is NULL.. have you called Celeste_P8_set_call_func()?");
    }

    PRELUDE();

    title_screen();
}

static void title_screen()
{
    int i;
    for (i = 0; i <= 29; i++)
    {
        got_fruit[i] = false;
    }
    frames = 0;
    deaths = 0;
    max_djump = 1;
    start_game = false;
    start_game_flash = 0;

    P8music(40, 0, 7);
    load_room(7, 3);
}

static void begin_game()
{
    frames = 0;
    seconds = 0;
    minutes = 0;
    music_timer = 0;
    start_game = false;

    P8music(0, 0, 7);
    load_room(0, 0);
}

static int level_index()
{
    return room.x % 8 + room.y * 8;
}

static bool is_title()
{
    return level_index() == 31;
}

// effects //
/////////////
typedef struct
{
    float x, y, spd, w;
} CLOUD;

static CLOUD clouds[17];

// Top level init code has been moved into a function.
static void PRELUDE_initclouds()
{
    int i;
    for (i = 0; i <= 16; i++)
    {
        clouds[i] = (CLOUD){
            .x = P8rnd(128),
            .y = P8rnd(128),
            .spd = 1 + P8rnd(4),
            .w = 32 + P8rnd(32),
        };
    }
}

typedef struct
{
    bool active;
    float x, y, s, spd, off, c, h, t;
    VEC spd2; // Used by dead particles, moved from spd.
} PARTICLE;

static PARTICLE particles[25];
static PARTICLE dead_particles[8];

// Top level init code has been moved into a function.
static void PRELUDE_initparticles()
{
    int i;
    for (i = 0; i <= 24; i++)
    {
        particles[i].x = P8rnd(128);
        particles[i].y = P8rnd(128);
        particles[i].s = 0 + P8flr(P8rnd(5) / 4);
        particles[i].spd = 0.25f + P8rnd(5);
        particles[i].off = P8rnd(1);
        particles[i].c = 6 + P8flr(0.5 + P8rnd(1));
    }
}

typedef struct
{
    int x, y, w, h;

} HITBOX;

typedef struct
{
    float x, y, size;
    bool  isLast;

} HAIR;

//OBJECT strucutre
typedef struct
{
    bool  active;
    short id; // Unique identifier for each object, incremented per object.

    //inherited
    OBJTYPE type;
    bool    collideable, solids;
    float   spr;
    bool    flip_x, flip_y;
    float   x, y;
    HITBOX  hitbox;
    VEC     spd;
    VEC     rem;

    // Player.
    bool  p_jump, p_dash;
    int   grace, jbuffer, djump, dash_time;
    short dash_effect_time; // Can underflow in normal gameplay (after 18 minutes)
    VEC   dash_target;
    VEC   dash_accel;
    float spr_off;
    bool  was_on_ground;
    HAIR  hair[5]; // Also player_spawn.

    // Player_spawn.
    int state, delay;
    VEC target;

    // Spring.
    int hide_in, hide_for;

    // Balloon.
    int   timer;
    float offset, start;

    // Fruit.
    float off;

    // Fly_fruit.
    bool  fly;
    float step;
    int   sfx_delay;

    // Lifeup.
    int   duration;
    float flash;

    // Platform.
    float last, dir;

    // Message.
    const char* text;
    float       index;
    VECI        off2; //changed from off..

    // Big chest.
    PARTICLE particles[50];
    int      particle_count;

    // Flag.
    int  score;
    bool show;

} OBJ;

// OBJ function declarations.
#define when_Y(x) static void x(OBJ* this);
#define when_N(x) enum { x = 0 }; //OBJTYPE_prop definition requires a constant value, and `static cost void* x = NULL` doesn't count
#define X(name,t,has_init,has_update,has_draw,if_not_fruit) \
    when_##has_init (name##_init)                           \
    when_##has_update (name##_update)                       \
    when_##has_draw (name##_draw)
OBJ_PROP_LIST()
#undef X

typedef void (*obj_callback_t)(OBJ*);

struct objprop
{
    int tile;
    obj_callback_t init;
    obj_callback_t update;
    obj_callback_t draw;
    const char* nam;
    bool if_not_fruit;
};

static const struct objprop OBJTYPE_prop[] =
{
#define X(name,t,has_init,has_update,has_draw,_if_not_fruit) \
    [OBJ_##name] = {                                         \
        .tile = t,                                           \
        .init = (obj_callback_t)name##_init,                 \
        .update = (obj_callback_t)name##_update,             \
        .draw = (obj_callback_t)name##_draw,                 \
        .nam = #name,                                        \
        .if_not_fruit = _if_not_fruit                        \
    },
    OBJ_PROP_LIST()
#undef X
    {0}
};

#define OBJ_PROP(o) OBJTYPE_prop[(o)->type]

static OBJ objects[MAX_OBJECTS] = {
    {.active = false }
};

static void create_hair(OBJ* obj);
static void set_hair_color(int c);
static void draw_hair(OBJ* obj, int facing);
static void unset_hair_color(void);
static void kill_player(OBJ* obj);
static void break_fall_floor(OBJ* obj);
static void draw_time(float x, float y);
static OBJ* init_object(OBJTYPE type, float x, float y);
static void destroy_object(OBJ* obj);
static void draw_object(OBJ* obj);

//OBJECT FUNCTIONS MOVED HERE

static bool OBJ_is_solid(OBJ* obj, float ox, float oy);
static bool OBJ_is_ice(OBJ* obj, float ox, float oy);
static OBJ* OBJ_collide(OBJ* obj, OBJTYPE type, float ox, float oy);
static bool OBJ_check(OBJ* obj, OBJTYPE type, float ox, float oy);
static void OBJ_move(OBJ* obj, float ox, float oy);
static void OBJ_move_x(OBJ* obj, float amount, float start);
static void OBJ_move_y(OBJ* obj, float amount);

static bool OBJ_is_solid(OBJ* obj, float ox, float oy)
{
    if (oy > 0 && !OBJ_check(obj, OBJ_PLATFORM, ox, 0) && OBJ_check(obj, OBJ_PLATFORM, ox, oy))
    {
        return true;
    }
    return solid_at(obj->x + obj->hitbox.x + ox, obj->y + obj->hitbox.y + oy, obj->hitbox.w, obj->hitbox.h)
        || OBJ_check(obj, OBJ_FALL_FLOOR, ox, oy)
        || OBJ_check(obj, OBJ_FAKE_WALL, ox, oy);
}

static bool OBJ_is_ice(OBJ* obj, float ox, float oy)
{
    return ice_at(obj->x + obj->hitbox.x + ox, obj->y + obj->hitbox.y + oy, obj->hitbox.w, obj->hitbox.h);
}

static OBJ* OBJ_collide(OBJ* obj, OBJTYPE type, float ox, float oy)
{
    int i;
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        OBJ* other = &objects[i];
        if (other->active && other->type == type && other != obj && other->collideable &&
            other->x + other->hitbox.x + other->hitbox.w > obj->x + obj->hitbox.x + ox &&
            other->y + other->hitbox.y + other->hitbox.h > obj->y + obj->hitbox.y + oy &&
            other->x + other->hitbox.x < obj->x + obj->hitbox.x + obj->hitbox.w + ox &&
            other->y + other->hitbox.y < obj->y + obj->hitbox.y + obj->hitbox.h + oy)
        {
            return other;
        }
    }
    return NULL;
}

static bool OBJ_check(OBJ* obj, OBJTYPE type, float ox, float oy)
{
    return OBJ_collide(obj, type, ox, oy) != NULL;
}

static void OBJ_move(OBJ* obj, float ox, float oy)
{
    float amount;
    // [x] get move amount
    obj->rem.x += ox;
    amount = P8flr(obj->rem.x + 0.5);
    obj->rem.x -= amount;
    OBJ_move_x(obj, amount, 0);

    // [y] get move amount
    obj->rem.y += oy;
    amount = P8flr(obj->rem.y + 0.5);
    obj->rem.y -= amount;
    OBJ_move_y(obj, amount);
}

static void OBJ_move_x(OBJ* obj, float amount, float start)
{
    if (obj->solids)
    {
        float step = sign(amount);
        float i;
        for (i = start; i <= P8abs(amount); i += 1)
        {
            if (!OBJ_is_solid(obj, step, 0))
            {
                obj->x += step;
            }
            else
            {
                obj->spd.x = 0;
                obj->rem.x = 0;
                break;
            }
        }
    }
    else
    {
        obj->x += amount;
    }
}

static void OBJ_move_y(OBJ* obj, float amount)
{
    if (obj->solids)
    {
        float step = sign(amount);
        int   i;
        for (i = 0; i <= P8abs(amount); i++)
        {
            if (!OBJ_is_solid(obj, 0, step))
            {
                obj->y += step;
            }
            else
            {
                obj->spd.y = 0;
                obj->rem.y = 0;
                break;
            }
        }
    }
    else
    {
        obj->y += amount;
    }
}


// player entity //
///////////////////
static void PLAYER_init(OBJ* this)
{
    this->p_jump = false;
    this->p_dash = false;
    this->grace = 0;
    this->jbuffer = 0;
    this->djump = max_djump;
    this->dash_time = 0;
    this->dash_effect_time = 0;
    this->dash_target = (VEC){ .x = 0,.y = 0 };
    this->dash_accel = (VEC){ .x = 0,.y = 0 };
    this->hitbox = (HITBOX){ .x = 1,.y = 3,.w = 6,.h = 5 };
    this->spr_off = 0;
    this->was_on_ground = false;
    create_hair(this);
}

static OBJ player_dummy_copy; //see below
static void PLAYER_update(OBJ* this)
{
    bool on_ground;
    bool on_ice;
    bool dash;
    bool jump;

    int  input = P8btn(k_right) ? 1 : (P8btn(k_left) ? -1 : 0);

    /*LEMON: in order to kill the player in these lines, while maintaining object slots in the same order as they would be in pico-8,
     *       we need to remove the object there but that shifts back the objects array which will make it so the rest of the player_update()
     *       function modifies data from a newly loaded object; which is bad, so we simulate the pico-8 behaviour of reading from and writing to
     *       a table that is not referenced in the objects table by switching to a dummy copy of the player object */

    bool do_kill_player = false;

    if (pause_player)
    {
        return;
    }

    // Spikes collide.
    if (spikes_at(this->x + this->hitbox.x, this->y + this->hitbox.y, this->hitbox.w, this->hitbox.h, this->spd.x, this->spd.y))
    {
        do_kill_player = true;
    }

    //Bottom death.
    if (this->y > 128)
    {
        do_kill_player = true;
    }
    if (do_kill_player)
    {
        //switch to dummy copy, need to copy before destroying the object
        player_dummy_copy = *this;
        kill_player(this);
        this = &player_dummy_copy;
    }

    on_ground = OBJ_is_solid(this, 0, 1);
    on_ice = OBJ_is_ice(this, 0, 1);

    // smoke particles
    if (on_ground && !this->was_on_ground)
    {
        init_object(OBJ_SMOKE, this->x, this->y + 4);
    }

    jump = P8btn(k_jump) && !this->p_jump;
    this->p_jump = P8btn(k_jump);
    if ((jump))
    {
        this->jbuffer = 4;
    }
    else if (this->jbuffer > 0)
    {
        this->jbuffer -= 1;
    }

    dash = P8btn(k_dash) && !this->p_dash;
    this->p_dash = P8btn(k_dash);

    if (on_ground)
    {
        this->grace = 6;
        if (this->djump < max_djump)
        {
            psfx(54);
            this->djump = max_djump;
        }
    }
    else if (this->grace > 0)
    {
        this->grace -= 1;
    }

    this->dash_effect_time -= 1;
    if (this->dash_time > 0)
    {
        init_object(OBJ_SMOKE, this->x, this->y);
        this->dash_time -= 1;
        this->spd.x = appr(this->spd.x, this->dash_target.x, this->dash_accel.x);
        this->spd.y = appr(this->spd.y, this->dash_target.y, this->dash_accel.y);
    }
    else
    {
        // Move.
        int   maxrun = 1;
        float accel = 0.6;
        float d_full;
        float d_half;
        float maxfall;
        float gravity;

        if (!on_ground)
        {
            accel = 0.4;
        }
        else if (on_ice)
        {
            accel = 0.05;
            if (input == (this->flip_x ? -1 : 1))
            {
                accel = 0.05;
            }
        }

        if (P8abs(this->spd.x) > maxrun)
        {
            float deccel = 0.15;
            this->spd.x = appr(this->spd.x, sign(this->spd.x) * maxrun, deccel);
        }
        else
        {
            this->spd.x = appr(this->spd.x, input * maxrun, accel);
        }

        // Facing.
        if (this->spd.x != 0)
        {
            this->flip_x = (this->spd.x < 0);
        }

        // Gravity.
        maxfall = 2;
        gravity = 0.21;

        if (P8abs(this->spd.y) <= 0.15)
        {
            gravity *= 0.5;
        }

        // Wall slide.
        if (input != 0 && OBJ_is_solid(this, input, 0) && !OBJ_is_ice(this, input, 0))
        {
            maxfall = 0.4;
            if (P8rnd(10) < 2)
            {
                init_object(OBJ_SMOKE, this->x + input * 6, this->y);
            }
        }

        if (!on_ground)
        {
            this->spd.y = appr(this->spd.y, maxfall, gravity);
        }

        // Jump.
        if (this->jbuffer > 0)
        {
            if (this->grace > 0)
            {
                // Normal jump.
                psfx(1);
                this->jbuffer = 0;
                this->grace = 0;
                this->spd.y = -2;
                init_object(OBJ_SMOKE, this->x, this->y + 4);
            }
            else
            {
                // Wall jump.
                int wall_dir = (OBJ_is_solid(this, -3, 0) ? -1 : (OBJ_is_solid(this, 3, 0) ? 1 : 0));
                if (wall_dir != 0)
                {
                    psfx(2);
                    this->jbuffer = 0;
                    this->spd.y = -2;
                    this->spd.x = -wall_dir * (maxrun + 1);
                    if (!OBJ_is_ice(this, wall_dir * 3, 0))
                    {
                        init_object(OBJ_SMOKE, this->x + wall_dir * 6, this->y);
                    }
                }
            }
        }

        // Dash.
        d_full = 5;
        d_half = d_full * 0.70710678118;

        if (this->djump > 0 && dash)
        {
            int v_input;
            init_object(OBJ_SMOKE, this->x, this->y);
            this->djump -= 1;
            this->dash_time = 4;
            has_dashed = true;
            this->dash_effect_time = 10;
            v_input = (P8btn(k_up) ? -1 : (P8btn(k_down) ? 1 : 0));
            if (input != 0)
            {
                if (v_input != 0)
                {
                    this->spd.x = input * d_half;
                    this->spd.y = v_input * d_half;
                }
                else
                {
                    this->spd.x = input * d_full;
                    this->spd.y = 0;
                }
            }
            else if (v_input != 0)
            {
                this->spd.x = 0;
                this->spd.y = v_input * d_full;
            }
            else
            {
                this->spd.x = (this->flip_x ? -1 : 1);
                this->spd.y = 0;
            }

            psfx(3);
            freeze = 2;
            shake = 6;
            this->dash_target.x = 2 * sign(this->spd.x);
            this->dash_target.y = 2 * sign(this->spd.y);
            this->dash_accel.x = 1.5;
            this->dash_accel.y = 1.5;

            if (this->spd.y < 0)
            {
                this->dash_target.y *= .75;
            }

            if (this->spd.y != 0)
            {
                this->dash_accel.x *= 0.70710678118f;
            }

            if (this->spd.x != 0)
            {
                this->dash_accel.y *= 0.70710678118f;
            }
        }
        else if (dash && this->djump <= 0)
        {
            psfx(9);
            init_object(OBJ_SMOKE, this->x, this->y);
        }
    }

    // Animation.
    this->spr_off += 0.25;
    if (!on_ground)
    {
        if (OBJ_is_solid(this, input, 0))
        {
            this->spr = 5;
        }
        else
        {
            this->spr = 3;
        }
    }
    else if (P8btn(k_down))
    {
        this->spr = 6;
    }
    else if (P8btn(k_up))
    {
        this->spr = 7;
    }
    else if ((this->spd.x == 0) || (!P8btn(k_left) && !P8btn(k_right)))
    {
        this->spr = 1;
    }
    else
    {
        this->spr = 1 + ((int)this->spr_off) % 4;
    }

    // Next level.
    if (this->y < -4 && level_index() < 30)
    {
        next_room();
    }

    // Was on the ground.
    this->was_on_ground = on_ground;
}

static void PLAYER_draw(OBJ* this)
{
    // Clamp in screen.
    if (this->x < -1 || this->x>121)
    {
        this->x = clamp(this->x, -1, 121);
        this->spd.x = 0;
    }

    set_hair_color(this->djump);
    draw_hair(this, this->flip_x ? -1 : 1);
    P8spr(this->spr, this->x, this->y, 1, 1, this->flip_x, this->flip_y);
    unset_hair_color();
}

static void psfx(int num)
{
    if (sfx_timer <= 0)
    {
        P8sfx(num);
    }
}

void create_hair(OBJ* obj)
{
    int i;
    for (i = 0; i <= 4; i++)
    {
        obj->hair[i] = (HAIR){
            .x = obj->x,
            .y = obj->y,
            .size = P8max(1,P8min(2,3 - i)),
            .isLast = i == 4
        };
    }
}

static void set_hair_color(int djump)
{
    P8pal(8, (djump == 1 ? 8 : (djump == 2 ? (7 + P8flr(((int)(((float)frames) / 3.0)) % 2) * 4) : 12)));
}

static void draw_hair(OBJ* obj, int facing)
{
    float last_x = obj->x + 4 - facing * 2;
    float last_y = obj->y + (P8btn(k_down) ? 4 : 3);
    HAIR* h;
    int i = 0;
    do
    {
        h = &obj->hair[i++];
        h->x += (last_x - h->x) / 1.5;
        h->y += (last_y + 0.5 - h->y) / 1.5;
        P8circfill(h->x, h->y, h->size, 8);
        last_x = h->x;
        last_y = h->y;
    } while (!h->isLast);
}

static void unset_hair_color()
{
    P8pal(8, 8);
}

// Player_spawn.
static void PLAYER_SPAWN_init(OBJ* this)
{
    P8sfx(4);
    this->spr = 3;
    this->target.x = this->x;
    this->target.y = this->y;
    this->y = 128;
    this->spd.y = -4;
    this->state = 0;
    this->delay = 0;
    this->solids = false;
    create_hair(this);
}
static void PLAYER_SPAWN_update(OBJ* this)
{
    // Jumping up.
    if (this->state == 0)
    {
        if (this->y < this->target.y + 16)
        {
            this->state = 1;
            this->delay = 3;
        }
        // Falling.
    }
    else if (this->state == 1)
    {
        this->spd.y += 0.5;
        if (this->spd.y > 0 && this->delay > 0)
        {
            this->spd.y = 0;
            this->delay -= 1;
        }
        if (this->spd.y > 0 && this->y > this->target.y)
        {
            this->y = this->target.y;
            this->spd.x = this->spd.y = 0;
            this->state = 2;
            this->delay = 5;
            shake = 5;
            init_object(OBJ_SMOKE, this->x, this->y + 4);
            P8sfx(5);
        }
        // landing
    }
    else if (this->state == 2)
    {
        this->delay -= 1;
        this->spr = 6;
        if (this->delay < 0)
        {
            float x = this->x, y = this->y;
            destroy_object(this);
            init_object(OBJ_PLAYER, x, y);
        }
    }
}

static void PLAYER_SPAWN_draw(OBJ* this)
{
    set_hair_color(max_djump);
    draw_hair(this, 1);
    P8spr(this->spr, this->x, this->y, 1, 1, this->flip_x, this->flip_y);
    unset_hair_color();
}

// Spring.
static void SPRING_init(OBJ* this)
{
    this->hide_in = 0;
    this->hide_for = 0;
}
static void SPRING_update(OBJ* this)
{
    if (this->hide_for > 0)
    {
        this->hide_for -= 1;
        if (this->hide_for <= 0)
        {
            this->spr = 18;
            this->delay = 0;
        }
    }
    else if (this->spr == 18)
    {
        OBJ* hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
        if (hit != NULL && hit->spd.y >= 0)
        {
            OBJ* below;

            this->spr = 19;
            hit->y = this->y - 4;
            hit->spd.x *= 0.2;
            hit->spd.y = -3;
            hit->djump = max_djump;
            this->delay = 10;
            init_object(OBJ_SMOKE, this->x, this->y);

            // breakable below us
            below = OBJ_collide(this, OBJ_FALL_FLOOR, 0, 1);
            if (below != NULL)
            {
                break_fall_floor(below);
            }

            psfx(8);
        }
    }
    else if (this->delay > 0)
    {
        this->delay -= 1;
        if (this->delay <= 0)
        {
            this->spr = 18;
        }
    }

    // Begin hiding.
    if (this->hide_in > 0)
    {
        this->hide_in -= 1;
        if (this->hide_in <= 0)
        {
            this->hide_for = 60;
            this->spr = 0;
        }
    }
}

static void break_spring(OBJ* obj)
{
    obj->hide_in = 15;
}

// Balloon.
static void BALLOON_init(OBJ* this)
{
    this->offset = P8rnd(1);
    this->start = this->y;
    this->timer = 0;
    this->hitbox = (HITBOX){ .x = -1,.y = -1,.w = 10,.h = 10 };
}

static void BALLOON_update(OBJ* this)
{
    if (this->spr == 22)
    {
        OBJ* hit;
        this->offset += 0.01;
#ifdef CELESTE_P8_HACKED_BALLOONS
        // Hacked balloons: constant y coord and hitbox. for TASes.
        this->hitbox = (HITBOX){ .x = -1,.y = -3,.w = 10,.h = 14 };
#else
        this->y = this->start + P8sin(this->offset) * 2;
#endif
        hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
        if (hit != NULL && hit->djump < max_djump)
        {
            psfx(6);
            init_object(OBJ_SMOKE, this->x, this->y);
            hit->djump = max_djump;
            this->spr = 0;
            this->timer = 60;
        }
    }
    else if (this->timer > 0)
    {
        this->timer -= 1;
    }
    else
    {
        psfx(7);
        init_object(OBJ_SMOKE, this->x, this->y);
        this->spr = 22;
    }
}

static void BALLOON_draw(OBJ* this)
{
    if (this->spr == 22)
    {
        P8spr(13 + (int)(this->offset * 8) % 3, this->x, this->y + 6, 1, 1, false, false);
        P8spr(this->spr, this->x, this->y, 1, 1, false, false);
    }
}

// Fall_floor.
static void FALL_FLOOR_init(OBJ* this)
{
    this->state = 0;
}

static void FALL_FLOOR_update(OBJ* this)
{
    // idling
    if (this->state == 0)
    {
        if (OBJ_check(this, OBJ_PLAYER, 0, -1) || OBJ_check(this, OBJ_PLAYER, -1, 0) || OBJ_check(this, OBJ_PLAYER, 1, 0))
        {
            break_fall_floor(this);
        }
        // Shaking.
    }
    else if (this->state == 1)
    {
        this->delay -= 1;
        if (this->delay <= 0)
        {
            this->state = 2;
            this->delay = 60; //how long it hides for
            this->collideable = false;
        }
        // Invisible, waiting to reset.
    }
    else if (this->state == 2)
    {
        this->delay -= 1;
        if (this->delay <= 0 && !OBJ_check(this, OBJ_PLAYER, 0, 0))
        {
            psfx(7);
            this->state = 0;
            this->collideable = true;
            init_object(OBJ_SMOKE, this->x, this->y);
        }
    }
}
static void FALL_FLOOR_draw(OBJ* this)
{
    if (this->state != 2)
    {
        if (this->state != 1) {
            P8spr(23, this->x, this->y, 1, 1, false, false);
        }
        else
        {
            P8spr(23 + (15 - this->delay) / 5, this->x, this->y, 1, 1, false, false);
        }
    }
}

static void break_fall_floor(OBJ* obj)
{
    if (obj->state == 0)
    {
        OBJ* hit;
        psfx(15);
        obj->state = 1;
        obj->delay = 15;        // How long until it falls.
        init_object(OBJ_SMOKE, obj->x, obj->y);
        hit = OBJ_collide(obj, OBJ_SPRING, 0, -1);
        if (hit != NULL)
        {
            break_spring(hit);
        }
    }
}

static void SMOKE_init(OBJ* this)
{
    this->spr = 29;
    this->spd.y = -0.1;
    this->spd.x = 0.3 + P8rnd(0.2);
    this->x += -1 + P8rnd(2);
    this->y += -1 + P8rnd(2);
    this->flip_x = maybe();
    this->flip_y = maybe();
    this->solids = false;
}

static void SMOKE_update(OBJ* this)
{
    this->spr += 0.2;
    if (this->spr >= 32)
    {
        destroy_object(this);
    }
}

static void FRUIT_init(OBJ* this)
{
    this->start = this->y;
    this->off = 0;
}

static void FRUIT_update(OBJ* this)
{
    OBJ* hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
    if (hit != NULL)
    {
        hit->djump = max_djump;
        sfx_timer = 20;
        P8sfx(13);
        got_fruit[level_index()] = true;
        init_object(OBJ_LIFEUP, this->x, this->y);
        destroy_object(this);
        return; //LEMON: added return to not modify dead object
    }
    this->off += 1;
    this->y = this->start + P8sin(this->off / 40) * 2.5f;
}

static void FLY_FRUIT_init(OBJ* this)
{
    this->start = this->y;
    this->fly = false;
    this->step = 0.5;
    this->solids = false;
    this->sfx_delay = 8;
}

static void FLY_FRUIT_update(OBJ* this)
{
    bool do_destroy_object = false; //LEMON: see PLAYER_update..
    OBJ* hit;
    // Fly away.
    if (this->fly)
    {
        if (this->sfx_delay > 0)
        {
            this->sfx_delay -= 1;
            if (this->sfx_delay <= 0)
            {
                sfx_timer = 20;
                P8sfx(14);
            }
        }
        this->spd.y = appr(this->spd.y, -3.5, 0.25);
        if (this->y < -16)
        {
            do_destroy_object = true;
        }
        // Wait.
    }
    else
    {
        if (has_dashed)
        {
            this->fly = true;
        }
        this->step += 0.05;
        this->spd.y = P8sin(this->step) * 0.5;
    }
    // Collect.
    hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
    if (hit != NULL)
    {
        hit->djump = max_djump;
        sfx_timer = 20;
        P8sfx(13);
        got_fruit[level_index()] = true;
        init_object(OBJ_LIFEUP, this->x, this->y);
        do_destroy_object = true;
    }
    if (do_destroy_object)
    {
        destroy_object(this);
    }
}

static void FLY_FRUIT_draw(OBJ* this)
{
    float off = 0;
    if (!this->fly)
    {
        float dir = P8sin(this->step);
        if (dir < 0)
        {
            off = 1 + P8max(0, sign(this->y - this->start));
        }
    }
    else
    {
        off = P8modulo(off + 0.25, 3);
    }
    P8spr(45 + off, this->x - 6, this->y - 2, 1, 1, true, false);
    P8spr(this->spr, this->x, this->y, 1, 1, false, false);
    P8spr(45 + off, this->x + 6, this->y - 2, 1, 1, false, false);
}

static void LIFEUP_init(OBJ* this)
{
    this->spd.y = -0.25;
    this->duration = 30;
    this->x -= 2;
    this->y -= 4;
    this->flash = 0;
    this->solids = false;
}

static void LIFEUP_update(OBJ* this)
{
    this->duration -= 1;
    if (this->duration <= 0)
    {
        destroy_object(this);
    }
}

static void LIFEUP_draw(OBJ* this)
{
    this->flash += 0.5;

    P8print("1000", this->x - 2, this->y, 7 + ((int)this->flash) % 2);
}

static void FAKE_WALL_update(OBJ* this)
{
    OBJ* hit;
    this->hitbox = (HITBOX){ .x = -1,.y = -1,.w = 18,.h = 18 };
    hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
    if (hit != NULL && hit->dash_effect_time > 0)
    {
        hit->spd.x = -sign(hit->spd.x) * 1.5;
        hit->spd.y = -1.5;
        hit->dash_time = -1;
        sfx_timer = 20;
        P8sfx(16);
        //destroy_object(this);
        init_object(OBJ_SMOKE, this->x, this->y);
        init_object(OBJ_SMOKE, this->x + 8, this->y);
        init_object(OBJ_SMOKE, this->x, this->y + 8);
        init_object(OBJ_SMOKE, this->x + 8, this->y + 8);
        init_object(OBJ_FRUIT, this->x + 4, this->y + 4);
        destroy_object(this); //LEMON: moved here. see PLAYER_update. also returning to avoid modifying removed object
        return;
    }
    this->hitbox = (HITBOX){
        .x = 0,
        .y = 0,
        .w = 16,
        .h = 16
    };
}

static void FAKE_WALL_draw(OBJ* this)
{
    P8spr(64, this->x, this->y, 1, 1, false, false);
    P8spr(65, this->x + 8, this->y, 1, 1, false, false);
    P8spr(80, this->x, this->y + 8, 1, 1, false, false);
    P8spr(81, this->x + 8, this->y + 8, 1, 1, false, false);
}

static void KEY_update(OBJ* this)
{
    int is;
    int was = P8flr(this->spr);
    this->spr = 9 + (P8sin((float)frames / 30.0) + 0.5) * 1;
    is = P8flr(this->spr);
    if (is == 10 && is != was)
    {
        this->flip_x = !this->flip_x;
    }
    if (OBJ_check(this, OBJ_PLAYER, 0, 0))
    {
        P8sfx(23);
        sfx_timer = 10;
        destroy_object(this);
        has_key = true;
    }
}

static void CHEST_init(OBJ* this)
{
    this->x -= 4;
    this->start = this->x;
    this->timer = 20;
}

static void CHEST_update(OBJ* this)
{
    if (has_key)
    {
        this->timer -= 1;
        this->x = this->start - 1 + P8rnd(3);
        if (this->timer <= 0)
        {
            sfx_timer = 20;
            P8sfx(16);
            init_object(OBJ_FRUIT, this->x, this->y - 4);
            destroy_object(this);
        }
    }
}

static void PLATFORM_init(OBJ* this)
{
    this->x -= 4;
    this->solids = false;
    this->hitbox.w = 16;
    this->last = this->x;
}

static void PLATFORM_update(OBJ* this)
{
    this->spd.x = this->dir * 0.65;
    if (this->x < -16)
    {
        this->x = 128;
    }
    else if (this->x > 128)
    {
        this->x = -16;
    }
    if (!OBJ_check(this, OBJ_PLAYER, 0, 0))
    {
        OBJ* hit = OBJ_collide(this, OBJ_PLAYER, 0, -1);
        if (hit != NULL)
        {
            OBJ_move_x(hit, this->x - this->last, 1);
        }
    }
    this->last = this->x;
}

static void PLATFORM_draw(OBJ* this)
{
    P8spr(11, this->x, this->y - 1, 1, 1, false, false);
    P8spr(12, this->x + 8, this->y - 1, 1, 1, false, false);
}

static void MESSAGE_draw(OBJ* this)
{
    this->text = "-- celeste mountain --#this memorial to those# perished on the climb";
    if (OBJ_check(this, OBJ_PLAYER, 4, 0))
    {
        int i;
        if (this->index < strlen(this->text))
        {
            this->index += 0.5;
            if (this->index >= this->last + 1)
            {
                this->last += 1;
                P8sfx(35);
            }
        }
        this->off2.x = 8;
        this->off2.y = 96;
        for (i = 0; i < this->index; i++)
        {
            if (this->text[i] != '#')
            {
                char charstr[2];
                P8rectfill(this->off2.x - 2, this->off2.y - 2, this->off2.x + 7, this->off2.y + 6, 7);
                charstr[0] = this->text[i], charstr[1] = '\0';
                P8print(charstr, this->off2.x, this->off2.y, 0);
                this->off2.x += 5;
            }
            else
            {
                this->off2.x = 8;
                this->off2.y += 7;
            }
        }
    }
    else
    {
        this->index = 0;
        this->last = 0;
    }
}

static void BIG_CHEST_init(OBJ* this)
{
    this->state = 0;
    this->hitbox.w = 16;
}

static void BIG_CHEST_draw(OBJ* this)
{
    if (this->state == 0)
    {
        OBJ* hit = OBJ_collide(this, OBJ_PLAYER, 0, 8);
        if (hit != NULL && OBJ_is_solid(hit, 0, 1))
        {
            P8music(-1, 500, 7);
            P8sfx(37);
            pause_player = true;
            hit->spd.x = 0;
            hit->spd.y = 0;
            this->state = 1;
            init_object(OBJ_SMOKE, this->x, this->y);
            init_object(OBJ_SMOKE, this->x + 8, this->y);
            this->timer = 60;
            this->particle_count = 0;
        }
        P8spr(96, this->x, this->y, 1, 1, false, false);
        P8spr(97, this->x + 8, this->y, 1, 1, false, false);
    }
    else if (this->state == 1)
    {
        int i;
        this->timer -= 1;
        shake = 5;
        flash_bg = true;
        if (this->timer <= 45 && this->particle_count < 50)
        {
            this->particles[this->particle_count].x = 1 + P8rnd(14);
            this->particles[this->particle_count].y = 0;
            this->particles[this->particle_count].spd = 8 + P8rnd(8);
            this->particles[this->particle_count].h = 32 + P8rnd(32);
            this->particle_count++;
        }
        if (this->timer < 0)
        {
            this->state = 2;
            this->particle_count = 0;
            flash_bg = false;
            new_bg = true;
            init_object(OBJ_ORB, this->x + 4, this->y + 4);
            pause_player = false;
        }
        for (i = 0; i < this->particle_count; i++)
        {
            PARTICLE* p = &this->particles[i];
            p->y += p->spd;
            P8line(this->x + p->x, this->y + 8 - p->y, this->x + p->x, P8min(this->y + 8 - p->y + p->h, this->y + 8), 7);
        }
    }
    P8spr(112, this->x, this->y + 8, 1, 1, false, false);
    P8spr(113, this->x + 8, this->y + 8, 1, 1, false, false);
}

static void ORB_init(OBJ* this)
{
    this->spd.y = -4;
    this->solids = false;
    this->particle_count = 0;
}

static void ORB_draw(OBJ* this)
{
    bool  destroy_self = false;
    OBJ* hit;
    float off;
    float i;

    this->spd.y = appr(this->spd.y, 0, 0.5);
    hit = OBJ_collide(this, OBJ_PLAYER, 0, 0);
    if (this->spd.y == 0 && hit != NULL)
    {
        music_timer = 45;
        P8sfx(51);
        freeze = 10;
        shake = 10;
        destroy_self = true;    //LEMON: to avoid reading off dead object
        max_djump = 2;
        hit->djump = 2;
    }

    P8spr(102, this->x, this->y, 1, 1, false, false);
    off = (float)frames / 30.f;
    for (i = 0; i <= 7; i += 1)
    {
        P8circfill(this->x + 4 + P8cos(off + i / 8.f) * 8, this->y + 4 + P8sin(off + i / 8.f) * 8, 1, 7);
    }
    if (destroy_self)
    {
        destroy_object(this);
    }
}

static void FLAG_init(OBJ* this)
{
    int i;
    this->x += 5;
    this->score = 0;
    this->show = false;
    for (i = 0; i < FRUIT_COUNT; i++)
    {
        if (got_fruit[i])
        {
            this->score += 1;
        }
    }
}

static void FLAG_draw(OBJ* this)
{
    this->spr = 118 + P8modulo(((float)frames / 5.f), 3);
    P8spr(this->spr, this->x, this->y, 1, 1, false, false);
    if (this->show)
    {
        P8rectfill(32, 2, 96, 31, 0);
        P8spr(26, 55, 6, 1, 1, false, false);
        {
            char str[16];
            SDL_snprintf(str, sizeof(str), "x%i", this->score);
            P8print(str, 64, 9, 7);
        }
        draw_time(49, 16);
        {
            char str[16];
            SDL_snprintf(str, sizeof(str), "deaths:%i", deaths);
            P8print(str, 48, 24, 7);
        }
    }
    else if (OBJ_check(this, OBJ_PLAYER, 0, 0))
    {
        P8sfx(55);
        sfx_timer = 30;
        this->show = true;
    }
}

static void ROOM_TITLE_init(OBJ* this)
{
    this->delay = 5;
}

static void ROOM_TITLE_draw(OBJ* this)
{
    this->delay -= 1;
    if (this->delay < -30)
    {
        destroy_object(this);
    }
    else if (this->delay < 0)
    {
        P8rectfill(24, 58, 104, 70, 0);
        if (room.x == 3 && room.y == 1)
        {
            P8print("old site", 48, 62, 7);
        }
        else if (level_index() == 30)
        {
            P8print("summit", 52, 62, 7);
        }
        else
        {
            int level = (1 + level_index()) * 100;
            {
                char str[16];
                SDL_snprintf(str, sizeof(str), "%i m", level);
                P8print(str, 52 + (level < 1000 ? 2 : 0), 62, 7);
            }
        }

        draw_time(4, 4);
    }
}

// object functions //
//////////////////////-
static OBJ* init_object(OBJTYPE type, float x, float y)
{
    static short next_id = 0;
    OBJ* obj = NULL;
    int i;

    if (OBJTYPE_prop[type].if_not_fruit && got_fruit[level_index()])
    {
        return NULL;
    }
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        if (!objects[i].active) {
            obj = &objects[i];
            break;
        }
    }
    if (!obj)
    {
        // No more free space for objects, give up.
        SDL_Log("Exhausted object memory..");
        return NULL;
    }
    obj->active = true;
    obj->id = next_id++;

    obj->type = type;
    obj->collideable = true;
    obj->solids = true;

    obj->spr = OBJTYPE_prop[type].tile;
    obj->flip_x = obj->flip_y = false;

    obj->x = x;
    obj->y = y;
    obj->hitbox = (HITBOX){ .x = 0,.y = 0,.w = 8,.h = 8 };

    obj->spd = (VEC){ .x = 0,.y = 0 };
    obj->rem = (VEC){ .x = 0,.y = 0 };

    if (OBJ_PROP(obj).init != NULL)
    {
        OBJ_PROP(obj).init(obj);
    }
    return obj;
}

static void destroy_object(OBJ* obj)
{
    // Shift all slots to the right of this object to the left, necessary to simulate loading jank
    SDL_assert(obj >= objects && obj < objects + MAX_OBJECTS);
    for (; obj + 1 < objects + MAX_OBJECTS; obj++)
    {
        *obj = *(obj + 1);
    }
    objects[MAX_OBJECTS - 1].active = false;
}

static void kill_player(OBJ* obj)
{
    int   dead_particles_count = 0;
    float dir;
    sfx_timer = 12;
    P8sfx(0);
    deaths += 1;
    shake = 10;
    for (dir = 0; dir <= 7; dir += 1)
    {
        float angle = (dir / 8);
        dead_particles[dead_particles_count].active = true;
        dead_particles[dead_particles_count].x = obj->x + 4;
        dead_particles[dead_particles_count].y = obj->y + 4;
        dead_particles[dead_particles_count].t = 10;
        dead_particles[dead_particles_count].spd2.x = P8sin(angle) * 3;
        dead_particles[dead_particles_count].spd2.y = P8cos(angle) * 3;
        dead_particles_count++;
        restart_room();
    }
    destroy_object(obj); // LEMON: moved here to avoid using ->x and ->y from dead object
}

// room functions //
////////////////////
static void restart_room()
{
    will_restart = true;
    delay_restart = 15;
}

static void next_room()
{
    if (room.x == 2 && room.y == 1)
    {
        P8music(30, 500, 7);
    }
    else if (room.x == 3 && room.y == 1)
    {
        P8music(20, 500, 7);
    }
    else if (room.x == 4 && room.y == 2)
    {
        P8music(30, 500, 7);
    }
    else if (room.x == 5 && room.y == 3)
    {
        P8music(30, 500, 7);
    }

    if (room.x == 7)
    {
        load_room(0, room.y + 1);
    }
    else
    {
        load_room(room.x + 1, room.y);
    }
}

static bool room_just_loaded = false; // For debugging loading jank.

static void load_room(int x, int y)
{
    int i, tx, ty;
    has_dashed = false;
    has_key = false;
    room_just_loaded = true;

    for (i = 0; i < MAX_OBJECTS; i++)
    {
        objects[i].active = false;
    }

    // Current room.
    room.x = x;
    room.y = y;

    // Entities.
    for (tx = 0; tx <= 15; tx++)
    {
        for (ty = 0; ty <= 15; ty++)
        {
            int tile = P8mget(room.x * 16 + tx, room.y * 16 + ty);
            if (tile == 11)
            {
                init_object(OBJ_PLATFORM, tx * 8, ty * 8)->dir = -1;
            }
            else if (tile == 12)
            {
                init_object(OBJ_PLATFORM, tx * 8, ty * 8)->dir = 1;
            }
            else
            {
                int type;
                for (type = 0; type < OBJTYPE_COUNT; type++) //safe since types are ordered starting at 0
                {
                    if (tile == OBJTYPE_prop[type].tile)
                    {
                        init_object((OBJTYPE)type, tx * 8, ty * 8);
                    }
                }
            }
        }
    }

    if (!is_title())
    {
        init_object(OBJ_ROOM_TITLE, 0, 0);
    }
}

// update function //
/////////////////////
void Celeste_P8_update()
{
    int i;

    frames = ((frames + 1) % 30);
    if (frames == 0 && level_index() < 30)
    {
        seconds = ((seconds + 1) % 60);
        if (seconds == 0)
        {
            minutes += 1;
        }
    }

    if (music_timer > 0)
    {
        music_timer -= 1;
        if (music_timer <= 0)
        {
            P8music(10, 0, 7);
        }
    }

    if (sfx_timer > 0)
    {
        sfx_timer -= 1;
    }

    // cancel if (freeze
    if (freeze > 0)
    {
        freeze -= 1;
        return;
    }

    // Screenshake.
    if (shake > 0)
    {
        shake -= 1;
        P8camera(0, 0);
        if (shake > 0)
        {
            P8camera(-2 + P8rnd(5), -2 + P8rnd(5));
        }
    }

    // Restart (soon).
    if (will_restart && delay_restart > 0)
    {
        delay_restart -= 1;
        if (delay_restart <= 0)
        {
            will_restart = false;
            load_room(room.x, room.y);
        }
    }

    room_just_loaded = false;
    // Update each object.
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        OBJ* obj = &objects[i];
        short this_id;

redo_update_slot:
        if (!obj->active)
        {
            continue;
        }

        OBJ_move(obj, obj->spd.x, obj->spd.y);
        //printf("update #%i (%s)\n", i, OBJ_PROP(obj).nam);
        this_id = obj->id;
        if (OBJ_PROP(obj).update != NULL)
        {
            OBJ_PROP(obj).update(obj);
        }

        if (room_just_loaded)
        {
            room_just_loaded = false;
        }
        /*LEMON: necessary to correctly simulate loading jank: due to the way pico-8's foreach() works,
         *       when element #i is removed and replaced by another different object, the function iterates
         *       over this index again. thus for example, the player is in slot N before a new room is loaded,
         *       all objects are deleted and new objects are spawned, and the objects now in slots [N, last] are updated
         */
        if (this_id != obj->id)
        {
            goto redo_update_slot;
        }

    }

    // Start game
    if (is_title())
    {
        if (!start_game && (P8btn(k_jump) || P8btn(k_dash)))
        {
            P8music(-1, 0, 0);
            start_game_flash = 50;
            start_game = true;
            P8sfx(38);
        }
        if (start_game)
        {
            start_game_flash -= 1;
            if (start_game_flash <= -30)
            {
                begin_game();
            }
        }
    }
}

// drawing functions //
//////////////////////-
void Celeste_P8_draw()
{
    int bg_col = 0;
    int i;
    int off;

    if (freeze > 0)
    {
        return;
    }

    // Reset all palette values.
    P8pal_reset();

    // Start game flash.
    if (start_game)
    {
        int c = 10;

        if (start_game_flash > 10)
        {
            if (frames % 10 < 5)
            {
                c = 7;
            }
        }
        else if (start_game_flash > 5)
        {
            c = 2;
        }
        else if (start_game_flash > 0)
        {
            c = 1;
        }
        else
        {
            c = 0;
        }
        if (c < 10)
        {
            P8pal(6, c);
            P8pal(12, c);
            P8pal(13, c);
            P8pal(5, c);
            P8pal(1, c);
            P8pal(7, c);
        }
    }

    // Clear screen.
    if (flash_bg)
    {
        bg_col = frames / 5;
    }
    else if (new_bg)
    {
        bg_col = 2;
    }
    P8rectfill(0, 0, 128, 128, bg_col);

    // Clouds.
    if (!is_title())
    {
        for (i = 0; i <= 16; i++)
        {
            CLOUD* c = &clouds[i];
            c->x += c->spd;
            P8rectfill(c->x, c->y, c->x + c->w, c->y + 4 + (1 - c->w / 64.0) * 12, new_bg ? 14 : 1);
            if (c->x > 128)
            {
                c->x = -c->w;
                c->y = P8rnd(128 - 8);
            }
        }
    }

    // Draw bg terrain.
    P8map(room.x * 16, room.y * 16, 0, 0, 16, 16, 4);

    // Platforms/big chest.
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        OBJ* o = &objects[i];
        if (o->active && (o->type == OBJ_PLATFORM || o->type == OBJ_BIG_CHEST))
        {
            draw_object(o);
        }
    }

    // Draw terrain.
    off = is_title() ? -4 : 0;
    P8map(room.x * 16, room.y * 16, off, 0, 16, 16, 2);

    // Draw objects.
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        OBJ* o = &objects[i];
        short this_id;
redo_draw:;
        this_id = o->id;
        if (o->active && (o->type != OBJ_PLATFORM && o->type != OBJ_BIG_CHEST))
        {
            draw_object(o);
        }

        // LEMON: draw_object() could have deleted obj, and something could have been moved in its place, so check for that in order not to skip drawing an object.
        if (this_id != o->id) goto redo_draw;
    }

    // Draw fg terrain.
    P8map(room.x * 16, room.y * 16, 0, 0, 16, 16, 8);

    // Particles.
    for (i = 0; i <= 24; i++)
    {
        PARTICLE* p = &particles[i];
        p->x += p->spd;
        p->y += P8sin(p->off);
        p->off += P8min(0.05, p->spd / 32);
        P8rectfill(p->x, p->y, p->x + p->s, p->y + p->s, p->c);
        if (p->x > 128 + 4)
        {
            p->x = -4;
            p->y = P8rnd(128);
        }
        p++;
    }

    // Dead particles.
    for (i = 0; i <= 7; i++)
    {
        PARTICLE* p = &dead_particles[i];
        if (p->active)
        {
            p->x += p->spd2.x;
            p->y += p->spd2.y;
            p->t -= 1;
            if (p->t <= 0)
            {
                p->active = false;
            }
            P8rectfill(p->x - p->t / 5, p->y - p->t / 5, p->x + p->t / 5, p->y + p->t / 5, 14 + P8modulo(p->t, 2));
        }

        p++;
    }

    // Draw outside of the screen for screenshake.
    P8rectfill(-5, -5, -1, 133, 0);
    P8rectfill(-5, -5, 133, -1, 0);
    P8rectfill(-5, 128, 133, 133, 0);
    P8rectfill(128, -5, 133, 133, 0);

    // Credits.
    if (is_title())
    {
        P8print("v1.03", 54, 55, 5);
#if defined (SDL_PLATFORM_NGAGE)
        P8print("5+7", 58, 80, 5);
#else
        P8print("x+c", 58, 80, 5);
#endif
        P8print("maddy thorson", 41, 96, 5);
        P8print("noel berry", 46, 102, 5);
    }

    if (level_index() == 30)
    {
        OBJ* p = NULL;
        for (i = 0; i < MAX_OBJECTS; i++)
        {
            if (objects[i].active && objects[i].type == OBJ_PLAYER)
            {
                p = &objects[i];
                break;
            }
        }
        if (p != NULL)
        {
            float diff = P8min(24, 40 - P8abs(p->x + 4 - 64));
            P8rectfill(0, 0, diff, 128, 0);
            P8rectfill(128 - diff, 0, 128, 128, 0);
        }
    }
}

static void draw_object(OBJ* obj)
{
    if (OBJ_PROP(obj).draw != NULL)
    {
        OBJ_PROP(obj).draw(obj);
    }
    else if (obj->spr > 0)
    {
        P8spr(obj->spr, obj->x, obj->y, 1, 1, obj->flip_x, obj->flip_y);
    }
}

static void draw_time(float x, float y)
{
    int s = seconds;
    int m = minutes % 60;
    int h = minutes / 60;

    P8rectfill(x, y, x + 32, y + 6, 0);
    {
        char str[27];
        SDL_snprintf(str, sizeof(str), "%.2i:%.2i:%.2i", h, m, s);
        P8print(str, x + 1, y + 1, 7);
    }
}

// helper functions //
//////////////////////
static float clamp(float val, float a, float b)
{
    return P8max(a, P8min(b, val));
}

static float appr(float val, float target, float amount)
{
    return val > target
        ? P8max(val - amount, target)
        : P8min(val + amount, target);
}

static float sign(float v)
{
    return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

static bool maybe()
{
    return P8rnd(1) < 0.5;
}

static bool solid_at(int x, int y, int w, int h)
{
    return tile_flag_at(x, y, w, h, 0);
}

static bool ice_at(int x, int y, int w, int h)
{
    return tile_flag_at(x, y, w, h, 4);
}

static bool tile_flag_at(int x, int y, int w, int h, int flag)
{
    int i;
    for (i = (int)P8max(0, P8flr(x / 8)); i <= P8min(15, (x + w - 1) / 8); i++)
    {
        int j;
        for (j = (int)P8max(0, P8flr(y / 8)); j <= P8min(15, (y + h - 1) / 8); j++)
        {
            if (P8fget(tile_at(i, j), flag))
            {
                return true;
            }
        }
    }
    return false;
}

static int tile_at(int x, int y)
{
    return P8mget(room.x * 16 + x, room.y * 16 + y);
}

static bool spikes_at(float x, float y, int w, int h, float xspd, float yspd)
{
    int i;
    for (i = (int)P8max(0, P8flr(x / 8)); i <= P8min(15, (x + w - 1) / 8); i++)
    {
        int j;
        for (j = (int)P8max(0, P8flr(y / 8)); j <= P8min(15, (y + h - 1) / 8); j++)
        {
            int tile = tile_at(i, j);
            if (tile == 17 && (P8modulo(y + h - 1, 8) >= 6 || y + h == j * 8 + 8) && yspd >= 0)
            {
                return true;
            }
            else if (tile == 27 && P8modulo(y, 8) <= 2 && yspd <= 0)
            {
                return true;
            }
            else if (tile == 43 && P8modulo(x, 8) <= 2 && xspd <= 0)
            {
                return true;
            }
            else if (tile == 59 && (P8modulo(x + w - 1, 8) >= 6 || x + w == i * 8 + 8) && xspd >= 0)
            {
                return true;
            }
        }
    }
    return false;
}

//////////END/////////
void Celeste_P8__DEBUG(void)
{
    if (is_title())
    {
        start_game = true, start_game_flash = 1;
    }
    else
    {
        next_room();
    }
}

//all of the global game variables; this holds the entire game state (exc. music/sounds playing)
#define LISTGVARS(V)                                                    \
    V(rnd_seed_lo) V(rnd_seed_hi)                                       \
        V(room) V(freeze) V(shake) V(will_restart) V(delay_restart) V(got_fruit) \
        V(has_dashed) V(sfx_timer) V(has_key) V(pause_player) V(flash_bg) V(music_timer) \
        V(new_bg) V(frames) V(seconds) V(minutes) V(deaths) V(max_djump) V(start_game) \
        V(start_game_flash) V(clouds) V(particles) V(dead_particles) V(objects)

size_t Celeste_P8_get_state_size(void)
{
#define V_SIZE(v) (sizeof v) +
    enum
    { //force comptime evaluation
        sz = LISTGVARS(V_SIZE) - 0
    };
    return sz;
#undef V_SIZE
}

void Celeste_P8_save_state(void* st_)
{
    char* st;
    SDL_assert(st_ != NULL);
    st = (char*)st_;
#define V_SAVE(v) memcpy(st, &v, sizeof v), st += sizeof v;
    LISTGVARS(V_SAVE)
#undef V_SAVE
}

void Celeste_P8_load_state(const void* st_)
{
    const char* st;
    SDL_assert(st_ != NULL);
    st = (const char*)st_;
#define V_LOAD(v) memcpy(&v, st, sizeof v), st += sizeof v;
    LISTGVARS(V_LOAD)
#undef V_LOAD
}

#undef LISTGVARS

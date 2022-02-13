#ifndef _RENDER_H
#define _RENDER_H
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <stdint.h>
#include <system_error>
#include <vector>
#include <string>
#include <windows.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_FontCache.h"
#include "SoLoud/soloud.h"
#include "SoLoud/soloud_wav.h"
#include "SoLoud/soloud_wavstream.h"
#include "SoLoud/soloud_modplug.h"
#include "SoLoud/soloud_openmpt.h"
#include "SoLoud/MIDI/soloud_midi.h"
#include "SoLoud/soloud_speech.h"
#include "SoLoud/soloud_vizsn.h"
#include "SoLoud/soloud_waveshaperfilter.h"
#include "SoLoud/soloud_biquadresonantfilter.h"
#include "SoLoud/soloud_fftfilter.h"

#include "SDL2/SDL_stbimage.h"

#include "toml.hpp"
#include <fstream>

#include "guicon.h"

#ifdef _WIN32 || WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

using namespace std;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define FRAMERATE 60

#define MAX_SE 10

extern string SOUNDFONT;

struct Vector2
{
  float x = 1;
  float y = 1;
};

struct Vector2i
{
  int x = 1;
  int y = 1;
};

enum AXIS {
    X,
    Y,
    XY
};

enum CLOSE_CODES {
    NO_ERROR_CODE,
    ERROR_CODE,
    UNKNOWN
};

enum LOG_TYPE {
    NORMAL,
    WARNING,
    ERROR_
};

namespace Render {
    extern SDL_Window* window;
    extern SDL_Renderer* renderer;

    /*
    * The object class is a class that contains functionality to make it do... stuff.
    */
    class Object {
        public:
            Object();
            ~Object();

            int id = NULL;

            /*
            * Create a new Object instance.
            */
            virtual void create(int x = 0, int y = 0, string path = "");

            virtual void Draw(float dt);

            int x, y, w, h;

            Vector2i offset = {0, 0};

            Vector2 scale;

            float angle = 0.00001;

            float alpha = 100;

            SDL_Point center = {0, 0};

            map<string, bool> get_properties() const;
            int _sc_x, _sc_y, _sc_w, _sc_h;
            SDL_Texture* _tex = nullptr;
            void set_property(string name, bool value);

            SDL_Rect get_src() {return src_rect;};

            SDL_Rect src_rect = {0, 0, 0, 0};

            /*
            * Center object on the center of the screen on a certain axis. Defaults to both X and Y.
            */
            void centerSelf(AXIS axis = XY);

            void setCamera(SDL_Rect* cam_p);
        private:
            int _x, _y, _w, _h;
            int _ori_w, _ori_h;
            SDL_Rect du = {0, 0, 0, 0};
            SDL_Rect* cam_rect = &du;
            map<string, bool> _properties;
    };

    class AnimatedObject : public Object {
        public:
            /*
            * Create a new AnimatedObject instance.
            */
            virtual void create(int x = 0, int y = 0, string path = "");
            /*
            * Add an animation to said object. Uses SDL_Rects for frames.
            */
            void AddAnimation(string anim_name, vector<SDL_Rect> points);
            /*
            * Play an animation.
            */
            void PlayAnimation(string anim_name);
            virtual void Draw(float dt);
            
            int framerate = 24;
            map<string, vector<SDL_Rect>> frameRects;
            string current_framename = "";
            int current_frame = 0;

        private:
            int startTime;
    };

    class TextObject : public Object {
        public:
            /*
            * Create a new TextObject instance.
            */
            virtual void create(int x = 0, int y = 0, string text = "", string font_name = "data/monogram.ttf", SDL_Color color = {255, 255, 255, 255}, int style = TTF_STYLE_NORMAL, int size = 20);

            virtual void Draw(float dt);

            FC_Font* font;
            FC_AlignEnum alignment = FC_ALIGN_LEFT;
            SDL_Color color;
            string text = "";
            bool antialiasing = false;
        private:
            int font_size = 20;
    };

    template <typename T>
    using Func = std::function<T(T)>;

    extern vector<float> _sec;
    extern vector<Func<int>> _call;
    extern vector<bool> _repeats;
    extern vector<int> _ticks;

    class Timer {
        public:
            void start(float seconds, Func<int> callback, bool repeat = false) {
                _sec.push_back(seconds);
                _call.push_back(callback);
                _repeats.push_back(repeat);
                _ticks.push_back(0);
            }
    };

    /*
    * A state is where you would contain said objects.
    */
    class State {
        public:
            State();
            ~State();
            /*
            * State entrypoint.
            */
            virtual void Create() {
                
            }
            /*
            * State update point.
            */
            virtual void Update(float dt) {}
            /*
            * State draw point. Make sure to call State::Draw() first when overriding this!!!
            */
            virtual void Draw(float dt){
                SDL_RenderClear(renderer);
                if (obj.size() > 0)
                    for (int i = 0; i < obj.size(); i++) {
                        obj[i]->Draw(dt);
                        if (obj[i]->_tex != nullptr) {
                            SDL_Rect r = {obj[i]->_sc_x+obj[i]->offset.x, obj[i]->_sc_y+obj[i]->offset.y, obj[i]->_sc_w, obj[i]->_sc_h};
                            SDL_Rect r2 = obj[i]->src_rect;
                            if (r2.w != 0 && r2.h != 0)
                                SDL_RenderCopyEx(renderer, obj[i]->_tex, &r2, &r, obj[i]->angle, &obj[i]->center, SDL_FLIP_NONE);
                            else
                                SDL_RenderCopyEx(renderer, obj[i]->_tex, NULL, &r, obj[i]->angle, &obj[i]->center, SDL_FLIP_NONE);
                        }
                    }
                SDL_RenderPresent(renderer);
            }
            /*
            * Use this to add objects. Make sure to pass the address, not the object itself.
            */
            virtual void AddObject(Object* object);
            vector<Object*> get_obj();
        private:
            vector<Object*> obj;
    };

    /*
    * Init EVERYTHING. Also makes sure everything works.
    */
    bool Init(string window_name);
    /*
    * Internal main update loop. Only call once.
    */
    bool Update();
    /*
    * Switch our current state. Pass the address of the state, not the state itself.
    */
    void SwitchState(State* state);

    extern SDL_Event event;

    extern HWND hwnd;
    extern HWND consoleD;

    extern State* current_state;

    extern SoLoud::Soloud se;
    extern SoLoud::Soloud music;
    extern SoLoud::WavStream waveLoader;
    extern SoLoud::Openmpt modLoader;
    extern SoLoud::Midi midiLoader;
    extern SoLoud::SoundFont current_sf;
    extern string currentMusic;
    extern int seIndex;
    /*
    * Play a sound. Will not override unless said so.
    */
    bool playSound(string path, bool override = false); 
    /*
    * Play music. Always loops.
    * Passing a blank string (e.g. "") will stop the current playing music.
    */
    bool playMusic(string path); 
    /*
    * Play music thru the openMPT api (669, amf, ams, dbm, digi, dmf, dsm, far, gdm, ice, imf, it, itp, j2b, m15, mdl, med, mo3, mod, mptm, mt2, mtm, okt, plm, psm, ptm, s3m, stm, ult, umx, wow, xm). Always loops.
    * When a midi is passed (mid), it will use a TSF-based midi loader instead.
    * Passing a blank string (e.g. "") will stop the current playing music.
    */
    bool playModPlug(string path); 

    /*
    * Make the camera center itself on an object.
    */
    void pointTo(SDL_Rect* camera, Object object);

    inline void log(string prefix, string msg, LOG_TYPE type = NORMAL, string file = "???.cpp", int line = 0) {
        clock_t now = std::clock();

        double now_n = (double)now / CLOCKS_PER_SEC;

        string typeName = "LOG";

        switch (type) {
            case NORMAL:
                break;
            case WARNING:
                typeName = "WARNING";
                break;
            case ERROR_:
                typeName = "ERROR";
                break;
        }

        std::stringstream buf;

        buf << (int)(now_n/60) << ":"
            << std::setfill('0') << std::setw(2) << (int)((int)now_n % 60) << "."
            << std::setfill('0') << std::setw(3) << (int)((now_n - (int)now_n) * 1000) << " "
            << typeName << ": (" << file << ":" << line << ") " << prefix << msg << endl;

        std::ofstream logFile;
        logFile.open("log.txt", std::ios::app);
        logFile << buf.str();
        logFile.close();

        cout << buf.str();
    }

    inline int Sec2Tick(float time) {
        return FRAMERATE*time;
    }

    // Parse a TOML at a given path with a table and key.
    // Setting table name to "" will look for the key without a table.
    template <typename T>
    T tomlParse(string path, string table_name = "", string key = "") {
        auto parsed = toml::parse(path);
        if (table_name != "") {
            auto& config_table = toml::find(parsed, table_name);
            return toml::find<T>(config_table, key);
        }
        return toml::find<T>(parsed, key);
    }

    // "touch" a file at a given path.
    // Returns false if file is empty, returns true if not
    inline bool touchFile(string path) {
        std::ofstream file;
        file.open(path, std::ios::app);
        file.close();
        fstream oFile(path);
        oFile.seekg(0,std::ios::end);
        unsigned int size = oFile.tellg();
        if (!size) {
            oFile.close();
            return false;
        }
        oFile.close();
        return true;
    }

    // Export a TOML file to a path from a toml::value.
    inline void tomlExport(string path, toml::value values) {
        string export_ = toml::format(values, 0, 2);
        std::ofstream config;
        config.open(path, std::ofstream::out | std::ofstream::trunc);
        config << export_ << endl;
        config.close();
    }

    // SPEAK you fucking BITCH
    extern SoLoud::Speech SPEAK_BITCH;

    extern SoLoud::FFTFilter bass;

    static HMENU exitButton;

    // makes A BITCH SPEAK out shit
    inline void SPEAK(string words, unsigned int aBaseFrequency = 3000.0f, float aBaseSpeed = 10.0f, float aBaseDeclination = 0.5f, int aBaseWaveform = KW_SQUARE) {
        SPEAK_BITCH.setFilter(1, &bass);
        SPEAK_BITCH.setParams(aBaseFrequency, aBaseSpeed, aBaseDeclination, aBaseWaveform);
        SPEAK_BITCH.setVolume(5.0);
        SPEAK_BITCH.setText(words.c_str());
        seIndex = se.play(SPEAK_BITCH);
    }

    inline void AddExitButton() {
        exitButton = CreateMenu();

        AppendMenu(exitButton, MF_STRING, 1, "Exit");
        SetMenu(hwnd, exitButton);
    }
}
#endif
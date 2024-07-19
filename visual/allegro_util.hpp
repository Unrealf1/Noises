#pragma once

#include <stdexcept>
#include <functional>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <util.hpp>


struct AllegroInitException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<typename Raw, typename Actual>
struct AllegroDecorator {
protected:
    Raw* al_pointer;

public:
    template<typename... Args>
    AllegroDecorator(const Args&... args) : al_pointer(Actual::init(args...)) {
        if (al_pointer == nullptr) {
            throw AllegroInitException(std::string(util::type_name<Raw>()));
        }
    }

    ~AllegroDecorator() {
        Actual::destroy(al_pointer);
    }

    AllegroDecorator(const AllegroDecorator&) = delete;
    AllegroDecorator& operator=(const AllegroDecorator&) = delete;
    AllegroDecorator& operator=(AllegroDecorator&& other) {
        Actual::destroy(al_pointer);
        al_pointer = other.al_pointer;
        other.al_pointer = nullptr;
        return *this;
    }

    AllegroDecorator(AllegroDecorator&& other) {
        al_pointer = other.al_pointer;
        other.al_pointer = nullptr;
    }

public:
    Raw* get_raw() {
        return al_pointer;
    }
};

struct Bitmap : public AllegroDecorator<ALLEGRO_BITMAP, Bitmap> {
    using AllegroDecorator::AllegroDecorator;
    using Raw = ALLEGRO_BITMAP;

    static Raw* init(int w, int h, int additional_flags = 0);
    static void destroy(Raw*);

    int height();
    int width();
};

struct Timer : public AllegroDecorator<ALLEGRO_TIMER, Timer> {
    using AllegroDecorator::AllegroDecorator;
    using Raw = ALLEGRO_TIMER;

    static Raw* init(double period);
    static void destroy(Raw*);

    void start();
    void stop();
    void change_rate(double new_rate);
    double get_rate() const;
    ALLEGRO_EVENT_SOURCE* event_source();
};

struct EventQueue : public AllegroDecorator<ALLEGRO_EVENT_QUEUE, EventQueue> {
    using AllegroDecorator::AllegroDecorator;
    using Raw = ALLEGRO_EVENT_QUEUE;

    static Raw* init();
    static void destroy(Raw*);
    
    bool empty() const;
    void register_source(ALLEGRO_EVENT_SOURCE*);
    void unregister_source(ALLEGRO_EVENT_SOURCE*);
    
    void get(ALLEGRO_EVENT&);
    void peek(ALLEGRO_EVENT&);
    void drop_one();
    void drop_all();
    void wait();
    void wait(ALLEGRO_EVENT&);
    void wait_for(ALLEGRO_EVENT&, float seconds);
};

class EventReactor : public EventQueue {
public:
    using EventQueue::EventQueue;
    using Reaction = std::function<void(const ALLEGRO_EVENT&)>;

    void add_reaction(const ALLEGRO_EVENT_SOURCE*, Reaction);
    void wait_and_react(ALLEGRO_EVENT&);
    void wait_and_react();
private:
    std::unordered_map<const ALLEGRO_EVENT_SOURCE*, std::vector<Reaction>> m_reactions;
};

struct TargetBitmapOverride{
  TargetBitmapOverride(ALLEGRO_BITMAP* new_target) {
    m_old_target = al_get_target_bitmap();
    if (m_old_target != new_target) {
      al_set_target_bitmap(new_target);
    }
  }

  ~TargetBitmapOverride() {
    al_set_target_bitmap(m_old_target);
  }
  ALLEGRO_BITMAP* m_old_target;

  TargetBitmapOverride(const TargetBitmapOverride&) = delete;
  // Can be implemented, deleted for now to avoid confusion
  TargetBitmapOverride(TargetBitmapOverride&&) = delete;
};

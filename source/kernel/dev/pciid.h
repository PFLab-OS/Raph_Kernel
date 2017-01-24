#include <stdint.h>

#pragma once

namespace PciData {
  class Base {
  public:
  Base(const int a, const char *s)
    : id(a),
      name(s){
      }
    ~Base() = default;
    const int id;
    const char *name;
  };
  
  template<class T>
    class List : public Base {
  public:
  List(const int a, const char *s)
    : Base(a, s),
      next(nullptr) {
      }
    ~List() = default;
    T *next;
  };

  class Subsystem : public List<Subsystem> {
  public:
  Subsystem(const int a,const int b, const char *s)
    : List(a, s),
      id2(b) {
      }
    ~Subsystem() = default;
    const int id2; //id:subvendorID, id2:subdeviceID
  };

  class Device : public List<Device> {
  public:
  Device(const int a, const char *s)
    : List(a, s),
      subsystem(nullptr){
      }
    ~Device() = default;
    Subsystem *subsystem;
  };

  class Vendor : public List<Vendor> {
  public:
  Vendor(const int a, const char *s)
    : List(a, s),
      device(nullptr){
      }
    ~Vendor() = default;
    Device *device;
  };

  class Table {
  public:
    Table() = default;
    ~Table();
    void Search(uint8_t bus, uint8_t device, uint8_t function, const char *search);
    void Init();
  private:
    Vendor *vendor;
  };
}

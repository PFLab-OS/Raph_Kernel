class PciBase {
 public:
 PciBase(const int a, const char *s)
   :id(a),
    name(s){
  }
  ~PciBase() = default;
  const int id;
  const char *name;
};

template<class T>
class PciList:public PciBase {
 public:
  PciList(const int a, const char *s)
    :PciBase(a, s),
    next(nullptr) {
  }
  ~PciList() = default;
  T *next;
};

class PciSubsystem:public PciList<PciSubsystem> {
 public:
  PciSubsystem(const int a,const  int b, const char *s)
    :PciList(a, s),
    id2(b) {
  }
  ~PciSubsystem() = default;
  const int id2; //id:subvendorID, id2:subdeviceID
};

class PciDevice : public PciList<PciDevice> {
 public:
  PciDevice(const int a, const char *s)
    :PciList(a, s),
    subsystem(nullptr){
  }
  ~PciDevice() = default;
  PciSubsystem *subsystem;
};

class PciVendor:public PciList<PciVendor> {
 public:
  PciVendor(const int a, const char *s)
    :PciList(a, s),
    device(nullptr){
  }
  ~PciVendor() = default;
  PciDevice *device;
};

class PciTable {
 public:
  PciTable() = default;
  ~PciTable();
  void SearchDevice(int, int, int, int);
  void Init();
 private:
  PciVendor *vendor;
};

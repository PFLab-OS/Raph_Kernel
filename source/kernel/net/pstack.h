/*
 *
 * Copyright (c) 2016 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_PSTACK_H__
#define __RAPH_KERNEL_NET_PSTACK_H__

#include <string.h>
#include <arpa/inet.h>
#include <global.h>
#include <queue.h>
#include <dev/netdev.h>


class ProtocolStackLayer;
class ProtocolStackBaseLayer;


/**
 * A class handling network protocol suites.
 *
 * It has a main packet queue, base layers and stacking layers.
 * The base layers has instances of duplicated packet queue,
 * which are enqueued after the main queue.
 * Stacking layers stack on the base layers recursively.
 * Clients (socket) are bound to the top layers.
 *
 * [RX sequence]
 * First it enques packets received from the network devices
 * into main queue, and duplicates to enqueue into the base layer.
 * When clients demand packets, the top layers receive the requests,
 * and they are propagated down to the base layer, which pops the packet
 * to return. Clients MUST return the received packets to the protocol stack.
 *
 * [TX sequence]
 * First clients fetch availale buffers from protocol stack.
 * They fill the buffers with messages, and push into the top layer.
 * Pushed buffers propagates down to the base layer, which pushes
 * the received buffers directly into the network device.
 */
class ProtocolStack final {
public:
  ProtocolStack() {}

  virtual ~ProtocolStack() {
    while (!_reserved_queue.IsEmpty()) {
      NetDev::Packet *packet = nullptr;
      _reserved_queue.Pop(packet);
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
    }
  }

  /** packet queue depth */
  static const int kQueueDepth = 100;

  /** type of queue used in ProtocolStack */
  using FunctionalPacketQueue = FunctionalRingBuffer<NetDev::Packet *, kQueueDepth>;
  using PacketQueue = RingBuffer<NetDev::Packet *, kQueueDepth>;

  /**
   * @return if succeeds.
   */
  bool Setup() {
    // init reserved queue
    while (!_reserved_queue.IsFull()) {
      NetDev::Packet *addr = reinterpret_cast<NetDev::Packet *>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
      NetDev::Packet *packet = new(addr) NetDev::Packet();
      _reserved_queue.Push(packet);
    }

    // init callback functions
    ClassFunction<ProtocolStack> escalation_callback;
    escalation_callback.Init(this, &ProtocolStack::EscalationHandler, nullptr);
    _buffered_queue.SetFunction(0, escalation_callback);

    return true;
  }

  /**
   * Set a network device to protocol stack.
   *
   * @param device
   */
  void SetDevice(NetDev *device) {
    assert(_device == nullptr);
    _device = device;

    // set callback function to network device
    ClassFunction<ProtocolStack> netdev_rx_callback;
    netdev_rx_callback.Init(this, &ProtocolStack::DeviceBufferHandler, nullptr);
    _device->SetReceiveCallback(2, netdev_rx_callback);
  }

  /**
   * Set a base layer.
   *
   * @param layer must be initialized before passed to this method.
   * @return if no more space is free.
   */
  bool SetBaseLayer(ProtocolStackBaseLayer *layer);

  /**
   * Transmit the packet to the network device.
   *
   * @param packet MUST be fetched by FetchTxBuffer in advance.
   * @return if succeeds.
   */
  virtual bool TransmitPacket(NetDev::Packet *packet) {
    return _device->TransmitPacket(packet);
  }

  /**
   * Fetch the buffer aimed at transmission from the network device.
   *
   * @param packet MUST be passed to TransmitPacket.
   * @return if succeeds.
   */
  virtual bool FetchTxBuffer(NetDev::Packet *&packet) {
    return _device->GetTxPacket(packet);
  }

  /**
   * Callback which fetches newly arrived packets from the network device,
   * duplicates and inserts into the main queue. Set to NetDev RX queue.
   */
  void DeviceBufferHandler(void *_);

  /**
   * Callback which fetches newly arrived packets from the main queue,
   * duplicates and inserts into base layer queue. Set to main queue.
   */
  void EscalationHandler(void *_);

private:
  /** network device holding this protocol stack */
  NetDev *_device = nullptr;

  /** the maximum number of connections */
  // TODO: enable dynamically change
  static const int kMaxConnectionNumber = 16;

  /** enqueued from network devices at first */
  FunctionalPacketQueue _buffered_queue;
  PacketQueue           _reserved_queue;

  /** the base layer, which is enqueued from the main queue at first */
  ProtocolStackBaseLayer *_base_layers[kMaxConnectionNumber] = {nullptr};
};


/**
 * A class serving as one layer of protocol stack, retaining protocol
 * header information, attaching / detaching protocol header
 * while receiving / transmitting packets.
 *
 * This class itself does not have any protocol information,
 * so you have to make subclasses (e.g. ArpLayer, Ipv4Layer, etc.) to
 * define protocol information.
 *
 * When you make subclasses, you need to override the followings:
 *
 * + (SetupSubclass)
 * + GetProtocolHeaderLength
 * + FilterPacket
 * + PreparePacket
 *
 * in order to define protocol information.
 */
class ProtocolStackLayer {
public:
  ProtocolStackLayer() {}

  /**
   * @param parent reference to its parent layer.
   * @return if succeeds.
   */
  bool Setup(ProtocolStackLayer *parent) {
    if (parent && parent->hasNext()) {
      return false;
    } else {
      _prev_layer = parent;
      _next_layer = nullptr;
      parent->AddLayer(this);
      return this->SetupSubclass();
    }
  }

  virtual void Destroy() {}

  /**
   * Return if it has child layer or not.
   *
   * @return if this layer has child or not.
   */
  bool hasNext() {
    return _next_layer != nullptr;
  }

  /**
   * Add a child layer.
   *
   * @param layer must be initialized before passed to this method.
   * @return if succeeds.
   */
  bool AddLayer(ProtocolStackLayer *layer) {
    if (this->hasNext()) {
      return false;
    } else {
      _next_layer = layer;
      return true;
    }
  }

  /**
   * Set callback function used when receiving packets.
   * This is called recursively, finnaly setting the callback to
   * FunctionalQueue.
   *
   * @param cpuid specifies the serving core.
   * @param func callback.
   */
  virtual void SetReceiveCallback(int cpuid, const Function &func) {
    _prev_layer->SetReceiveCallback(cpuid, func);
  }

  /**
   * Receive packet from the parent layer.
   *
   * @param packet MUST be freed by ReuseRxBuffer once it's no longer necessary.
   * @return if succeeds.
   */
  virtual bool ReceivePacket(NetDev::Packet *&packet) {
    if (_prev_layer->ReceivePacket(packet)) {
      if (this->FilterPacket(packet)) {
        this->DetachProtocolHeader(packet);
        return true;
      } else {
        _prev_layer->ReuseRxBuffer(packet);
        return false;
      }
    } else {
      return false;
    }
  }

  /**
   * Return packet buffer to the parent layer to free.
   *
   * @param packet MUST be fetched by ReceivedPacket.
   * @return if succeeds.
   */
  virtual bool ReuseRxBuffer(NetDev::Packet *packet) {
    this->AttachProtocolHeader(packet);

    if (_prev_layer->ReuseRxBuffer(packet)) {
      return true;
    } else {
      this->DetachProtocolHeader(packet);
      return false;
    }
  }

  /**
   * Transmit the packet to the parent layer.
   *
   * @param packet MUST be fetched by FetchTxBuffer in advance.
   * @return if succeeds.
   */
  virtual bool TransmitPacket(NetDev::Packet *packet) {
    // attach protocol header of this layer
    this->AttachProtocolHeader(packet);

    if (PreparePacket(packet) && _prev_layer->TransmitPacket(packet)) {
      return true;
    } else {
      this->DetachProtocolHeader(packet);
      return false;
    }
  }

  /**
   * Fetch the buffer aimed at transmission from the parent layer.
   *
   * @param packet MUST be passed to TransmitPacket.
   * @return if succeeds.
   */
  virtual bool FetchTxBuffer(NetDev::Packet *&packet) {
    if (_prev_layer->FetchTxBuffer(packet)) {
      this->DetachProtocolHeader(packet);
      return true;
    } else {
      return false;
    }
  }

protected:
  /**
   * Setup sequence of subclasses.
   *
   * @return if succeeds.
   */
  virtual bool SetupSubclass() {
    return true;
  }

  /**
   * Return the length of protocol header, e.g., ArpLayer is expected to
   * return 28 (when Ethernet and IPv4 is used).
   * Some layers may have variable length of headers such as TcpLayer.
   * In such cases, Each instance of this class should return
   * different appropriate values.
   * 
   * @return protocol header length.
   */
  virtual int GetProtocolHeaderLength() {
    return 0;
  }

  /**
   * Filter the received packet by its header content.
   *
   * @param packet
   * @return if the packet is to be received or not.
   */
  virtual bool FilterPacket(NetDev::Packet *packet) {
    return true;
  }

  /**
   * Make contents of the header before transmitting the packet.
   *
   * @param packet
   * @return if succeeds.
   */
  virtual bool PreparePacket(NetDev::Packet *packet) {
    return true;
  }

  /**
   * Attacch protocol header of this layer to the packet.
   * Internally, it's just calculation of pointer offset.
   *
   * @param packet
   */
  void AttachProtocolHeader(NetDev::Packet *packet) {
    packet->buf -= this->GetProtocolHeaderLength();
    packet->len += this->GetProtocolHeaderLength();
  }

  /**
   * Detach protocol header of this layer to the packet.
   * Internally, it's just calculation of pointer offset.
   *
   * @param packet
   */
  void DetachProtocolHeader(NetDev::Packet *packet) {
    packet->buf += this->GetProtocolHeaderLength();
    packet->len -= this->GetProtocolHeaderLength();
  }

  /** reference to the children layer */
  ProtocolStackLayer *_next_layer = nullptr;

private:
  /** reference to the parent layer */
  ProtocolStackLayer *_prev_layer = nullptr;
};


/**
 * Protocol stack base layer, which is the gateway between "stacking layers"
 * and the network device.
 */
class ProtocolStackBaseLayer final : public ProtocolStackLayer {
public:
  ProtocolStackBaseLayer() {}

  void Destroy() override {
    while (!_reserved_queue.IsEmpty()) {
      NetDev::Packet *packet = nullptr;
      _reserved_queue.Pop(packet);
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
    }
  }

  /**
   * Set protocol stack to the base layer. This is used from
   * ProtocolStack::SetBaseLayer.
   *
   * @param pstack
   */
  void SetProtocolStack(ProtocolStack *pstack) {
    _pstack = pstack;
  }

  /**
   * Delegate the received packet into protocol stack base layer.
   * This method is used by ProtocolStack, right after duplicates
   * the received packets.
   *
   * @param packet
   * @return if succeeds.
   */
  bool Delegate(NetDev::Packet *packet) {
    return _buffered_queue.Push(packet);
  }

  /**
   * Set callback function used when receiving packets.
   *
   * @param cpuid specifies the serving core.
   * @param func callback.
   */
  void SetReceiveCallback(int cpuid, const Function &func) override {
    _buffered_queue.SetFunction(cpuid, func);
  }

  /**
   * Receive the packet to pass down to the child layer.
   * Received packets have been inserted into queue.
   *
   * @param packet MUST be freed by ReuseRxBuffer once it's no longer necessary.
   * @return if succeeds.
   */
  bool ReceivePacket(NetDev::Packet *&packet) override {
    return _buffered_queue.Pop(packet);
  }

  /**
   * Return packet buffer to the parent layer to free.
   *
   * @param packet MUST be fetched by ReceivedPacket.
   * @return if succeeds.
   */
  bool ReuseRxBuffer(NetDev::Packet *packet) override {
    return _reserved_queue.Push(packet);
  }

  /**
   * Transmit the packet to protocol stack itself.
   *
   * @param packet MUST be fetched by FetchTxBuffer in advance.
   * @return if succeeds.
   */
  bool TransmitPacket(NetDev::Packet *packet) override {
    return _pstack->TransmitPacket(packet);
  }

  /**
   * Fetch the buffer aimed at transmission from the protocol stack.
   *
   * @param packet MUST be passed to TransmitPacket.
   * @return if succeeds.
   */
  bool FetchTxBuffer(NetDev::Packet *&packet) override {
    return _pstack->FetchTxBuffer(packet);
  }

  /**
   * Fetch the buffer aimed at reception from the protocol stack.
   *
   * @param packet MUST be `Delegate`ed.
   * @return if succeeds.
   */
  bool FetchRxBuffer(NetDev::Packet *&packet) {
    return _reserved_queue.Pop(packet);
  }

  /**
   * @return if the queue is full.
   */
  bool IsFull() {
    return _buffered_queue.IsFull();
  }

protected:
  bool SetupSubclass() override {
    // init reserved queue
    while (!_reserved_queue.IsFull()) {
      NetDev::Packet *addr = reinterpret_cast<NetDev::Packet *>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
      NetDev::Packet *packet = new(addr) NetDev::Packet();
      _reserved_queue.Push(packet);
    }

    return true;
  }

private:
  /** protocol stack main queue duplicates packets and inserts into it */
  ProtocolStack::FunctionalPacketQueue _buffered_queue;
  ProtocolStack::PacketQueue           _reserved_queue;

  /** reference to the protocol stack itself */
  ProtocolStack *_pstack = nullptr;
};


/**
 * Physical layer in protocol stacks, which respond to Ethernet,
 * Infiniband and so on.
 * This class is interface class, so you must override virtual pure functions.
 */
class ProtocolStackPhysicalLayer : public ProtocolStackLayer {
public:
  /**
   * Set physical address connected to network devices, e.g.,
   * MAC address in case of Ethernet.
   *
   * @param addr physical address.
   */
  virtual void SetAddress(uint8_t *addr) = 0;

protected:
  virtual int GetProtocolHeaderLength() = 0;

  virtual bool FilterPacket(NetDev::Packet *packet) = 0;

  virtual bool PreparePacket(NetDev::Packet *packet) = 0;
};


#endif // __RAPH_KERNEL_NET_PSTACK_H__

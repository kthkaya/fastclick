// -*- related-file-name: "../include/click/packet.hh" -*-
/*
 * packet.{cc,hh} -- a packet structure. In the Linux kernel, a synonym for
 * `struct sk_buff'
 * Eddie Kohler, Robert Morris, Nickolai Zeldovich
 *
 * Copyright (c) 1999-2001 Massachusetts Institute of Technology
 * Copyright (c) 2008 Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/packet.hh>
#include <click/glue.hh>
#ifdef CLICK_USERLEVEL
#include <unistd.h>
#endif
CLICK_DECLS

/** @file packet.hh
 * @brief The Packet class models packets in Click.
 */

/** @class Packet
 * @brief A network packet.
 * @nosubgrouping
 *
 * Click's Packet class represents network packets within a router.  Packet
 * objects are passed from Element to Element via the Element::push() and
 * Element::pull() functions.  The vast majority of elements handle packets.
 *
 * A packet consists of a <em>data buffer</em>, which stores the actual packet
 * wire data, and a set of <em>annotations</em>, which store extra information
 * calculated about the packet, such as the destination address to be used for
 * routing.  Every Packet object has different annotations, but a data buffer
 * may be shared among multiple Packet objects, saving memory and speeding up
 * packet copies.  (See Packet::clone.)  As a result a Packet's data buffer is
 * not writable.  To write into a packet, turn it into a nonshared
 * WritablePacket first, using uniqueify(), push(), or put().
 *
 * <h3>Data Buffer</h3>
 *
 * A packet's data buffer is a single flat array of bytes.  The buffer may be
 * larger than the actual packet data, leaving unused spaces called
 * <em>headroom</em> and <em>tailroom</em> before and after the data proper.
 * Prepending headers or appending data to a packet can be quite efficient if
 * there is enough headroom or tailroom.
 *
 * The relationships among a Packet object's data buffer variables is shown
 * here:
 *
 * <pre>
 *                     data()               end_data()
 *                        |                      |
 *       |<- headroom() ->|<----- length() ----->|<- tailroom() ->|
 *       |                v                      v                |
 *       +================+======================+================+
 *       |XXXXXXXXXXXXXXXX|   PACKET CONTENTS    |XXXXXXXXXXXXXXXX|
 *       +================+======================+================+
 *       ^                                                        ^
 *       |<------------------ buffer_length() ------------------->|
 *       |                                                        |
 *    buffer()                                               end_buffer()
 * </pre>
 *
 * Most code that manipulates packets is interested only in data() and
 * length().
 *
 * To create a Packet, call one of the make() functions.  To destroy a Packet,
 * call kill().  To clone a Packet, which creates a new Packet object that
 * shares this packet's data, call clone().  To uniqueify a Packet, which
 * unshares the packet data if necessary, call uniqueify().  To allocate extra
 * space for headers or trailers, call push() and put().  To remove headers or
 * trailers, call pull() and take().
 *
 * <pre>
 *                data()                          end_data()
 *                   |                                |
 *           push()  |  pull()                take()  |  put()
 *          <======= | =======>              <======= | =======>
 *                   v                                v                 
 *       +===========+================================+===========+
 *       |XXXXXXXXXXX|        PACKET CONTENTS         |XXXXXXXXXXX|
 *       +===========+================================+===========+
 * </pre>
 *
 * Packet objects are implemented in different ways in different drivers.  The
 * userlevel driver has its own C++ implementation.  In the linuxmodule
 * driver, however, Packet is an overlay on Linux's native sk_buff
 * object: the Packet methods access underlying sk_buff data directly, with no
 * overhead.  (For example, Packet::data() returns the sk_buff's data field.)
 *
 * <h3>Annotations</h3>
 *
 * Annotations are extra information about a packet above and beyond the
 * packet data.  Packet supports several specific annotations, plus a <em>user
 * annotation area</em> available for arbitrary use by elements.
 *
 * <ul>
 * <li><b>Header pointers:</b> Each packet has three header pointers, designed
 * to point to the packet's MAC header, network header, and transport header,
 * respectively.  Convenience functions like ip_header() access these pointers
 * cast to common header types.  The header pointers are kept up to date when
 * operations like push() or uniqueify() change the packet's data buffer.
 * Header pointers can be null, and they can even point to memory outside the
 * current packet data bounds.  For example, a MAC header pointer will remain
 * set even after pull() is used to shift the packet data past the MAC header.
 * As a result, functions like mac_header_offset() can return negative
 * numbers.</li>
 * <li><b>Timestamp:</b> A timestamp associated with the packet.  Most packet
 * sources timestamp packets when they enter the router; other elements
 * examine or modify the timestamp.</li>
 * <li><b>Device:</b> A pointer to the device on which the packet arrived.
 * Only meaningful in the linuxmodule driver, but provided in every
 * driver.</li>
 * <li><b>Packet type:</b> A small integer indicating whether the packet is
 * meant for this host, broadcast, multicast, or some other purpose.  Several
 * elements manipulate this annotation; in linuxmodule, setting the annotation
 * is required for the host network stack to process incoming packets
 * correctly.</li>
 * <li><b>Performance counter</b> (linuxmodule only): A 64-bit integer
 * intended to hold a performance counter value.  Used by SetCycleCount and
 * others.</li>
 * <li><b>Next packet:</b> A pointer is provided to allow elements to chain
 * packets into a singly linked list.</li>
 * <li><b>Address:</b> Each packet has ADDR_ANNO_SIZE bytes available for a
 * network address.  Routing elements, such as RadixIPLookup, set the address
 * annotation to indicate the desired next hop; ARPQuerier uses this
 * annotation to query the next hop's MAC.  All address annotations share the
 * same space.  The most common use for the area is a destination
 * IP address annotation.</li>
 * <li><b>User annotations:</b> An additional USER_ANNO_SIZE bytes of space
 * are provided for user annotations, which are uninterpreted by Linux or by
 * Click's core.  Instead, elements agree to use portions of the user
 * annotation area in ways that make mutual sense.  The user annotation area
 * can be accessed as a byte array, or as an array of 16- or 32-bit integers.
 * Macros in the <click/packet_anno.hh> header file define the user
 * annotations used by Click's current elements.</li>
 * </ul>
 *
 * New packets start wth all annotations set to zero or null.  Cloning a
 * packet copies its annotations.
 */

/** @class WritablePacket
 * @brief A network packet believed not to be shared.
 *
 * The WritablePacket type represents Packet objects whose data buffers are
 * not shared.  As a result, WritablePacket's versions of functions that
 * access the packet data buffer, such as data(), end_buffer(), and
 * ip_header(), return mutable pointers (<tt>char *</tt> rather than <tt>const
 * char *</tt>).
 *
 * WritablePacket objects are created by Packet::make(), Packet::uniqueify(),
 * Packet::push(), and Packet::put(), which ensure that the returned packet
 * does not share its data buffer.
 *
 * WritablePacket's interface is the same as Packet's except for these type
 * differences.  For documentation, see Packet.
 *
 * @warning The Packet/WritablePacket convention does not eliminate, the
 * likelihood of error when modifying packet data.  It is possible to create a
 * shared WritablePacket; for instance:
 * @code
 * Packet *p = ...;
 * if (WritablePacket *q = p->uniqueify()) {
 *     Packet *p2 = q->clone();
 *     assert(p2);
 *     q->ip_header()->ip_v = 6;   // modifies p2's data as well
 * }
 * @endcode
 * Avoid writing buggy code like this!  Use WritablePacket selectively, and
 * try to avoid calling WritablePacket::clone() when possible. */

inline
Packet::Packet()
{
#if CLICK_LINUXMODULE
    static_assert(sizeof(Anno) <= sizeof(((struct sk_buff *)0)->cb));
    panic("Packet constructor");
#else
    _use_count = 1;
    _data_packet = 0;
    _head = _data = _tail = _end = 0;
# if CLICK_USERLEVEL
    _destructor = 0;
# elif CLICK_BSDMODULE
    _m = 0;
# endif
    clear_annotations();
#endif
}

Packet::~Packet()
{
#if CLICK_LINUXMODULE
    panic("Packet destructor");
#else
    if (_data_packet)
	_data_packet->kill();
# if CLICK_USERLEVEL
    else if (_head && _destructor)
	_destructor(_head, _end - _head);
    else
	delete[] _head;
# elif CLICK_BSDMODULE
    else
	m_freem(_m);
# endif
    _head = _data = 0;
#endif
}

#if !CLICK_LINUXMODULE

inline WritablePacket *
Packet::make(int, int, int)
{
  return static_cast<WritablePacket *>(new Packet(6, 6, 6));
}

bool
Packet::alloc_data(uint32_t headroom, uint32_t len, uint32_t tailroom)
{
  uint32_t n = len + headroom + tailroom;
  if (n < MIN_BUFFER_LENGTH) {
    tailroom = MIN_BUFFER_LENGTH - len - headroom;
    n = MIN_BUFFER_LENGTH;
  }
#if CLICK_USERLEVEL
  unsigned char *d = new unsigned char[n];
  if (!d)
    return false;
  _head = d;
  _data = d + headroom;
  _tail = _data + len;
  _end = _head + n;
#elif CLICK_BSDMODULE
  if (n > MCLBYTES) {
    click_chatter("trying to allocate %d bytes: too many\n", n);
    return false;
  }
  struct mbuf *m;
  MGETHDR(m, M_DONTWAIT, MT_DATA);
  if (!m)
    return false;
  if (n > MHLEN) {
    MCLGET(m, M_DONTWAIT);
    if (!(m->m_flags & M_EXT)) {
      m_freem(m);
      return false;
    }
  }
  _m = m;
  _m->m_data += headroom;
  _m->m_len = len;
  _m->m_pkthdr.len = len;
  assimilate_mbuf();
#endif
  return true;
}

#endif


/** @brief Create and return a new packet.
 * @param headroom headroom in new packet
 * @param data data to be copied into the new packet
 * @param length length of packet
 * @param tailroom tailroom in new packet
 * @return new packet, or null if no packet could be created
 *
 * The @a data is copied into the new packet.  If @a data is null, the
 * packet's data is left uninitialized.  The resulting packet's
 * buffer_length() will be at least MIN_BUFFER_LENGTH; if @a headroom + @a
 * length + @a tailroom would be less, then @a tailroom is increased to make
 * the total MIN_BUFFER_LENGTH.
 *
 * As with all packet creation functions, the packet's annotations are
 * initially cleared and its header pointers are initially null. */
WritablePacket *
Packet::make(uint32_t headroom, const unsigned char *data,
	     uint32_t length, uint32_t tailroom)
{
#if CLICK_LINUXMODULE
    int want = 1;
    if (struct sk_buff *skb = skbmgr_allocate_skbs(headroom, length + tailroom, &want)) {
	assert(want == 1);
	// packet comes back from skbmgr with headroom reserved
	__skb_put(skb, length);	// leave space for data
	if (data)
	    memcpy(skb->data, data, length);
	skb->pkt_type = HOST | PACKET_CLEAN;
	WritablePacket *q = reinterpret_cast<WritablePacket *>(skb);
	q->clear_annotations();
	return q;
    } else
	return 0;
#else
    WritablePacket *p = new WritablePacket;
    if (!p)
	return 0;
    if (!p->alloc_data(headroom, length, tailroom)) {
	delete p;
	return 0;
    }
    if (data)
	memcpy(p->data(), data, length);
    return p;
#endif
}

#if CLICK_USERLEVEL
/** @brief Create and return a new packet (userlevel).
 * @param data data used in the new packet
 * @param length length of packet
 * @param destructor destructor function
 * @return new packet, or null if no packet could be created
 *
 * The packet's data pointer becomes the @a data: the data is not copied into
 * the new packet, rather the packet owns the @a data pointer.  When the
 * packet's data is eventually destroyed, either because the packet is deleted
 * or because of something like a push() or full(), the @a destructor will be
 * called with arguments @a destructor(@a data, @a length).  (If @a destructor
 * is null, the packet data will be freed by <tt>delete[] @a data</tt>.)  The
 * packet has zero headroom and tailroom. */
WritablePacket *
Packet::make(unsigned char *data, uint32_t length,
	     void (*destructor)(unsigned char *, size_t))
{
    WritablePacket *p = new WritablePacket;
    if (p) {
	p->_head = p->_data = data;
	p->_tail = p->_end = data + length;
	p->_destructor = destructor;
    }
    return p;
}
#endif


//
// UNIQUEIFICATION
//

/** @brief Create a clone of this packet.
 * @return the cloned packet
 *
 * The returned clone has independent annotations, initially copied from this
 * packet, but shares this packet's data.  shared() returns true for both the
 * packet and its clone.  Returns null if there's no memory for the clone. */
Packet *
Packet::clone()
{
#if CLICK_LINUXMODULE
  
  struct sk_buff *nskb = skb_clone(skb(), GFP_ATOMIC);
  return reinterpret_cast<Packet *>(nskb);
  
#elif CLICK_USERLEVEL || CLICK_BSDMODULE
# if CLICK_BSDMODULE
  struct mbuf *m;

  if ((m = m_dup(this->_m, M_DONTWAIT)) == NULL)
    return 0;
# endif
  
  // timing: .31-.39 normal, .43-.55 two allocs, .55-.58 two memcpys
  Packet *p = Packet::make(6, 6, 6); // dummy arguments: no initialization
  if (!p)
    return 0;
  memcpy(p, this, sizeof(Packet));
  p->_use_count = 1;
  p->_data_packet = this;
# if CLICK_USERLEVEL
  p->_destructor = 0;
# else
  p->_m = m;
# endif
  // increment our reference count because of _data_packet reference
  _use_count++;
  return p;
  
#endif /* CLICK_LINUXMODULE */
}

WritablePacket *
Packet::expensive_uniqueify(int32_t extra_headroom, int32_t extra_tailroom,
			    bool free_on_failure)
{
  assert(extra_headroom >= (int32_t)(-headroom()) && extra_tailroom >= (int32_t)(-tailroom()));
  
#if CLICK_LINUXMODULE
  
  struct sk_buff *nskb = skb();
  unsigned char *old_head = nskb->head;
  uint32_t old_headroom = headroom(), old_length = length();
  
  uint32_t size = buffer_length() + extra_headroom + extra_tailroom;
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
  size = ((size + 15) & ~15); 
  unsigned char *new_data = reinterpret_cast<unsigned char *>(kmalloc(size + sizeof(atomic_t), GFP_ATOMIC));
# else
  size = SKB_DATA_ALIGN(size);
  unsigned char *new_data = reinterpret_cast<unsigned char *>(kmalloc(size + sizeof(struct skb_shared_info), GFP_ATOMIC));
# endif
  if (!new_data) {
    if (free_on_failure)
      kill();
    return 0;
  }

  unsigned char *start_copy = old_head + (extra_headroom >= 0 ? 0 : -extra_headroom);
  unsigned char *end_copy = old_head + buffer_length() + (extra_tailroom >= 0 ? 0 : extra_tailroom);
  memcpy(new_data + (extra_headroom >= 0 ? extra_headroom : 0), start_copy, end_copy - start_copy);

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
  if (!nskb->cloned || atomic_dec_and_test(skb_datarefp(nskb)))
    kfree(old_head);
# else
  if (!nskb->cloned || atomic_dec_and_test(&(skb_shinfo(nskb)->dataref))) {
    assert(!skb_shinfo(nskb)->nr_frags && !skb_shinfo(nskb)->frag_list);
    kfree(old_head);
  }
# endif
  
  nskb->head = new_data;
  nskb->data = new_data + old_headroom + extra_headroom;
  nskb->tail = nskb->data + old_length;
  nskb->end = new_data + size;
  nskb->len = old_length;
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
  nskb->is_clone = 0;
# endif
  nskb->cloned = 0;

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
  nskb->truesize = size;
  atomic_set(skb_datarefp(nskb), 1);
# else
  nskb->truesize = size + sizeof(struct sk_buff);
  struct skb_shared_info *nskb_shinfo = skb_shinfo(nskb);
  atomic_set(&nskb_shinfo->dataref, 1);
  nskb_shinfo->nr_frags = 0;
  nskb_shinfo->frag_list = 0;
#  if HAVE_LINUX_SKB_SHINFO_GSO_SIZE
  nskb_shinfo->gso_size = 0;
  nskb_shinfo->gso_segs = 0;
  nskb_shinfo->gso_type = 0;
#  elif HAVE_LINUX_SKB_SHINFO_TSO_SIZE
  nskb_shinfo->tso_size = 0;
  nskb_shinfo->tso_segs = 0;
#  endif
#  if HAVE_LINUX_SKB_SHINFO_UFO_SIZE
  nskb_shinfo->ufo_size = 0;
#  endif
#  if HAVE_LINUX_SKB_SHINFO_IP6_FRAG_ID
  nskb_shinfo->ip6_frag_id = 0;
#  endif
# endif

  shift_header_annotations(nskb->head + extra_headroom - old_head);
  return static_cast<WritablePacket *>(this);

#else		/* User-level or BSD kernel module */

  // If someone else has cloned this packet, then we need to leave its data
  // pointers around. Make a clone and uniqueify that.
  if (_use_count > 1) {
    Packet *p = clone();
    WritablePacket *q = (p ? p->expensive_uniqueify(extra_headroom, extra_tailroom, true) : 0);
    if (q || free_on_failure)
      kill();
    return q;
  }
  
  uint8_t *old_head = _head, *old_end = _end;
# if CLICK_BSDMODULE
  struct mbuf *old_m = _m;
# endif
  
  if (!alloc_data(headroom() + extra_headroom, length(), tailroom() + extra_tailroom)) {
    if (free_on_failure)
      kill();
    return 0;
  }
  
  unsigned char *start_copy = old_head + (extra_headroom >= 0 ? 0 : -extra_headroom);
  unsigned char *end_copy = old_end + (extra_tailroom >= 0 ? 0 : extra_tailroom);
  memcpy(_head + (extra_headroom >= 0 ? extra_headroom : 0), start_copy, end_copy - start_copy);

  // free old data
  if (_data_packet)
    _data_packet->kill();
# if CLICK_USERLEVEL
  else if (_destructor)
    _destructor(old_head, old_end - old_head);
  else
    delete[] old_head;
  _destructor = 0;
# elif CLICK_BSDMODULE
  else
    m_freem(old_m);
# endif

  _use_count = 1;
  _data_packet = 0;
  shift_header_annotations(_head + extra_headroom - old_head);
  return static_cast<WritablePacket *>(this);
  
#endif /* CLICK_LINUXMODULE */
}



#ifdef CLICK_BSDMODULE		/* BSD kernel module */
struct mbuf *
Packet::steal_m()
{
  struct Packet *p;
  struct mbuf *m2;

  p = uniqueify();
  m2 = p->m();

  /* Clear the mbuf from the packet: otherwise kill will MFREE it */
  p->_m = 0;
  p->kill();
  return m2;
}
#endif /* CLICK_BSDMODULE */

//
// EXPENSIVE_PUSH, EXPENSIVE_PUT
//

/*
 * Prepend some empty space before a packet.
 * May kill this packet and return a new one.
 */
WritablePacket *
Packet::expensive_push(uint32_t nbytes)
{
  static int chatter = 0;
  if (headroom() < nbytes && chatter < 5) {
    click_chatter("expensive Packet::push; have %d wanted %d",
                  headroom(), nbytes);
    chatter++;
  }
  if (WritablePacket *q = expensive_uniqueify((nbytes + 128) & ~3, 0, true)) {
#ifdef CLICK_LINUXMODULE	/* Linux kernel module */
    __skb_push(q->skb(), nbytes);
#else				/* User-space and BSD kernel module */
    q->_data -= nbytes;
# ifdef CLICK_BSDMODULE
    q->m()->m_data -= nbytes;
    q->m()->m_len += nbytes;
    q->m()->m_pkthdr.len += nbytes;
# endif
#endif
    return q;
  } else
    return 0;
}

WritablePacket *
Packet::expensive_put(uint32_t nbytes)
{
  static int chatter = 0;
  if (tailroom() < nbytes && chatter < 5) {
    click_chatter("expensive Packet::put; have %d wanted %d",
                  tailroom(), nbytes);
    chatter++;
  }
  if (WritablePacket *q = expensive_uniqueify(0, nbytes + 128, true)) {
#ifdef CLICK_LINUXMODULE	/* Linux kernel module */
    __skb_put(q->skb(), nbytes);
#else				/* User-space and BSD kernel module */
    q->_tail += nbytes;
# ifdef CLICK_BSDMODULE
    q->m()->m_len += nbytes;
    q->m()->m_pkthdr.len += nbytes;
# endif
#endif
    return q;
  } else
    return 0;
}

/** @brief Shift packet data within the data buffer.
 * @param offset amount to shift packet data
 * @param free_on_failure if true, then delete the input packet on failure
 * @return a packet with shifted data, or null on failure
 *
 * Useful to align packet data.  For example, if the packet's embedded IP
 * header is located at pointer value 0x8CCA03, then shift_data(1) or
 * shift_data(-3) will both align the header on a 4-byte boundary.
 *
 * If the packet is shared() or there isn't enough headroom or tailroom for
 * the operation, the packet is passed to uniqueify() first.  This can fail if
 * there isn't enough memory.  If it fails, shift_data returns null, and if @a
 * free_on_failure is true (the default), the input packet is freed.
 *
 * @post new data() == old data() + @a offset (if no copy is made)
 * @post new buffer() == old buffer() (if no copy is made) */
Packet *
Packet::shift_data(int offset, bool free_on_failure)
{
  if (offset == 0)
    return this;
  else if (!shared() && (offset < 0 ? headroom() >= (uint32_t)(-offset) : tailroom() >= (uint32_t)offset)) {
    WritablePacket *q = static_cast<WritablePacket *>(this);
    uint8_t *old_head, *new_head;
    if (offset < 0)
      old_head = q->data() - offset, new_head = q->data();
    else
      old_head = q->data(), new_head = q->data() + offset;
    memmove(new_head, old_head, q->end_data() - old_head);
#if CLICK_LINUXMODULE
    struct sk_buff *mskb = q->skb();
    mskb->data += offset;
    mskb->tail += offset;
#else				/* User-space and BSD kernel module */
    q->_data += offset;
    q->_tail += offset;
# if CLICK_BSDMODULE
    q->m()->m_data += offset;
# endif
#endif
    shift_header_annotations(offset);
    return this;
  } else {
    int tailroom_offset = (offset < 0 ? -offset : 0);
    if (offset < 0 && headroom() < (uint32_t)(-offset))
      offset = -headroom() + ((uintptr_t)(data() + offset) & 7);
    else
      offset += ((uintptr_t)buffer() & 7);
    return expensive_uniqueify(offset, tailroom_offset, free_on_failure);
  }
}

CLICK_ENDDECLS

/*
 * ipprint.{cc,hh} -- element prints packet contents to system log
 * Max Poletto, Eddie Kohler
 *
 * Copyright (c) 2000 Mazu Networks, Inc.
 * Copyright (c) 2005-2008 Regents of the University of California
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
#include "ipprint.hh"
#include <click/glue.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include <click/packet_anno.hh>
#include <click/router.hh>
#include <click/nameinfo.hh>

#include <clicknet/ip.h>
#include <clicknet/icmp.h>
#include <clicknet/tcp.h>
#include <clicknet/udp.h>

#if CLICK_USERLEVEL
# include <stdio.h>
#endif

CLICK_DECLS

IPPrint::IPPrint()
{
#if CLICK_USERLEVEL
  _outfile = 0;
#endif
}

IPPrint::~IPPrint()
{
}

int
IPPrint::configure(Vector<String> &conf, ErrorHandler *errh)
{
  _bytes = 1500;
  String contents = "no";
  String payload = "no";
  _label = "";
  _swap = false;
  _payload = false;
  _active = true;
  bool print_id = false;
  bool print_time = true;
  bool print_paint = false;
  bool print_tos = false;
  bool print_ttl = false;
  bool print_len = false;
  bool print_aggregate = false;
  bool bcontents;
  String channel;
  
  if (cp_va_kparse(conf, this, errh,
		   "LABEL", cpkP, cpString, &_label,
		   "CONTENTS", 0, cpWord, &contents,
		   "PAYLOAD", 0, cpWord, &payload,
		   "MAXLENGTH", 0, cpInteger, &_bytes,
		   "NBYTES", 0, cpInteger, &_bytes, // deprecated
		   "ID", 0, cpBool, &print_id,
		   "TIMESTAMP", 0, cpBool, &print_time,
		   "PAINT", 0, cpBool, &print_paint,
		   "TOS", 0, cpBool, &print_tos,
		   "TTL", 0, cpBool, &print_ttl,
		   "SWAP", 0, cpBool, &_swap,
		   "LENGTH", 0, cpBool, &print_len,
		   "AGGREGATE", 0, cpBool, &print_aggregate,
		   "ACTIVE", 0, cpBool, &_active,
#if CLICK_USERLEVEL
		   "OUTFILE", 0, cpFilename, &_outfilename,
#endif
		   "CHANNEL", 0, cpWord, &channel,
		   cpEnd) < 0)
    return -1;

  if (cp_bool(contents, &bcontents))
      _contents = bcontents;
  else if ((contents = contents.upper()), contents == "NONE")
      _contents = 0;
  else if (contents == "HEX")
      _contents = 1;
  else if (contents == "ASCII")
      _contents = 2;
  else
      return errh->error("bad contents value '%s'; should be 'NONE', 'HEX', or 'ASCII'", contents.c_str());

  int payloadv;
  payload = payload.upper();
  if (payload == "NO" || payload == "FALSE")
    payloadv = 0;
  else if (payload == "YES" || payload == "TRUE" || payload == "HEX")
    payloadv = 1;
  else if (payload == "ASCII")
    payloadv = 2;
  else
    return errh->error("bad payload value '%s'; should be 'false', 'hex', or 'ascii'", contents.c_str());

  if (payloadv > 0 && _contents > 0)
    return errh->error("specify at most one of PAYLOAD and CONTENTS");
  else if (payloadv > 0)
    _contents = payloadv, _payload = true;
  
  _print_id = print_id;
  _print_timestamp = print_time;
  _print_paint = print_paint;
  _print_tos = print_tos;
  _print_ttl = print_ttl;
  _print_len = print_len;
  _print_aggregate = print_aggregate;
  _errh = router()->chatter_channel(channel);
  return 0;
}

int
IPPrint::initialize(ErrorHandler *errh)
{
#if CLICK_USERLEVEL
  if (_outfilename) {
    _outfile = fopen(_outfilename.c_str(), "wb");
    if (!_outfile)
      return errh->error("%s: %s", _outfilename.c_str(), strerror(errno));
  }
#else
  (void) errh;
#endif
  return 0;
}

void
IPPrint::cleanup(CleanupStage)
{
#if CLICK_USERLEVEL
    if (_outfile)
	fclose(_outfile);
    _outfile = 0;
#endif
}

void
IPPrint::tcp_line(StringAccum &sa, const Packet *p, int transport_length) const
{
    const click_ip *iph = p->ip_header();
    const click_tcp *tcph = p->tcp_header();
    int ip_len, seqlen;
    uint32_t seq;

    if (transport_length < 4 || !IP_FIRSTFRAG(iph)) {
	sa << IPAddress(iph->ip_src) << " > " << IPAddress(iph->ip_dst) << ": "
	   << (transport_length < 4 ? "truncated-tcp" : "tcp");
	return;
    }

    sa << IPAddress(iph->ip_src) << '.' << ntohs(tcph->th_sport) << " > "
       << IPAddress(iph->ip_dst) << '.' << ntohs(tcph->th_dport) << ": ";
    
    if (transport_length < 14)
	goto truncated_tcp;

    ip_len = ntohs(iph->ip_len);
    seqlen = ip_len - (tcph->th_off << 2);
    if (tcph->th_flags & TH_SYN)
	sa << 'S', seqlen++;
    if (tcph->th_flags & TH_FIN)
	sa << 'F', seqlen++;
    if (tcph->th_flags & TH_RST)
	sa << 'R';
    if (tcph->th_flags & TH_PUSH)
	sa << 'P';
    if (!(tcph->th_flags & (TH_SYN | TH_FIN | TH_RST | TH_PUSH)))
	sa << '.';
    
    seq = ntohl(tcph->th_seq);
    sa << ' ' << seq << ':' << (seq + seqlen)
       << '(' << seqlen << ',' << p->length() << ',' << ip_len << ')';
    if (tcph->th_flags & TH_ACK)
	sa << " ack " << ntohl(tcph->th_ack);

    if (transport_length < 16)
	goto truncated_tcp;
    
    sa << " win " << ntohs(tcph->th_win);
    return;

  truncated_tcp:
    sa << "truncated-tcp";
}

void
IPPrint::udp_line(StringAccum &sa, const Packet *p, int transport_length) const
{
    const click_ip *iph = p->ip_header();
    const click_udp *udph = p->udp_header();

    if (transport_length < 4 || !IP_FIRSTFRAG(iph)) {
	sa << IPAddress(iph->ip_src) << " > " << IPAddress(iph->ip_dst) << ": "
	   << (transport_length < 4 ? "truncated-udp" : "udp");
	return;
    }

    sa << IPAddress(iph->ip_src) << '.' << ntohs(udph->uh_sport) << " > "
       << IPAddress(iph->ip_dst) << '.' << ntohs(udph->uh_dport) << ": ";

    if (transport_length < 8)
	goto truncated_udp;

    sa << "udp " << ntohs(udph->uh_ulen);
    return;

  truncated_udp:
    sa << "truncated-udp";
}

static String
unparse_proto(int ip_p, bool prepend)
{
    if (String s = NameInfo::revquery_int(NameInfo::T_IP_PROTO, 0, ip_p))
	return s;
    else if (prepend)
	return String::stable_string("protocol ", 9) + String(ip_p);
    else
	return String(ip_p);
}

void
IPPrint::icmp_line(StringAccum &sa, const Packet *p, int transport_length) const
{
    const click_ip *iph = p->ip_header();
    const click_icmp *icmph = p->icmp_header();

    sa << IPAddress(iph->ip_src) << " > " << IPAddress(iph->ip_dst) << ": ";
    if (transport_length < 2)
	goto truncated_icmp;

    switch (icmph->icmp_type) {
	
      case ICMP_ECHOREPLY:
	sa << "icmp echo-reply ";
	goto icmp_echo;
      case ICMP_ECHO:
	sa << "icmp echo ";
	/* fallthru */
      icmp_echo: {
	    if (transport_length < 8)
		goto truncated_icmp;
	    const click_icmp_sequenced *seqh = reinterpret_cast<const click_icmp_sequenced *>(icmph);
#define swapit(x) (_swap ? ((((x) & 0xff) << 8) | ((x) >> 8)) : (x))
	    sa << '(' << swapit(seqh->icmp_identifier) << ", " << swapit(seqh->icmp_sequence) << ')';
	    break;
	}
	
      case ICMP_UNREACH: {
	  const click_ip *eiph = reinterpret_cast<const click_ip *>(icmph + 1);
	  int eiph_len = transport_length - sizeof(click_icmp);
	  if (eiph_len < (int) sizeof(click_ip)) {
	      sa << "icmp unreachable ";
	      goto truncated_icmp;
	  }
	  const click_udp *eudph = reinterpret_cast<const click_udp *>(reinterpret_cast<const uint8_t *>(eiph) + (eiph->ip_hl << 2));
	  int eudph_len = eiph_len - (eiph->ip_hl << 2);

	  sa << "icmp " << IPAddress(eiph->ip_dst) << " unreachable ";
	  if (String s = NameInfo::revquery_int(NameInfo::T_ICMP_CODE + icmph->icmp_type, this, icmph->icmp_code))
	      sa << s;
	  else if (icmph->icmp_code)
	      sa << "code " << (int)icmph->icmp_code;
	  switch (icmph->icmp_code) {
	    case ICMP_UNREACH_PROTOCOL:
	      sa << ' ' << unparse_proto(eiph->ip_p, false);
	      break;
	    case ICMP_UNREACH_PORT:
	      sa << ' ' << unparse_proto(eiph->ip_p, true);
	      if (eudph_len < 4)
		  goto truncated_icmp;
	      sa << '/' << ntohs(eudph->uh_dport);
	      break;
	    case ICMP_UNREACH_NEEDFRAG: {
		const click_icmp_needfrag *nfh = reinterpret_cast<const click_icmp_needfrag *>(icmph);
		if (nfh->icmp_nextmtu)
		    sa << " (mtu " << ntohs(nfh->icmp_nextmtu) << ')';
		break;
	    }
	  }
	  break;
      }

      default:
	sa << "icmp ";
	if (String s = NameInfo::revquery_int(NameInfo::T_ICMP_TYPE, this, icmph->icmp_type))
	    sa << s;
	else
	    sa << "type " << (int)icmph->icmp_type;
	if (String s = NameInfo::revquery_int(NameInfo::T_ICMP_CODE + icmph->icmp_type, this, icmph->icmp_code))
	    sa << ' ' << s;
	else if (icmph->icmp_code)
	    sa << " code " << (int)icmph->icmp_code;
	break;
    }
    return;

  truncated_icmp:
    sa << "truncated-icmp";
}

Packet *
IPPrint::simple_action(Packet *p)
{
    const click_ip *iph = p->ip_header();
    if (!_active || !iph)
	return p;

    StringAccum sa;

    if (_label)
	sa << _label << ": ";
    if (_print_timestamp)
	sa << p->timestamp_anno() << ": ";
    if (_print_aggregate)
	sa << '#' << AGGREGATE_ANNO(p);
    if (_print_paint)
	sa << (_print_aggregate ? "." : "paint ") << (int)PAINT_ANNO(p);
    if (_print_aggregate || _print_paint)
	sa << ": ";

    if (p->network_length() < (int) sizeof(click_ip))
	sa << "truncated-ip";
    else {
	int ip_len = ntohs(iph->ip_len);
	int payload_len = ip_len - (iph->ip_hl << 2);
	int transport_length = p->transport_length();
	if (p->end_data() > p->network_header() + ip_len)
	    transport_length = p->end_data() - (p->network_header() + ip_len);

	if (_print_id)
	    sa << "id " << ntohs(iph->ip_id) << ' ';
	if (_print_ttl)
	    sa << "ttl " << (int)iph->ip_ttl << ' ';
	if (_print_tos)
	    sa << "tos " << (int)iph->ip_tos << ' ';
	if (_print_len)
	    sa << "length " << ip_len << ' ';

	if (iph->ip_p == IP_PROTO_TCP)
	    tcp_line(sa, p, transport_length);
	else if (iph->ip_p == IP_PROTO_UDP)
	    udp_line(sa, p, transport_length);
	else if (iph->ip_p == IP_PROTO_ICMP)
	    icmp_line(sa, p, transport_length);
	else
	    sa << IPAddress(iph->ip_src) << " > " << IPAddress(iph->ip_dst) << ": ip-proto-" << (int)iph->ip_p;

	// print fragment info
	if (IP_ISFRAG(iph))
	    sa << " (frag " << ntohs(iph->ip_id) << ':' << payload_len << '@'
	       << ((ntohs(iph->ip_off) & IP_OFFMASK) << 3)
	       << ((iph->ip_off & htons(IP_MF)) ? "+" : "") << ')';

	// print payload
	if (_contents > 0) {
	    const uint8_t *data;
	    if (_payload) {
		if (IP_FIRSTFRAG(iph) && iph->ip_p == IP_PROTO_TCP)
		    data = p->transport_header() + (p->tcp_header()->th_off << 2);
		else if (IP_FIRSTFRAG(iph) && iph->ip_p == IP_PROTO_UDP)
		    data = p->transport_header() + sizeof(click_udp);
		else
		    data = p->transport_header();
	    } else
		data = p->data();

	    int bytes = _bytes;
	    if (bytes < 0 || (int) (p->end_data() - data) < bytes)
		bytes = p->end_data() - data;
	    int amt = 3*bytes + (bytes/4+1) + 3*(bytes/24+1) + 1;
	    
	    char *buf = sa.reserve(amt);
	    char *orig_buf = buf;
	    
	    if (buf && _contents == 1) {
		for (int i = 0; i < bytes; i++, data++) {
		    if ((i % 24) == 0) {
			*buf++ = '\n'; *buf++ = ' '; *buf++ = ' ';
		    } else if ((i % 4) == 0)
			*buf++ = ' ';
		    sprintf(buf, "%02x", *data & 0xff);
		    buf += 2;
		}
	    } else if (buf && _contents == 2) {
		for (int i = 0; i < bytes; i++, data++) {
		    if ((i % 48) == 0) {
			*buf++ = '\n'; *buf++ = ' '; *buf++ = ' ';
		    } else if ((i % 8) == 0)
			*buf++ = ' ';
		    if (*data < 32 || *data > 126)
			*buf++ = '.';
		    else
			*buf++ = *data;
		}
	    }

	    if (orig_buf) {
		assert(buf <= orig_buf + amt);
		sa.adjust_length(buf - orig_buf);
	    }
	}
    }

#if CLICK_USERLEVEL
    if (_outfile) {
	sa << '\n';
	fwrite(sa.data(), 1, sa.length(), _outfile);
    } else
#endif
	_errh->message("%s", sa.c_str());

    return p;
}

void
IPPrint::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(IPPrint)

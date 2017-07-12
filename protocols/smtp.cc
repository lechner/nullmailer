// nullmailer -- a simple relay-only MTA
// Copyright (C) 1999,2000  Bruce Guenter <bruceg@em.ca>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact me at <bruceg@em.ca>.  There is also a mailing list
// available to discuss this package.  To subscribe, send an email to
// <nullmailer-subscribe@lists.em.ca>.

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include "connect.h"
#include "errcodes.h"
#include "fdbuf/fdbuf.h"
#include "hostname.h"
#include "itoa.h"
#include "mystring/mystring.h"
#include "protocol.h"

int port = 25;
const char* cli_program = "smtp";
const char* cli_help_prefix = "Send an email message via SMTP\n";

class smtp 
{
  fdibuf in;
  fdobuf out;
public:
  smtp(int fd);
  ~smtp();
  int get(mystring& str);
  int put(mystring cmd, mystring& result);
  void docmd(mystring cmd, int range, bool nofail=false);
  void send_data(fdibuf* msg);
  void send_envelope(fdibuf* msg);
  void send(fdibuf* msg);
};

smtp::smtp(int fd)
  : in(fd), out(fd)
{
}

smtp::~smtp()
{
}

int smtp::get(mystring& str)
{
  mystring tmp;
  str = "";
  int code = -1;
  while(in.getline(tmp)) {
    if(tmp[tmp.length()-1] == '\r')
      tmp = tmp.left(tmp.length()-1);
    code = atoi(tmp.c_str());
    if(!!str)
      str += "/";
    str += tmp.right(4);
    if(tmp[3] != '-')
      break;
  }
  return code;
}

int smtp::put(mystring cmd, mystring& result)
{
  out << cmd << "\r\n";
  if(!out.flush())
    return -1;
  return get(result);
}

void smtp::docmd(mystring cmd, int range, bool nofail)
{
  mystring msg;
  int code;
  if(!cmd)
    code = get(msg);
  else
    code = put(cmd, msg);
  if(!nofail) {
    if(code < range || code >= (range+100)) {
      int e;
      if(code >= 500)
	e = ERR_MSG_PERMFAIL;
      else if(code >= 400)
	e = ERR_MSG_TEMPFAIL;
      else
	e = ERR_PROTO;
      exit(e);
    }
  }
}

void smtp::send_envelope(fdibuf* msg)
{
  mystring tmp;
  msg->getline(tmp);
  docmd("MAIL FROM: <" + tmp + ">", 200);
  while(msg->getline(tmp) && !!tmp)
    docmd("RCPT TO: <" + tmp + ">", 200);
}

void smtp::send_data(fdibuf* msg)
{
  docmd("DATA", 300);
  mystring tmp;
  while(msg->getline(tmp)) {
    if(tmp[0] == '.' && !(out << ".")) exit(ERR_MSG_WRITE);
    if(!(out << tmp << "\r\n")) exit(ERR_MSG_WRITE);
  }
  docmd(".", 200);
}

void smtp::send(fdibuf* msg)
{
  send_envelope(msg);
  send_data(msg);
}

int protocol_prep(fdibuf*)
{
  return 0;
}

int protocol_send(fdibuf* in, int fd)
{
  smtp conn(fd);
  conn.docmd("", 200);
  conn.docmd("HELO " + hostname(), 200);
  conn.send(in);
  conn.docmd("QUIT", 200, true);
  return 0;
}

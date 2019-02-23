#pragma once

#ifndef HAVE_GETTEXT
#define ngettext(msgid1, msgid2, n) ((n==1) ? (msgid1) : (msgid2))
#define gettext(msgid) (msgid)
#endif


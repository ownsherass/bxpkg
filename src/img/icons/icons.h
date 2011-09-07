#ifndef _XPKG_ICONS_H_
#define	_XPKG_ICONS_H_

#include "accessibility.h"
#include "arabic.h"
#include "archivers.h"
#include "astro.h"
#include "audio.h"
#include "benchmarks.h"
#include "biology.h"
#include "cad.h"
#include "chinese.h"
#include "comms.h"
#include "converters.h"
#include "databases.h"
#include "deskutils.h"
#include "devel.h"
#include "dns.h"
#include "editors.h"
#include "emulators.h"
#include "finance.h"
#include "french.h"
#include "file.h"
#include "ftp.h"
#include "games.h"
#include "german.h"
#include "graphics.h"
#include "hebrew.h"
#include "hungarian.h"
#include "irc.h"
#include "japanese.h"
#include "java.h"
#include "korean.h"
#include "lang.h"
#include "mail.h"
#include "math.h"
#include "mbone.h"
#include "misc.h"
#include "multimedia.h"
#include "net-mgmt.h"
#include "net-p2p.h"
#include "net.h"
#include "news.h"
#include "packages.h"
#include "palm.h"
#include "polish.h"
#include "portuguese.h"
#include "print.h"
#include "russian.h"
#include "science.h"
#include "security.h"
#include "shells.h"
#include "sysutils.h"
#include "textproc.h"
#include "ukrainian.h"
#include "vietnamese.h"
#include "www.h"
#include "x11-clocks.h"
#include "x11-drivers.h"
#include "x11-fm.h"
#include "x11-fonts.h"
#include "x11-themes.h"
#include "x11-wm.h"
#include "x11.h"

#include "terminal.h"
#include "delete.h"

struct icons_str {
	char *category;
	void **xpm_d;
};

static struct icons_str icons_s[] = {
{"accessibility", (void *)&accessibility_xpm},
{"archivers", (void *)&archivers_xpm},
{"arabic", (void *)&arabic_xpm},
{"archivers", (void *)&archivers_xpm},
{"astro", (void *)&astro_xpm},
{"audio", (void *)&audio_xpm},
{"benchmarks", (void *)&benchmarks_xpm},
{"biology", (void *)&biology_xpm},
{"cad", (void *)&cad_xpm},
{"chinese", (void *)&chinese_xpm},
{"comms", (void *)&comms_xpm},
{"converters", (void *)&converters_xpm},
{"databases", (void *)&databases_xpm},
{"deskutils", (void *)&deskutils_xpm},
{"devel", (void *)&devel_xpm},
{"dns", (void *)&dns_xpm},
{"editors", (void *)&editors_xpm},
{"emulators", (void *)&emulators_xpm},
{"finance", (void *)&finance_xpm},
{"french", (void *)&french_xpm},
{"ftp", (void *)&ftp_xpm},
{"games", (void *)&games_xpm},
{"german", (void *)&german_xpm},
{"graphics", (void *)&graphics_xpm},
{"hebrew", (void *)&hebrew_xpm},
{"hungarian", (void *)&hungarian_xpm},
{"irc", (void *)&irc_xpm},
{"japanese", (void *)&japanese_xpm},
{"java", (void *)&java_xpm},
{"korean", (void *)&korean_xpm},
{"lang", (void *)&lang_xpm},
{"mail", (void *)&mail_xpm},
{"math", (void *)&math_xpm},
{"mbone", (void *)&mbone_xpm},
{"misc", (void *)&misc_xpm},
{"multimedia", (void *)&multimedia_xpm},
{"net-mgmt", (void *)&net_mgmt_xpm},
{"net-p2p", (void *)&net_p2p_xpm},
{"net-im", (void *)&irc_xpm},
{"net", (void *)&net_xpm},
{"news", (void *)&news_xpm},
{"ports_mgmt", (void *)&packages_xpm},
{"palm", (void *)&palm_xpm},
{"polish", (void *)&polish_xpm},
{"portuguese", (void *)&portuguese_xpm},
{"print", (void *)&print_xpm},
{"russian", (void *)&russian_xpm},
{"science", (void *)&science_xpm},
{"security", (void *)&security_xpm},
{"shells", (void *)&shells_xpm},
{"sysutils", (void *)&sysutils_xpm},
{"textproc", (void *)&textproc_xpm},
{"ukrainian", (void *)&ukrainian_xpm},
{"vietnamese", (void *)&vietnamese_xpm},
{"www", (void *)&www_xpm},
{"x11-clocks", (void *)&x11_clocks_xpm},
{"x11-drivers", (void *)&x11_drivers_xpm},
{"x11-fm", (void *)&x11_fm_xpm},
{"x11-fonts", (void *)&x11_fonts_xpm},
{"x11-themes", (void *)&x11_themes_xpm},
{"x11-wm", (void *)&x11_wm_xpm},
{"x11", (void *)&x11_xpm},
{"x11-servers", (void *)&x11_xpm},
{"x11-toolkits", (void *)&x11_xpm},
{NULL, NULL}
};

#endif

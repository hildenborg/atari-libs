/*
	Copyright (C) 2026 Mikael Hildenborg
	SPDX-License-Identifier: MIT
*/

/*
	MiNT GEMDOS extensions. Without MiNT, plain TOS answers every call with EINVFN,
	which still checks the GEMDOS opcode and that the arguments are popped correctly
	(a wrong stack cleanup crashes on return).
	Paths are on the test floppy, so TOS and not Hatari's GEMDOS drive emulation answers them.
*/

#include "harness.h"

static void dummy_handler(int sig)
{
}

#define EXPECT_EINVFN(func, call) \
	do { t_run(func); long _r = (long)(call); \
		t_check(_r == EINVFN, func, "%s = %ld, expected EINVFN without MiNT", #call, _r); } while (0)

#define CALL_VOID(func, call) \
	do { t_run(func); call; t_check(1, func, "%s returned (void, no MiNT)", #call); } while (0)

int main(void)
{
	t_begin("T_MINT");
	if (t_cookie(0x4D694E54, 0))	// MiNT
	{
		t_print("MiNT is installed, these tests only cover plain TOS");
		return t_end();
	}
	short fds[2];
	int rus[8];
	char buf[64];
	MSG msg;
	XATTR xa;
	SIGACTION sa = { dummy_handler, 0, 0 };

	EXPECT_EINVFN("Syield", Syield());
	EXPECT_EINVFN("Fpipe", Fpipe(fds));
	EXPECT_EINVFN("Fcntl", Fcntl(0, 0, 0));
	EXPECT_EINVFN("Finstat", Finstat(0));
	EXPECT_EINVFN("Foutstat", Foutstat(1));
	EXPECT_EINVFN("Fgetchar", Fgetchar(0, 0));
	EXPECT_EINVFN("Fputchar", Fputchar(1, 'x', 0));
	EXPECT_EINVFN("Pwait", Pwait());
	EXPECT_EINVFN("Pnice", Pnice(0));
	EXPECT_EINVFN("Pgetpid", Pgetpid());
	EXPECT_EINVFN("Pgetppid", Pgetppid());
	EXPECT_EINVFN("Pgetpgrp", Pgetpgrp());
	EXPECT_EINVFN("Psetpgrp", Psetpgrp(0, 0));
	EXPECT_EINVFN("Pgetuid", Pgetuid());
	EXPECT_EINVFN("Psetuid", Psetuid(0));
	EXPECT_EINVFN("Pkill", Pkill(0, SIGNULL));
	EXPECT_EINVFN("Psignal", Psignal(SIGUSR1, dummy_handler));
	EXPECT_EINVFN("Pvfork", Pvfork());
	EXPECT_EINVFN("Pgetgid", Pgetgid());
	EXPECT_EINVFN("Psetgid", Psetgid(0));
	EXPECT_EINVFN("Psigblock", Psigblock(0));
	EXPECT_EINVFN("Psigsetmask", Psigsetmask(0));
	EXPECT_EINVFN("Pusrval", Pusrval(-1));
	EXPECT_EINVFN("Pdomain", Pdomain(-1));
	CALL_VOID("Psigreturn", Psigreturn());
	EXPECT_EINVFN("Pfork", Pfork());
	EXPECT_EINVFN("Pwait3", Pwait3(1, rus));
	EXPECT_EINVFN("Fselect", Fselect(1, 0, 0, 0));
	CALL_VOID("Prusage", Prusage(rus));
	EXPECT_EINVFN("Psetlimit", Psetlimit(0, -1));
	EXPECT_EINVFN("Talarm", Talarm(-1));
	CALL_VOID("Pause", Pause());
	EXPECT_EINVFN("Sysconf", Sysconf(-1));
	EXPECT_EINVFN("Psigpending", Psigpending());
	EXPECT_EINVFN("Dpathconf", Dpathconf(t_fpath("\\"), 0));
	EXPECT_EINVFN("Pmsg", Pmsg(0x8000, 0, &msg));
	EXPECT_EINVFN("Fmidipipe", Fmidipipe(0, 0, 1));
	EXPECT_EINVFN("Prenice", Prenice(0, 0));
	EXPECT_EINVFN("Dopendir", Dopendir(t_fpath("\\"), 0));
	EXPECT_EINVFN("Dreaddir", Dreaddir(sizeof(buf), 0, buf));
	EXPECT_EINVFN("Drewinddir", Drewinddir(0));
	EXPECT_EINVFN("Dclosedir", Dclosedir(0));
	EXPECT_EINVFN("Fxattr", Fxattr(0, t_fpath("\\"), &xa));
	EXPECT_EINVFN("Flink", Flink(t_fpath("\\X"), t_fpath("\\Y")));
	EXPECT_EINVFN("Fsymlink", Fsymlink(t_fpath("\\X"), t_fpath("\\Y")));
	EXPECT_EINVFN("Freadlink", Freadlink(sizeof(buf), buf, t_fpath("\\X")));
	EXPECT_EINVFN("Dcntl", Dcntl(0, t_fpath("\\"), 0));
	EXPECT_EINVFN("Fchown", Fchown(t_fpath("\\X"), 0, 0));
	EXPECT_EINVFN("Fchmod", Fchmod(t_fpath("\\X"), 0644));
	EXPECT_EINVFN("Pumask", Pumask(022));
	EXPECT_EINVFN("Psemaphore", Psemaphore(2, 0, 0));
	EXPECT_EINVFN("Dlock", Dlock(0, t_env.flop));
	CALL_VOID("Psigpause", Psigpause(0));
	EXPECT_EINVFN("Psigaction", Psigaction(SIGUSR1, &sa, 0));
	EXPECT_EINVFN("Pgeteuid", Pgeteuid());
	EXPECT_EINVFN("Pgetegid", Pgetegid());
	EXPECT_EINVFN("Pwaitpid", Pwaitpid(-1, 1, rus));
	EXPECT_EINVFN("Dgetcwd", Dgetcwd(buf, 2, sizeof(buf)));
	CALL_VOID("Salert", Salert("test"));
	return t_end();
}

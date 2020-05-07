
#include <katy/katy.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
const char *msg = "Hello world! this is a test of the libkaty library";

	stat(msg);
	staterr("everything is working fine");
	hexdump((uint8_t *)msg, strlen(msg));
	//crash("crash test");
	
	char hn[1024];
	gethostname(hn, sizeof(hn));
	
	DString str("Congratulations - ");
	str.AppendString("your computer has been katified");
	stat("Host \e[1;33m%s\e[0m: %s", hn, stprintf("%s!", str.String()));
	
	// test list.foreach
/*	DString str1("test1"); DString str2("test2");
	List<DString> list;
	list.AddItem(&str1);
	list.AddItem(&str2);

	list.foreach([](auto *s) { stat("%s", s->String()); });
*/

	// test new stat functionality having to do with logging
/*	SetLogFilename("/tmp/test.log");
	stat("this is a single line");
	statnocr("statnocr");
	stat(" can be prepended and logged");
	statnocr("even"); statnocr(" if "); statnocr("there are more than "); stat("one statnocr");
	stat("and...\nwe can handle\nmultiple lines");
	stat("\n...even if CR appears at the start");
	system("cat /tmp/test.log"); remove("/tmp/test.log");*/
}

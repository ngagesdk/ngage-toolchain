extern "C" {
extern int main(int argc, char *argv[]);
 int __gccmain(int argc, char* argv[]) {
    /* FIXME: abort */
    return 0;
}
}

/* Called by E32Startup(void) */
int E32Main(void);
int E32Main(void) {
    char *argv[2];
    char arg0[4];
     arg0[0] = 'a';
     arg0[1] = 'p';
     arg0[2] = 'p';
     arg0[3] = '\0';
     argv[0] = arg0;
     argv[1] = (char*)0;
    return main(1, argv);
}

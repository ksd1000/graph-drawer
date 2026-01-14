
#define graph_bmpsize(gp) (*(uint32_t *)((gp)->buf-54+2))
#define graph_getbmp(gp) ((gp)->buf-54)
#endif

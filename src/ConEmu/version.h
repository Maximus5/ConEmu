
#define MVV_1 2010
#define MVV_2 2
#define MVV_3 22
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define CONEMUVERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3) MVV_4a
#define CONEMUVERN MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _WIN64
#define CONEMUPLTFRM " (x64)"
#else
#define CONEMUPLTFRM " (x86)"
#endif

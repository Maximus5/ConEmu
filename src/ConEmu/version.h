
#define MVV_1 11
#define MVV_2 02
#define MVV_3 25
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define CONEMUVERS STRING(MVV_1) STRING(MVV_2) STRING(MVV_3) MVV_4a
#define CONEMUVERN 2000+MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _WIN64
#define CONEMUPLTFRM " (x64)"
#else
#ifdef WIN64
#define CONEMUPLTFRM " (x64)"
#else
#define CONEMUPLTFRM " (x86)"
#endif
#endif

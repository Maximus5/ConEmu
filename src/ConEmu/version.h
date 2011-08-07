
#define MVV_1 11
#define MVV_2 8
#define MVV_3 7
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if (MVV_2<10)
  #define MVV_2s "0" STRING(MVV_2)
#else
  #define MVV_2s STRING(MVV_2)
#endif
#if (MVV_3<10)
  #define MVV_3s "0" STRING(MVV_3)
#else
  #define MVV_3s STRING(MVV_3)
#endif
#if (MVV_4>0)
  #define MVV_3n MVV_4##MVV_3
#else
  #define MVV_3n MVV_3
#endif

#define CONEMUVERS STRING(MVV_1) MVV_2s MVV_3s MVV_4a

#ifdef __GNUC__
#define CONEMUVERN MVV_1,MVV_2,MVV_3,MVV_4
#else
//#define CONEMUVERN 20##MVV_1,MVV_2,MVV_3n,MVV_4
#define CONEMUVERN MVV_1,MVV_2,MVV_3n,MVV_4
#endif

#ifdef _WIN64
#define CONEMUPLTFRM " (x64)"
#else
#ifdef WIN64
#define CONEMUPLTFRM " (x64)"
#else
#define CONEMUPLTFRM " (x86)"
#endif
#endif

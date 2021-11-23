

typedef struct xava_cairo_module {
    char *name;
    XAVA_CAIRO_FEATURE features;
    xava_cairo_region  region;

    //xava_version (*version)(void);
    //void         (*config_load)(XAVAGLModuleOptions*);
    //void         (*init)(XAVAGLModuleOptions*);
    //void         (*apply)(XAVAGLModuleOptions*);
    //XG_EVENT     (*event)(XAVAGLModuleOptions*);
    //void         (*clear)(XAVAGLModuleOptions*);
    //void         (*draw)(XAVAGLModuleOptions*);
    //void         (*cleanup)(XAVAGLModuleOptions*);
} xava_cairo_module;

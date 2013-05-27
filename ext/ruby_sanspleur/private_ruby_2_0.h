/* Manual exports for Ruby 2.0 */


typedef struct rb_iseq_location_struct {
    VALUE path;
    VALUE absolute_path;
    VALUE base_label;
    VALUE label;
    size_t first_lineno;
} rb_iseq_location_t;

struct rb_iseq_struct;

struct rb_iseq_struct {
    /***************/
    /* static data */
    /***************/

    enum iseq_type {
    ISEQ_TYPE_TOP,
    ISEQ_TYPE_METHOD,
    ISEQ_TYPE_BLOCK,
    ISEQ_TYPE_CLASS,
    ISEQ_TYPE_RESCUE,
    ISEQ_TYPE_ENSURE,
    ISEQ_TYPE_EVAL,
    ISEQ_TYPE_MAIN,
    ISEQ_TYPE_DEFINED_GUARD
    } type;              /* instruction sequence type */

    rb_iseq_location_t location;

    VALUE *iseq;         /* iseq (insn number and operands) */
    VALUE *iseq_encoded; /* encoded iseq */
    unsigned long iseq_size;
    VALUE mark_ary; /* Array: includes operands which should be GC marked */
    VALUE coverage;     /* coverage array */

    /* insn info, must be freed */
    void /* struct iseq_line_info_entry */ *line_info_table;
    size_t line_info_size;

    ID *local_table;        /* must free */
    int local_table_size;

    /* sizeof(vars) + 1 */
    int local_size;

    void /* struct iseq_inline_cache_entry */ *ic_entries;
    int ic_size;

    void /* rb_call_info_t */ * callinfo_entries;
    int callinfo_size;

    int argc;
    int arg_simple;
    int arg_rest;
    int arg_block;
    int arg_opts;
    int arg_post_len;
    int arg_post_start;
    int arg_size;
    VALUE *arg_opt_table;
    int arg_keyword;
    int arg_keyword_check; /* if this is true, raise an ArgumentError when unknown keyword argument is passed */
    int arg_keywords;
    ID *arg_keyword_table;

    size_t stack_max; /* for stack overflow check */

    /* catch table */
    struct iseq_catch_table_entry *catch_table;
    int catch_table_size;

    /* for child iseq */
    struct rb_iseq_struct *parent_iseq;
    struct rb_iseq_struct *local_iseq;

    /****************/
    /* dynamic data */
    /****************/

    VALUE self;
    VALUE orig;         /* non-NULL if its data have origin */

    /* block inlining */
    /*
     * NODE *node;
     * void *special_block_builder;
     * void *cached_special_block_builder;
     * VALUE cached_special_block;
     */

    /* klass/module nest information stack (cref) */
    void *cref_stack;
    VALUE klass;

    /* misc */
    ID defined_method_id;   // Ruby_Sanspleur note: this is the last one we need. Don't go beyond this, we only access frames as pointers.a;
};


typedef struct rb_iseq_struct rb_iseq_t;

typedef enum {
    NOEX_PUBLIC    = 0x00,
    NOEX_NOSUPER   = 0x01,
    NOEX_PRIVATE   = 0x02,
    NOEX_PROTECTED = 0x04,
    NOEX_MASK      = 0x06,
    NOEX_BASIC     = 0x08,
    NOEX_UNDEF     = NOEX_NOSUPER,
    NOEX_MODFUNC   = 0x12,
    NOEX_SUPER     = 0x20,
    NOEX_VCALL     = 0x40,
    NOEX_RESPONDS  = 0x80,

    NOEX_BIT_WIDTH = 8,
    NOEX_SAFE_SHIFT_OFFSET = ((NOEX_BIT_WIDTH+3)/4)*4 /* round up to nibble */
} rb_method_flag_t;

typedef enum {
    VM_METHOD_TYPE_ISEQ,
    VM_METHOD_TYPE_CFUNC,
    VM_METHOD_TYPE_ATTRSET,
    VM_METHOD_TYPE_IVAR,
    VM_METHOD_TYPE_BMETHOD,
    VM_METHOD_TYPE_ZSUPER,
    VM_METHOD_TYPE_UNDEF,
    VM_METHOD_TYPE_NOTIMPLEMENTED,
    VM_METHOD_TYPE_OPTIMIZED, /* Kernel#send, Proc#call, etc */
    VM_METHOD_TYPE_MISSING,   /* wrapper for method_missing(id) */
    VM_METHOD_TYPE_REFINED
} rb_method_type_t;

typedef struct rb_method_definition_struct {
    rb_method_type_t type; /* method type */
    ID original_id;
    // Ruby_Sanspleur note: we don't need anything further this point and we use this struct only as pointer
} rb_method_definition_t;



typedef struct rb_method_entry_struct {
    rb_method_flag_t flag;
    char mark;
    rb_method_definition_t *def;
    ID called_id;
    VALUE klass;                    /* should be mark */
} rb_method_entry_t;


typedef struct rb_control_frame_struct {
    VALUE *pc;          /* cfp[0] */
    VALUE *sp;          /* cfp[1] */
    rb_iseq_t *iseq;        /* cfp[2] */
    VALUE flag;         /* cfp[3] */
    VALUE self;         /* cfp[4] / block[0] */
    VALUE klass;        /* cfp[5] / block[1] */
    VALUE *ep;          /* cfp[6] / block[2] */
    rb_iseq_t *block_iseq;  /* cfp[7] / block[3] */
    VALUE proc;         /* cfp[8] / block[4] */
    const rb_method_entry_t *me;/* cfp[9] */
} rb_control_frame_t;


typedef struct rb_thread_struct {
    VALUE self;
    void /*rb_vm_t*/ *vm;

    VALUE *stack;       
    unsigned long stack_size;
    rb_control_frame_t *cfp;
} rb_thread_t;



#define RUBY_VM_IFUNC_P(ptr)        (BUILTIN_TYPE(ptr) == T_NODE)
#define VM_FRAME_MAGIC_MASK_BITS   8
#define VM_FRAME_MAGIC_MASK   (~(~0<<VM_FRAME_MAGIC_MASK_BITS))
#define VM_FRAME_TYPE(cfp) ((cfp)->flag & VM_FRAME_MAGIC_MASK)

#define ISEQ_FILENAME(iseq_ptr) (RSTRING_PTR(iseq_ptr->location.path))
#define CFP_LINENO(cfp_ptr) (0)
